#include "Decal.h"
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
	enum CVTEXSIZE {CVTEX_SIZE = 1024};
	unsigned char cameraTexBuf[CVTEX_SIZE][CVTEX_SIZE][3];
	unsigned list;
	Vec2d cvTexCoord[4];

	Contents();
	void Draw(bool isInit = true);
	void Release();
	void Capture(char* buf, unsigned len);
	void UpdateCameraTex();
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
