#include "Contents.h"
#include "Env.h"
#include <cv.h>
#include <cxcore.h>
#include <highgui.h>
#pragma comment(lib, "libcv200.dll.a") 
#pragma comment(lib, "libcxcore200.dll.a") 
#pragma comment(lib, "libhighgui200.dll.a") 
#pragma comment(lib, "libhighgui200.dll.a") 

#ifdef USE_GLEW
#include <GL/glew.h>
#endif
#include <GL/glut.h>


Contents contents;
Contents::Contents():list(0){
	cvCam = NULL;
	cvImg = NULL;
	cvTex = 0;
}
void Contents::LoadPhoto(){
	decals.folderName = "texs";
	decals.Load();
}
void Contents::DrawPhoto(){
	if (env.cameraMode == Env::CM_TILE){
		glTranslated(0, 0, -5);
	}else{
		glRotated(90, 1,0,0);
		glTranslated(0, 5, -5);
	}
	decals.front().Draw();
}
void Contents::DrawRandom(){
	glBegin(GL_LINE_LOOP);
	for(int i=0; i<400; ++i){
		Vec3d rp(rand(), rand(), rand());
		rp /= RAND_MAX/2;
		glColor3dv(rp*2);
		rp -= Vec3d(1, 1, 1);
		rp *= 50;
		glVertex3dv(rp);
	}
	glEnd();
}
void Contents::DrawCeil(){
	glBegin(GL_LINES);
	int Y = 15;
	int SIZE = 100;
	for(int i=-SIZE; i<SIZE; ++i){
		glLineWidth(i%2 ? 5 : 2);
		glColor3d((i+SIZE)%3/3.0,1,0);
		glVertex3d(i, Y, -SIZE);
		glVertex3d(i, Y,  SIZE);
		glColor3d(1,0,(i+SIZE)%3/3.0);
		glVertex3d(-SIZE, Y, i);
		glVertex3d( SIZE, Y, i);
	}
/*	Y=-Y;
	for(int i=-SIZE; i<SIZE; ++i){
		glLineWidth(i%2 ? 5 : 2);
		glColor3d((i+SIZE)%3/3.0,1,0);
		glVertex3d(i, Y, -SIZE);
		glVertex3d(i, Y,  SIZE);
		glColor3d(1,0,(i+SIZE)%3/3.0);
		glVertex3d(-SIZE, Y, i);
		glVertex3d( SIZE, Y, i);
	}
*/	glEnd();
}
void Contents::DrawTile(){
	double d = -8;
	glBegin(GL_LINES);
	glColor3d(1,0,0);
	glVertex3d(0, 0, d);
	glVertex3d(1, 0, d);
	glColor3d(0,1,0);
	glVertex3d(0, 0, d);
	glVertex3d(0, 1, d);
	glEnd();
	glColor3d(1,1,1);
	glBegin(GL_LINE_LOOP);
	glVertex3d(-1, -1, d);
	glVertex3d(-1,  1, d);
	glVertex3d( 1,  1, d);
	glVertex3d( 1, -1, d);	
	glEnd();

	d = -8.1;
	glColor3d(1,0,0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, cvTex);
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2dv(cvTexCoord[0]);
	glVertex3d(-1,-1*0.75, d);
	glTexCoord2dv(cvTexCoord[1]);
	glVertex3d( 1,-1*0.75, d);
	glTexCoord2dv(cvTexCoord[2]);
	glVertex3d(-1, 1*0.75, d);
	glTexCoord2dv(cvTexCoord[3]);
	glVertex3d( 1, 1*0.75, d);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}
void Contents::DrawCam(){
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	if (env.cameraMode == Env::CM_TILE){
		double d = -5;
		glColor3d(1,0,0);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, cvTex);
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2dv(cvTexCoord[0]);
		glVertex3d(-1,-1*0.75, d);
		glTexCoord2dv(cvTexCoord[1]);
		glVertex3d( 1,-1*0.75, d);
		glTexCoord2dv(cvTexCoord[2]);
		glVertex3d(-1, 1*0.75, d);
		glTexCoord2dv(cvTexCoord[3]);
		glVertex3d( 1, 1*0.75, d);
		glEnd();
		glDisable(GL_TEXTURE_2D);
	}else{
		double d = 3;
		double x = 15.0/2;
		double z = 20.0/2;
		glColor3d(1,0,0);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, cvTex);
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2dv(cvTexCoord[0]);
		glVertex3d(-x, d, -z);
		glTexCoord2dv(cvTexCoord[1]);
		glVertex3d( x, d, -z);
		glTexCoord2dv(cvTexCoord[2]);
		glVertex3d(-x, d, z);
		glTexCoord2dv(cvTexCoord[3]);
		glVertex3d( x, d, z);
		glEnd();
		glDisable(GL_TEXTURE_2D);		
	}
}

void Contents::Draw(bool isInit){
	if (mode==CO_CAM) Capture();
	if (!isInit && (mode==CO_CAM || mode==CO_RANDOM || mode==CO_CEIL || mode==CO_TILE) ) 
		return;

	glNewList(list, GL_COMPILE);
	glDisable(GL_LIGHTING);
	glLineWidth(5);
	switch(mode){
		case CO_CAM:
			DrawCam();
			break;
		case CO_RANDOM:
			DrawRandom();
			break;
		case CO_CEIL:
			DrawCeil();
			break;
		case CO_TILE:
			DrawTile();
			break;
		case CO_PHOTO:
			DrawPhoto();
			break;
	}
	glLineWidth(1);
	glEnable(GL_LIGHTING);
	glEndList();
}

void Contents::Init(){
	cvCam = cvCreateCameraCapture(CV_CAP_ANY);       //ƒJƒƒ‰‰Šú‰»
	if (cvCam) {
		cvImg = cvQueryFrame(cvCam);
		int h = min(cvImg->height, CVTEX_SIZE);
		int w = min(cvImg->width, CVTEX_SIZE);
		cvTexCoord[3] = Vec2d();
		cvTexCoord[2] = Vec2d((double)cvImg->width/CVTEX_SIZE, 0);
		cvTexCoord[1] = Vec2d(0, (double)cvImg->height/CVTEX_SIZE);
		cvTexCoord[0] = Vec2d(cvTexCoord[2].x, cvTexCoord[1].y);
	}else{
		std::cout << "ƒJƒƒ‰‚ªŒ©‚Â‚©‚è‚Ü‚¹‚ñ" << std::endl;
	}
	
	list = glGenLists(1);
	glGenTextures( 1, &cvTex);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	Draw(true);
}
void Contents::Release(){
	cvReleaseCapture(&cvCam);
	decals.Release();
	cvCam = NULL;
}
void Contents::Capture(){
	if (cvCam) cvImg = cvQueryFrame(cvCam);
	static char buf[CVTEX_SIZE][CVTEX_SIZE][3];
	if (cvCam) {
		int h = min(cvImg->height, CVTEX_SIZE);
		int w = min(cvImg->width, CVTEX_SIZE);
		for(int y=0; y<h; ++y){
			memcpy(buf[y], cvImg->imageData + (y*cvImg->width*3), w*3);
		}
	}
	//	texBuf
	glBindTexture( GL_TEXTURE_2D, cvTex );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, CVTEX_SIZE, CVTEX_SIZE, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, buf);
}