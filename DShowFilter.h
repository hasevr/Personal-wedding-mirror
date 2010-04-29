#include <dshow.h>
#include <qedit.h>
#include <vector>


class CMediaType:public AM_MEDIA_TYPE{
public:
	CMediaType();
	CMediaType(const CMediaType& m);
	CMediaType(const AM_MEDIA_TYPE& m);
	~CMediaType();
	CMediaType& operator=(const AM_MEDIA_TYPE& m);
};

class CMyMediaSample: public IMediaSample{
public:
	STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
	STDMETHODIMP_(ULONG) AddRef(){ return 2; }
	STDMETHODIMP_(ULONG) Release(){ return 1; }
	STDMETHODIMP GetPointer(BYTE** pBuf){ *pBuf = (BYTE*)this; return S_OK; }
	STDMETHODIMP_(long) GetSize(){ return sizeof(this); }
	STDMETHODIMP GetTime(REFERENCE_TIME* pStart, REFERENCE_TIME* pEnd){ return VFW_E_SAMPLE_TIME_NOT_SET; }
	STDMETHODIMP SetTime(REFERENCE_TIME* pStart, REFERENCE_TIME* pEnd){ return E_NOTIMPL; }
	STDMETHODIMP IsSyncPoint() { return S_OK; }
	STDMETHODIMP SetSyncPoint(BOOL b) { return S_OK; }
	STDMETHODIMP_(long) GetActualDataLength(){ return GetSize(); }
	STDMETHODIMP SetActualDataLength(long l){ return E_NOTIMPL; }
	STDMETHODIMP GetMediaType(AM_MEDIA_TYPE ** ppMedia){ return E_NOTIMPL; }
	STDMETHODIMP SetMediaType(AM_MEDIA_TYPE * pMedia){ return E_NOTIMPL; }
	STDMETHODIMP IsDiscontinuity(){ return S_FALSE; }
	STDMETHODIMP SetDiscontinuity(BOOL l){ return S_OK; }
	STDMETHODIMP IsPreroll(){ return S_FALSE; }
	STDMETHODIMP SetPreroll(BOOL l){ return S_OK; }
	STDMETHODIMP GetMediaTime(REFERENCE_TIME* pStart, REFERENCE_TIME* pEnd){ return E_NOTIMPL; }
	STDMETHODIMP SetMediaTime(REFERENCE_TIME* pStart, REFERENCE_TIME* pEnd){ return E_NOTIMPL; }
};

class CMyPin;
class CMyEnumMedia: public IEnumMediaTypes{
	CMyPin* pPin;
	unsigned cur;
public:
	std::vector<CMediaType> mts;
	CMyEnumMedia(CMyPin* p): pPin(p), cur(0){}
	~CMyEnumMedia();
	STDMETHODIMP GetClassID(__RPC__out CLSID *pClassID);
	STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
	STDMETHODIMP Next(ULONG c, AM_MEDIA_TYPE **ppMt, ULONG *pc);
	STDMETHODIMP Skip(ULONG c){ cur += c; return NOERROR; }
	STDMETHODIMP Reset(){ cur = 0; return NOERROR; }
	STDMETHODIMP Clone(IEnumMediaTypes**){ return E_NOTIMPL; }
	STDMETHODIMP_(ULONG) AddRef(){ return 2; }
	STDMETHODIMP_(ULONG) Release(){ return 1; }
};
class CMySrc;
class CMyPin: public IPin{
public:
	CMySrc* pSrc;
	bool isConnected;
	IPin* to;
	IMemInputPin* toMem;
	CMyEnumMedia enumMedia;

	CMyPin(CMySrc* s);
	STDMETHODIMP_(ULONG) AddRef(){ return 2; }
	STDMETHODIMP_(ULONG) Release(){ return 1; }
	STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
	STDMETHODIMP Connect(IPin *pReceivePin, const AM_MEDIA_TYPE *pmt);
	STDMETHODIMP ReceiveConnection(IPin *pConnector, const AM_MEDIA_TYPE *pmt){ return E_NOTIMPL; }
	STDMETHODIMP Disconnect(){ isConnected = false; return S_OK; }
	STDMETHODIMP ConnectedTo(IPin **pPin){ if(pPin) *pPin = to; return S_OK; }
	STDMETHODIMP ConnectionMediaType(AM_MEDIA_TYPE *pmt);
	STDMETHODIMP QueryPinInfo(PIN_INFO *pInfo);
	STDMETHODIMP QueryDirection(PIN_DIRECTION *pPinDir){ if(pPinDir) *pPinDir=PINDIR_OUTPUT; return S_OK; }
	STDMETHODIMP QueryId(LPWSTR *id){ if(id) *id=L"out"; return S_OK; }
	STDMETHODIMP QueryAccept(const AM_MEDIA_TYPE *pmt);
	STDMETHODIMP EnumMediaTypes(IEnumMediaTypes **ppEnum);
	STDMETHODIMP QueryInternalConnections(IPin **apPin, ULONG *nPin){ return E_NOTIMPL; }
	STDMETHODIMP EndOfStream(){ return S_OK; }
	STDMETHODIMP BeginFlush(){ return S_OK; }
	STDMETHODIMP EndFlush(){ return S_OK; }
	STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate){ return S_OK; }
	bool WaitForMediaType();
};

