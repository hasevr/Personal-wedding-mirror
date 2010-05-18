#include "DShowFilter.h"
#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "Quartz.lib") 
#include <Base/Affine.h>
#include <iostream>

//----------------------------------------------------------------------------------------
//	CMyMediaSample
//
CMediaType::CMediaType(){
	ZeroMemory(this, sizeof(*this));
}

CMediaType::CMediaType(const CMediaType& m){
	memcpy(this, &m, sizeof(m));
	if (m.pbFormat){
		pbFormat = (BYTE*)CoTaskMemAlloc(m.cbFormat);
		memcpy(pbFormat, m.pbFormat, m.cbFormat);
	}
}
CMediaType::CMediaType(const AM_MEDIA_TYPE& m){
	memcpy(this, &m, sizeof(m));
	if (m.pbFormat){
		pbFormat = (BYTE*)CoTaskMemAlloc(m.cbFormat);
		memcpy(pbFormat, m.pbFormat, m.cbFormat);
	}
}
CMediaType::~CMediaType(){
	CoTaskMemFree(pbFormat);
}
CMediaType& CMediaType::operator=(const AM_MEDIA_TYPE& m){
	CoTaskMemFree(pbFormat);
	memcpy(this, &m, sizeof(m));
	if (m.pbFormat){
		pbFormat = (BYTE*)CoTaskMemAlloc(m.cbFormat);
		memcpy(pbFormat, m.pbFormat, m.cbFormat);
	}
	return *this;
}

//----------------------------------------------------------------------------------------
//	CMyMediaSample
//
STDMETHODIMP CMyMediaSample::QueryInterface(REFIID riid, void ** ppv){
	if( riid == IID_IMediaSample || riid == IID_IUnknown ){
		*ppv = (void *) static_cast<IMediaSample*> ( this );
		return NOERROR;
	}
	return E_NOINTERFACE;
}


//----------------------------------------------------------------------------------------
//	CMyEnumMedia
//
CMyEnumMedia::~CMyEnumMedia(){
}
STDMETHODIMP CMyEnumMedia::GetClassID(CLSID *pClassID){
	memcpy(pClassID->Data4, "CMyEnumMedia", 8);
	return S_OK;
}
STDMETHODIMP CMyEnumMedia::QueryInterface(REFIID riid, void ** ppv){
	if( riid == IID_IEnumMediaTypes || riid == IID_IUnknown ){
		*ppv = (void *) static_cast<IEnumMediaTypes*> ( this );
		return NOERROR;
	}
	return E_NOINTERFACE;
}
STDMETHODIMP CMyEnumMedia::Next(ULONG c, AM_MEDIA_TYPE** pp, ULONG* pc){
	if (cur>=mts.size()){
		if (pc) *pc = 0;		
		return E_FAIL;
	}
	if (c>mts.size()-cur) c = mts.size()-cur;
	if (pc) *pc= c;
	if (pp){
		for(unsigned i=0; i<c; ++i){
			pp[i] = (AM_MEDIA_TYPE*)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
			memcpy(pp[i], &mts[cur+i], sizeof(AM_MEDIA_TYPE));
			pp[i]->pbFormat = (BYTE*)CoTaskMemAlloc(mts[cur+i].cbFormat);
			memcpy(pp[i]->pbFormat, mts[cur+i].pbFormat, mts[cur+i].cbFormat);
		}
	}	
	cur += c;
	return NOERROR;
}
//----------------------------------------------------------------------------------------
//	CMyPin
//
bool CMyPin::WaitForMediaType(){
	for(int i=0; i<10; ++i){
		if (enumMedia.mts.size()){
			if (i) std::cout << std::endl;
			return true;
		}
		if (i==0) std::cout << "CMyPin::WaitForMediaType() waiting ";
		else std::cout << ".";
		Sleep(10);
	}
	std::cout << " failed." << std::endl;
	return false;
}

CMyPin::CMyPin(CMySrc* s):pSrc(s), isConnected(false), to(NULL), toMem(NULL), enumMedia(this){
}
STDMETHODIMP CMyPin::ConnectionMediaType(AM_MEDIA_TYPE *pmt){ 
	if(pmt){
		if (WaitForMediaType()){
			memcpy(pmt, &enumMedia.mts[0], sizeof(enumMedia.mts[0]));
			if (pmt->pbFormat){
				pmt->pbFormat = (BYTE*)CoTaskMemAlloc(enumMedia.mts[0].cbFormat);
				memcpy(pmt->pbFormat, enumMedia.mts[0].pbFormat, enumMedia.mts[0].cbFormat);
			}
		}
		return S_OK;
	}
	return VFW_E_TIMEOUT;
}

