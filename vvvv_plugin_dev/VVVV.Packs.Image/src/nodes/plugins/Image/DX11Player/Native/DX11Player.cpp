// Native.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "DX11Player.h"
#include "ImageFormat.h"
#include <sstream>
#include "Timer.h"
#include <WinIoCtl.h>
#include <Shlwapi.h>
#include <set>
#include <iostream>

#pragma comment(lib, "Kernel32.lib")


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

DX11Player::DX11Player(const std::string & fileForFormat, size_t ringBufferSize)
	:m_UploaderThreadRunning(false)
	,m_WaiterThreadRunning(false)
	,m_DroppedFrames(0)
	,m_RingBufferSize(ringBufferSize)
	,m_AvgDecodeDuration(0)
	,m_LastUpdateTime(HighResClock::now())
	,m_GotFirstFrame(false)
	,m_StatusCode(Init)
	,m_StatusDesc("Init")
	,m_AlwaysShowLastFrame(false)
{

	// uploader thread: initializes the whole pipeline and then
	// reads async from disk to mapped GPU memory
	// and sends an event to wait on to the waiter thread
	//OutputDebugString( L"creating upload thread\n" );
	auto running = &m_UploaderThreadRunning;
	auto currentFrame = &m_CurrentFrame;
	auto readyToUpload = &m_ReadyToUpload;
	auto readyToWait = &m_ReadyToWait;
	auto readyToRender = &m_ReadyToRender;
	auto nextFrameChannel = &m_NextFrameChannel;
	auto self = this;
	auto systemFrames = &m_SystemFrames;
	auto systemFramesMutex = &m_SystemFramesMutex;
	auto dropped = &m_DroppedFrames;
	m_UploaderThread = std::thread([dropped,systemFrames,systemFramesMutex,running,self,currentFrame,readyToUpload,readyToWait,nextFrameChannel,ringBufferSize,fileForFormat]{
		ImageFormat::FileInfo fileInfo;
		try{
			fileInfo = ImageFormat::FormatFor(fileForFormat);
		}catch(std::exception & e){
			self->ChangeStatus(Error,e.what());
			return;
		}

		// Add enough bytes to read the header + data but we need to read
		// a multiple of the physical sector size since we are reading 
		// directly from disk with no buffering
		if(!PathFileExistsA(fileForFormat.c_str())){
			self->ChangeStatus(Error,"Cannot find folder");
			return;
		}
		char buffer[4096] = {0};
		auto ret = GetFullPathNameA(fileForFormat.c_str(),4096,buffer,nullptr);
		if(ret==0){
			self->ChangeStatus(Error,"Cannot recover file for format drive");
			return;
		}
		auto sectorSize = SectorSize(buffer[0]);
		if(sectorSize<0){
			self->ChangeStatus(Error,"Cannot retrieve drive sector size");
			return;
		}
		auto numbytesdata = NextMultiple((DWORD)fileInfo.bytes_data + fileInfo.data_offset,(DWORD)sectorSize);

		self->m_Context = Pool::GetInstance().AquireContext(fileInfo.format);
		//self->m_Context->Clear();

		// init the ring buffer:
		// - map all the upload buffers and send them to the uploader thread.
		try{
			for(auto i=0;i<ringBufferSize;i++){
				auto frame = self->m_Context->GetFrame();
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
		std::shared_ptr<Frame> nextFrame;
		while(readyToUpload->recv(nextFrame)){
			auto now = HighResClock::now();
			auto in_system=false;
			std::set<std::string> s_system;
			std::string localCurrentFrame;
			auto dropped_now=0;
			do{
				if (!nextFrameChannel->recv(localCurrentFrame)){
					*running = false;
					return;
				}
				(dropped_now)+=1;
				s_system.clear();
				std::lock_guard<std::mutex> lock(*systemFramesMutex);
				s_system.insert( systemFrames->begin(), systemFrames->end() );
			} while (s_system.find(localCurrentFrame) == s_system.end() && !PathFileExistsA(localCurrentFrame.c_str()));
			dropped_now-=1;
			*dropped += dropped_now;
			*currentFrame = localCurrentFrame;

			auto offset = (fileInfo.row_pitch+fileInfo.format.row_padding*4)*4-fileInfo.data_offset;
			if(nextFrame->ReadFile(*currentFrame,offset,numbytesdata,now)){
				if(!readyToWait->send(make_pair(readyToWait->size()+1,nextFrame)))
				{
					*running = false;
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
	auto alwaysShowLastFrame = &m_AlwaysShowLastFrame;
	m_WaiterThread = std::thread([running, readyToWait, readyToRender, readyToUpload, avgDecodeDuration, systemFrames, systemFramesMutex, dropped, alwaysShowLastFrame]{
		std::pair<int,std::shared_ptr<Frame>> nextFrame;
		while (readyToWait->recv(nextFrame)){
			std::set<std::string> s_system;
			if (!alwaysShowLastFrame){
				std::lock_guard<std::mutex> lock(*systemFramesMutex);
				s_system.insert(systemFrames->begin(), systemFrames->end());
			}
			if (alwaysShowLastFrame || s_system.find(nextFrame.second->SourcePath()) != s_system.end()){
				auto ontime = nextFrame.second->Wait(INFINITE);
				if (ontime){ // since t=INIFINITE only false if read failed
					*avgDecodeDuration = std::chrono::milliseconds(std::chrono::duration_cast<std::chrono::milliseconds>(nextFrame.second->DecodeDuration()).count() / nextFrame.first);
					readyToRender->send(nextFrame.second);
				}else{
					readyToUpload->send(nextFrame.second);
					(*dropped)++;
				}
			}else{
				readyToUpload->send(nextFrame.second);
				(*dropped)++;
			}
		}
	});
}

DX11Player::~DX11Player(){
	m_WaiterThreadRunning = false;
	m_UploaderThreadRunning = false;
	m_ReadyToUpload.close();
	m_ReadyToRender.close();
	m_ReadyToWait.close();
	m_NextFrameChannel.close();
	m_UploaderThread.join();
	m_WaiterThread.join();
}

void DX11Player::Update(){
	if(!m_UploaderThreadRunning) return;
	auto now = HighResClock::now();
	HighResClock::duration currentLatency;
	std::vector<std::shared_ptr<Frame>> receivedFrames;
	std::shared_ptr<Frame> frame;
	std::shared_ptr<Frame> lastFrame;
	while(m_ReadyToRender.try_recv(frame)){
		currentLatency += now - frame->LoadTime();
		receivedFrames.push_back(frame);
	}
	std::set<std::string> s_system(m_SystemFrames.begin(), m_SystemFrames.end());
	if(!receivedFrames.empty()){
		m_GotFirstFrame=true;
		ChangeStatus(Status::FirstFrame,"FirstFrame");
		m_AvgPipelineLatency = std::chrono::nanoseconds(std::chrono::duration_cast<std::chrono::nanoseconds>(currentLatency).count() / receivedFrames.size());
		if (m_AlwaysShowLastFrame){
			lastFrame = receivedFrames.back();
			receivedFrames.pop_back();
		}
		for(auto & frame: receivedFrames){
			if (s_system.find(frame->SourcePath()) == s_system.end()){
				m_DroppedFrames++;
				m_ReadyToUpload.send(frame);
			}else{
				auto prev_frame = m_WaitingToPresent.find(frame->SourcePath());
				if(prev_frame!=m_WaitingToPresent.end()){
					m_ReadyToUpload.send(prev_frame->second);
					m_WaitingToPresent.erase(prev_frame);
				}
				m_WaitingToPresent[frame->SourcePath()] = frame;
			}
		}
	}
	for(auto frame = m_WaitingToPresent.begin(); frame!=m_WaitingToPresent.end();){
		if ((!m_AlwaysShowLastFrame || lastFrame || m_WaitingToPresent.size()>1) && s_system.find(frame->first) == s_system.end()){
			m_ReadyToUpload.send(frame->second);
			m_WaitingToPresent.erase(frame++);
		}else{
			++frame;
		}
	}
	if (lastFrame){
		if (m_WaitingToPresent.empty()){
			m_WaitingToPresent[lastFrame->SourcePath()] = lastFrame;
		}else{
			m_ReadyToUpload.send(lastFrame);
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

std::string DX11Player::GetStatusMessage() const{
	return m_StatusDesc;
}

std::string DX11Player::GetCurrentLoadFrame() const{
	return m_CurrentFrame;
}

std::string DX11Player::GetCurrentRenderFrame() const{
	return m_NextRenderFrame;
}

HANDLE DX11Player::GetSharedHandle(const std::string & nextFrame){
	m_LastUpdateTime = HighResClock::now();
	if(m_WaitingToPresent.empty()){
		return nullptr;
	}	
	auto next = m_WaitingToPresent.find(nextFrame);
	if(next==m_WaitingToPresent.end()){
		next = m_WaitingToPresent.begin();
		m_NextRenderFrame = next->second->SourcePath();
	}else{
		m_NextRenderFrame = nextFrame;
	}
	if(!next->second->IsReadyToPresent()){
		next->second->Render();
	}
	return next->second->RenderTextureSharedHandle();
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

int DX11Player::GetPresentBufferSize() const{
	if(!m_UploaderThreadRunning) return 0;
	return (int)m_WaitingToPresent.size();
}

int DX11Player::GetDroppedFrames() const
{
	return m_DroppedFrames;
}

int DX11Player::GetAvgLoadDurationMs() const
{
	return (int)std::chrono::duration_cast<std::chrono::milliseconds>(m_AvgDecodeDuration).count();
}

void DX11Player::SendNextFrameToLoad(const std::string & nextFrame)
{
	if(!m_UploaderThreadRunning){
		std::stringstream str;
		str << "trying to send load frame  " << nextFrame << " before ready " << std::endl;
		OutputDebugStringA(str.str().c_str());
		return;
	}
	if (m_NextFrameChannel.size() < m_RingBufferSize * 2){
		m_NextFrameChannel.send(nextFrame);
	}else{
		m_DroppedFrames++;
	}
}

void DX11Player::SetSystemFrames(std::vector<std::string> & frames){
	std::lock_guard<std::mutex> lock(m_SystemFramesMutex);
	std::swap(frames,m_SystemFrames);
}

bool DX11Player::IsReady() const{
	return m_UploaderThreadRunning && (m_StatusCode!=Error);
}

bool DX11Player::GotFirstFrame() const{
	return m_GotFirstFrame && m_StatusCode!=Error;
}

void DX11Player::SetAlwaysShowLastFrame(bool alwaysShowLastFrame){
	m_AlwaysShowLastFrame = alwaysShowLastFrame;
}


extern "C"{
	typedef void * DX11HANDLE;
	NATIVE_API DX11HANDLE DX11Player_Create(const char * fileForFormat, int ringBufferSize)
	{
		return new DX11Player(fileForFormat,ringBufferSize);
	}

	NATIVE_API void DX11Player_Destroy(DX11HANDLE player)
	{
		delete static_cast<DX11Player*>(player);
	}

	NATIVE_API void DX11Player_Update(DX11HANDLE player)
	{
		static_cast<DX11Player*>(player)->Update();
	}

	NATIVE_API HANDLE DX11Player_GetSharedHandle(DX11HANDLE player,const char * nextFrame)
	{
		return static_cast<DX11Player*>(player)->GetSharedHandle(nextFrame);
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

	NATIVE_API int DX11Player_GetPresentBufferSize(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetPresentBufferSize();
	}

	NATIVE_API int DX11Player_GetDroppedFrames(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetDroppedFrames();
	}

	NATIVE_API int DX11Player_GetAvgLoadDurationMs(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetAvgLoadDurationMs();
	}
	
	NATIVE_API void DX11Player_SendNextFrameToLoad(DX11HANDLE player, const char * nextFrame)
	{
		static_cast<DX11Player*>(player)->SendNextFrameToLoad(nextFrame);
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

	NATIVE_API const char * DX11Player_GetCurrentLoadFrame(DX11HANDLE player)
	{
		std::string str = static_cast<DX11Player*>(player)->GetCurrentLoadFrame();
		ULONG ulSize = str.size() + sizeof(char);
		char* pszReturn = NULL;

		pszReturn = (char*)::CoTaskMemAlloc(ulSize);
		// Copy the contents of szSampleString
		// to the memory pointed to by pszReturn.
		strcpy(pszReturn, str.c_str());
		// Return pszReturn.
		return pszReturn;
	}

	NATIVE_API const char * DX11Player_GetCurrentRenderFrame(DX11HANDLE player)
	{
		std::string str = static_cast<DX11Player*>(player)->GetCurrentRenderFrame();
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

	NATIVE_API void DX11Player_SetSystemFrames(DX11HANDLE player, char**frames, int numframes){
		std::vector<std::string> framesString(frames,frames+numframes);
		static_cast<DX11Player*>(player)->SetSystemFrames(framesString);
	}

	NATIVE_API void DX11Player_SetAlwaysShowLastFrame(DX11HANDLE player, bool always){
		static_cast<DX11Player*>(player)->SetAlwaysShowLastFrame(always);
	}
}