class CMySrc;
class CMyEnumPins: public IEnumPins{
public:
	CMySrc* pSrc;
	unsigned cur;
public:
	std::vector<IPin*> pins;
	CMyEnumPins(CMySrc* p): pSrc(p), cur(0){}
	STDMETHODIMP GetClassID(__RPC__out CLSID *pClassID);
	STDMETHODIMP_(ULONG) AddRef(){ return 2; }
	STDMETHODIMP_(ULONG) Release(){ return 1; }
	STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
	STDMETHODIMP Next(ULONG cPins, IPin **ppPins, ULONG *pcFetched);
	STDMETHODIMP Skip(ULONG cPins){ cur += cPins; return NOERROR; }
	STDMETHODIMP Reset(){ cur = 0;return NOERROR; }
	STDMETHODIMP Clone(IEnumPins**){ return E_NOTIMPL; }
};
class CMySrc: public IBaseFilter{
public:
	IFilterGraph* pGraph;
	IReferenceClock* pClock;
	FILTER_STATE state;
	CMyEnumPins enumPins;
	CMyPin pin;
	WCHAR name[128];
public:
	CMySrc();
	STDMETHODIMP GetClassID(CLSID *pClassID);
	STDMETHODIMP_(ULONG) AddRef(){ return 2; }
	STDMETHODIMP_(ULONG) Release(){ return 1; }
	STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);

	STDMETHODIMP Stop( void);
	STDMETHODIMP Pause( void);
	STDMETHODIMP Run( REFERENCE_TIME tStart);
	STDMETHODIMP STDMETHODCALLTYPE GetState(DWORD dwMilliSecsTimeout, FILTER_STATE *State);
	STDMETHODIMP SetSyncSource(IReferenceClock *pClock);
	STDMETHODIMP GetSyncSource(IReferenceClock **pClock);
	STDMETHODIMP EnumPins(IEnumPins **ppEnum){ if (ppEnum) *ppEnum = &enumPins; return NOERROR; }
    STDMETHODIMP FindPin(LPCWSTR Id, IPin **ppPin);
    STDMETHODIMP QueryFilterInfo(FILTER_INFO *pInfo);        
    STDMETHODIMP JoinFilterGraph(IFilterGraph *pGraph, LPCWSTR pName);
    STDMETHODIMP QueryVendorInfo(LPWSTR *pVendorInfo);
};

class CMySampleGrabberCB : public ISampleGrabberCB{
public:
	void SetImage(unsigned char * pBuffer,int nBufferSize);
	CMySampleGrabberCB();
	virtual ~CMySampleGrabberCB();

	STDMETHODIMP_(ULONG) AddRef(){ return 2; }
	STDMETHODIMP_(ULONG) Release(){ return 1; }
	STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
	STDMETHODIMP SampleCB( double SampleTime, IMediaSample * pSample );
	STDMETHODIMP BufferCB( double dblSampleTime, BYTE * pBuffer, long lBufferSize );
};

struct DShowCap{
	IBaseFilter *pSrc;
	IMediaControl *pMediaControl;
	IGraphBuilder *pGraph;
	DWORD rotId;
	CMySampleGrabberCB callBack;
	DShowCap();
	IBaseFilter* FindSrc(char* cameraName);
	void Prop();
	bool Init(char* cameraName);
	void Release();
	void Set();
};
///	フィルタから指定の方向のピンを取り出す
IPin *GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir);

///	GraphEditで見るために、フィルタグラフをランタイムオブジェクトに登録する。
///	pdwRegister は 登録時のID
HRESULT AddToRot(IUnknown *pUnkGraph, DWORD *pdwRegister);
///	ランタイムオブジェクトからの登録解除pdwRegister は 登録時のID
void RemoveFromRot(DWORD pdwRegister);

//	GUIDの名前リスト
class CGuidNameList {
    public:
        CHAR *operator [] (const GUID& guid);
};
extern CGuidNameList GuidNames;
