#include "Mirror.h"

struct Loop: public std::vector<Vec3d>{
};
struct Sheet{
	std::vector<Loop> loops;
};
struct Env{
	Config config;
	Cell cell[DIVY][DIVX];
	Support support;	
	std::vector<Sheet> sheets;
	Affined projPose;	//	World���猩���v���W�F�N�^�̈ʒu�A�p��(�������AprojectionPitch �̉�]�͊܂߂Ȃ�)
	Vec3d centerSeat;

	enum DrawMode{
		DM_WORLD,
		DM_DESIGN,
		DM_SHEET,
		DM_MIRROR,
		DM_FRONT,
	};
	DrawMode drawMode;

	enum CameraMode{
		CM_WINDOW,
		CM_TILE
	};
	CameraMode cameraMode;

	void InitCamera();
	void InitMirror();
	void InitSupport();
	void PlaceMirror();
	void PlaceSupport();
	void WritePs();
	void Init();
	void InitGL();
	void Draw();
	void DrawSheet();
	void DrawMirror();
	void DrawDesign();
	void DrawWorld();
};
extern Env env;
