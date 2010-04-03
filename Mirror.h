#include <Springhead.h>
#include <gl/glut.h>
#ifndef MIRROR_H
#define MIRROR_H

#define DIVX	8
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
	double ceil, wall;	//	�V��̍����A��������ǂ̋���
	double outY[2];
	double t;			//	�T�|�[�g�̔�
	double hSupportDepth;
	double hSupportGroove;
	void Init();
}; 

struct Mirror{
	Vec3d normal;		//	���̖@��
	Vec3d vertex[4];	//	���_
	Vec3d center;		//	����inDirCenter�̌�_
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
	int outPlace;	//	-1: ���̕�, 0:�V��, 1:�E�̕�
	Mirror mirror;
	//	�ŏ��̃����_�����O�p�̃J����
	Affined view[2];
	Affined projection[2];
	Vec3d screenCenter;
	Vec2d screenSize;
	Vec3d screen[2][4];
	Vec2d texCoord[2][4];
	//	�����_�����O�o�b�t�@
	GLuint  texName[2];
#ifdef USE_GLEW
	GLuint  renderName;
	GLuint  frameName;
#endif
	int	texSize;

	void Init(int xIn, int yIn);
	void CalcPosition(double depth, int id);	//	���_id ��depth�ɂȂ�悤�ɂ���
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