#include "Env.h"
#ifdef USE_GLEW
#include <GL/glew.h>
#endif
#include <GL/glut.h>
#include <sstream>
#include <fstream>
#include "Mirror.h"
using namespace Spr;


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
		CameraRotY = Spr::max(Rad(-180.0), Spr::min(CameraRotY, Rad(180.0)));
		CameraRotX += yrel * 0.01;
		CameraRotX = Spr::max(Rad(-90.0), Spr::min(CameraRotX, Rad(90.0)));
	}
	// 右ボタン
	if(bRightButton){
		CameraZoom *= exp(yrel/10.0);
		CameraZoom = Spr::max(0.1, Spr::min(CameraZoom, 100.0));
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

void keyboard(unsigned char key, int x, int y){
	if (key == 'w') env.drawMode = Env::DM_WORLD;
	if (key == 'd') env.drawMode = Env::DM_DESIGN;
	if (key == 's') env.drawMode = Env::DM_SHEET;
	if (key == 'm') {
		env.drawMode = Env::DM_MIRROR;
		glutFullScreen();
	}
	if (key == 'f') env.drawMode = Env::DM_FRONT;

	if (key == 't'){
		env.cameraMode = Env::CM_TILE;
		env.InitCamera();
	}
	if (key == 'l'){
		env.cameraMode = Env::CM_WINDOW;
		env.InitCamera();
	}

	if (key == 0x1b) exit(0);
	if (key == 'q') exit(0);
	glutPostRedisplay();
}	

void drawContents(){
	glNewList(contents, GL_COMPILE);
	glDisable(GL_LIGHTING);
	glLineWidth(5);
#if 0
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
#elif 0
//	glMultMatrixd(Affined::Rot(Rad(90), 'z'));
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
#else
	double d = -10;
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

#endif
	glLineWidth(1);
	glEnable(GL_LIGHTING);
//	glTranslated(0, 20, 0);
//	glutSolidTeapot(4);
	glEndList();
}


static Affined afMove;
void display(){
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
	env.Init();
	drawContents();
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
//	glutIdleFunc(idle);

	glutMainLoop();
}
