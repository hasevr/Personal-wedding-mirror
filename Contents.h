#include "Decal.h"
#include <cxcore.h>
struct CvCapture;
struct Contents{
	enum ContentsMode{
		CO_CAM,
		CO_TILE,
		CO_CEIL,
		CO_RANDOM,
		CO_PHOTO,
		CO_SHIP
	};
	ContentsMode mode;
	Decals decals;
	Decals backs;
	std::vector<Path> paths;

	unsigned cvTex;
	unsigned list;
	CvCapture* cvCam;
	IplImage* cvImg;
	Vec2d cvTexCoord[4];
	enum CVTEXSIZE {CVTEX_SIZE = 1024};

	Contents();
	void Draw(bool isInit = true);
	void Release();
	void Capture();
	void Init();
	void DrawRandom();
	void DrawCeil();
	void DrawPhoto();
	void DrawCam();
	void DrawTile();
	void LoadPhoto();
	void DrawShip();

	void Step(double dt);
};
extern Contents contents;
