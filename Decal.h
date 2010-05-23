#ifndef DECAL_H
#define DECAL_H
#include <string>
#include <Base/Affine.h>
#include <Base/TQuaternion.h>
using namespace Spr;

struct Decal{
	Vec4d color;
	unsigned id;
	std::string fileName;
	Vec2d sheetSize;
	Vec2d imageSize;
	Decal();
	bool Load();
	void Release();
	void Draw();
	double time;
	Affined posture;
	Vec2d texOffset;
	Vec2d texScale;
};

struct Decals:public std::vector<Decal>{
	std::string folderName;
	void Load();
	void Release();
};
struct Key{
	Posed posture;
	double alpha;
	double duration;
	double transition;
	Key();
	Key(double d, double t, Posed p, double a=1);
};
struct Path: public std::vector<Key>{
	Posed GetPose(double time);
	double GetAlpha(double time);
};

struct Fairy{
	Decal* decal;
	int count;
	Vec3d pos;
	Vec3d vel;
	Vec3d dir;
	Vec3d goal;
	Quaterniond ori;
	Quaterniond oriGoal;
	bool onCeil;
	Fairy();
	void Update(double dt);
	void Draw();
	Vec3d NewGoal();
};
struct Fairies:public std::vector<Fairy>{
	Decals decals;
	void Update(double dt);
};

#endif
