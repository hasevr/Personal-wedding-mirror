#include <Springhead.h>
#include <GL/glut.h>
#include <sstream>
#include <fstream>
using namespace Spr;

double	CameraRotX = 0.0, CameraRotY = Rad(90.0), CameraZoom = 2.0;
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
		CameraRotX = Spr::max(Rad(-80.0), Spr::min(CameraRotX, Rad(80.0)));
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

void initialize(){
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	setLight();
}

void reshape(int w, int h){
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, (GLfloat)w/(GLfloat)h, 0.01, 50.0);
	glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int x, int y){
	if (key == 0x1b) exit(0);
	if (key == 'q') exit(0);
}	
#define DIVX	8
#define DIVY	6
struct Env{
	double w;
	double h;
	double hOff;
	double d;
	double projectionPitch;
	double outXRad[2];
	double ceil, wall;	//	天井の高さ、中央から壁の距離
	double outY[2];
	double t;			//	サポートの板厚
	double hSupportDepth;
	double hSupportGroove;
	void Init(){
		outXRad[0] = Rad(75);
		outXRad[1] = Rad(-75);
		outY[0] = 1.5;
		outY[1] = -6.0;
		ceil = 4.5-0.9;	//	机の高さが90cm?
		wall = 5;
		d = 1.2;
		hOff = 0.05;
		h = 0.53 - hOff;
		w = h*4/3;
		projectionPitch = Rad(30);
		double mul = (1.0/d) * 0.4;
		d *= mul;	hOff *= mul;	h *= mul;
		w *= mul;
		
		hSupportDepth = 0.02;		//	hSupportの長さは2cm
		hSupportGroove = 0.003;		//	hSupport側の溝 3mm
		t = 0.002;
	}
} env;
struct Mirror{
	Vec3d normal;		//	鏡の法線
	Vec3d vertex[4];	//	頂点
	Vec3d center;		//	鏡とinDirCenterの交点
};
struct Cell{
	int x, y;
	Vec3d inDirCenter;
	Vec3d outDirCenter;
	Vec3d inDir[4];
	Vec3d outDir[4];
	Mirror mirror;
	void Init(int xIn, int yIn){
		//	プロジェクタが原点。上がy、プロジェクタから出る光の向きがz、プロジェクタに向かって右がx
		//	inDirの計算：プロジェクタの仕様と分割数で決まる
		x = xIn;
		y = yIn;
		for(int i=0; i<4; ++i){
			inDir[i].x = -env.w/2 + (env.w/DIVX) * (x+i%2);
			inDir[i].y = env.hOff + (env.h/DIVY) * (y+i/2);
			inDir[i].z = env.d;
			inDir[i].unitize();
		}
		inDirCenter.x = -env.w/2 + (env.w/DIVX) * (x+0.5);
		inDirCenter.y = env.hOff + (env.h/DIVY) * (y+0.5);
		inDirCenter.z = env.d;
		inDirCenter.unitize();

		//	全体をx軸回転
		Quaterniond rot = Quaterniond::Rot(env.projectionPitch, 'x');
		for(int i=0; i<4; ++i){
			inDir[i] = rot * inDir[i];
		}
		inDirCenter = rot * inDirCenter;		

		//	outDirの計算
		//  dist(天井高)=ceil  H=16m のスクリーン。 outY[0]m ～ outY[1]m
		//	横は、-80度～80度で角度等間隔
		double radX = env.outXRad[0] + (env.outXRad[1]-env.outXRad[0]) * (x + (y%2)/2.0) / (DIVX-0.5);
		outDirCenter.x = tan(radX) * env.ceil;
		if (-env.wall<outDirCenter.x && outDirCenter.x < env.wall){
			outDirCenter.y = env.ceil;
		}else{
			outDirCenter.x = outDirCenter.x>0 ? env.wall : -env.wall;
			outDirCenter.y = tan(Rad(90)-radX) * env.wall;
			if (outDirCenter.y < 0) outDirCenter.y *= -1;
		}
		outDirCenter.z = env.outY[0]+(env.outY[1]-env.outY[0])*y/(DIVY-1);


		outDirCenter.unitize();


		mirror.normal = (-inDirCenter + outDirCenter).unit();
		for(int i=0; i<4; ++i){
			outDir[i] = -inDir[i] + 2*(inDir[i] - (inDir[i]*mirror.normal)*mirror.normal);
		}
	}
	void CalcPosition(double depth, int id){	//	頂点id がdepthになるようにする
		//	一番真ん中下に近いものの奥行きをdepthに設定する
		//	頂点は、-x -y, +x, -y, -x +y, +x +y の順
		mirror.vertex[id] = inDir[id] * (depth / inDir[id].z);
		double mirrorOff = mirror.normal * mirror.vertex[id];
		for(int i=0; i<4; ++i){
			mirror.vertex[i] = mirrorOff / (inDir[i] * mirror.normal) * inDir[i];
		}
		mirror.center = mirrorOff / (inDirCenter * mirror.normal) * inDirCenter;
	}
	Vec3d OutPos(int id){
		Vec3d oc = outDir[id];
		oc *= env.ceil / oc.y;
		if (oc.x > env.wall) oc *= env.wall/oc.x;
		if (oc.x < -env.wall) oc *= -env.wall/oc.x;
		return mirror.vertex[id] + oc;
	}
};
Cell cell[DIVY][DIVX];

