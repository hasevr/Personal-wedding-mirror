#ifndef DECAL_H
#define DECAL_H
#include <string>
#include <Base/Affine.h>
#include <Base/TQuaternion.h>
using namespace Spr;

struct Decal{
	unsigned id;
	std::string fileName;
	Vec2d sheetSize;
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
	double duration;
	double transition;
	Key();
	Key(double d, double t, Posed p);
};
struct Path: public std::vector<Key>{
	Posed GetPose(double time);
};

#endif
