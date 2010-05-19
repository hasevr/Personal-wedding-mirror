#include "Env.h"
#include "Contents.h"
#include "DShowRecv.h"
#include "Packet.h"
#include <dshow.h>
#include <qedit.h>
#include <windows.h>
#pragma comment(lib, "strmiids.lib")

DShowRecv dshowRecv;

CMySrcRecv::CMySrcRecv():pAlloc(NULL), pSample(NULL), bufferSize(0), hThread(NULL){
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
	hThread = CreateThread(NULL, 0, thread, this, 0, NULL);
	SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
}
void CMySrcRecv::StopThread(){
	if (hThread){
		bStopThread = true;
		for(int i=0; i<100&&bStopThread; ++i) Sleep(100);
	}
	if (!bStopThread) hThread = NULL;
}

static bool flags[1000];
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
				memcpy(ptr + pData->count*PMediaData::DATALEN, pData->data, PMediaData::DATALEN);
				flags[pData->count] = true;
			}
		}
	}else if(buf[0] == ptype.packetId[0]){	//	メディアタイプと長さ	
		memcpy(&ptype, buf, sizeof(ptype));
		//	メディアタイプの追加
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

		//	1回前のバッファを送信
		bool bOK= true;
		for(int i=0; i<(ptype.length+PMediaData::DATALEN-1)/PMediaData::DATALEN ; ++i){
			if (!flags[i]){
				std::cout << i << " ";
				bOK = false;
			}
			flags[i] = false;
		}
		std::cout << "/" << (ptype.length+PMediaData::DATALEN-1)/PMediaData::DATALEN << std::endl;
		FILTER_STATE state=State_Stopped;
		GetState(100, &state);
		if (bOK && ptype.length && pin.isConnected && state == State_Running && pSample){
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
		
		//	次のバッファの長さを取得
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
				hr = pAlloc->Commit();
				hr = pAlloc->GetBuffer(&pSample, NULL, NULL, 0);
			}
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

DShowRecv::DShowRecv():	pSrcOut(NULL), pSGrabIn(NULL), pSGrabOut(NULL), pSGF(NULL), pSGrab(NULL){
	bGood = false;
}

bool DShowRecv::Init(){
	if(!pGraph){	
		CoInitialize(NULL);

		// 1. フィルタグラフ作成
		CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IGraphBuilder, (void **)&pGraph);
		AddToRot(pGraph, &rotId);
	}

	// 2. ソースフィルタ（カメラ）の取得
	if (!pSrc){
		//	単体動作時
		//	"IP Camera [JPEG/MJPEG]", "Logicool Qcam Pro 9000"
	//	pSrc = FindSrc("Logicool Qcam Pro 9000");
		//	カメラがなければ、ネット動作する
		if (!pSrc){
			pSrc = &mySrc;
			mySrc.Init();
		}
	}
	if (!pSrc) return false;
	pGraph->AddFilter(pSrc, L"Video Capture");

	// 4. コールバックの設定
	// 4-1. サンプルグラバの生成
	if (!pSGrab){
		CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (LPVOID *)&pSGF);
		pSGF->QueryInterface(IID_ISampleGrabber, (void **)&pSGrab);
		AM_MEDIA_TYPE mt;
		ZeroMemory(&mt, sizeof(mt));
		mt.majortype = MEDIATYPE_Video;
		mt.subtype = MEDIASUBTYPE_RGB24;
		pSGrab->SetMediaType(&mt);

		// 4-3. フィルタグラフへ追加
		pGraph->AddFilter(pSGF, L"Grabber");
	}

	// 4-4. サンプルグラバの接続
	// [pSrc](o) -> (i)[pSGrab](o) -> [VideoRender]
	//       ↑A     ↑B       ↑C
	if (!pSrcOut) pSrcOut = GetPin(pSrc, PINDIR_OUTPUT);		// A
	if (!pSGrabIn) pSGrabIn = GetPin(pSGF, PINDIR_INPUT);		// B
	if (!pSGrabOut) pSGrabOut = GetPin(pSGF, PINDIR_OUTPUT);		// C
	HRESULT hr = pGraph->Connect(pSrcOut, pSGrabIn);
	if (hr){
		TCHAR buf[1024];
		AMGetErrorText(hr, buf, sizeof(buf));
		DSTR << "DShowRecv Connect() Failed:" << buf << std::endl;
		return false;
	}else{
	//	pGraph->Render(pSGrabOut);

		// 4-5. グラバのモードを適切に設定
		pSGrab->SetBufferSamples(FALSE);
		pSGrab->SetOneShot(FALSE);
		pSGrab->SetCallback(&callBack, 0);  // 第2引数でコールバックを指定 (0:SampleCB, 1:BufferCB)

		// 5. キャプチャ開始
		pGraph->QueryInterface(IID_IMediaControl, (void **)&pMediaControl);
		hr = pMediaControl->Run();
		if (hr != S_OK){
			TCHAR buf[1024];
			AMGetErrorText(hr, buf, sizeof(buf));
			DSTR << "DShowRecv Run() Failed:" << buf << std::endl;
		}
		bGood = true;
		return true;
	}
}
void DShowRecv::Release(){
	mySrc.StopThread();
	// 6. 終了
	if(pSrc) pSrc->Release();
	if(pMediaControl) pMediaControl->Release();
	if(pGraph) pGraph->Release();
	pSrc = NULL;
	pMediaControl = NULL;
	pGraph = NULL;
	CoUninitialize();
}
