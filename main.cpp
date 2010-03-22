#include "Env.h"
#ifdef USE_GLEW
#include <GL/glew.h>
#endif
#include <GL/glut.h>
#include <sstream>
#include <fstream>

#include <cv.h>
#include <cxcore.h>
#include <highgui.h>
#pragma comment(lib, "libcv200.dll.a") 
#pragma comment(lib, "libcxcore200.dll.a") 
#pragma comment(lib, "libhighgui200.dll.a") 

#include "Mirror.h"

using namespace Spr;

CvCapture* cvCam = NULL;
IplImage* cvImg = NULL;
GLuint cvTex = 0;
Vec2d cvTexCoord[4];
#define CVTEX_SIZE	1024

enum CONTENTS_MODE{
	CO_CAM,
	CO_TILE,
	CO_CEIL,
	CO_RANDOM,
} contentsMode;


double	CameraRotX = Rad(-90.0), CameraRotY = Rad(90.0), CameraZoom = 2.0;
bool bLeftButton = false, bRightButton = false;
int xlast, ylast;
void __cdecl mouse(int button, int state, int x, int y){
	xlast = x, ylast = y;
	if(button == GLUT_LEFT_BUTTON)
		bLeftButton = (state == GLUT_DOWN);
	if(button == GLUT_RIGHT_BUTTON)
		bRightButton = (state == GLUT_DOWN);
	glutPostRedisplay();
}

void __cdecl motion(int x, int y){
	static bool bFirst = true;
	int xrel = x - xlast, yrel = y - ylast;
	xlast = x;
	ylast = y;
	if(bFirst){
		bFirst = false;
		return;
	}
	// 左ボタン
	if(bLeftButton){
		CameraRotY += xrel * 0.01;
		CameraRotY = max(Rad(-180.0), min(CameraRotY, Rad(180.0)));
		CameraRotX += yrel * 0.01;
		CameraRotX = max(Rad(-90.0), min(CameraRotX, Rad(90.0)));
	}
	// 右ボタン
	if(bRightButton){
		CameraZoom *= exp(yrel/10.0);
		CameraZoom = max(0.1, min(CameraZoom, 100.0));
	}
	glutPostRedisplay();
}


// 光源の設定 
static GLfloat light_position[] = { 0.0, 10.0, 20.0, 0.0 };
static GLfloat light_ambient[]  = { 0.0, 0.0, 0.0, 1.0 };
static GLfloat light_diffuse[]  = { 1.0, 1.0, 1.0, 1.0 }; 
static GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
// 材質の設定
static GLfloat mat_red[]        = { 1.0, 0.0, 0.0, 1.0 };
static GLfloat mat_blue[]       = { 0.0, 0.0, 1.0, 1.0 };
static GLfloat mat_specular[]   = { 1.0, 1.0, 1.0, 1.0 };
static GLfloat mat_shininess[]  = { 120.0 };


void setLight() {
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
}

Vec2d windowSize;
void reshape(int w, int h){
	windowSize.x = w;
	windowSize.y = h;
	glViewport(0, 0, windowSize.x, windowSize.y);
}



GLuint contents;

void initialize(){
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	setLight();
	contents = glGenLists(1);
}

void drawContents(){
	glNewList(contents, GL_COMPILE);
	glDisable(GL_LIGHTING);
	glLineWidth(5);

	if (contentsMode == CO_RANDOM){
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
	}else if (contentsMode == CO_CEIL){
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
	}else if (contentsMode == CO_TILE){
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
		for(int i=0; i<4; ++i){
			glTexCoord2dv(cvTexCoord[0]);
			glVertex3d(-1,-1*0.75, d);
			glTexCoord2dv(cvTexCoord[1]);
			glVertex3d( 1,-1*0.75, d);
			glTexCoord2dv(cvTexCoord[2]);
			glVertex3d(-1, 1*0.75, d);
			glTexCoord2dv(cvTexCoord[3]);
			glVertex3d( 1, 1*0.75, d);
		}
		glEnd();
		glDisable(GL_TEXTURE_2D);
	}else if (contentsMode == CO_CAM){
		if (env.cameraMode == Env::CM_TILE){
			double d = -5;
			glColor3d(1,0,0);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, cvTex);
			glBegin(GL_TRIANGLE_STRIP);
			for(int i=0; i<4; ++i){
				glTexCoord2dv(cvTexCoord[0]);
				glVertex3d(-1,-1*0.75, d);
				glTexCoord2dv(cvTexCoord[1]);
				glVertex3d( 1,-1*0.75, d);
				glTexCoord2dv(cvTexCoord[2]);
				glVertex3d(-1, 1*0.75, d);
				glTexCoord2dv(cvTexCoord[3]);
				glVertex3d( 1, 1*0.75, d);
			}
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
			for(int i=0; i<4; ++i){
				glTexCoord2dv(cvTexCoord[0]);
				glVertex3d(-x, d, -z);
				glTexCoord2dv(cvTexCoord[1]);
				glVertex3d( x, d, -z);
				glTexCoord2dv(cvTexCoord[2]);
				glVertex3d(-x, d, z);
				glTexCoord2dv(cvTexCoord[3]);
				glVertex3d( x, d, z);
			}
			glEnd();
			glDisable(GL_TEXTURE_2D);		
		}
	}
	glLineWidth(1);
	glEnable(GL_LIGHTING);
