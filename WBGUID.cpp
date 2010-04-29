#include "WBGuid.h"
#pragma hdrstop
#include <sstream>
#include <iomanip>

WBGuid::WBGuid(std::string s){
	std::istringstream ss(s);
	char ch;
	ss >> std::hex >> Data1;
	ss >> ch;

	ss >> std::hex >> Data2;
	ss >> ch;

	ss >> std::hex >> Data3;
	ss >> ch;

	unsigned long val;
	ss >> std::hex >> val;
	Data4[0] = (unsigned char) (val>>8);
	Data4[1] = (unsigned char) (val&0xFF);

	ss >> ch;
	char buf[4];
	ss.read(buf,4);
	std::istringstream(std::string(buf,4)) >> std::hex >> val;
	Data4[2] = (unsigned char) (val>>8);
	Data4[3] = (unsigned char) (val&0xFF);

	ss >> std::hex >> val;
	Data4[4] = (unsigned char) ( (val>>24) & 0xFF );
	Data4[5] = (unsigned char) ( (val>>16) & 0xFF );
	Data4[6] = (unsigned char) ( (val>>8) & 0xFF );
	Data4[7] = (unsigned char) ( val&0xFF );
}
void WBGuid::Print(std::ostream& os) const {
	using namespace std;
	os << hex << setfill('0') << setw(8) << Data1;
	os << "-"  << setw(4)<< Data2 << "-"  << setw(4) << Data3 << "-";
	os << setw(2) << (int)Data4[0]  << setw(2) << (int)Data4[1] << "-";
	for(int i=2; i<8; ++i){
		os  << setw(2) << (int)Data4[i];
	}
}
