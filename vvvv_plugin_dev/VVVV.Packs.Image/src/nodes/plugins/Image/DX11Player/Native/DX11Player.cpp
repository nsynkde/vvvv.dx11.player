// Native.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "DX11Player.h"
#include <sstream>
#include "Timer.h"
#include <WinIoCtl.h>
#include <Shlwapi.h>
#include <set>
#include <iostream>

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

static DWORD NextMultiple(DWORD in, DWORD multiple){
	if(in % multiple==0){
		return in;
	}else{
		return in + (multiple - in % multiple);
	}
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
	,m_LastUpdateTime(HighResClock::now())
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
	auto systemFrames = &m_SystemFrames;
	auto systemFramesMutex = &m_SystemFramesMutex;
	auto dropped = &m_DroppedFrames;
	m_UploaderThread = std::thread([dropped,systemFrames,systemFramesMutex,running,directory,wildcard,self,currentFrame,sequence_ptr,readyToUpload,readyToWait,fps,externalRate,nextFrameChannel,internalRate,ringBufferSize]{
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
			self->ChangeStatus(Error,"Cannot retrieve drive sector size");
			return;
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
			sequence->RequiresByteSwap(),
			sequence->RowPadding()
		};
		self->m_Context = Pool::GetInstance().AquireContext(format);
		//self->m_Context->Clear();

		// init the ring buffer:
		// - map all the upload buffers and send them to the uploader thread.
		try{
			for(auto i=0;i<ringBufferSize;i++){
				auto frame = self->m_Context->GetFrame();
				frame->Reset();
				self->m_ReadyToUpload.send(frame);
			}
		}catch(std::exception & e){
			self->ChangeStatus(Error,e.what());
			return;
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
			auto in_system=false;
			std::set<size_t> s_system;
			auto dropped_now=0;
			do{
				if(!nextFrameChannel->recv(*currentFrame)){
					return;
				}
				(dropped_now)+=1;
				s_system.clear();
				std::lock_guard<std::mutex> lock(*systemFramesMutex);
				s_system.insert( systemFrames->begin(), systemFrames->end() );
			}while(s_system.find(*currentFrame)==s_system.end());
			dropped_now-=1;
			*dropped += dropped_now;

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

			auto offset = (sequence->RowPitch()+sequence->RowPadding()*4)*4-sequence->DataOffset();
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
	m_WaiterThread = std::thread([running,readyToWait,readyToRate,readyToRender,readyToUpload,avgDecodeDuration,internalRate,fps,systemFrames,systemFramesMutex,dropped]{
		auto start = HighResClock::now();
		std::shared_ptr<Frame> nextFrame;
		while(readyToWait->recv(nextFrame)){
			std::set<size_t> s_system;
			{
				std::lock_guard<std::mutex> lock(*systemFramesMutex);
				s_system.insert( systemFrames->begin(), systemFrames->end() );
			}
			if(s_system.find(nextFrame->NextToLoad())==s_system.end()){
				nextFrame->Cancel();
				nextFrame->Reset();
				readyToUpload->send(nextFrame);
				(*dropped)++;
			}else{
				auto ontime = nextFrame->Wait(INFINITE);//1000.0 / (double)*fps * (RING_BUFFER_SIZE/2));
				*avgDecodeDuration = std::chrono::milliseconds(std::chrono::duration_cast<std::chrono::milliseconds>(nextFrame->DecodeDuration()).count()/(readyToWait->size()+1));
				if(*internalRate){
					readyToRate->send(nextFrame);
				}else{
					readyToRender->send(nextFrame);
				}
			}
		}
	});


	// rate thread: receives frames already in the GPU and controls
	// the playback rate, once a frame is on time to be shown it sends it
	// to the main thread
	m_RateThreadRunning = true;
	running = &m_RateThreadRunning;
	m_RateThread = std::thread([running,readyToRate,readyToUpload,readyToRender,fps,internalRate] {
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

void DX11Player::Update(){
	if(!m_UploaderThreadRunning) return;
	std::shared_ptr<Frame> frame;
	auto now = HighResClock::now();
	HighResClock::duration currentLatency;
	std::vector<std::shared_ptr<Frame>> receivedFrames;
	if(m_InternalRate){
		bool late = true;
		auto max_lateness = m_AvgPipelineLatency + std::chrono::milliseconds(2);
		while(late && m_ReadyToRender.try_recv(frame)){
			currentLatency += now - frame->LoadTime();
			receivedFrames.push_back(frame);
			late = IsFrameLate(frame->PresentationTime(),abs(m_Fps),now,max_lateness);
		}

		if(!receivedFrames.empty()){
			m_GotFirstFrame=true;
			ChangeStatus(Status::FirstFrame,"FirstFrame");
			m_AvgPipelineLatency = std::chrono::nanoseconds(std::chrono::duration_cast<std::chrono::nanoseconds>(currentLatency).count() / receivedFrames.size());
			auto frame = receivedFrames.back();
			frame->Render();
			m_NextRenderFrame = frame->NextToLoad();
			m_DroppedFrames+=int(receivedFrames.size())-1;
			for(auto frame: receivedFrames){
				frame->Reset();
				m_ReadyToUpload.send(frame);
			}
		}
	}else{
		while(m_ReadyToRender.try_recv(frame)){
			currentLatency += now - frame->LoadTime();
			receivedFrames.push_back(frame);
		}
		std::set<size_t> s_system( m_SystemFrames.begin(), m_SystemFrames.end() );
		if(!receivedFrames.empty()){
			m_GotFirstFrame=true;
			ChangeStatus(Status::FirstFrame,"FirstFrame");
			m_AvgPipelineLatency = std::chrono::nanoseconds(std::chrono::duration_cast<std::chrono::nanoseconds>(currentLatency).count() / receivedFrames.size());
			for(auto & frame: receivedFrames){
				if(s_system.find(frame->NextToLoad())==s_system.end()){
					m_DroppedFrames++;
					frame->Reset();
					m_ReadyToUpload.send(frame);
				}else{
					auto prev_frame = m_WaitingToPresent.find(frame->NextToLoad());
					if(prev_frame!=m_WaitingToPresent.end()){
						prev_frame->second->Reset();
						m_ReadyToUpload.send(prev_frame->second);
						m_WaitingToPresent.erase(prev_frame);
					}
					m_WaitingToPresent[frame->NextToLoad()] = frame;
				}
			}
		}
		for(auto frame = m_WaitingToPresent.begin(); frame!=m_WaitingToPresent.end();){
			if(s_system.find(frame->first)==s_system.end()){
				frame->second->Reset();
				m_ReadyToUpload.send(frame->second);
				m_WaitingToPresent.erase(frame++);
			}else{
				++frame;
			}
		}
	}

}

void DX11Player::ChangeStatus(Status code, const std::string & status){
	if(code!=m_StatusCode){
		m_StatusCode = code;
		OutputDebugStringA(status.c_str());
	}
	m_StatusDesc = status;
}

DX11Player::Status DX11Player::GetStatus(){
	return m_StatusCode;
}

std::string DX11Player::GetStatusMessage(){
	return m_StatusDesc;
}

HANDLE DX11Player::GetSharedHandle(int nextFrame){
	m_LastUpdateTime = HighResClock::now();
	if(m_WaitingToPresent.empty()){
		return nullptr;
	}
	auto nextSequencedFrame = nextFrame % m_Sequence->NumImages();

	std::set<size_t> s_current( m_SystemFrames.begin(), m_SystemFrames.end() );
	for(auto frame = m_WaitingToPresent.begin(); frame!=m_WaitingToPresent.end();){
		if(frame->first != nextSequencedFrame && s_current.find(frame->first)==s_current.end()){
			frame->second->Reset();
			m_ReadyToUpload.send(frame->second);
			m_WaitingToPresent.erase(frame++);
		}else{
			++frame;
		}
	}
	
	auto next = m_WaitingToPresent.find(nextSequencedFrame);
	if(next==m_WaitingToPresent.end()){
		return nullptr;	
	}
	m_NextRenderFrame = nextSequencedFrame;
	if(!next->second->IsReadyToPresent()){
		next->second->Render();
	}
	return next->second->RenderTextureSharedHandle();
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
	return (int)m_WaitingToPresent.size();
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
	if(!m_UploaderThreadRunning){
		std::stringstream str;
		str << "trying to send load frame  " << nextFrame << " before ready " << std::endl;
		OutputDebugStringA(str.str().c_str());
		return;
	}

	auto sequencedFrameNum = nextFrame%m_Sequence->NumImages();
	{
		std::lock_guard<std::mutex> lock(m_SystemFramesMutex);
		m_SystemFrames.push_back(sequencedFrameNum);
		while(m_SystemFrames.size()>m_RingBufferSize){
			m_SystemFrames.erase(m_SystemFrames.begin());
		}
	}
	m_NextFrameChannel.send(sequencedFrameNum);
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
	return m_UploaderThreadRunning && (m_StatusCode!=Error);
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

	NATIVE_API void DX11Player_Update(DX11HANDLE player)
	{
		static_cast<DX11Player*>(player)->Update();
	}

	NATIVE_API HANDLE DX11Player_GetSharedHandle(DX11HANDLE player,int nextFrame)
	{
		return static_cast<DX11Player*>(player)->GetSharedHandle(nextFrame);
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

	NATIVE_API int DX11Player_GetContextPoolSize(){
		return Pool::GetInstance().Size();
	}

}
