// Native.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "DX11Player.h"
#include <sstream>
#include "Timer.h"
#include <WinIoCtl.h>
#include <Shlwapi.h>

#pragma comment(lib, "Kernel32.lib")

//static const int RING_BUFFER_SIZE = 12;


static bool IsFrameLate(const HighResClock::time_point & presentationTime, int fps_video, HighResClock::time_point now, HighResClock::duration max_lateness){
	if(fps_video==0) return false;
	auto duration = std::chrono::nanoseconds(1000000000/fps_video);
	return presentationTime+duration+max_lateness<now;
}

static bool IsFrameReady(const HighResClock::time_point & presentationTime, int fps_video, int fps_thread, HighResClock::time_point now, HighResClock::duration max_lateness){
	if(fps_video==0 || fps_thread==0) return false;
	auto duration_video = std::chrono::nanoseconds(1000000000/fps_video);
	auto duration_thread = std::chrono::nanoseconds(1000000000/fps_thread);
	return presentationTime<=now+duration_thread && now<=presentationTime+duration_video+max_lateness;
}

static uint32_t SectorSize(char cDisk)
{
	HANDLE hDevice;

    // Build the logical drive path and get the drive device handle
    std::wstring logicalDrive = L"\\\\.\\";
    wchar_t drive[3];
    drive[0] = cDisk;
    drive[1] = L':';
    drive[2] = L'\0';
    logicalDrive.append(drive);

    hDevice = CreateFile(
        logicalDrive.c_str(),
        0, 
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (hDevice == INVALID_HANDLE_VALUE)
    {
		//throw std::exception("Error finding out disk sector size");
        return -1;
    }   

    // Now that we have the device handle for the disk, let us get disk's metadata
    DWORD outsize;
    STORAGE_PROPERTY_QUERY storageQuery;
    memset(&storageQuery, 0, sizeof(STORAGE_PROPERTY_QUERY));
    storageQuery.PropertyId = StorageAccessAlignmentProperty;
    storageQuery.QueryType  = PropertyStandardQuery;

    STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR diskAlignment = {0};
    memset(&diskAlignment, 0, sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR));

    if (!DeviceIoControl(hDevice, 
        IOCTL_STORAGE_QUERY_PROPERTY, 
        &storageQuery, 
        sizeof(STORAGE_PROPERTY_QUERY), 
        &diskAlignment,
        sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR), 
        &outsize,
        NULL)
        )
    {
		//throw std::exception("Error finding out disk sector size");
        //return -1;
		return 512;
    }

	return diskAlignment.BytesPerLogicalSector;
}

DWORD NextMultiple(DWORD in, DWORD multiple){
	return in + (multiple - in % multiple);
}