struct Plane{
	Vec3d normal;
	Vec3d dir;
	std::vector<Vec3d> vertices;
};
struct Support{
	Plane vPlane[DIVX];
	Plane hPlane[DIVY];
	double baseHeight;
	double yMin;
} support;

void initMirror(){
	for(int y=0; y<DIVY; ++y){
		for(int x=0; x<DIVX; ++x){
			cell[y][x].Init(x,y);
		}
	}
	double depth = env.d;
	for(int y=0; y<DIVY; ++y){
		cell[y][DIVX/2-1].CalcPosition(depth, 1);
		cell[y][DIVX/2].CalcPosition(depth, 0);
		for(int i=1; i<DIVX/2; ++i){
			cell[y][DIVX/2-i-1].CalcPosition(cell[y][DIVX/2-i].mirror.vertex[0].z, 1);
			cell[y][DIVX/2+i].CalcPosition(cell[y][DIVX/2+i-1].mirror.vertex[1].z, 0);
		}
		depth = cell[y][DIVX/2].mirror.vertex[2].z;
	}
}
void initSupport(){
	//	サポートの設計
	support.baseHeight = 0.005;	//	床から5mmの位置が一番低い鏡

	//	鏡のYの最小値を求める
	support.yMin = DBL_MAX;
	for(int y=0; y<DIVY; ++y){
		for(int x=0; x<DIVX; ++x){
			for(int i=0; i<4;++i){
				support.yMin = min(cell[y][x].mirror.vertex[i].y , support.yMin);
			}
		}
	}
	support.yMin -= support.baseHeight;

	//	支持面の設計。支持面は原点と mirror.center を通る
	//	vPlane 
	for(int x=0; x<DIVX; ++x){
		support.vPlane[x].normal = (cell[0][x].mirror.center ^ cell[DIVY-1][x].mirror.center).unit();
		support.vPlane[x].dir = (Vec3d(0,1,0) - support.vPlane[x].normal.y * support.vPlane[x].normal).unit();
		/*
		//	for debug
		Vec3d n1 = support.vPlane[x].normal;
		Vec3d n2 = (cell[0][x].mirror.center ^ cell[1][x].mirror.center).unit();
		DSTR << n1 << n2 << ((n1-n2).norm() < 0.0001?"same ":"diff ");
		*/
		for(int y=0; y<DIVY; ++y){
			for(int i=0; i<2; ++i){
				// (vertex[0] + k*dir) * normal = 0
				//	k = -vertex[0]*normal / dir*normal
				Vec3d dir = cell[y][x].mirror.vertex[2*i+1] - cell[y][x].mirror.vertex[2*i];
				Vec3d vtx = cell[y][x].mirror.vertex[2*i] - 
					(cell[y][x].mirror.vertex[2*i] * support.vPlane[x].normal) / (dir * support.vPlane[x].normal) * dir;
				//DSTR << vtx * support.vPlane[x].normal << " ";
				if (i==0 && y==0){
					Vec3d vBegin = vtx;
					vBegin += (support.yMin - vBegin.y) / support.vPlane[x].dir.y * support.vPlane[x].dir;
					support.vPlane[x].vertices.push_back(vBegin);
				}
				support.vPlane[x].vertices.push_back(vtx);
				
				if (i==0){
					Vec3d ctr = cell[y][x].mirror.center;
					Vec3d line = (support.vPlane[x].normal ^ cell[y][x].inDirCenter).unit();
					Vec3d lineOnMirror = (support.vPlane[x].normal ^ cell[y][x].mirror.normal).unit();
					Vec3d diffOnMirror = env.t/2 / (lineOnMirror*line) * lineOnMirror;
					support.vPlane[x].vertices.push_back(ctr - diffOnMirror);
					double d = env.hSupportDepth - (env.hSupportGroove - 0.0006);
					support.vPlane[x].vertices.push_back(ctr + cell[y][x].inDirCenter*d - line*env.t/2);
					support.vPlane[x].vertices.push_back(ctr + cell[y][x].inDirCenter*d + line*env.t/2);
					support.vPlane[x].vertices.push_back(ctr + diffOnMirror);
				}

				if (i==1 && y==DIVY-1){
					Vec3d n = vtx;
					n.y = 0;
					n.unitize();
					vtx += n * env.hSupportDepth;
					support.vPlane[x].vertices.push_back(vtx);
					vtx += (support.yMin - vtx.y) / support.vPlane[x].dir.y * support.vPlane[x].dir;
					support.vPlane[x].vertices.push_back(vtx);
				}
			}
		}
		//	DSTR << std::endl;
	}

	//	hPlane
	for(int y=0; y<DIVY; ++y){
		support.hPlane[y].normal = (cell[y][0].mirror.center ^ cell[y][DIVX-1].mirror.center).unit();
		support.hPlane[y].dir = (Vec3d(1,0,0) - support.hPlane[y].normal.x * support.hPlane[y].normal).unit();
		std::vector<Vec3d> backs;
		for(int x=0; x<DIVX; ++x){
			for(int i=0; i<2; ++i){
				Vec3d dir = cell[y][x].mirror.vertex[i+2] - cell[y][x].mirror.vertex[i];
				Vec3d vtx = cell[y][x].mirror.vertex[i] - 
					(cell[y][x].mirror.vertex[i] * support.hPlane[y].normal) / (dir * support.hPlane[y].normal) * dir;
				support.hPlane[y].vertices.push_back(vtx);
				double d1 = env.hSupportDepth;
				double d2 = env.hSupportDepth - env.hSupportGroove;
				if (x==0 && i==0) backs.push_back(vtx + cell[y][x].inDirCenter*d1);
				if (i==1){
					Vec3d ctr = cell[y][x].mirror.center;
					Vec3d line = (support.hPlane[y].normal ^ cell[y][x].inDirCenter).unit();
					backs.push_back(ctr + cell[y][x].inDirCenter*d1 - line*env.t/2);
					backs.push_back(ctr + cell[y][x].inDirCenter*d2 - line*env.t/2);
					backs.push_back(ctr + cell[y][x].inDirCenter*d2 + line*env.t/2);
					backs.push_back(ctr + cell[y][x].inDirCenter*d1 + line*env.t/2);
				}
				if (x==DIVX-1 && i==1) backs.push_back(vtx + cell[y][x].inDirCenter*d1);
			}
		}
		support.hPlane[y].vertices.insert(support.hPlane[y].vertices.end(), backs.rbegin(), backs.rend());
		//	DSTR << std::endl;
	}
}