STDMETHODIMP CMyPin::QueryInterface(REFIID riid, void ** ppv){
	if( riid == IID_IPin || riid == IID_IUnknown ){
		*ppv = (void *) static_cast<IPin*> ( this );
		return NOERROR;
	}
	return E_NOINTERFACE;
}
STDMETHODIMP CMyPin::EnumMediaTypes(IEnumMediaTypes **ppEnum){
	*ppEnum = &enumMedia;
	return S_OK;
}
STDMETHODIMP CMyPin::QueryAccept(const AM_MEDIA_TYPE *pmt){
	return S_OK;
}

STDMETHODIMP CMyPin::Connect(struct IPin *pin, struct _AMMediaType const * mt){
	if (isConnected) return VFW_E_ALREADY_CONNECTED;
//	if (!mt) return VFW_E_NO_ACCEPTABLE_TYPES;
	to = pin;
	if (to){
		to->QueryInterface(IID_IMemInputPin, (void**)&toMem);
	}
	if (!mt){
		WaitForMediaType();
		if (enumMedia.mts.size()) mt = &enumMedia.mts[0];
	}
	if (mt){
		HRESULT hr = to->ReceiveConnection(this, mt);
		if (hr == S_OK){
			isConnected = true;
		}else{
			TCHAR buf[1024];
			AMGetErrorText(hr, buf, sizeof(buf));
			DSTR << "CMyPin::Connect() Failed:" << buf << std::endl;
		}
		return hr;
	}
	DSTR << "CMyPin::Connect() No Media Type."  << std::endl;
	return E_NOTDETERMINED;
}
STDMETHODIMP CMyPin::QueryPinInfo(PIN_INFO *pInfo){
	if (pInfo){
		pInfo->pFilter = pSrc;
		pInfo->dir = PINDIR_OUTPUT;
		WCHAR* arc = L"MySourcePin";
		wcscpy(pInfo->achName, arc);
	}
	return S_OK;
}


//----------------------------------------------------------------------------------------
//	CMyEnumPins
//

STDMETHODIMP CMyEnumPins::QueryInterface(REFIID riid, void ** ppv){
	if( riid == IID_IEnumPins || riid == IID_IUnknown ){
		*ppv = (void *) static_cast<IEnumPins*> ( this );
		return NOERROR;
	}
	return E_NOINTERFACE;
}
STDMETHODIMP CMyEnumPins::Next(ULONG cPins, IPin **ppPins, ULONG *pcFetched){
	if (cur>=pins.size()){
		if (pcFetched) *pcFetched = 0;		
		return E_FAIL;
	}
	if (pcFetched) *pcFetched = 1;
	if (ppPins){
		*ppPins = pins[cur];
	}	
	cur ++;
	return NOERROR;
}

//----------------------------------------------------------------------------------------
//	CMySrc
//
CMySrc::CMySrc():pGraph(NULL),pClock(NULL), pin(this), enumPins(this){ 
	enumPins.pins.push_back(&pin); 
/*	AM_MEDIA_TYPE mt;
	mt.bFixedSizeSamples = false;
	mt.bTemporalCompression = false;
	mt.cbFormat = 
	mt.formattype = 
	pin.enumMedia.mts.push_back();
*/
}

STDMETHODIMP CMySrc::GetClassID(CLSID *pClassID){
	memcpy(pClassID->Data4, "CMySrc ", 8);
	return NOERROR;
}
STDMETHODIMP CMySrc::QueryInterface(REFIID riid, void ** ppv){
	if( riid == IID_IBaseFilter || riid == IID_IMediaFilter || riid == IID_IUnknown ){
		*ppv = (void *) static_cast<IBaseFilter*> ( this );
		return NOERROR;
	}
	return E_NOINTERFACE;
}

