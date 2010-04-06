#include <dshow.h>
#include <qedit.h>


class CMySampleGrabberCB : public ISampleGrabberCB{
public:
	void SetImage(unsigned char * pBuffer,int nBufferSize);
	CMySampleGrabberCB();
	virtual ~CMySampleGrabberCB();

	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();
	STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
	STDMETHODIMP SampleCB( double SampleTime, IMediaSample * pSample );
	STDMETHODIMP BufferCB( double dblSampleTime, BYTE * pBuffer, long lBufferSize );
};

struct DShowCap{
	IBaseFilter *pSrc;
	IMediaControl *pMediaControl;
	ICaptureGraphBuilder2 *pBuilder;
	IGraphBuilder *pGraph;
	CMySampleGrabberCB callBack;
	DShowCap();
	bool Init(char* cameraName);
	void Release();
};

extern DShowCap dshowCap;
