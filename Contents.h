#include "Decal.h"
struct Contents{
	enum ContentsMode{
		CO_CAM,
		CO_BLACK,
		CO_WHITE,
		CO_FAIRY,
		CO_SHIP,

		//	ˆÈ‰º”ñŽg—p
		CO_TILE,
		CO_CEIL,
		CO_RANDOM,
		CO_PHOTO,
	};
	ContentsMode mode;
	Decals decals;
	Decals backs;
	Fairies fairies;
	std::vector<Path> paths;
	unsigned startCount;

	unsigned cvTex;
	enum CVTEXSIZE {CVTEX_SIZE = 1024};
	unsigned char cameraTexBuf[CVTEX_SIZE][CVTEX_SIZE][3];
	unsigned list;
	Vec2d cvTexCoord[4];

	Contents();
	void Draw(bool isInit = true);
	void Release();
	void Capture(unsigned char* buf, unsigned len);
	void UpdateCameraTex();
	void Init();

	void DrawCam();
	void DrawColor(Vec4d c);
	void DrawShip();
	void DrawFairy();

	void DrawRandom();
	void DrawCeil();
	void DrawPhoto();
	void DrawTile();
	void LoadPhoto();
	void ResetShip();

	void Step(double dt);
};
extern Contents contents;