STDMETHODIMP CMySrc::Stop(){ 
	state = State_Stopped;
	return NOERROR;
}
STDMETHODIMP CMySrc::Pause(){ 
	state = State_Paused;
	return NOERROR; 
}
STDMETHODIMP CMySrc::Run( REFERENCE_TIME tStart){
	state = State_Running;
	return NOERROR;	
}
STDMETHODIMP CMySrc::GetState(DWORD dwMilliSecsTimeout, FILTER_STATE *pState){
	dwMilliSecsTimeout = 10;
	*pState = state;
	return NOERROR;	
}
STDMETHODIMP CMySrc::SetSyncSource(IReferenceClock *p){
	if (pClock) pClock->Release();
	pClock = p;
	if (pClock) pClock->AddRef();
	return NOERROR;
}
STDMETHODIMP CMySrc::GetSyncSource(IReferenceClock **pClock){
	return E_NOTIMPL;
}
STDMETHODIMP CMySrc::FindPin(LPCWSTR Id, IPin **ppPin){
	return E_NOTIMPL;
}
STDMETHODIMP CMySrc::QueryFilterInfo(FILTER_INFO *pInfo){
	if (pInfo){
		wcscpy(pInfo->achName,L"Hasegawa");
		pInfo->pGraph = pGraph;
		if (pGraph) pGraph->AddRef();
	}
	return S_OK;
}
STDMETHODIMP CMySrc::JoinFilterGraph(IFilterGraph *pG, LPCWSTR pName){
	if (pName) wcscpy(name, pName);
	else name[0] = 0;
	pGraph = pG;
	return S_OK;
}
STDMETHODIMP CMySrc::QueryVendorInfo(LPWSTR *pVendorInfo){
	*pVendorInfo = L"HASEGAWA";
	return S_OK;
}

//----------------------------------------------------------------------------------------
//	CMySampleGrabberCB
//
CMySampleGrabberCB::CMySampleGrabberCB(){}
CMySampleGrabberCB::~CMySampleGrabberCB(){}
void CMySampleGrabberCB::SetImage(unsigned char * pBuffer,int nBufferSize){
	std::cout << "CMySampleGrabberCB::SetImage(pBuffer, " << nBufferSize << "); called." << std::endl; 
}
STDMETHODIMP CMySampleGrabberCB::QueryInterface(REFIID riid, void ** ppv){
	if( riid == IID_ISampleGrabberCB || riid == IID_IUnknown ){
		*ppv = (void *) static_cast<ISampleGrabberCB*> ( this );
		return NOERROR;
	}
	return E_NOINTERFACE;
}
STDMETHODIMP CMySampleGrabberCB::SampleCB( double SampleTime, IMediaSample * pSample ){
	std::cout << "CMySampleGrabberCB::SampleCB( time:" << SampleTime << "); called." << std::endl;
	return S_OK;
}

STDMETHODIMP CMySampleGrabberCB::BufferCB( double dblSampleTime, BYTE * pBuffer, long lBufferSize ){
	std::cout << "CMySampleGrabberCB::BufferCB( time:" << dblSampleTime << " len:" << lBufferSize << "); called." << std::endl;
	return S_OK;
}


//--------------------------------------------------------------------------------
//	DShowCap
//
IBaseFilter* FindSrc(char* keyName, bool bVideo){
	IBaseFilter* pSrc = NULL;

	// 2. �V�X�e���f�o�C�X�񋓎q���쐬
	ICreateDevEnum *pDevEnum = NULL;
	CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, IID_ICreateDevEnum, (void **)&pDevEnum);

	IEnumMoniker *pClassEnum = NULL;
	if (bVideo) pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pClassEnum, 0);
	else pDevEnum->CreateClassEnumerator(CLSID_AudioInputDeviceCategory, &pClassEnum, 0);

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
		std::cout << "Enum Filter:" << filterName << std::endl;
		//	���O���m�F���ăo�C���h
		if (!pSrc && (keyName==NULL || strstr(filterName, keyName))){
			// ���j�J���t�B���^�I�u�W�F�N�g�Ƀo�C���h����
			pMonikers[i]->BindToObject(0, 0, IID_IBaseFilter, (void **)&pSrc);
		}
		pMonikers[i]->Release();
	}
	pClassEnum->Release();
	pDevEnum->Release();
	return pSrc;
}

IPin *GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, int n){
	BOOL			 bFound = FALSE;
	IEnumPins	*pEnum;
	IPin			 *pPin;
	int count = 0;

	pFilter->EnumPins(&pEnum);
	while(pEnum->Next(1, &pPin, 0) == S_OK)
		{
			PIN_DIRECTION PinDirThis;
			pPin->QueryDirection(&PinDirThis);
			if (PinDir == PinDirThis){ // �����Ŏw�肵�������̃s���Ȃ�break
				if (n==count){
					bFound = true;
					break;
				}
				count ++;
			}
			pPin->Release();
		}
	pEnum->Release();
	return (bFound ? pPin : 0);
}

