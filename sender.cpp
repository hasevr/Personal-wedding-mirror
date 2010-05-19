#include "Sender.h"
#include "Packet.h"
#include <iostream>
#include <Base/BaseDebug.h>
#include <conio.h>
#include <assert.h>
#include "WBGuid.h"

#include <time.h>



STDMETHODIMP CMySGCBSend::SampleCB( double SampleTime, IMediaSample * pSample ){
/*	//	間引き
	static int ct;
	if (ct <2){
		ct++;
		return S_OK;
	}
	ct = 0;
*/
	//	MediaTypeとデータサイズの送信
	BYTE* data=NULL;
	pSample->GetPointer(&data);
	mediaType.bufferSize = pSample->GetSize();
	mediaType.length = pSample->GetActualDataLength();
	send(sockSend, (char*)&mediaType, sizeof(mediaType), MSG_DONTROUTE);
	static int count = 0;
	static DWORD lastTime;
	count ++;
	if (count >= 30){
		count = 0;
		DWORD time = GetTickCount();
		int dt = (int)time - (int)lastTime;
		lastTime = time;
		std::cout << "Len:" << mediaType.length << " Sz:" << mediaType.bufferSize << "  30frame / " << dt << " msec" << std::endl;
	}
	
	int nP = (mediaType.length+PMediaData::DATALEN-1)/PMediaData::DATALEN;
	static PMediaData packet;
	if (data){
		int i;
		for(i=0; i<nP-1; ++i){
			packet.count = i;
			memcpy(packet.data, data+i*PMediaData::DATALEN, PMediaData::DATALEN);
			for(int t=0; t<10; ++t){
				int rv = send(sockSend, (char*)&packet, sizeof(packet), MSG_DONTROUTE);
				if (rv != SOCKET_ERROR)	break;
				std::cout << "Retry packet " << i << " " << t << std::endl;
			}
//			if (i%3==0) Sleep(5);
		}
		packet.count = i;
		memcpy(packet.data, data+i*PMediaData::DATALEN, mediaType.length%PMediaData::DATALEN);
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
	if (i < nis.size()){
		adrBcast = nis[i].Broadcast();
	}else if (nis.size()>2){
		adrBcast = nis[1].Broadcast();	
	}
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
	BOOL bOpt=false;
	setsockopt(sockSend, SOL_SOCKET, SO_BROADCAST, (char*)&bOpt, sizeof(bOpt));
	setsockopt(sockSend, SOL_SOCKET, SO_DONTROUTE, (char*)&bOpt, sizeof(bOpt));

}


DShowSender dshowSender;
DShowSender::DShowSender(){
}

bool DShowSender::Init(char* cameraName){
	HRESULT hr=S_OK;
	callBack.InitSock();
	CoInitialize(NULL);

	// 1. フィルタグラフ作成
	CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IGraphBuilder, (void **)&pGraph);
	AddToRot(pGraph, &rotId);
	HFILE h = (HFILE)CreateFile("Filter.log", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
	pGraph->SetLogFile(h);


	// 2. ソースフィルタ（カメラ）の取得
#if 1
	pSrc = FindSrc(cameraName);
	if (!pSrc) return false;
	pGraph->AddFilter(pSrc, L"Video Capture");
#else
	pGraph->RenderFile(L"bothhand.mpg", NULL);
	IBaseFilter* pRender=NULL;
	pGraph->FindFilterByName(L"Video Renderer", &pRender);
	if (pRender){
		IPin* pin = GetPin(pRender, PINDIR_INPUT);
		IPin* pinSrc;
		pin->ConnectedTo(&pinSrc);
		PIN_INFO pi;
		pinSrc->QueryPinInfo(&pi);
		pSrc = pi.pFilter;
		hr = pin->Disconnect();
		hr = pGraph->RemoveFilter(pRender);
	}
	IEnumFilters* pEnum=NULL;
	hr = pGraph->EnumFilters(&pEnum);
	for(IBaseFilter* pGet=NULL; pEnum->Next(1, &pGet, NULL) == S_OK; ){
		FILTER_INFO fi;
		pGet->QueryFilterInfo(&fi);
		char buf[1024];
		wcstombs(buf, fi.achName, sizeof(buf));
		std::cout << buf << std::endl;
	}
	if (!pSrc) return false;
#endif
		
	// 4. コールバックの設定
	// 4-1. サンプルグラバの生成
	IBaseFilter *pF = NULL;
	ISampleGrabber *pSGrab;
	CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (LPVOID *)&pF);
	pF->QueryInterface(IID_ISampleGrabber, (void **)&pSGrab);
	hr = pGraph->AddFilter(pF, L"Grabber");
	
	//	圧縮フィルタの生成
	IBaseFilter* pComp=NULL;
#if 0
	WBGuid xvid("D76E2820-1563-11CF-AC98-00AA004C0FA9");
	CoCreateInstance(xvid, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (LPVOID *)&pComp);
	pGraph->AddFilter(pComp, L"Xvid Compressor");
#else
	CoCreateInstance(CLSID_MJPGEnc, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (LPVOID *)&pComp);
	pGraph->AddFilter(pComp, L"MJPG Compressor");
#endif

	//	Teeの作成
	IBaseFilter *pTee=NULL;
//	CoCreateInstance(CLSID_SmartTee, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (LPVOID *)&pTee);
	CoCreateInstance(CLSID_InfTee, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (LPVOID *)&pTee);
	hr = pGraph->AddFilter(pTee, L"Tee");
	//	AVIMuxの作成
	IBaseFilter *pVDest=NULL;
	CoCreateInstance(CLSID_AviDest, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (LPVOID *)&pVDest);
	hr = pGraph->AddFilter(pVDest, L"VideoDest");

	//	ファイルライターの生成
	IBaseFilter *pVFile=NULL;
	CoCreateInstance(CLSID_FileWriter, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (LPVOID *)&pVFile);
	hr = pGraph->AddFilter(pVFile, L"VideoFile");

	IFileSinkFilter* pVFileSink= NULL;
	hr = pVFile->QueryInterface(IID_IFileSinkFilter, (void**)&pVFileSink);

	WCHAR buf[1024];	
	time_t t = time(NULL);
    struct tm* tmp = localtime(&t);	
	wcsftime(buf, sizeof(buf)/sizeof(buf[0]), L"SenderRec_%Y_%m_%d.%H.%M.%S.avi", tmp);
	pVFileSink->SetFileName(buf, NULL);


	//	レンダラーの作成
	IBaseFilter* pVRender;
	CoCreateInstance(CLSID_VideoRenderer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (LPVOID *)&pVRender);
	hr = pGraph->AddFilter(pVRender, L"VideoRender");



	// 4-4. サンプルグラバの接続 src->comp->grab
	// ピンの取得
	IPin* pSrcOut = GetPin(pSrc, PINDIR_OUTPUT);
	IPin* pSGrabIn = GetPin(pF, PINDIR_INPUT);	
	IPin* pSGrabOut = GetPin(pF, PINDIR_OUTPUT);
	
	IPin* pCompIn = GetPin(pComp, PINDIR_INPUT);
	IPin* pCompOut = GetPin(pComp, PINDIR_OUTPUT); 


#if 1	//	圧縮の設定
	IAMVideoCompression* pVc=NULL;
	pCompOut->QueryInterface(IID_IAMVideoCompression, (void**)&pVc);
	hr = pVc->put_Quality(0.4);
	pVc->Release();
#endif


#if 1	//	カメラの設定
	IAMStreamConfig* pSc=NULL;
	pSrcOut->QueryInterface(IID_IAMStreamConfig, (void**)&pSc);
	if(pSc){
		int nCaps; int sizeCaps;
		pSc->GetNumberOfCapabilities(&nCaps, &sizeCaps);
		for(int i=0; i<nCaps; ++i){
			AM_MEDIA_TYPE* pmt=NULL;
			VIDEO_STREAM_CONFIG_CAPS caps;
			assert(sizeCaps <= sizeof(caps));
			pSc->GetStreamCaps(i, &pmt, (BYTE*)&caps);
			if (pmt->pbFormat){
				VIDEOINFOHEADER* vh = (VIDEOINFOHEADER*)pmt->pbFormat;
				std::cout << i << ":\t" << vh->bmiHeader.biWidth << "x" << vh->bmiHeader.biHeight;
				char *name=GuidNames[pmt->subtype];
				std::cout << name << std::endl;
	//			if (vh->bmiHeader.biWidth == 160 && vh->bmiHeader.biHeight == 120){
	//			if (vh->bmiHeader.biWidth == 320 && vh->bmiHeader.biHeight == 240){
				if (vh->bmiHeader.biWidth == 640 && vh->bmiHeader.biHeight == 480){
					pSc->SetFormat(pmt);
					CoTaskMemFree(pmt->pbFormat);
					CoTaskMemFree(pmt);
					break;
				}
			}
			CoTaskMemFree(pmt->pbFormat);
			CoTaskMemFree(pmt);
		}
		pSc->Release();
	}
#endif
	hr = pGraph->Connect(pSrcOut, pCompIn);
	if (hr != S_OK){
		TCHAR buf[1024];
		AMGetErrorText(hr, buf, sizeof(buf));
		DSTR << "CMyPin::Connect() Failed:" << buf << std::endl;
/*		HRESULT hrs[] = {
			0x80004002,
			0x80040207,
			0x80040200,
			0x80040217,
			0x80040231,
		};
		for(int i=0; i<sizeof(hrs)/sizeof(hrs[0]); ++i){
			TCHAR buf[1024];
			AMGetErrorText(hrs[i], buf, sizeof(buf));
			DSTR << "CMyPin::Connect()" << hrs[i] << " : " << buf << std::endl;
		}
		*/
	}
	hr = pGraph->Connect(pCompOut, pSGrabIn);
	if (!hr==S_OK){
		TCHAR buf[1024];
		AMGetErrorText(hr, buf, sizeof(buf));
		DSTR << "CMyPin::Connect()" << hr << " : " << buf << std::endl;
	}
	IPin* pTeeIn = GetPin(pTee, PINDIR_INPUT); 
	IPin* pTeeOut1 = GetPin(pTee, PINDIR_OUTPUT, 0); 
	IPin* pVDestIn = GetPin(pVDest, PINDIR_INPUT);
	IPin* pVDestOut = GetPin(pVDest, PINDIR_OUTPUT);
	IPin* pVFileIn = GetPin(pVFile, PINDIR_INPUT);
	hr = pGraph->Connect(pSGrabOut, pTeeIn);
	hr = pGraph->Connect(pTeeOut1, pVDestIn);
	hr = pGraph->Connect(pVDestOut, pVFileIn);

	IPin* pTeeOut2 = GetPin(pTee, PINDIR_OUTPUT, 1); 
	IPin* pVRenderIn = GetPin(pVRender, PINDIR_INPUT);
	hr = pGraph->Connect(pTeeOut2, pVRenderIn);

	// 4-5. グラバのモードを適切に設定
	pSGrab->SetBufferSamples(FALSE);
	pSGrab->SetOneShot(FALSE);
	pSGrab->SetCallback(&callBack, 0);  // 第2引数でコールバックを指定 (0:SampleCB, 1:BufferCB)

	//	メディアタイプの取得
	callBack.mediaType.mt = CMediaType();
	pCompOut->ConnectionMediaType(&callBack.mediaType.mt);
	assert(callBack.mediaType.mt.cbFormat < sizeof(callBack.mediaType.format));
	memcpy(callBack.mediaType.format, callBack.mediaType.mt.pbFormat, callBack.mediaType.mt.cbFormat);

	// 5. キャプチャ開始
	pGraph->QueryInterface(IID_IMediaControl, (void **)&pMediaControl);
	pMediaControl->Run();

	return true;
}


int main(){
	dshowSender.Init("Logicool");
	while(1){
		char ch = _getch();
		if (ch == 0x1b || ch=='q') break;
		dshowSender.Prop();
	}
	dshowSender.Release();
}
