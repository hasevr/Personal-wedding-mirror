#include "Env.h"
#include "Contents.h"
#include "DShowRecv.h"
#include "Packet.h"
#include <dshow.h>
#include <qedit.h>
#include <windows.h>
#pragma comment(lib, "strmiids.lib")

DShowRecv dshowRecv;

CMySrcRecv::CMySrcRecv():pAlloc(NULL), pSample(NULL), bufferSize(0){
	bStopThread = false;
}
CMySrcRecv::~CMySrcRecv(){
	if(pSample) pSample->Release();
	pSample=NULL;
}
DWORD WINAPI thread(void* p){
	CMySrcRecv* This = (CMySrcRecv*)p;
	while(!This->bStopThread){
		This->Recv();
	}
	This->bStopThread = false;
	return 0;
}
void CMySrcRecv::Init(){
	sockRecv.Init(AF_INET, SOCK_DGRAM, 0);
	WBSockAddr adr;
	adr.AdrIn().sin_family = AF_INET;
	adr.AdrIn().sin_port = htons(PORT_SEND);
	adr.AdrIn().sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sockRecv, (LPSOCKADDR)&adr, sizeof(adr))==SOCKET_ERROR){
		int error = WSAGetLastError();
		closesocket(sockRecv);
		sockRecv = INVALID_SOCKET;
		exit(0);
	}
	CreateThread(NULL, 0, thread, this, 0, NULL);
}
void CMySrcRecv::StopThread(){
	bStopThread = true;
	for(int i=0; i<100&&bStopThread; ++i) Sleep(100);
}

bool CMySrcRecv::Recv(){
	unsigned char buf[2048];
	WBSockAddr adr;
	sockRecv.RecvFrom(buf, sizeof(buf), adr);
	if (buf[0] == pdata.packetId[0]){
		if (pSample){
			BYTE* ptr=NULL;
			pSample->GetPointer(&ptr);
			if (ptr){
				PMediaData* pData = (PMediaData*)buf;
				memcpy(ptr + pData->count*1024, pData->data, 1024);
			}
		}
	}else if(buf[0] == ptype.packetId[0]){	//	���f�B�A�^�C�v�ƒ���	
		memcpy(&ptype, buf, sizeof(ptype));
		//	���f�B�A�^�C�v�̒ǉ�
		unsigned i;
		for(i=0; i<pin.enumMedia.mts.size(); ++i){
			if (memcmp(&pin.enumMedia.mts[i], &ptype.mt, sizeof(AM_MEDIA_TYPE)-8)==0) break;
		}
		if (i == pin.enumMedia.mts.size()) {
			CMediaType mt;
			memcpy(&mt, &ptype.mt, sizeof(AM_MEDIA_TYPE));
			mt.pbFormat = (BYTE*)CoTaskMemAlloc(ptype.mt.cbFormat);
			memcpy(mt.pbFormat, ptype.format, ptype.mt.cbFormat);
			pin.enumMedia.mts.push_back(mt);
		}

		//	1��O�̃o�b�t�@�𑗐M
		FILTER_STATE state=State_Stopped;
		GetState(100, &state);
		if (ptype.length && pin.isConnected && state == State_Running && pSample){
			REFERENCE_TIME time, endTime;
			pClock->GetTime(&time);
			endTime = time+10000000/30;
			pSample->SetTime(&time, &endTime);
			HRESULT hr = pin.toMem->Receive(pSample);
			if (hr!=S_OK){
				TCHAR buf[1024];
				AMGetErrorText(hr, buf, sizeof(buf));
				DSTR << "CMySrcRecv::Recv(): " << buf << std::endl;
			}
		}
		
		//	���̃o�b�t�@�̒������擾
		if (bufferSize != ptype.bufferSize && pin.toMem){
			assert(!pAlloc);
			HRESULT hr = pin.toMem->GetAllocator(&pAlloc);
			ALLOCATOR_PROPERTIES prop = {4, 0, 8, 0}, act={0,0,8,0};
			prop.cbBuffer = ptype.bufferSize;
			hr = pAlloc->SetProperties(&prop, &act);
			hr = pin.toMem->NotifyAllocator(pAlloc, false);
			hr = pAlloc->Commit();
			bufferSize = ptype.bufferSize;
		}
		if(pAlloc && pClock){
			if (pSample){
				pSample->Release();
				pSample=NULL;
			}
			HRESULT hr = pAlloc->GetBuffer(&pSample, NULL, NULL, 0);
			if (hr != S_OK){
				TCHAR buf[1024];
				AMGetErrorText(hr, buf, sizeof(buf));
				DSTR << "CMySrcRecv::Recv(): " << buf << std::endl;				
			}
			if (pSample){
				pSample->SetActualDataLength(ptype.length);
				pSample->SetDiscontinuity(false);
				pSample->SetPreroll(false);
				pSample->SetSyncPoint(true);
			}
		}
		return false;
	}else if(buf[0] == ptype.packetId[0]){
	}
	return true;
}

