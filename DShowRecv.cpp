#include "Env.h"
#include "Contents.h"
#include "DShowRecv.h"
#include <dshow.h>
#include <qedit.h>
#include <windows.h>
#pragma comment(lib, "strmiids.lib")

DShowRecv dshowRecv;


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

	// 1. フィルタグラフ作成
	CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IGraphBuilder, (void **)&pGraph);

	// 2. ソースフィルタ（カメラ）の取得
#if 0
	pSrc = FindSrc(cameraName);
#else
	static CMySrc mySrc;
	pSrc = &mySrc;
#endif
	if (!pSrc) return false;
	pGraph->AddFilter(pSrc, L"Video Capture");

	// 4. コールバックの設定
	// 4-1. サンプルグラバの生成
	IBaseFilter *pF = NULL;
	ISampleGrabber *pSGrab;
	CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (LPVOID *)&pF);
	pF->QueryInterface(IID_ISampleGrabber, (void **)&pSGrab);

	// 4-3. フィルタグラフへ追加
	pGraph->AddFilter(pF, L"Grabber");

	// 4-4. サンプルグラバの接続
	// [pSrc](o) -> (i)[pF](o) -> [VideoRender]
	//       ↑A     ↑B       ↑C
	IPin *pSrcOut = GetPin(pSrc, PINDIR_OUTPUT);	// A
	IPin *pSGrabIN = GetPin(pF, PINDIR_INPUT);		// B
	pGraph->Connect(pSrcOut, pSGrabIN);

	// 4-5. グラバのモードを適切に設定
	pSGrab->SetBufferSamples(FALSE);
	pSGrab->SetOneShot(FALSE);
	pSGrab->SetCallback(&callBack, 0);  // 第2引数でコールバックを指定 (0:SampleCB, 1:BufferCB)

	// 5. キャプチャ開始
	pGraph->QueryInterface(IID_IMediaControl, (void **)&pMediaControl);
	pMediaControl->Run();

	return true;
}
void DShowRecv::Release(){
	// 6. 終了
	if(pSrc) pSrc->Release();
	if(pMediaControl) pMediaControl->Release();
	if(pGraph) pGraph->Release();
	pSrc = NULL;
	pMediaControl = NULL;
	pGraph = NULL;
	CoUninitialize();
}

void DShowRecv::Set(){
	static CMyMediaSample ms;
	CMySrc* src = (CMySrc*)pSrc;	
	src->pin.toMem->Receive(&ms);
}
