#include "WBSocket.h"
#include "DShowFilter.h"
#include "Packet.h"
#include <deque>

class CMySrcRecv:public CMySrc{
public:
	std::deque<unsigned char> keys;
	CRITICAL_SECTION csec;
	volatile bool bStopThread;
	HANDLE hThread;
	PMediaData pdata;
	PMediaTypeAndLen ptype;
	PKey pkey;
	WBSocket sockRecv;
	IMemAllocator* pAlloc;
	IMediaSample* pSample;
	int bufferSize;
	CMySrcRecv();
	~CMySrcRecv();
	void Init();
	bool Recv();
	void StopThread();
};

#if 0
class CMySampleRecv: public CMyMediaSample{
public:
	CMySrcRecv* pSrc;
	CMySampleRecv(CMySrcRecv* s):pSrc(s){}
	STDMETHODIMP GetMediaType(AM_MEDIA_TYPE ** ppMedia);
	STDMETHODIMP GetPointer(BYTE** pBuf);
	STDMETHODIMP_(long) GetSize();
};
#endif

class CMySGCBRecv : public CMySampleGrabberCB{
public:
	STDMETHODIMP SampleCB( double SampleTime, IMediaSample * pSample );
	STDMETHODIMP BufferCB( double dblSampleTime, BYTE * pBuffer, long lBufferSize );
};

struct DShowRecv: public DShowCap{
	bool bGood;
	bool IsGood(){ return bGood; }
	CMySrcRecv mySrc;
	CMySGCBRecv callBack;
	IBaseFilter *pSGF;
	ISampleGrabber *pSGrab;
	IPin* pSrcOut;
	IPin* pSGrabIn;
	IPin* pSGrabOut;
	DShowRecv();
	bool Init();
	bool Init(char*){ assert(0); }
	void Release();
};

extern DShowRecv dshowRecv;
