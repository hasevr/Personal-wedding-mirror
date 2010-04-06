#include "Env.h"
#include "Contents.h"
#include "DShowCap.h"
#include <dshow.h>
#include <qedit.h>
#include <windows.h>
#pragma comment(lib, "strmiids.lib")

DShowCap dshowCap;


CMySampleGrabberCB::CMySampleGrabberCB(){}
CMySampleGrabberCB::~CMySampleGrabberCB(){}
void CMySampleGrabberCB::SetImage(unsigned char * pBuffer,int nBufferSize){
	DSTR << "CMySampleGrabberCB::SetImage(pBuffer, " << nBufferSize << "); called." << std::endl; 
}
STDMETHODIMP_(ULONG) CMySampleGrabberCB::AddRef() { return 2; }
STDMETHODIMP_(ULONG) CMySampleGrabberCB::Release() { return 1; }
STDMETHODIMP CMySampleGrabberCB::QueryInterface(REFIID riid, void ** ppv){
	if( riid == IID_ISampleGrabberCB || riid == IID_IUnknown ){
		*ppv = (void *) static_cast<ISampleGrabberCB*> ( this );
		return NOERROR;
	}
	return E_NOINTERFACE;
}
STDMETHODIMP CMySampleGrabberCB::SampleCB( double SampleTime, IMediaSample * pSample ){
	DSTR << "CMySampleGrabberCB::SampleCB( time:" << SampleTime << "); called." << std::endl;
	BYTE* pBuf;
	HRESULT hr = pSample->GetPointer(&pBuf);
	if (hr==S_OK){
		contents.Capture(pBuf, pSample->GetSize());
	}
	return S_OK;
}

STDMETHODIMP CMySampleGrabberCB::BufferCB( double dblSampleTime, BYTE * pBuffer, long lBufferSize ){
//	DSTR << "CMySampleGrabberCB::BufferCB( time:" << dblSampleTime << " len:" << lBufferSize << "); called." << std::endl;
	contents.Capture(pBuffer, lBufferSize);
	return S_OK;
}


IPin *GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir){
	BOOL			 bFound = FALSE;
	IEnumPins	*pEnum;
	IPin			 *pPin;

	pFilter->EnumPins(&pEnum);
	while(pEnum->Next(1, &pPin, 0) == S_OK)
		{
			PIN_DIRECTION PinDirThis;
			pPin->QueryDirection(&PinDirThis);
			if (bFound = (PinDir == PinDirThis)) // 引数で指定した方向のピンならbreak
				break;
			pPin->Release();
		}
	pEnum->Release();
	return (bFound ? pPin : 0);
}
DShowCap::DShowCap(){
	pSrc = NULL;
	pMediaControl = NULL;
	pBuilder = NULL;
	pGraph = NULL;
}
bool DShowCap::Init(char* cameraName){
	CoInitialize(NULL);

	// 1. フィルタグラフ作成
	CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IGraphBuilder, (void **)&pGraph);

	// 2. システムデバイス列挙子を作成
	ICreateDevEnum *pDevEnum = NULL;
	CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, IID_ICreateDevEnum, (void **)&pDevEnum);

	IEnumMoniker *pClassEnum = NULL;
	pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pClassEnum, 0);

	const int N_ENUM=100;
	IMoniker *pMonikers[N_ENUM];
	ULONG nMoniker;
	pClassEnum->Next(N_ENUM, pMonikers, &nMoniker);
	for(unsigned i=0; i<nMoniker; ++i){
		// フィルタのフレンドリ名を取得
		IPropertyBag *pPropBag=NULL;
        HRESULT hr = pMonikers[i]->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
		VARIANT varName;
		VariantInit(&varName);
		hr = pPropBag->Read(L"FriendlyName", &varName, 0);
		char filterName[1024];
		if (SUCCEEDED(hr)){
			int len = SysStringLen(varName.bstrVal);
			int n=WideCharToMultiByte(CP_ACP, 0, varName.bstrVal, len, filterName, sizeof(filterName), NULL, NULL);
			filterName[n] = '\0';
		}
		VariantClear(&varName);
		pPropBag->Release();
		DSTR << "Enum Filter:" << filterName << std::endl;
		//	名前を確認してバインド
		if (!pSrc && (cameraName==NULL || strcmp(filterName, cameraName) == 0)){
			// モニカをフィルタオブジェクトにバインドする
			pMonikers[i]->BindToObject(0, 0, IID_IBaseFilter, (void **)&pSrc);
		}
		pMonikers[i]->Release();
	}
	pClassEnum->Release();
	pDevEnum->Release();
	if (!pSrc) return false;

	// フィルタを列挙し、それらのプロパティ ページを表示する。
	ISpecifyPropertyPages *pSpecify;
	HRESULT hr = pSrc->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pSpecify);
	if (SUCCEEDED(hr)) {
		FILTER_INFO FilterInfo;
		pSrc->QueryFilterInfo(&FilterInfo);

		CAUUID caGUID;
		pSpecify->GetPages(&caGUID);
		pSpecify->Release();

		OleCreatePropertyFrame(
			NULL,									 // 親ウィンドウ
			0,											// x (予約済み)
			0,											// y (予約済み)
			FilterInfo.achName,		 // ダイアログ ボックスのキャプション
			1,											// フィルタの数
			(IUnknown **)&pSrc,	// フィルタへのポインタ
			caGUID.cElems,					// プロパティ ページの数
			caGUID.pElems,					// プロパティ ページ CLSID へのポインタ
			0,											// ロケール識別子
			0,											// 予約済み
			NULL										// 予約済み
			);
		CoTaskMemFree(caGUID.pElems);
		if (FilterInfo.pGraph) FilterInfo.pGraph->Release();
	}
	pGraph->AddFilter(pSrc, L"Video Capture");

	// 3. キャプチャビルダの作成
	CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC, IID_ICaptureGraphBuilder2, (void **)&pBuilder);
	pBuilder->SetFiltergraph(pGraph);
  

	// 4. 一枚撮る
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
//	IPin *pSGrabOut = GetPin(pF, PINDIR_OUTPUT);	// C
	pGraph->Connect(pSrcOut, pSGrabIN);
//	pGraph->Render(pSGrabOut);

	// 4-5. グラバのモードを適切に設定
	pSGrab->SetBufferSamples(FALSE);
	pSGrab->SetOneShot(FALSE);
	pSGrab->SetCallback(&callBack, 0);  // 第2引数でコールバックを指定 (0:SampleCB, 1:BufferCB)

	// ディスプレイへ
//	pBuilder->RenderStream( &PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, pSrc, NULL, NULL );

	// 5. キャプチャ開始
	pGraph->QueryInterface(IID_IMediaControl, (void **)&pMediaControl);
	pMediaControl->Run();

	return true;
}
void DShowCap::Release(){
	// 6. 終了
	pSrc->Release();
	pMediaControl->Release();
	pBuilder->Release();
	pGraph->Release();
	pSrc = NULL;
	pMediaControl = NULL;
	pBuilder = NULL;
	pGraph = NULL;
	CoUninitialize();
}
