#include "WBSocket.h"
#include "DShowFilter.h"
#include "Packet.h"

class CMySGCBSend : public CMySampleGrabberCB{
public:
	PMediaType mediaType;
	WBSocket sockSend;
	WBSockAddr adrBcast;
	STDMETHODIMP SampleCB( double SampleTime, IMediaSample * pSample );
	void InitSock();
};

struct DShowSender: public DShowCap{
	CMySGCBSend callBack;
	bool Init(char* cameraName);
};

extern DShowSender dshowSender;