DX11Player::DX11Player(const std::string & directory, const std::string & wildcard, size_t ringBufferSize)
	:m_UploaderThreadRunning(false)
	,m_WaiterThreadRunning(false)
	,m_RateThreadRunning(false)
	,m_ExternalRate(false)
	,m_InternalRate(false)
	,m_NextRenderFrame(-1)
	,m_DroppedFrames(0)
	,m_Fps(60)
	,m_RingBufferSize(ringBufferSize)
	,m_CurrentFrame(0)
	,m_AvgDecodeDuration(0)
	,m_AvgPipelineLatency(std::chrono::milliseconds(1000/m_Fps * 2))
	,m_GotFirstFrame(false)
	,m_StatusCode(Init)
	,m_StatusDesc("Init")
{

	// uploader thread: initializes the whole pipeline and then
	// reads async from disk to mapped GPU memory
	// and sends an event to wait on to the waiter thread
	//OutputDebugString( L"creating upload thread\n" );
	auto running = &m_UploaderThreadRunning;
	auto currentFrame = &m_CurrentFrame;
	auto sequence_ptr = &m_Sequence;
	auto readyToUpload = &m_ReadyToUpload;
	auto readyToWait = &m_ReadyToWait;
	auto readyToRender = &m_ReadyToRender;
	auto readyToRate = &m_ReadyToRate;
	auto fps = &m_Fps;
	auto externalRate = &m_ExternalRate;
	auto nextFrameChannel = &m_NextFrameChannel;
	auto internalRate = &m_InternalRate;
	auto self = this;
	m_UploaderThread = std::thread([running,directory,wildcard,self,currentFrame,sequence_ptr,readyToUpload,readyToWait,fps,externalRate,nextFrameChannel,internalRate,ringBufferSize]{
		try{
			*sequence_ptr = std::make_shared<ImageSequence>(directory,wildcard);
		}catch(std::exception & e){
			self->ChangeStatus(Error,e.what());
			return;
		}
		auto sequence = *sequence_ptr;

		// Add enough bytes to read the header + data but we need to read
		// a multiple of the physical sector size since we are reading 
		// directly from disk with no buffering
		if(!PathFileExistsA(directory.c_str())){
			self->ChangeStatus(Error,"Cannot find folder");
			return;
		}
		char buffer[4096] = {0};
		auto ret = GetFullPathNameA(directory.c_str(),4096,buffer,nullptr);
		if(ret==0){
			self->ChangeStatus(Error,"Cannot recover folder drive");
			return;
		}
		auto sectorSize = SectorSize(buffer[0]);
		if(sectorSize<0){
			self->ChangeStatus(Error,"Cannot drive retrieve sector size");
		}
		auto numbytesdata = NextMultiple((DWORD)sequence->BytesData()+sequence->DataOffset(),(DWORD)sectorSize);


		Format format = {
			sequence->InputWidth(),
			sequence->Height(),
			sequence->InputFormat(),
			sequence->TextureFormat(),
			sequence->TextureOutFormat(),
			sequence->InputDepth(),
			sequence->Width(),
			sequence->RequiresVFlip(),
			sequence->RequiresByteSwap()
		};
		self->m_Context = Pool::GetInstance().AquireContext(format);
		//self->m_Context->Clear();

		// init the ring buffer:
		// - map all the upload buffers and send them to the uploader thread.
		for(auto i=0;i<ringBufferSize;i++){
			auto frame = self->m_Context->GetFrame();
			if(frame->Map()!=0){
				self->ChangeStatus(Error,"Couldn't map frame");
				return;
			}
			self->m_ReadyToUpload.send(frame);
		}

		// start the upload loop
		*running = true;
		self->ChangeStatus(Ready,"Ready");
		auto nextFrameTime = HighResClock::now();
		bool paused = false;
		int nextFps = *fps;
		std::shared_ptr<Frame> nextFrame;
		while(readyToUpload->recv(nextFrame)){
			auto now = HighResClock::now();
			auto absFps = abs(nextFps);
			if(*externalRate){
				if(!nextFrameChannel->recv(*currentFrame)){
					break;
				}
				/*while(nextFrameChannel->size()>2 && nextFrameChannel->try_recv(*currentFrame)){
					++self->m_DroppedFrames;
				}*/
				now = HighResClock::now();
				nextFrameTime = now;
			}else{
				if(absFps!=0){
					do{
						if(*internalRate){
							nextFrameTime += std::chrono::nanoseconds((uint64_t)floor(1000000000/double(absFps)+0.5));
						}else{
							nextFrameTime = now;
						}
						if(nextFps>0){
							(*currentFrame)+=1;
							(*currentFrame)%=sequence->NumImages();
						}else if(nextFps<0){
							(*currentFrame)-=1;
							(*currentFrame)%=sequence->NumImages();
						}
					}while(nextFrameTime<now);
				}else{
					paused = true;
				}
			}
			*currentFrame%=sequence->NumImages();
			nextFps = *fps;
			if(nextFrame->NextToLoad()==-1){
				nextFrame->SetNextToLoad(*currentFrame);
			}else{
				nextFrame->SetNextToLoad(nextFrame->NextToLoad()%sequence->NumImages());
				*currentFrame = nextFrame->NextToLoad();
			}
			if(paused && nextFps!=0){
				nextFrameTime = now;
				paused = false;
			}

			auto offset = sequence->RowPitch()*4-sequence->DataOffset();
			if(nextFrame->ReadFile(sequence->Image(nextFrame->NextToLoad()),offset,numbytesdata,now,nextFrameTime,nextFps)){
				if(!readyToWait->send(nextFrame))
				{
					nextFrame->Wait(INFINITE);
					break;
				}
			}
		}
	});
	
	// waiter thread: waits for the transfer from disk to GPU mem to finish
	// and sends the frame num to the rate thread
	//OutputDebugString( L"creating waiter thread\n" );
	m_WaiterThreadRunning = true;
	running = &m_WaiterThreadRunning;
	auto avgDecodeDuration = &m_AvgDecodeDuration;
	auto dropped = &m_DroppedFrames;
	m_WaiterThread = std::thread([running,readyToWait,readyToRate,readyToRender,readyToUpload,avgDecodeDuration,internalRate,fps,dropped]{
		auto start = HighResClock::now();
		std::shared_ptr<Frame> nextFrame;
		while(readyToWait->recv(nextFrame)){
			auto ontime = nextFrame->Wait(INFINITE);//1000.0 / (double)*fps * (RING_BUFFER_SIZE/2));
			*avgDecodeDuration = std::chrono::milliseconds(std::chrono::duration_cast<std::chrono::milliseconds>(nextFrame->DecodeDuration()).count()/(readyToWait->size()+1));
			//if(ontime){
				if(*internalRate){
					readyToRate->send(nextFrame);
				}else{
					readyToRender->send(nextFrame);
				}
			/*}else{
				++(*dropped);
				nextFrame->SetNextToLoad(-1);
				readyToUpload->send(nextFrame);
			}*/
		}
	});


	// rate thread: receives frames already in the GPU and controls
	// the playback rate, once a frame is on time to be shown it sends it
	// to the main thread
	m_RateThreadRunning = true;
	running = &m_RateThreadRunning;
	auto droppedFrames = &m_DroppedFrames;
	m_RateThread = std::thread([running,readyToRate,readyToUpload,readyToRender,fps,droppedFrames,internalRate] {
		std::shared_ptr<Frame> nextFrame;
		auto prevFps = *fps;
		auto absFps = abs(prevFps);
		uint64_t periodns;
		if(absFps!=0){
			periodns = 1000000000 / absFps;
		}else{
			periodns = 1000000000 / 60;
		}
		std::chrono::nanoseconds p(periodns);
		Timer timer(p);
		bool wasInternalRate = *internalRate;
		while(*running){
			auto useInternalRate = *internalRate;
			if(useInternalRate){
				if(!wasInternalRate){
					timer.reset();
				}
				if(!readyToRate->recv(nextFrame)){
					break;
				}
				timer.wait_next();
				auto nextFps = *fps;
				if(prevFps != nextFps){
					if(nextFrame->Fps()!=nextFps && ((prevFps>0 && nextFps<0) || (nextFps>0 && prevFps<0))){
						auto nextToLoad = nextFrame->NextToLoad();
						auto waitingFor = nextToLoad;
						do{
							nextFrame->SetNextToLoad(nextToLoad);
							if(!readyToUpload->send(nextFrame)){
								return;
							}
							nextToLoad = -1;
							if(!readyToRate->recv(nextFrame)){
								return;
							}
						}while(nextFrame->NextToLoad()!=waitingFor);
					}
					prevFps = nextFps;
					absFps = abs(prevFps);
					if(absFps!=0){
						periodns = 1000000000 / absFps;
					}else{
						periodns = 1000000000 / 60;
					}
					timer.set_period(std::chrono::nanoseconds(periodns));
				}
				if(!readyToRender->send(nextFrame)){
					break;
				}
			}else{
				if(!readyToRate->recv(nextFrame) ||	!readyToRender->send(nextFrame)){
					break;
				}
			}
			wasInternalRate = useInternalRate;
		}
	});
}

