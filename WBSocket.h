#ifndef WBSOCKET_H
#define WBSOCKET_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>

class WBSockAddr{
protected:
	SOCKADDR adr;
public:
	WBSockAddr();
	WBSockAddr(const SOCKADDR& a):adr(a){}
	WBSockAddr(const SOCKADDR_IN& a):adr((const SOCKADDR&)a){}
	WBSockAddr(const char* adr, int port){Set(adr, port);}
	SOCKADDR& Adr(){return adr;}
	const SOCKADDR& Adr() const {return adr;}
	SOCKADDR_IN& AdrIn(){return (SOCKADDR_IN&)adr;}
	const SOCKADDR_IN& AdrIn() const {return (SOCKADDR_IN&)adr;}
	operator SOCKADDR&(){return adr;}
	operator const SOCKADDR&() const {return adr;}
	operator SOCKADDR_IN&(){return (SOCKADDR_IN&)adr;}
	operator const SOCKADDR_IN&() const {return (SOCKADDR_IN&)adr;}
	operator SOCKADDR*(){return &adr;}
	operator const SOCKADDR*() const {return &adr;}
	operator SOCKADDR_IN*(){return (SOCKADDR_IN*)&adr;}
	operator const SOCKADDR_IN*() const {return (const SOCKADDR_IN*)&adr;}
	void Set(const char* adr, int port);
	void Print(std::ostream& os) const;
	friend bool operator == (const WBSockAddr& a, const WBSockAddr& b);
};
inline std::ostream& operator << (std::ostream& os, const WBSockAddr& w){ w.Print(os); return os; }
inline bool operator == (const WBSockAddr& a, const WBSockAddr& b){
	return a.AdrIn().sin_family == b.AdrIn().sin_family
		&& a.AdrIn().sin_addr.S_un.S_addr == b.AdrIn().sin_addr.S_un.S_addr
		&& a.AdrIn().sin_port == b.AdrIn().sin_port;
}

class WBSocket{
	SOCKET sock;
	void InitWinsock();
public:
	WBSocket(int af=AF_INET, int type=SOCK_DGRAM, int protocol=0);
	void Init(int af=AF_INET, int type=SOCK_DGRAM, int protocol=0);
	~WBSocket();
	operator SOCKET(){ return sock; }
	int SendTo(void* buf, size_t len, const WBSockAddr& a);
	bool CanRecv();
	int RecvFrom(void* buf, size_t len, WBSockAddr& a);
	bool Bind(const WBSockAddr& a);
	static char* GetErrorString();
	static char* GetErrorString(int errorNum);
};
class WBNetInterface{
protected:
	unsigned long flags;
	WBSockAddr addr;
	WBSockAddr mask;
	WBSockAddr broadcast;
public:
	WBNetInterface(const INTERFACE_INFO& info);
	const WBSockAddr& Addr(){ return addr; }
	const WBSockAddr& Mask(){ return mask; }
	const WBSockAddr& Broadcast(){ return broadcast; }
	unsigned long Flags(){ return flags; }
	void Print(std::ostream& os) const;
};
inline std::ostream& operator << (std::ostream& os, const WBNetInterface& w){ w.Print(os); return os; }
class WBNetInterfaces:public std::vector<WBNetInterface>{
protected:
public:
	void Init();
	void Print(std::ostream& os) const;
};
inline std::ostream& operator << (std::ostream& os, const WBNetInterfaces& w){ w.Print(os); return os; }
#endif
