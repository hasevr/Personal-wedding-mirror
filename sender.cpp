#include "Sender.h"




DShowSender dshowSender;
int main(){
	dshowSender.Init("Logicool Qcam Pro 9000");
	std::cin.get();
	dshowSender.Release();
}
