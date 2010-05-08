#include <Springhead.h>
#include <gl/glut.h>
#ifndef MIRROR_H
#define MIRROR_H

#define DIVX	3
#define DIVY	6
using namespace Spr;

struct Front{
	double w;
	double h;
	double hOff;
	double d;
	void Init();
};
struct Config{
	double w;
	double h;
	double hOff;
	double d;
	double projectionPitch;
	double outXRad[2];
	double ceil, wall;	//	天井の高さ、中央から壁の距離
	double outY[2];
	double outYC[2];
	double t;			//	サポートの板厚
	double hSupportDepth;
	double hSupportGroove;
	void Init();
}; 

struct Mirror{
	Vec3d normal;		//	鏡の法線
	Vec3d vertex[4];	//	頂点
	Vec3d center;		//	鏡とinDirCenterの交点
};
struct Cell{
	int x, y;
	Vec3d inDirCenter;
	Vec3d outDirCenter;
	Vec3d inDir[4];
	Vec3d outDir[4];
	Vec3d outPosCenter;
	Vec3d outPos[4];
	Vec3d imagePos[4];
	Vec3d inPos[4];
	int outPlace;	//	-1: 左の壁, 0:天井, 1:右の壁
	Mirror mirror;
	//	最初のレンダリング用のカメラ
	Affined view[2];
	Affined projection[2];
	Vec3d screenCenter;
	Vec2d screenSize;
	Vec3d screen[2][4];
	Vec2d texCoord[2][4];
	//	レンダリングバッファ
	GLuint  texName[2];
#ifdef USE_GLEW
	GLuint  renderName;
	GLuint  frameName;
#endif
	int	texSize;

	void Init(int xIn, int yIn);
	void CalcPosition(double depth, int id);	//	頂点id がdepthになるようにする
	void InitCamera(int fb);
	void InitFrontCamera(int fb);
	void InitGL();
	void BeforeDrawTex(int fb);	//	0:front	1:back
	void AfterDrawTex(int fb);
};

struct Plane{
	Vec3d normal;
	Vec3d dir;
	std::vector<Vec3d> vertices;
};
struct Support{
	Plane vPlane[DIVX];
	Plane hPlane[DIVY];
	double baseHeight;
	double yMin;
};
#endif
