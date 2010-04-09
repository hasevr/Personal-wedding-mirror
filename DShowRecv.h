#include "DShowFilter.h"


class CMySGCBRecv : public CMySampleGrabberCB{
public:
	STDMETHODIMP SampleCB( double SampleTime, IMediaSample * pSample );
	STDMETHODIMP BufferCB( double dblSampleTime, BYTE * pBuffer, long lBufferSize );
};

struct DShowRecv: public DShowCap{
	CMySGCBRecv callBack;
	bool Init(char* cameraName);
	void Release();
	void Set();
};

extern DShowRecv dshowRecv;