DX11Player::~DX11Player(){
	m_RateThreadRunning = false;
	m_WaiterThreadRunning = false;
	m_UploaderThreadRunning = false;
	m_ReadyToUpload.close();
	m_ReadyToRate.close();
	m_ReadyToRender.close();
	m_ReadyToWait.close();
	m_NextFrameChannel.close();
	m_RateThread.join();
	m_UploaderThread.join();
	m_WaiterThread.join();
}


int64_t distance(const std::vector<size_t> & frames, size_t current, size_t expected){
	int d = 0;
	return std::find(frames.begin(),frames.end(),expected) - std::find(frames.begin(),frames.end(),current);
}

void DX11Player::OnRender(int nextFrame){
	if(!m_UploaderThreadRunning) return;
	std::shared_ptr<Frame> frame;
	auto now = HighResClock::now();
	auto max_lateness = m_AvgPipelineLatency + std::chrono::milliseconds(2);
	HighResClock::duration currentLatency;
	std::vector<std::shared_ptr<Frame>> receivedFrames;
	if(m_InternalRate){
		bool late = true;
		while(late && m_ReadyToRender.try_recv(frame)){
			currentLatency += now - frame->LoadTime();
			receivedFrames.push_back(frame);
			late = IsFrameLate(frame->PresentationTime(),abs(m_Fps),now,max_lateness);
		}
	}else{
		auto nextSequencedFrame = nextFrame % m_Sequence->NumImages();		
		std::vector<size_t> currentFrames(m_SystemFrames.begin(),m_SystemFrames.end());
		//std::stringstream str;
		//str << currentFrames.size() << " curretn: " << m_NextRenderFrame << " next " << nextSequencedFrame << " dist: " << distance(currentFrames,m_NextRenderFrame,nextSequencedFrame) << std::endl;
		//OutputDebugStringA(str.str().c_str());
		if(m_NextRenderFrame==-1 || distance(currentFrames,m_NextRenderFrame,nextSequencedFrame)>0){
			while(m_ReadyToRender.try_recv(frame)){
				currentLatency += now - frame->LoadTime();
				receivedFrames.push_back(frame);
				if(distance(currentFrames,nextSequencedFrame,frame->NextToLoad())>=0){
					break;
				}
			}
		}
	}

	if(!receivedFrames.empty()){
		m_GotFirstFrame=true;
		ChangeStatus(Status::FirstFrame,"FirstFrame");
		m_AvgPipelineLatency = std::chrono::nanoseconds(std::chrono::duration_cast<std::chrono::nanoseconds>(currentLatency).count() / receivedFrames.size());
		auto frame = receivedFrames.back();
		frame->Unmap();
		m_Context->CopyFrameToOutTexture(frame);
		frame->Map();
		if(m_NextRenderFrame!=-1){
			m_SystemFrames.erase(std::find(m_SystemFrames.begin(),m_SystemFrames.end(),m_NextRenderFrame));
		}
		m_NextRenderFrame = frame->NextToLoad();
		m_DroppedFrames+=int(receivedFrames.size())-1;
		for(auto frame: receivedFrames){
			if(frame->NextToLoad()!=m_NextRenderFrame){
				m_SystemFrames.erase(std::find(m_SystemFrames.begin(),m_SystemFrames.end(),frame->NextToLoad()));
			}
			frame->SetNextToLoad(-1);
			m_ReadyToUpload.send(frame);
		}
	}

}

