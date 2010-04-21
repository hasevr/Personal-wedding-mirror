#include "WBSocket.h"
#include "DShowFilter.h"
#include "Packet.h"

class CMySrcRecv:public CMySrc{
public:
	unsigned char* pMediaBuf;
	PMediaData pdata;
	PMediaLen plen;
	PMediaType ptype;
	WBSocket sockRecv;
	CMySrcRecv();
	~CMySrcRecv();
	void Init();
	bool Recv();
};
class CMySampleRecv: public CMyMediaSample{
public:
	CMySrcRecv* pSrc;
	CMySampleRecv(CMySrcRecv* s):pSrc(s){}
	STDMETHODIMP GetMediaType(AM_MEDIA_TYPE ** ppMedia);
	STDMETHODIMP GetPointer(BYTE** pBuf);
	STDMETHODIMP_(long) GetSize();
};


class CMySGCBRecv : public CMySampleGrabberCB{
public:
	STDMETHODIMP SampleCB( double SampleTime, IMediaSample * pSample );
	STDMETHODIMP BufferCB( double dblSampleTime, BYTE * pBuffer, long lBufferSize );
};

struct DShowRecv: public DShowCap{
	CMySrcRecv mySrc;
	CMySGCBRecv callBack;
	bool Init(char* cameraName);
	void Release();
	void Set();
};

extern DShowRecv dshowRecv;