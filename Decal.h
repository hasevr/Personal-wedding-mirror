#ifndef DECAL_H
#define DECAL_H
#include <string>
#include <Base/Affine.h>
using namespace Spr;

struct Decal{
	unsigned id;
	std::string fileName;
	Vec2d sheetSize;
	Decal();
	bool Load();
	void Release();
	void Draw();
};

struct Decals:public std::vector<Decal>{
	std::string folderName;
	void Load();
	void Release();
};

#endif
