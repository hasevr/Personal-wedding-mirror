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

#endif
