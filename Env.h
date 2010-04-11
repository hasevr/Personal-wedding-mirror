#include "Mirror.h"

struct Loop: public std::vector<Vec3d>{
	std::string label;
};
struct Sheet{
	std::vector<Loop> loops;
};
struct Env{
	bool bZoomMirror;
	double dt;
	Config config;
	Front front;
	Cell cell[DIVY+1][DIVX];
	Support support;
	std::vector<Sheet> sheets;
	Affined projPose;	//	Worldから見たミラー用プロジェクタの位置、姿勢(ただし、projectionPitch の回転は含めない)
	Vec3d centerSeat;

	enum DrawMode{
		DM_DESIGN,
		DM_SHEET,
		DM_MIRROR,
		DM_MIRROR_BACK,
		DM_FRONT,
		DM_BACK,
	};
	DrawMode drawMode;

	enum CameraMode{
		CM_WINDOW,
		CM_TILE
	};
	CameraMode cameraMode;

	void Step();
	void InitCamera();
	void InitMirror();
	void InitSupport();
	void PlaceMirror();
	void PlaceSupport();
	void WritePs();
	void Init();
	void InitGL();
	void Draw();
	void RenderTex(int fb);
	void DrawSheet();
	void DrawMirror(int fb);
	void DrawFront();
	void DrawBack();
	void DrawDesign();
	void DrawHalf(int fb);
	void DrawHalfFront(int fb);
};
extern Env env;
