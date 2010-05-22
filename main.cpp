#include "Env.h"
#include "Mirror.h"
#include "Contents.h"
#include "DShowRecv.h"


#include <string>
#include <sstream>
#include <fstream>
#include <io.h>

#include <windows.h>

using namespace Spr;


double	CameraRotX = Rad(0), CameraRotY = Rad(-90.0), CameraZoom = 12.0;
bool bLeftButton = false, bRightButton = false;
int xlast, ylast;
Affinef mouseView;
void __cdecl mouse(int button, int state, int x, int y){
	xlast = x, ylast = y;
	if(button == GLUT_LEFT_BUTTON)
		bLeftButton = (state == GLUT_DOWN);
	if(button == GLUT_RIGHT_BUTTON)
		bRightButton = (state == GLUT_DOWN);
	glutPostRedisplay();
}

bool bFirstMouseMotion = false;
void __cdecl motion(int x, int y){
	int xrel = x - xlast, yrel = y - ylast;
	xlast = x;
	ylast = y;
	if(bFirstMouseMotion){
		bFirstMouseMotion = false;
		return;
	}
	// 左ボタン
	if(bLeftButton){
		CameraRotY -= xrel * 0.01;
		CameraRotY = max(Rad(-180.0), min(CameraRotY, Rad(180.0)));
		CameraRotX -= yrel * 0.01;
		CameraRotX = max(Rad(-90.0), min(CameraRotX, Rad(90.0)));
	}
	// 右ボタン
	if(bRightButton){
		CameraZoom *= exp(yrel/300.0);
		CameraZoom = max(0.1, min(CameraZoom, 100.0));
	}
	mouseView.Pos() = CameraZoom * Vec3f(
		cos(CameraRotX) * cos(CameraRotY),
		sin(CameraRotX),
		cos(CameraRotX) * sin(CameraRotY));
	mouseView.LookAtGL(Vec3f(), Vec3f(0.0f, 100.0f, 0.0f));
	mouseView.Pos() += -10*mouseView.Ez();

	glutPostRedisplay();
}


Vec2i windowSize;
void reshape(int w, int h){
	windowSize.x = w;
	windowSize.y = h;
}

bool bFullScreen;
Vec2i orgPos;
Vec2i orgSize;
void fullScreen(){
	glutFullScreen();
	bFullScreen = true;
}
void saveWindow(){
	if (!bFullScreen && (orgSize.x != 1024 || orgSize.x != 1280)){
		orgPos.x = glutGet(GLUT_WINDOW_X);
		orgPos.y = glutGet(GLUT_WINDOW_Y);
		orgSize.x = glutGet(GLUT_WINDOW_WIDTH);
		orgSize.y = glutGet(GLUT_WINDOW_HEIGHT);
	}
}
void loadWindow(){
	if (bFullScreen){
		glutPositionWindow(orgPos.x, orgPos.y);
		glutReshapeWindow(orgSize.x, orgSize.y);
		bFullScreen = false;
	}
}
void keyboard(unsigned char key, int x, int y){
	switch (key){
		case '1':
			env.drawMode = Env::DM_MIRROR;
			saveWindow();
			fullScreen();
			std::cout << "1 Draw mirror" << std::endl;
			break;
		case '2':
			env.drawMode = Env::DM_MIRROR_BACK;
			saveWindow();
			fullScreen();
			std::cout << "1 Draw mirror back" << std::endl;
			break;
		case '3':
			env.drawMode = Env::DM_FRONT;
			saveWindow();
			fullScreen();
			std::cout << "2 Draw front" << std::endl;
			break;
		case '4':
			env.drawMode = Env::DM_BACK;
			saveWindow();
			fullScreen();
			std::cout << "2 Draw back" << std::endl;
			break;
		case '5':
			env.drawMode = Env::DM_DESIGN;
			loadWindow();
			env.bZoomMirror = !env.bZoomMirror;
			std::cout << "5 Draw design" << std::endl;
			break;
		case '6':
			env.drawMode = Env::DM_SHEET;
			loadWindow();
			std::cout << "6 Draw sheet" << std::endl;
			break;

		case '0':
			std::cout << "0 Show the property sheet of the camera" << std::endl;
			dshowRecv.Prop();
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
		case 'l':
			std::cout << "l LoadPhoto...";
			contents.LoadPhoto();
			std::cout << " done." << std::endl;
			break;
		case 's':
			contents.mode = Contents::CO_SHIP;
			contents.ResetShip();
			std::cout << "s Contents=ship" << std::endl;
			break;
		case 0x1b:
		case 'q':
			contents.Release();
			dshowRecv.Release();
			exit(0);
			break;
	}
	glutPostRedisplay();
}	


void display(){
	contents.Draw(false);
	env.Draw();
	glutSwapBuffers();
}

void idle(){
	static int count;
	static double next;
	double time = timeGetTime() / 1000.0;
	if (!next) next = time + env.dt;
	while (next < time){
		env.Step();
		count ++;
		next += env.dt;
	}
	if (!dshowRecv.IsGood() && count%10==0){
		dshowRecv.Init();
	}
	EnterCriticalSection(&dshowRecv.mySrc.csec);
	while (dshowRecv.mySrc.keys.size()){
		keyboard(dshowRecv.mySrc.keys.front(), 0, 0);
		dshowRecv.mySrc.keys.pop_front();
	}
	LeaveCriticalSection(&dshowRecv.mySrc.csec);

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
	glutReshapeWindow(800, 600);
	glutPositionWindow(50, 50);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	glEnable(GL_TEXTURE_2D);
	setLight();
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	motion(0,0);
	bFirstMouseMotion = true;
	glutMotionFunc(motion);
	glutIdleFunc(idle);

	env.Init();
	contents.Init();

	glutMainLoop();
}