void DX11Player::ChangeStatus(Status code, const std::string & status){
	std::unique_lock<std::mutex> lock(m);
	if(code!=m_StatusCode){
		m_StatusCode = code;
		OutputDebugStringA(status.c_str());
	}
	m_StatusDesc = status;
}

DX11Player::Status DX11Player::GetStatus(){
	std::unique_lock<std::mutex> lock(m);
	return m_StatusCode;
}

std::string DX11Player::GetStatusMessage(){
	std::unique_lock<std::mutex> lock(m);
	return m_StatusDesc;
}

HANDLE DX11Player::GetSharedHandle(){
	if(!GotFirstFrame()) return nullptr;
	return m_Context->GetSharedHandle();
}

std::string DX11Player::GetDirectory() const
{
	if(!m_UploaderThreadRunning) return "";
	return m_Sequence->Directory();
}

int DX11Player::GetUploadBufferSize() const
{
	if(!m_UploaderThreadRunning) return 0;
	return (int)m_ReadyToUpload.size();
}

int DX11Player::GetWaitBufferSize() const
{
	if(!m_UploaderThreadRunning) return 0;
	return (int)m_ReadyToWait.size();
}

int DX11Player::GetRenderBufferSize() const
{
	if(!m_UploaderThreadRunning) return 0;
	return (int)m_ReadyToRender.size();
}

int DX11Player::GetDroppedFrames() const
{
	return m_DroppedFrames;
}


size_t DX11Player::GetCurrentLoadFrame() const
{
	return m_CurrentFrame;
}


size_t DX11Player::GetCurrentRenderFrame() const
{
	return m_NextRenderFrame;
}


void DX11Player::SetFPS(int fps){
	m_Fps = fps;
}

int DX11Player::GetAvgLoadDurationMs() const
{
	return (int)std::chrono::duration_cast<std::chrono::milliseconds>(m_AvgDecodeDuration).count();
}