struct Loop: public std::vector<Vec3d>{
};
struct Sheet{
	std::vector<Loop> loops;
};
std::vector<Sheet> sheets;

void placeSupport(){
	sheets.push_back(Sheet());
	for(int x=0; x<DIVX; ++x){
		Affined af;
		af.Ez() = support.vPlane[x].normal;
		af.Ex() = support.vPlane[x].vertices.front() - support.vPlane[x].vertices.back();
		af.Ex() -= af.Ex()*af.Ez() * af.Ez();
		af.Ex().unitize();
		af.Ey() = af.Ez() ^ af.Ex();
		Vec2d disp[] = {Vec2d(0.19, 0.165), Vec2d(0.19+0.214, 0.165), Vec2d(0.19, 0.165+0.165), Vec2d(0.19+0.214, 0.165+0.165)};
		if (x%2){
			af = af * Affined::Rot(Rad(180), 'z');
			af.Pos() = support.vPlane[x].vertices.back();
			af = af * Affined::Trn(-0.043-disp[x/2].x,0.16-disp[x/2].y, 0);
		}else{
			af.Pos() = support.vPlane[x].vertices.front();
			af = af * Affined::Trn(-disp[x/2].x, -disp[x/2].y, 0);
		}
		af = af.inv();
		sheets.back().loops.push_back(Loop());
		for(unsigned i=0; i<support.vPlane[x].vertices.size(); ++i) 
			sheets.back().loops.back().push_back(af * support.vPlane[x].vertices[i]);
	}
	double disp = 0;
	for(int y=0; y<DIVY; ++y){
		Affined af;
		af.Ez() = support.hPlane[y].normal;
		af.Ex() = support.hPlane[y].vertices[0] - support.hPlane[y].vertices[DIVX*2-1];
		af.Ex() -= af.Ex()*af.Ez() * af.Ez();
		af.Ex().unitize();
		af.Ey() = af.Ez() ^ af.Ex();
		af.Pos() = support.hPlane[y].vertices[0];
		double interval[] = {0.005, 0.03, 0.035, 0.03, 0.03, 0.03};
		disp += interval[y];
		af = af * Affined::Trn(-0.254, -disp-0.33, 0);
		af = af.inv();
		sheets.back().loops.push_back(Loop());
		for(unsigned i=0; i<support.hPlane[y].vertices.size(); ++i) 
			sheets.back().loops.back().push_back(af * support.hPlane[y].vertices[i]);
	}

}
void placeMirror(){
	sheets.push_back(Sheet());
	for(int y=0; y<DIVY; ++y){
		for(int x=0; x<DIVX; ++x){
			Affined af;
			af.Ez() = cell[y][x].mirror.normal;
			af.Ex() = cell[y][x].mirror.vertex[1] - cell[y][x].mirror.vertex[0];
			af.Ex() -= af.Ex()*af.Ez() * af.Ez();
			af.Ex().unitize();
			af.Ey() = af.Ez() ^ af.Ex();
			af.Pos() = cell[y][x].mirror.vertex[0];
			af = af * Affined::Trn(x*0.037 -0.27, y*0.045 -0.26, 0);
			af = af.inv();
			sheets.back().loops.push_back(Loop());
			for(int i=0; i<2; ++i){
				sheets.back().loops.back().push_back(af * cell[y][x].mirror.vertex[i]);
			}
			for(int i=3; i>=2; --i){
				sheets.back().loops.back().push_back(af * cell[y][x].mirror.vertex[i]);
			}
		}
	}
}