DShowCap::DShowCap(){
	pSrc = NULL;
	pMediaControl = NULL;
	pGraph = NULL;
	rotId = 0;
}
void DShowCap::Prop(){
	if (!pSrc) return;
	// �t�B���^�̃v���p�e�B�y�[�W��\������B
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
}

bool DShowCap::Init(char* cameraName){
	CoInitialize(NULL);

	// 1. �t�B���^�O���t�쐬
	CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IGraphBuilder, (void **)&pGraph);

	// 2. �\�[�X�t�B���^�i�J�����j�̎擾
#if 0
	pSrc = FindSrc(cameraName);
#else
	static CMySrc mySrc;
	pSrc = &mySrc;
#endif
	if (!pSrc) return false;
	pGraph->AddFilter(pSrc, L"Video Capture");

	// 4. �R�[���o�b�N�̐ݒ�
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
	IPin *pSGrabOut = GetPin(pF, PINDIR_OUTPUT);	// C
	pGraph->Connect(pSrcOut, pSGrabIN);
	pGraph->Render(pSGrabOut);

	// 4-5. �O���o�̃��[�h��K�؂ɐݒ�
	pSGrab->SetBufferSamples(FALSE);
	pSGrab->SetOneShot(FALSE);
	pSGrab->SetCallback(&callBack, 0);  // ��2�����ŃR�[���o�b�N���w�� (0:SampleCB, 1:BufferCB)

//	Sleep(1000);
	// 5. �L���v�`���J�n
	pGraph->QueryInterface(IID_IMediaControl, (void **)&pMediaControl);
	pMediaControl->Run();

	return true;
}
void DShowCap::Release(){
	// 6. �I��

	if (rotId) RemoveFromRot(rotId);
	if(pSrc) pSrc->Release();
	if(pMediaControl) pMediaControl->Release();
	if(pGraph) pGraph->Release();
	pSrc = NULL;
	pMediaControl = NULL;
	pGraph = NULL;
	CoUninitialize();
}

void DShowCap::Set(){
	static CMyMediaSample ms;
	CMySrc* src = (CMySrc*)pSrc;	
	src->pin.toMem->Receive(&ms);
}

















///	GraphEdit�Ō��邽�߂ɁA�t�B���^�O���t�������^�C���I�u�W�F�N�g�ɓo�^����B
///	pdwRegister �� �o�^����ID
HRESULT AddToRot(IUnknown *pUnkGraph, DWORD *pdwRegister) 
{
    IMoniker * pMoniker = NULL;
    IRunningObjectTable *pROT = NULL;

    if (FAILED(GetRunningObjectTable(0, &pROT))) 
    {
        return E_FAIL;
    }
    
    const size_t STRING_LENGTH = 256;

    WCHAR wsz[STRING_LENGTH];
    StringCchPrintfW(wsz, STRING_LENGTH, L"FilterGraph %08x pid %08x", (DWORD_PTR)pUnkGraph, GetCurrentProcessId());
    
    HRESULT hr = CreateItemMoniker(L"!", wsz, &pMoniker);
    if (SUCCEEDED(hr)) 
    {
        hr = pROT->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, pUnkGraph,
            pMoniker, pdwRegister);
        pMoniker->Release();
    }
    pROT->Release();
    
    return hr;
}
void RemoveFromRot(DWORD pdwRegister)
{
    IRunningObjectTable *pROT;
    if (SUCCEEDED(GetRunningObjectTable(0, &pROT))) {
        pROT->Revoke(pdwRegister);
        pROT->Release();
    }
}




//---------------------------------------------------------------------------------------
//	Duebug�p�c�[���@GUID�̖��O���X�g
//
typedef struct {
    CHAR   *szName;
    GUID    guid;
} GUID_STRING_ENTRY;
GUID_STRING_ENTRY g_GuidNames[] = {
#define OUR_GUID_ENTRY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
{ #name, { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } } },
    #include <uuids.h>
};

CGuidNameList GuidNames;
int g_cGuidNames = sizeof(g_GuidNames) / sizeof(g_GuidNames[0]);

char *CGuidNameList::operator [] (const GUID &guid)
{
    for (int i = 0; i < g_cGuidNames; i++) {
        if (g_GuidNames[i].guid == guid) {
            return g_GuidNames[i].szName;
        }
    }
    if (guid == GUID_NULL) {
        return "GUID_NULL";
    }

// !!! add something to print FOURCC guids?

// shouldn't this print the hex CLSID?
    return "Unknown GUID Name";
}
