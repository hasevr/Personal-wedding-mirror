#include "WBSocket.h"
#include <assert.h>
#include <strstream>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")


static int winSockInitCount;
WBSocket::WBSocket(int a, int t, int p){
	sock=INVALID_SOCKET;
	InitWinsock();
	Init(a, t, p);
}
void WBSocket::Init(int a, int t, int p){
	if (sock != INVALID_SOCKET){ closesocket(sock); }
	sock = socket(a, t, p);
}
WBSocket::~WBSocket(){
	winSockInitCount --;
	if (winSockInitCount == 0){
		WSACleanup();
	}
}
void WBSocket::InitWinsock(){
	if (winSockInitCount == 0){
		WSADATA wsaData;
		WORD wVersionRequested = MAKEWORD( 2, 2 );
		int rv = WSAStartup(wVersionRequested, &wsaData);
		assert(rv == 0);
	}
	winSockInitCount ++;
}
int WBSocket::SendTo(void* buf, size_t len, const WBSockAddr& a){
	int rv = sendto(sock, (const char*)buf, len, 0, &a.Adr(), sizeof(a.Adr()));
	return rv;
}
bool WBSocket::CanRecv(){
	DWORD avail=0;
	ioctlsocket(sock, FIONREAD, &avail);
	return avail != 0;
}
int WBSocket::RecvFrom(void* buf, size_t len, WBSockAddr& a){
	int fromLen = sizeof(WBSockAddr);
	return recvfrom(sock, (char*)buf, len, 0, a, &fromLen);
}
bool WBSocket::Bind(const WBSockAddr& a){
	if(bind(sock, (LPSOCKADDR)&a, sizeof(a))==SOCKET_ERROR){
		closesocket(sock);
		sock = INVALID_SOCKET;
		return false;
	}
	return true;
}
char* WBSocket::GetErrorString(){
	int e = WSAGetLastError();
	return GetErrorString(e);
}
char* WBSocket::GetErrorString(int errorNum){
	char* rtv = "";
	switch(errorNum){
		case WSANOTINITIALISED:	rtv = "A successful WSAStartup must occur before using this function.";	break;
		case WSAENETDOWN:		rtv = "The Windows Sockets implementation has detected that the network subsystem has failed.";	break;
		case WSAEADDRINUSE:		rtv = "An attempt has been made to listen on an address in use.";	break;
		case WSAEINPROGRESS:	rtv = "A blocking Windows Sockets operation is in progress.";	break;
		case WSAEINVAL:			rtv = "The socket has not been bound with bind or is already connected.";	break;
		case WSAEISCONN:		rtv = "The socket is already connected.";	break;
		case WSAEMFILE:			rtv = "No more file descriptors are available.";	break;
		case WSAENOBUFS:		rtv = "No buffer space is available.";	break;
		case WSAENOTSOCK:		rtv = "The descriptor is not a socket.";	break;
		case WSAEOPNOTSUPP:		rtv = "The referenced socket is not of a type that supports the listen operation.";	break;
		case WSAEFAULT:			rtv = "The addrlen argument is too small (less than the size of a sockets address structure).";	break;
		case WSAEINTR:			rtv = "The (blocking) call was canceled using WSACancelBlockingCall.";	break;
		case WSAEWOULDBLOCK:	rtv = "The socket is marked as nonblocking and no connections are present to be accepted.";	break;
		case WSAEADDRNOTAVAIL:	rtv = "The specified address is not available from the local computer.";	break;
		case WSAEAFNOSUPPORT:	rtv = "Addresses in the specified family cannot be used with this socket.";	break;
		case WSAECONNREFUSED:	rtv = "The attempt to connect was forcefully rejected.";	break;
		case WSAENETUNREACH:	rtv = "The network can't be reached from this host at this time.";	break;
		case WSAETIMEDOUT:		rtv = "Attempt to connect timed out without establishing a connection.";	break;
		case WSAEHOSTUNREACH:	rtv = "The host can't be reached from this host."; break;
		default:
			{
			static char buf[256];
			std::ostrstream ostr(buf, 256);
			ostr.setf(std::ios::dec);
			ostr << "Unknown error #" << errorNum << " occuered." << '\0';
			rtv = buf;
			}break;
	}
	return rtv;
}

WBSockAddr::WBSockAddr(){
	memset(&adr, 0, sizeof(adr));
	adr.sa_family = AF_INET;
}
void WBSockAddr::Print(std::ostream& os) const{
	if (Adr().sa_family == AF_INET){
		os << "inet ";
	}else if (Adr().sa_family == AF_UNIX){
		os << "unix ";
	}
	os << (int)AdrIn().sin_addr.S_un.S_un_b.s_b1 << ".";
	os << (int)AdrIn().sin_addr.S_un.S_un_b.s_b2 << ".";
	os << (int)AdrIn().sin_addr.S_un.S_un_b.s_b3 << ".";
	os << (int)AdrIn().sin_addr.S_un.S_un_b.s_b4 << " ";
	os << ntohs(AdrIn().sin_port);

}
	
WBNetInterface::WBNetInterface(const INTERFACE_INFO& info){
	flags = info.iiFlags;
	mask = info.iiNetmask.Address;
	addr = info.iiAddress.Address;
	broadcast = info.iiBroadcastAddress.Address;
	broadcast.AdrIn().sin_addr.S_un.S_addr = ~mask.AdrIn().sin_addr.S_un.S_addr;
	broadcast.AdrIn().sin_addr.S_un.S_addr |= mask.AdrIn().sin_addr.S_un.S_addr & addr.AdrIn().sin_addr.S_un.S_addr;
}
void WBNetInterface::Print(std::ostream& os) const{
	os << "Address   " << addr;
	os << "Mask      " << mask;
	os << "Broadcast " << broadcast;
}

void WBNetInterfaces::Init(){
	// ブロードキャストアドレス取得部分
	const int NUM_IF_MAX = 20;
	INTERFACE_INFO ifInfo[NUM_IF_MAX];
	memset(ifInfo, 0, sizeof(ifInfo));
	unsigned long nBytesReturned;
	WBSocket sock(AF_INET, SOCK_DGRAM, 0);
	int error = WSAIoctl(sock, SIO_GET_INTERFACE_LIST, 0, 0, ifInfo, sizeof(ifInfo), &nBytesReturned, 0, 0);
	if (error){
		error = WSAGetLastError();
		std::cout << "Error: Failed to query WinSock2, error code" << error << std::endl;
		return;
	}
	for(unsigned int i=0; ifInfo[i].iiFlags && i<(nBytesReturned/sizeof(INTERFACE_INFO)); ++i){
		push_back(WBNetInterface(ifInfo[i]));
	}
}
void WBNetInterfaces::Print(std::ostream& os) const{
	for(const_iterator it = begin(); it != end(); ++it){
		it->Print(os);
	}
}
/*
void main(){
	WBNetInterfaces ifs;
	ifs.Init();
	std::cout << ifs;
}
*/