#include "Decal.h"
#ifdef USE_GLEW
#include <GL/glew.h>
#endif
#include <GL/glut.h>
#include <io.h>
#include <../src/Graphics/GRLoadBmp.h>

Decal::Decal(){
	id = 0;
	sheetSize = Vec2d(40, 30);
}
void Decal::Draw(){
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2d(1, 1);
	glVertex3d(-sheetSize.x/2, -sheetSize.y/2, 0);
	glTexCoord2d(0, 1);
	glVertex3d( sheetSize.x/2, -sheetSize.y/2, 0);
	glTexCoord2d(1, 0);
	glVertex3d(-sheetSize.x/2,  sheetSize.y/2, 0);
	glTexCoord2d(0, 0);
	glVertex3d( sheetSize.x/2,  sheetSize.y/2, 0);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}
void Decal::Release(){
	glDeleteTextures(1, &id);
	id = 0;
}
bool Decal::Load(){
	// paintLib でファイルをロード．
	int h = LoadBmpCreate(fileName.c_str());
	if (!h) return false;
	int tx = LoadBmpGetWidth(h);
	int ty = LoadBmpGetHeight(h);
	int nc = LoadBmpGetBytePerPixel(h);
	char* texbuf = DBG_NEW char[tx*ty*nc];
	LoadBmpGetBmp(h, texbuf);
	LoadBmpRelease(h);
	
	if (!id) glGenTextures(1, &id);
	glBindTexture( GL_TEXTURE_2D, id );
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	const GLenum	pxfm[] = {GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_BGR_EXT, GL_BGRA_EXT};
	int rv = gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, tx, ty, pxfm[nc - 1], GL_UNSIGNED_BYTE, texbuf);
	delete texbuf;
	if (rv){
		DSTR << gluErrorString(rv) << std::endl;
		return false;
	}
	return true;
}










void Decals::Load(){
	struct _finddata_t cFile;
	intptr_t hFile;
	std::string findPath = folderName;
	findPath.append("\\*.*");
	hFile = _findfirst(findPath.c_str(), &cFile);
	do{
		Decal d;
		d.fileName = folderName;
		d.fileName.append("\\");
		d.fileName.append(cFile.name);
		if (d.Load()){
			push_back(d);
		}
	} while(_findnext( hFile, &cFile ) == 0 );
	_findclose( hFile );
}
void Decals::Release(){
	for(iterator it = begin(); it!=end(); ++it){
		it->Release();
	}
	clear();
}