#if 0
STDMETHODIMP CMySampleRecv::GetMediaType(AM_MEDIA_TYPE ** ppMedia){
	*ppMedia = &pSrc->ptype.mt;
	return S_OK;
}
STDMETHODIMP CMySampleRecv::GetPointer(BYTE** pBuf){
	*pBuf = pSrc->pMediaBuf;
	return S_OK;
}
STDMETHODIMP_(long) CMySampleRecv::GetSize(){
	return pSrc->plen.length;
}
#endif


//----------------------------------------------------------------------------------------
//	CMySGCBRecv
//
STDMETHODIMP CMySGCBRecv::SampleCB( double SampleTime, IMediaSample * pSample ){
	//DSTR << "CMySGCBRecv::SampleCB( time:" << SampleTime << "); called." << std::endl;
	if (contents.mode == Contents::CO_CAM){
		BYTE* pBuf;
		HRESULT hr = pSample->GetPointer(&pBuf);
		if (hr==S_OK){
			contents.Capture(pBuf, pSample->GetSize());
		}
	}
	return S_OK;
}

STDMETHODIMP CMySGCBRecv::BufferCB( double dblSampleTime, BYTE * pBuffer, long lBufferSize ){
//	DSTR << "CMySGCBRecv::BufferCB( time:" << dblSampleTime << " len:" << lBufferSize << "); called." << std::endl;
	if (contents.mode == Contents::CO_CAM) contents.Capture(pBuffer, lBufferSize);
	return S_OK;
}

//---------------------------------------------------------------------------------------------


bool DShowRecv::Init(char* cameraName){
	CoInitialize(NULL);

	// 1. �t�B���^�O���t�쐬
	CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IGraphBuilder, (void **)&pGraph);
	AddToRot(pGraph, &rotId);

	// 2. �\�[�X�t�B���^�i�J�����j�̎擾
#if 0
	pSrc = FindSrc(cameraName);
#else
	pSrc = &mySrc;
	mySrc.Init();
#endif
	if (!pSrc) return false;
	pGraph->AddFilter(pSrc, L"Video Capture");

	// 4. �R�[���o�b�N�̐ݒ�
	// 4-1. �T���v���O���o�̐���
	IBaseFilter *pF = NULL;
	ISampleGrabber *pSGrab;
	CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (LPVOID *)&pF);
	pF->QueryInterface(IID_ISampleGrabber, (void **)&pSGrab);
	AM_MEDIA_TYPE mt;
	ZeroMemory(&mt, sizeof(mt));
	mt.majortype = MEDIATYPE_Video;
	mt.subtype = MEDIASUBTYPE_RGB24;
	pSGrab->SetMediaType(&mt);

	// 4-3. �t�B���^�O���t�֒ǉ�
	pGraph->AddFilter(pF, L"Grabber");
	for(int i=0; i<100&& !mySrc.pin.enumMedia.mts.size(); ++i){
		Sleep(10);
	}
	// 4-4. �T���v���O���o�̐ڑ�
	// [pSrc](o) -> (i)[pF](o) -> [VideoRender]
	//       ��A     ��B       ��C
	IPin *pSrcOut = GetPin(pSrc, PINDIR_OUTPUT);	// A
	IPin *pSGrabIN = GetPin(pF, PINDIR_INPUT);		// B
	IPin *pSGrabOut = GetPin(pF, PINDIR_OUTPUT);	// C
	HRESULT hr = pGraph->Connect(pSrcOut, pSGrabIN);
	if (hr){
		TCHAR buf[1024];
		AMGetErrorText(hr, buf, sizeof(buf));
		DSTR << "DShowRecv Connect() Failed:" << buf << std::endl;
	}
//	pGraph->Render(pSGrabOut);

	// 4-5. �O���o�̃��[�h��K�؂ɐݒ�
	pSGrab->SetBufferSamples(FALSE);
	pSGrab->SetOneShot(FALSE);
	pSGrab->SetCallback(&callBack, 0);  // ��2�����ŃR�[���o�b�N���w�� (0:SampleCB, 1:BufferCB)

	// 5. �L���v�`���J�n
	pGraph->QueryInterface(IID_IMediaControl, (void **)&pMediaControl);
	hr = pMediaControl->Run();
	if (hr != S_OK){
		TCHAR buf[1024];
		AMGetErrorText(hr, buf, sizeof(buf));
		DSTR << "DShowRecv Run() Failed:" << buf << std::endl;
	}

	return true;
}
void DShowRecv::Release(){
	mySrc.StopThread();
	// 6. �I��
	if(pSrc) pSrc->Release();
	if(pMediaControl) pMediaControl->Release();
	if(pGraph) pGraph->Release();
	pSrc = NULL;
	pMediaControl = NULL;
	pGraph = NULL;
	CoUninitialize();
}

void DShowRecv::Set(){
//	while(mySrc.Recv());
}
