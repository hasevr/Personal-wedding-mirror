#include "Sender.h"
#include "Packet.h"
#include <iostream>
#include <conio.h>
#include <assert.h>


STDMETHODIMP CMySGCBSend::SampleCB( double SampleTime, IMediaSample * pSample ){
	//	MediaTypeの送信
	send(sockSend, (char*)&mediaType, sizeof(mediaType) + mediaType.mt.cbFormat, MSG_DONTROUTE);

	BYTE* data=NULL;
	pSample->GetPointer(&data);
	int len = pSample->GetSize();
	static PMediaLen lenPacket;
	lenPacket.bufferSize = pSample->GetSize();
	lenPacket.length = pSample->GetActualDataLength();
	send(sockSend, (char*)&lenPacket, sizeof(lenPacket), MSG_DONTROUTE);
	
	int nP = (len+1023)/1024;
	static PMediaData packet;
	if (data){
		int i;
		for(i=0; i<nP-1; ++i){
			packet.count = i;
			memcpy(packet.data, data+i*1024, 1024);
			send(sockSend, (char*)&packet, sizeof(packet), MSG_DONTROUTE);
		}
		packet.count = i;
		memcpy(packet.data, data+i*1024, len%1024);
		send(sockSend, (char*)&packet, sizeof(packet), MSG_DONTROUTE);
	}
	return S_OK;
}

void CMySGCBSend::InitSock(){
	sockSend.Init(AF_INET, SOCK_DGRAM, 0);
	WBNetInterfaces nis;
	nis.Init();
	unsigned i=0;
	for(i=0; i<nis.size(); ++i){
		std::cout << "Nic" << i << " A:" << nis[i].Addr() << "B:" << nis[i].Broadcast();
		if (!(nis[i].Flags()&IFF_UP)){
			std::cout << " is down ... skip." << std::endl;
			continue;
		}
		if (nis[i].Flags()&IFF_LOOPBACK){
			std::cout << " loopback ... skip." << std::endl;
			continue;
		}
		if (!(nis[i].Flags()&IFF_BROADCAST) ){
			std::cout << " no broadcast ... skip." << std::endl;
			continue;
		}
		if( nis[i].Addr().AdrIn().sin_addr.S_un.S_un_b.s_b1==192 && nis[i].Addr().AdrIn().sin_addr.S_un.S_un_b.s_b2==168 &&
			nis[i].Addr().AdrIn().sin_addr.S_un.S_un_b.s_b3==251){
				std::cout << " addr? ... skip." << std::endl;
				continue;
		}
		std::cout << std::endl;
		break;
	}
	adrBcast = nis[1].Broadcast();
	adrBcast.AdrIn().sin_port = htons(PORT_SEND);	//	ポート番号指定

	WBSockAddr adr;
	adr.AdrIn().sin_family = AF_INET;
	adr.AdrIn().sin_port = htons(0);
	adr.AdrIn().sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sockSend, (LPSOCKADDR)&adr, sizeof(adr))==SOCKET_ERROR){
		int error = WSAGetLastError();
		closesocket(sockSend);
		sockSend = INVALID_SOCKET;
		exit(0);
	}
	if (connect(sockSend, adrBcast, sizeof(adrBcast))==SOCKET_ERROR){
		std::cout << sockSend.GetErrorString();
		closesocket(sockSend);
		sockSend = INVALID_SOCKET;
		exit(0);
	}
}


DShowSender dshowSender;
bool DShowSender::Init(char* cameraName){
	callBack.InitSock();
	CoInitialize(NULL);

	// 1. フィルタグラフ作成
	CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IGraphBuilder, (void **)&pGraph);
	AddToRot(pGraph, &rotId);

	// 2. ソースフィルタ（カメラ）の取得
	pSrc = FindSrc(cameraName);

	if (!pSrc) return false;
	pGraph->AddFilter(pSrc, L"Video Capture");

	// 4. コールバックの設定
	// 4-1. サンプルグラバの生成
	IBaseFilter *pF = NULL;
	ISampleGrabber *pSGrab;
	CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (LPVOID *)&pF);
	pF->QueryInterface(IID_ISampleGrabber, (void **)&pSGrab);

	// 4-3. フィルタグラフへ追加
	pGraph->AddFilter(pF, L"Grabber");

	// 4-4. サンプルグラバの接続
	// [pSrc](o) -> (i)[pF](o) -> [VideoRender]
	//       ↑A     ↑B       ↑C
	IPin *pSrcOut = GetPin(pSrc, PINDIR_OUTPUT);	// A
	IPin *pSGrabIN = GetPin(pF, PINDIR_INPUT);		// B
	IPin *pSGrabOut = GetPin(pF, PINDIR_OUTPUT);	// C
	pGraph->Connect(pSrcOut, pSGrabIN);
	pGraph->Render(pSGrabOut);

	// 4-5. グラバのモードを適切に設定
	pSGrab->SetBufferSamples(FALSE);
	pSGrab->SetOneShot(FALSE);
	pSGrab->SetCallback(&callBack, 0);  // 第2引数でコールバックを指定 (0:SampleCB, 1:BufferCB)

	//	メディアタイプの取得
	callBack.mediaType.mt = CMediaType();
	pSrcOut->ConnectionMediaType(&callBack.mediaType.mt);
	assert(callBack.mediaType.mt.cbFormat < sizeof(callBack.mediaType.format));
	memcpy(callBack.mediaType.format, callBack.mediaType.mt.pbFormat, callBack.mediaType.mt.cbFormat);

	// 5. キャプチャ開始
	pGraph->QueryInterface(IID_IMediaControl, (void **)&pMediaControl);
	pMediaControl->Run();
	return true;
}


int main(){
	dshowSender.Init("Logicool Qcam Pro 9000");
	while(1){
		char ch = _getch();
		if (ch == 0x1b || ch=='q') break;
	}
	dshowSender.Release();
}
