#include "Env.h"
#include "Mirror.h"
#include "Contents.h"


#ifdef USE_GLEW
#include <GL/glew.h>
#endif
#include <GL/glut.h>
#include <string>
#include <sstream>
#include <fstream>


#include <io.h>

using namespace Spr;


Vec2d windowSize;
Vec2d orgSize;
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



void reshape(int w, int h){
	if (windowSize.x != w && windowSize.y != h) orgSize = windowSize;
	windowSize.x = w;
	windowSize.y = h;
	glViewport(0, 0, windowSize.x, windowSize.y);
}



void keyboard(unsigned char key, int x, int y){
	switch (key){
		case '1':
			env.drawMode = Env::DM_MIRROR;
			glutFullScreen();
			std::cout << "1 Draw mirror" << std::endl;
			break;
		case '2':
			env.drawMode = Env::DM_FRONT;
			glutFullScreen();
			std::cout << "2 Draw front" << std::endl;
			break;
		case '3':
			env.drawMode = Env::DM_WORLD;
			glutReshapeWindow(orgSize.x, orgSize.y);
			std::cout << "3 Draw world" << std::endl;
			break;
		case '4':
			env.drawMode = Env::DM_DESIGN;
			glutReshapeWindow(orgSize.x, orgSize.y);
			std::cout << "4 Draw design" << std::endl;
			break;
		case '5':
			env.drawMode = Env::DM_SHEET;
			glutReshapeWindow(orgSize.x, orgSize.y);
			std::cout << "5 Draw sheet" << std::endl;
			break;

		case 't':
			env.cameraMode = Env::CM_TILE;
			env.InitCamera();
			contents.Draw();
			std::cout << "t Tile camera" << std::endl;
			break;
		case 'w':
			env.cameraMode = Env::CM_WINDOW;
			env.InitCamera();
			contents.Draw();
			std::cout << "w World camera" << std::endl;
			break;
		case 'c':
			contents.mode = Contents::CO_CAM;
			contents.Draw();
			std::cout << "c Contents=camera" << std::endl;
			break;
		case 'e':
			contents.mode = Contents::CO_CEIL;
			contents.Draw();
			std::cout << "e Contents=ceil" << std::endl;
			break;
		case 'r':
			contents.mode = Contents::CO_RANDOM;
			contents.Draw();
			std::cout << "r Contents=random" << std::endl;
			break;
		case 'p':
			contents.mode = Contents::CO_PHOTO;
			contents.LoadPhoto();
			std::cout << "p Contents=photo" << std::endl;
			break;
		case 's':
			contents.mode = Contents::CO_SHIP;
			std::cout << "s Contents=ship" << std::endl;
			break;
		case 0x1b:
		case 'q':
			contents.Release();
			exit(0);
			break;
	}
	glutPostRedisplay();
}	


static Affined afMove;
void display(){
	glutPostRedisplay();
	contents.Draw(false);

	//	テクスチャへのレンダリング
//	afMove = Affined::Rot(Rad(0.3), 'y') * afMove;	
	for(int y=0; y<DIVY; ++y){
		for(int x=0; x<DIVX; ++x){
			env.cell[y][x].BeforeDrawTex();
			glMultMatrixd(afMove);
			glCallList(contents.list);
			env.cell[y][x].AfterDrawTex();
		}
	}
	
	//	メインのレンダリング
	glViewport(0, 0, windowSize.x, windowSize.y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, (GLfloat)windowSize.x/(GLfloat)windowSize.y, 0.01, 50.0);
	glMatrixMode(GL_MODELVIEW);
	
	//	マウス操作による視点の設定
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

	//	表示モードにあわせて表示
	env.Draw();
	glutSwapBuffers();
}


void setLight() {
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
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
}


int main(int argc, char* argv[]){
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutCreateWindow("mirror");
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	setLight();
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
//	glutIdleFunc(idle);

	env.Init();
	contents.Init();

	glutMainLoop();
}
