#include "Env.h"
#include "Contents.h"
#include "DShowRecv.h"
#include "Packet.h"
#include <dshow.h>
#include <qedit.h>
#include <windows.h>
#pragma comment(lib, "strmiids.lib")

DShowRecv dshowRecv;

CMySrcRecv::CMySrcRecv(){
	pMediaBuf=NULL;
}
CMySrcRecv::~CMySrcRecv(){
	delete pMediaBuf;
}
DWORD WINAPI thread(void* p){
	CMySrcRecv*This = (CMySrcRecv*)p;
	while(1){
		This->Recv();
	}
}
void CMySrcRecv::Init(){
	sockRecv.Init(AF_INET, SOCK_DGRAM, 0);
	WBSockAddr adr;
	adr.AdrIn().sin_family = AF_INET;
	adr.AdrIn().sin_port = htons(PORT_SEND);
	adr.AdrIn().sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sockRecv, (LPSOCKADDR)&adr, sizeof(adr))==SOCKET_ERROR){
		int error = WSAGetLastError();
		closesocket(sockRecv);
		sockRecv = INVALID_SOCKET;
		exit(0);
	}
	delete pMediaBuf;
	pMediaBuf = new unsigned char[1024 * 768 * 4];
	CreateThread(NULL, 0, thread, this, 0, NULL);
}
bool CMySrcRecv::Recv(){
	unsigned char buf[2048];
	WBSockAddr adr;
	sockRecv.RecvFrom(buf, sizeof(buf), adr);
	if (buf[0] == pdata.packetId[0]){		
		if (pMediaBuf){
			PMediaData* pData = (PMediaData*)buf;
			memcpy(pMediaBuf+ pData->count*1024, pData->data, 1024);
		}
	}else if(buf[0] == plen.packetId[0]){
		//	1��O�̃o�b�t�@�𑗐M
		static CMySampleRecv ms(this);
		if (pin.isConnected) pin.toMem->Receive(&ms);
		
		//	���̃o�b�t�@�̒������擾
		memcpy(&plen, buf, sizeof(plen));
		int bufLen = (plen.len+1023)/1024*1024;
		return false;
	}else if(buf[0] == ptype.packetId[0]){
		memcpy(&ptype, buf, sizeof(ptype));
	}
	return true;
}

STDMETHODIMP CMySampleRecv::GetMediaType(AM_MEDIA_TYPE ** ppMedia){
	*ppMedia = &pSrc->ptype.mt;
	return S_OK;
}
STDMETHODIMP CMySampleRecv::GetPointer(BYTE** pBuf){
	*pBuf = pSrc->pMediaBuf;
	return S_OK;
}
STDMETHODIMP_(long) CMySampleRecv::GetSize(){
	return pSrc->plen.len;
}



//----------------------------------------------------------------------------------------
//	CMySGCBRecv
//
STDMETHODIMP CMySGCBRecv::SampleCB( double SampleTime, IMediaSample * pSample ){
	//DSTR << "CMySGCBRecv::SampleCB( time:" << SampleTime << "); called." << std::endl;
	if (contents.mode == Contents::CO_CAM){
		BYTE* pBuf;
		HRESULT hr = pSample->GetPointer(&pBuf);
		if (hr==S_OK){
			contents.Capture(pBuf, pSample->GetSize());
		}
	}
	return S_OK;
}

STDMETHODIMP CMySGCBRecv::BufferCB( double dblSampleTime, BYTE * pBuffer, long lBufferSize ){
//	DSTR << "CMySGCBRecv::BufferCB( time:" << dblSampleTime << " len:" << lBufferSize << "); called." << std::endl;
	if (contents.mode == Contents::CO_CAM) contents.Capture(pBuffer, lBufferSize);
	return S_OK;
}

//---------------------------------------------------------------------------------------------


bool DShowRecv::Init(char* cameraName){
	CoInitialize(NULL);

	// 1. �t�B���^�O���t�쐬
	CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IGraphBuilder, (void **)&pGraph);

	// 2. �\�[�X�t�B���^�i�J�����j�̎擾
#if 1
	pSrc = FindSrc(cameraName);
#else
	pSrc = &mySrc;
	mySrc.Init();
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
	pGraph->Connect(pSrcOut, pSGrabIN);

	// 4-5. �O���o�̃��[�h��K�؂ɐݒ�
	pSGrab->SetBufferSamples(FALSE);
	pSGrab->SetOneShot(FALSE);
	pSGrab->SetCallback(&callBack, 0);  // ��2�����ŃR�[���o�b�N���w�� (0:SampleCB, 1:BufferCB)

	// 5. �L���v�`���J�n
	pGraph->QueryInterface(IID_IMediaControl, (void **)&pMediaControl);
	pMediaControl->Run();

	return true;
}
void DShowRecv::Release(){
	// 6. �I��
	if(pSrc) pSrc->Release();
	if(pMediaControl) pMediaControl->Release();
	if(pGraph) pGraph->Release();
	pSrc = NULL;
	pMediaControl = NULL;
	pGraph = NULL;
	CoUninitialize();
}

void DShowRecv::Set(){
//	while(mySrc.Recv());
}