//	glTranslated(0, 20, 0);
//	glutSolidTeapot(4);
	glEndList();
}


void keyboard(unsigned char key, int x, int y){
	if (key == '3') env.drawMode = Env::DM_WORLD;
	if (key == '4') env.drawMode = Env::DM_DESIGN;
	if (key == '5') env.drawMode = Env::DM_SHEET;
	if (key == '1') {
		env.drawMode = Env::DM_MIRROR;
		glutFullScreen();
	}
	if (key == '2') env.drawMode = Env::DM_FRONT;

	if (key == 't'){
		env.cameraMode = Env::CM_TILE;
		env.InitCamera();
		drawContents();
	}
	if (key == 'w'){
		env.cameraMode = Env::CM_WINDOW;
		env.InitCamera();
		drawContents();
	}

	if (key == 'c'){
		contentsMode = CO_CAM;
		drawContents();
	}
	if (key == 'r'){
		contentsMode = CO_RANDOM;
		drawContents();
	}

	if (key == 0x1b || key=='q'){
		cvReleaseCapture(&cvCam);
		exit(0);
	}
	glutPostRedisplay();
}	

void capture(){
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
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
//	for(int x=0; x<256; ++x) for(int y=0; y<256; ++y)
//		{ buf[x][y][0] = x; buf[x][y][1] = y; buf[x][y][2] = 3; }
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, CVTEX_SIZE, CVTEX_SIZE, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, buf);
}

static Affined afMove;
void display(){
	capture();

	glutPostRedisplay();
	//	テクスチャへのレンダリング
//	afMove = Affined::Rot(Rad(0.3), 'y') * afMove;	
	for(int y=0; y<DIVY; ++y){
		for(int x=0; x<DIVX; ++x){
			env.cell[y][x].BeforeDrawTex();
			glMultMatrixd(afMove);
			glCallList(contents);
			env.cell[y][x].AfterDrawTex();
		}
	}
	
	//	メインのレンダリング
	glViewport(0, 0, windowSize.x, windowSize.y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, (GLfloat)windowSize.x/(GLfloat)windowSize.y, 0.01, 50.0);
	glMatrixMode(GL_MODELVIEW);
	//	描画
	Affinef view;
	view.Pos() = CameraZoom * Vec3f(
		cos(CameraRotX) * cos(CameraRotY),
		sin(CameraRotX),
		cos(CameraRotX) * sin(CameraRotY));
	view.LookAtGL(Vec3f(), Vec3f(0.0f, 100.0f, 0.0f));

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(view.inv());
	glClearColor(0,0,0,1);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	env.Draw();
	glutSwapBuffers();
}


int main(int argc, char* argv[]){
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutCreateWindow("mirror");
	initialize();
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
//	glutIdleFunc(idle);

	env.Init();

	cvCam = cvCreateCameraCapture(CV_CAP_ANY);       //カメラ初期化
	if (cvCam) {
		cvImg = cvQueryFrame(cvCam);
		int h = min(cvImg->height, CVTEX_SIZE);
		int w = min(cvImg->width, CVTEX_SIZE);
		cvTexCoord[3] = Vec2d();
		cvTexCoord[2] = Vec2d((double)cvImg->width/CVTEX_SIZE, 0);
		cvTexCoord[1] = Vec2d(0, (double)cvImg->height/CVTEX_SIZE);
		cvTexCoord[0] = Vec2d(cvTexCoord[2].x, cvTexCoord[1].y);
	}else{
		std::cout << "カメラが見つかりません" << std::endl;
	}

	glGenTextures( 1, &cvTex);
	drawContents();
	glutMainLoop();
}
