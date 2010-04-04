#include "Decal.h"
#include "Env.h"
#ifdef USE_GLEW
#include <GL/glew.h>
#endif
#include <GL/glut.h>
#include <io.h>
#include <../src/Graphics/GRLoadBmp.h>

Key::Key():alpha(1){
	duration = 0;
	transition = 0;
}
Key::Key(double d, double t, Posed p, double a){
	duration = d;
	transition = t;
	posture = p;
	alpha = a;
}
Posed Path::GetPose(double time){
	double ratio=0;
	iterator it;
	for(it = begin(); it != end(); ++it){
		if (time > it->duration){
			time -= it->duration;
			if (time > it->transition){
				time -= it->transition;
			}else{
				ratio = time / it->transition;
				if (it+1 == end()) ratio = 0;
				break;
			}
		}else{
			ratio = 0;
			break;
		}
	}
	if (it == end()){
		return back().posture;
	}
	if (ratio == 0){
		return it->posture;
	}else{		
		//	(it+1)->posture = it->posture * dp
		Posed dp = it->posture.Inv() * (it+1)->posture;
		dp.Ori() = Quaterniond::Rot(dp.Ori().RotationHalf() * ratio);
		dp.Pos() *= ratio;
		return it->posture * dp;
	}
}
double Path::GetAlpha(double time){
	double ratio=0;
	iterator it;
	for(it = begin(); it != end(); ++it){
		if (time > it->duration){
			time -= it->duration;
			if (time > it->transition){
				time -= it->transition;
			}else{
				ratio = time / it->transition;
				if (it+1 == end()) ratio = 0;
				break;
			}
		}else{
			ratio = 0;
			break;
		}
	}
	if (it == end()){
		return back().alpha;
	}
	if (ratio == 0){
		return it->alpha;
	}else{		
		double da = (it+1)->alpha - it->alpha;
		return it->alpha + da*ratio;
	}
}

Decal::Decal(): texOffset(0,0), texScale(1,1), color(1,1,1,1){
	id = 0;
	sheetSize = Vec2d(4, 3)*4*2;
	time = 0;
}
void Decal::Draw(){
	glPushMatrix();
	glMultMatrixd(posture);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
	glEnable(GL_TEXTURE_2D);
	glColor4dv(color);
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2d(texOffset.x+texScale.x, texOffset.y+texScale.y);
	glVertex3d(-sheetSize.x/2, -sheetSize.y/2, 0);
	glTexCoord2d(texOffset.x, texOffset.y+texScale.y);
	glVertex3d( sheetSize.x/2, -sheetSize.y/2, 0);
	glTexCoord2d(texOffset.x+texScale.x, texOffset.y);
	glVertex3d(-sheetSize.x/2,  sheetSize.y/2, 0);
	glTexCoord2d(texOffset.x, texOffset.y);
	glVertex3d( sheetSize.x/2,  sheetSize.y/2, 0);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	glPopMatrix();
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
	imageSize = Vec2d(tx, ty);
	sheetSize.x = sheetSize.y/ty*tx;
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