bool showSheets = true;

void display(){
	Affinef view;
	view.Pos() = CameraZoom * Vec3f(
		cos(CameraRotX) * cos(CameraRotY),
		sin(CameraRotX),
		cos(CameraRotX) * sin(CameraRotY));
	view.LookAtGL(Vec3f(0.0, 0.0, 0.0), Vec3f(0.0f, 100.0f, 0.0f));

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(view.inv());
	glClearColor(0,0,0,1);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	

	if (showSheets){
		glDisable(GL_LIGHTING);
		for(unsigned i=0; i<sheets.size(); ++i){
			glColor3d((i+1)&1?1:0, (i+1)&2?1:0 , (i+1)&4?1:0);
			for(unsigned j=0; j<sheets[i].loops.size(); ++j){
				glBegin(GL_LINE_LOOP);
				for(unsigned v=0; v<sheets[i].loops[j].size(); ++v){
					glVertex3dv(sheets[i].loops[j][v]);
				}
				glEnd();
			}
		}
		glBegin(GL_LINES);
		glColor3d(1,0,0);
		glVertex3d(-1, 0, 0);
		glVertex3d(1, 0, 0);
		glColor3d(0,1,0);
		glVertex3d(0, -1, 0);
		glVertex3d(0, 1, 0);
		glEnd();

		glEnable(GL_LIGHTING);
	}else{
		Affinef af;
		af.Pos() = -Affined::Rot(env.projectionPitch, 'x') * Vec3d(0,0,env.d);
		glMultMatrixf(af);

		glDisable(GL_LIGHTING);
		glBegin(GL_LINES);
		for(int i=0; i<3; ++i){
			Vec3d v;
			v[i] = 1;
			glColor3dv(v);
			glVertex3dv(Vec3d());
			glVertex3dv(v);
		}
		glEnd();
		glEnable(GL_LIGHTING);

		for(int y=0; y<DIVY; ++y){
			for(int x=0; x<DIVX	; ++x){
				Vec3d mirrorCenter;
				for(int i=0; i<4; ++i) mirrorCenter += cell[y][x].mirror.vertex[i];
				mirrorCenter /= 4;
				glDisable(GL_LIGHTING);
				glColor3dv(Vec3d((float)y/DIVY, (float)x/DIVX, 0.5));
				glPointSize(4);
				glBegin(GL_POINTS);
				glVertex3dv(mirrorCenter);
				Vec3d oc = cell[y][x].outDirCenter;
				oc *= env.ceil / oc.y;
				if (oc.x > env.wall) oc *= env.wall/oc.x;
				if (oc.x < -env.wall) oc *= -env.wall/oc.x;
				glVertex3dv(mirrorCenter + oc);
				glEnd();
				glColor3dv(Vec3d((float)y/DIVY, (float)x/DIVX, 0.5) * 0.5);
				glBegin(GL_TRIANGLE_STRIP);
				for(int i=0; i<4; ++i) glVertex3dv(cell[y][x].OutPos(i));
				glEnd();
				
				glBegin(GL_LINES);
				glVertex3dv(mirrorCenter);
				glVertex3dv(mirrorCenter + oc);
				glEnd();

				glColor3dv(Vec3d(1,1,1));
				glBegin(GL_POINTS);
				glVertex3dv(cell[y][x].inDirCenter);
				glEnd();
				glEnable(GL_LIGHTING);

				glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, Vec4f((float)y/DIVY, (float)x/DIVX, 0, 1));
				glBegin(GL_TRIANGLE_STRIP);
				for(int i=0; i<4; ++i){
					glNormal3dv(cell[y][x].mirror.normal);
					glVertex3dv(cell[y][x].mirror.vertex[i]);
				}
				glEnd();
			}
		}	
		//	supportの表示
		for(int x=0; x<DIVX; ++x){
			glDisable(GL_LIGHTING);
			glColor3dv(Vec3d(1,1,0));
			glBegin(GL_LINE_LOOP);
			for(std::vector<Vec3d>::iterator it = support.vPlane[x].vertices.begin(); it != support.vPlane[x].vertices.end(); ++it){
				glVertex3dv(*it);
			}
			glEnd();
			glColor3dv(Vec3d(0,1,1));
			glBegin(GL_LINE_LOOP);
			for(std::vector<Vec3d>::iterator it = support.hPlane[x].vertices.begin(); it != support.hPlane[x].vertices.end(); ++it){
				glVertex3dv(*it);
			}
			glEnd();
			glEnable(GL_LIGHTING);
		}
	}
	glutSwapBuffers();
}

void writeDxf(){
	for(unsigned i=0; i<sheets.size(); ++i){
		std::ostringstream oss;
		oss << "sheet" << i << ".ps";
		std::ofstream of(oss.str().c_str());
		of << "/m { 2834.646 mul } def" << std::endl;	//	mmの定義
		for(unsigned j=0; j<sheets[i].loops.size(); ++j){
			of << "newpath" << std::endl;
			of << sheets[i].loops[j][sheets[i].loops[j].size()-1].x << " m " 
				<< sheets[i].loops[j][sheets[i].loops[j].size()-1].y << " m "
				<< "moveto" << std::endl;
			for(unsigned k=0; k<sheets[i].loops[j].size(); ++k){
				of << sheets[i].loops[j][k].x << " m " 
					<< sheets[i].loops[j][k].y << " m " << "lineto" << std::endl;
			}
			of << "stroke" << std::endl;
		}
		of << "showpage" << std::endl;
	}
}


int main(int argc, char* argv[]){
	env.Init();
	initMirror();
	initSupport();
	placeMirror();
	placeSupport();
	writeDxf();

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

	glutMainLoop();
}