void DX11Player::SendNextFrameToLoad(int nextFrame)
{
	m_ExternalRate = true;
	if(!m_UploaderThreadRunning) return;
	if(m_NextRenderFrame==-1 || m_NextFrameChannel.size()<m_RingBufferSize){
		m_NextFrameChannel.send(nextFrame%m_Sequence->NumImages());
		m_SystemFrames.push_back(nextFrame%m_Sequence->NumImages());
	}else{
		m_DroppedFrames++;
	}
}


void DX11Player::SetInternalRate(int enabled)
{
	m_InternalRate = enabled!=0;
	if(!m_InternalRate) m_ExternalRate = true;
	if(m_InternalRate && m_ExternalRate)
	{
		m_NextFrameChannel.send(m_CurrentFrame);
	}
}

bool DX11Player::IsReady() const{
	return m_UploaderThreadRunning && m_StatusCode!=Error;
}

bool DX11Player::GotFirstFrame() const{
	return m_GotFirstFrame && m_StatusCode!=Error;
}


extern "C"{
	NATIVE_API DX11HANDLE DX11Player_Create(const char * directory, const char * wildcard, int ringBufferSize)
	{
		return new DX11Player(directory,wildcard,ringBufferSize);
	}

	NATIVE_API void DX11Player_Destroy(DX11HANDLE player)
	{
		delete static_cast<DX11Player*>(player);
	}

	NATIVE_API void DX11Player_OnRender(DX11HANDLE player,int nextFrame)
	{
		static_cast<DX11Player*>(player)->OnRender(nextFrame);
	}

	NATIVE_API HANDLE DX11Player_GetSharedHandle(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetSharedHandle();
	}

	NATIVE_API const char * DX11Player_GetDirectory(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetDirectory().c_str();
	}

	NATIVE_API int DX11Player_DirectoryHasChanged(DX11HANDLE player, const char * dir)
	{
		return static_cast<DX11Player*>(player)->GetDirectory() != std::string(dir);
	}

	NATIVE_API int DX11Player_GetUploadBufferSize(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetUploadBufferSize();
	}

	NATIVE_API int DX11Player_GetWaitBufferSize(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetWaitBufferSize();
	}

	NATIVE_API int DX11Player_GetRenderBufferSize(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetRenderBufferSize();
	}

	NATIVE_API int DX11Player_GetDroppedFrames(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetDroppedFrames();
	}

	NATIVE_API int DX11Player_GetCurrentLoadFrame(DX11HANDLE player)
	{
		return (int)static_cast<DX11Player*>(player)->GetCurrentLoadFrame();
	}

	NATIVE_API int DX11Player_GetCurrentRenderFrame(DX11HANDLE player)
	{
		return (int)static_cast<DX11Player*>(player)->GetCurrentRenderFrame();
	}

	NATIVE_API int DX11Player_GetAvgLoadDurationMs(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetAvgLoadDurationMs();
	}
	
	NATIVE_API void DX11Player_SetFPS(DX11HANDLE player, int fps)
	{
		static_cast<DX11Player*>(player)->SetFPS(fps);
	}
	
	NATIVE_API void DX11Player_SendNextFrameToLoad(DX11HANDLE player, int nextFrame)
	{
		static_cast<DX11Player*>(player)->SendNextFrameToLoad(nextFrame);
	}

	NATIVE_API void DX11Player_SetInternalRate(DX11HANDLE player, int enabled)
	{
		static_cast<DX11Player*>(player)->SetInternalRate(enabled);
	}

	NATIVE_API bool DX11Player_IsReady(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->IsReady();
	}

	NATIVE_API bool DX11Player_GotFirstFrame(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GotFirstFrame();
	}

	NATIVE_API int DX11Player_GetStatus(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetStatus();
	}

	NATIVE_API const char * DX11Player_GetStatusMessage(DX11HANDLE player)
	{
		std::string str = static_cast<DX11Player*>(player)->GetStatusMessage();
		ULONG ulSize = str.size() + sizeof(char);
		char* pszReturn = NULL;

		pszReturn = (char*)::CoTaskMemAlloc(ulSize);
		// Copy the contents of szSampleString
		// to the memory pointed to by pszReturn.
		strcpy(pszReturn, str.c_str());
		// Return pszReturn.
		return pszReturn;
	}
}
