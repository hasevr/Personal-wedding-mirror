#include "DShowFilter.h"

class CMySGCBSend : public ISampleGrabberCB{
public:
	void SetImage(unsigned char * pBuffer,int nBufferSize);
	CMySGCBSend();
	virtual ~CMySGCBSend();

	STDMETHODIMP_(ULONG) AddRef(){ return 2; }
	STDMETHODIMP_(ULONG) Release(){ return 1; }
	STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
	STDMETHODIMP SampleCB( double SampleTime, IMediaSample * pSample );
	STDMETHODIMP BufferCB( double dblSampleTime, BYTE * pBuffer, long lBufferSize );
};

struct DShowSender{
	IBaseFilter *pSrc;
	IMediaControl *pMediaControl;
	IGraphBuilder *pGraph;
	CMySGCBSend callBack;
	DShowSender();
	IBaseFilter* FindSrc(char* cameraName);
	void Prop();
	bool Init(char* cameraName);
	void Release();
	void Set();
};

extern DShowSender dshowSender;
