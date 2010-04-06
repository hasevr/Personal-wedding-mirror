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
			if (bFound = (PinDir == PinDirThis)) // �����Ŏw�肵�������̃s���Ȃ�break
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

	// 1. �t�B���^�O���t�쐬
	CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IGraphBuilder, (void **)&pGraph);

	// 2. �V�X�e���f�o�C�X�񋓎q���쐬
	ICreateDevEnum *pDevEnum = NULL;
	CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, IID_ICreateDevEnum, (void **)&pDevEnum);

	IEnumMoniker *pClassEnum = NULL;
	pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pClassEnum, 0);

	const int N_ENUM=100;
	IMoniker *pMonikers[N_ENUM];
	ULONG nMoniker;
	pClassEnum->Next(N_ENUM, pMonikers, &nMoniker);
	for(unsigned i=0; i<nMoniker; ++i){
		// �t�B���^�̃t�����h�������擾
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
		//	���O���m�F���ăo�C���h
		if (!pSrc && (cameraName==NULL || strcmp(filterName, cameraName) == 0)){
			// ���j�J���t�B���^�I�u�W�F�N�g�Ƀo�C���h����
			pMonikers[i]->BindToObject(0, 0, IID_IBaseFilter, (void **)&pSrc);
		}
		pMonikers[i]->Release();
	}
	pClassEnum->Release();
	pDevEnum->Release();
	if (!pSrc) return false;

	// �t�B���^��񋓂��A�����̃v���p�e�B �y�[�W��\������B
	ISpecifyPropertyPages *pSpecify;
	HRESULT hr = pSrc->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pSpecify);
	if (SUCCEEDED(hr)) {
		FILTER_INFO FilterInfo;
		pSrc->QueryFilterInfo(&FilterInfo);

		CAUUID caGUID;
		pSpecify->GetPages(&caGUID);
		pSpecify->Release();

		OleCreatePropertyFrame(
			NULL,									 // �e�E�B���h�E
			0,											// x (�\��ς�)
			0,											// y (�\��ς�)
			FilterInfo.achName,		 // �_�C�A���O �{�b�N�X�̃L���v�V����
			1,											// �t�B���^�̐�
			(IUnknown **)&pSrc,	// �t�B���^�ւ̃|�C���^
			caGUID.cElems,					// �v���p�e�B �y�[�W�̐�
			caGUID.pElems,					// �v���p�e�B �y�[�W CLSID �ւ̃|�C���^
			0,											// ���P�[�����ʎq
			0,											// �\��ς�
			NULL										// �\��ς�
			);
		CoTaskMemFree(caGUID.pElems);
		if (FilterInfo.pGraph) FilterInfo.pGraph->Release();
	}
	pGraph->AddFilter(pSrc, L"Video Capture");

	// 3. �L���v�`���r���_�̍쐬
	CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC, IID_ICaptureGraphBuilder2, (void **)&pBuilder);
	pBuilder->SetFiltergraph(pGraph);
  

	// 4. �ꖇ�B��
	// 4-1. �T���v���O���o�̐���
	IBaseFilter *pF = NULL;
	ISampleGrabber *pSGrab;
	CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (LPVOID *)&pF);
	pF->QueryInterface(IID_ISampleGrabber, (void **)&pSGrab);

	// 4-3. �t�B���^�O���t�֒ǉ�
	pGraph->AddFilter(pF, L"Grabber");

	// 4-4. �T���v���O���o�̐ڑ�
	// [pSrc](o) -> (i)[pF](o) -> [VideoRender]
	//       ��A     ��B       ��C
	IPin *pSrcOut = GetPin(pSrc, PINDIR_OUTPUT);	// A
	IPin *pSGrabIN = GetPin(pF, PINDIR_INPUT);		// B
//	IPin *pSGrabOut = GetPin(pF, PINDIR_OUTPUT);	// C
	pGraph->Connect(pSrcOut, pSGrabIN);
//	pGraph->Render(pSGrabOut);

	// 4-5. �O���o�̃��[�h��K�؂ɐݒ�
	pSGrab->SetBufferSamples(FALSE);
	pSGrab->SetOneShot(FALSE);
	pSGrab->SetCallback(&callBack, 0);  // ��2�����ŃR�[���o�b�N���w�� (0:SampleCB, 1:BufferCB)

	// �f�B�X�v���C��
//	pBuilder->RenderStream( &PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, pSrc, NULL, NULL );

	// 5. �L���v�`���J�n
	pGraph->QueryInterface(IID_IMediaControl, (void **)&pMediaControl);
	pMediaControl->Run();

	return true;
}
void DShowCap::Release(){
	// 6. �I��
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
