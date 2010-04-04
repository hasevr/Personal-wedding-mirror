#include "Env.h"
#include "Contents.h"

Env env;

void Env::InitMirror(){
	int y;
	for(y=0; y<DIVY; ++y){
		for(int x=0; x<DIVX; ++x){
			cell[y][x].Init(x,y);
		}
	}
	for(int x=0; x<2; ++x){
		cell[y][x].Init(x,y);
	}
	double depth = config.d;
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
void Env::InitCamera(){
	int y;
	for(y=0; y<DIVY; ++y){
		for(int x=0; x<DIVX; ++x){
			cell[y][x].InitCamera(0);
			cell[y][x].InitCamera(1);
		}
	}
	cell[y][0].InitFrontCamera(0);
	cell[y][0].InitFrontCamera(1);
}

void Env::InitSupport(){
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
					Vec3d diffOnMirror = config.t/2 / (lineOnMirror*line) * lineOnMirror;
					support.vPlane[x].vertices.push_back(ctr - diffOnMirror);
					double d = config.hSupportDepth - (config.hSupportGroove - 0.0006);
					support.vPlane[x].vertices.push_back(ctr + cell[y][x].inDirCenter*d - line*config.t/2);
					support.vPlane[x].vertices.push_back(ctr + cell[y][x].inDirCenter*d + line*config.t/2);
					support.vPlane[x].vertices.push_back(ctr + diffOnMirror);
				}

				if (i==1 && y==DIVY-1){
					Vec3d n = vtx;
					n.y = 0;
					n.unitize();
					vtx += n * config.hSupportDepth;
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
				double d1 = config.hSupportDepth;
				double d2 = config.hSupportDepth - config.hSupportGroove;
				if (x==0 && i==0) backs.push_back(vtx + cell[y][x].inDirCenter*d1);
				if (i==1){
					Vec3d ctr = cell[y][x].mirror.center;
					Vec3d line = (support.hPlane[y].normal ^ cell[y][x].inDirCenter).unit();
					backs.push_back(ctr + cell[y][x].inDirCenter*d1 - line*config.t/2);
					backs.push_back(ctr + cell[y][x].inDirCenter*d2 - line*config.t/2);
					backs.push_back(ctr + cell[y][x].inDirCenter*d2 + line*config.t/2);
					backs.push_back(ctr + cell[y][x].inDirCenter*d1 + line*config.t/2);
				}
				if (x==DIVX-1 && i==1) backs.push_back(vtx + cell[y][x].inDirCenter*d1);
			}
		}
		support.hPlane[y].vertices.insert(support.hPlane[y].vertices.end(), backs.rbegin(), backs.rend());
		//	DSTR << std::endl;
	}
}

void Env::PlaceSupport(){
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
		double interval[] = {0.003, 0.039, 0.038, 0.031, 0.027, 0.025};
		disp += interval[y];
		af = af * Affined::Trn(-0.254, -disp-0.33, 0);
		af = af.inv();
		sheets.back().loops.push_back(Loop());
		for(unsigned i=0; i<support.hPlane[y].vertices.size(); ++i) 
			sheets.back().loops.back().push_back(af * support.hPlane[y].vertices[i]);
	}

}
void Env::PlaceMirror(){
	sheets.push_back(Sheet());
	double yPos = 0;
	for(int y=0; y<DIVY; ++y){
		double lenMax=0;
		for(int x=0; x<DIVX; ++x){
			lenMax = std::max(lenMax, (cell[y][x].mirror.vertex[2] - cell[y][x].mirror.vertex[0]).norm());
		}
		yPos += lenMax + 0.002;
		for(int x=0; x<DIVX; ++x){
			Affined af;
			af.Ez() = cell[y][x].mirror.normal;
			af.Ex() = cell[y][x].mirror.vertex[1] - cell[y][x].mirror.vertex[0];
			af.Ex() -= af.Ex()*af.Ez() * af.Ez();
			af.Ex().unitize();
			af.Ey() = af.Ez() ^ af.Ex();
			af.Pos() = cell[y][x].mirror.vertex[0];
			af = af * Affined::Trn(x*0.037 -0.27, -yPos, 0);
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
void Env::InitGL(){
	int y;
	for(y=0; y<DIVY; ++y){
		for(int x=0; x<DIVX; ++x){
			cell[y][x].InitGL();
		}
	}
	for(int x=0; x<2; ++x){
		cell[y][x].InitGL();
	}
}
void Env::WritePs(){
	for(unsigned i=0; i<sheets.size(); ++i){
		std::ostringstream oss;
		oss << "sheet" << i << ".ps";
		std::ofstream of(oss.str().c_str());
		double un = 2834.646;	//	単位変換 m/pt
		for(unsigned j=0; j<sheets[i].loops.size(); ++j){
			of << "newpath" << std::endl;
			of << sheets[i].loops[j][sheets[i].loops[j].size()-1].x*un << " " 
				<< sheets[i].loops[j][sheets[i].loops[j].size()-1].y*un << " "
				<< "moveto" << std::endl;
			for(unsigned k=0; k<sheets[i].loops[j].size(); ++k){
				of << sheets[i].loops[j][k].x*un << " " 
					<< sheets[i].loops[j][k].y*un << " " << "lineto" << std::endl;
			}
			of << "stroke" << std::endl;
		}
		of << "showpage" << std::endl;
	}
}
void Env::Init(){
	dt = 1.0 / 60;
	drawMode = DM_DESIGN;
	front.Init();
	config.Init();
	projPose.Pos() = Vec3d(0, 0, -config.outY[1]+1.2);
	centerSeat = Vec3d(config.wall, 0, 0);
	InitMirror();
	InitCamera();
	InitSupport();
	PlaceMirror();
	PlaceSupport();
	WritePs();
	InitGL();
}

extern Affinef mouseView;
extern Vec2i windowSize;

void Env::RenderTex(int fb){
	//	テクスチャへのレンダリング
	for(int y=0; y<DIVY+1; ++y){
		for(int x=0; x<DIVX; ++x){
			if (y==DIVY && x>=2) break;
			env.cell[y][x].BeforeDrawTex(fb);
			glCallList(contents.list);
			env.cell[y][x].AfterDrawTex(fb);
		}
	}	
}
void Env::Draw(){
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	if (drawMode == DM_MIRROR) RenderTex(0);
	if (drawMode == DM_MIRROR_BACK) RenderTex(1);
	if (drawMode == DM_DESIGN){
		RenderTex(0);
		RenderTex(1);
	}
	//	メインのレンダリングの準備
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glViewport(0, 0, windowSize.x, windowSize.y);	//	ビューポートをWindow全体に
	glClearColor(0,0,0,1);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	//	表示モードにあわせて表示
	if (drawMode == DM_SHEET) DrawSheet();
	else if (drawMode == DM_MIRROR) DrawMirror(0);
	else if (drawMode == DM_MIRROR_BACK) DrawMirror(1);
	else if (drawMode == DM_FRONT) DrawFront();
	else if (drawMode == DM_BACK) DrawBack();
	else if (drawMode == DM_DESIGN) DrawDesign();
	else {
		std::cout << "Env::Draw() do not suppot mode " << drawMode << std::endl;
		assert(0);
	}
}

void Env::DrawSheet(){
	//	マウス操作による視点の設定
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, (GLfloat)windowSize.x/(GLfloat)windowSize.y, 0.01, 50.0);
	glMatrixMode(GL_MODELVIEW);	
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(mouseView.inv());
	//	描画
	glDisable(GL_LIGHTING);
	for(unsigned i=0; i<sheets.size(); ++i){
		glColor3d(1,1,1);
		for(unsigned j=0; j<sheets[i].loops.size(); ++j){
			glBegin(GL_LINE_LOOP);
			for(unsigned v=0; v<sheets[i].loops[j].size(); ++v){
				if (i==0){
					glVertex3dv(sheets[i].loops[j][v]);
				}else{
					glVertex3dv(Affined::Rot(Rad(180), 'y') * sheets[i].loops[j][v]);						
				}
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
}
void Env::DrawFront(){
/*
	glMatrixMode(GL_PROJECTION);
	Vec3d screen(0, front.hOff + front.h/2, front.d);
	Vec2d size(front.w, front.h);
	Affined projection = Affined::ProjectionGL(screen, size, 0.1, 1000);
	glLoadMatrixd(projection);
	glMatrixMode(GL_MODELVIEW);
	Affined view;
	view.LookAtGL(Vec3d(0, 0, 1), Vec3d(0,1,0));
	glLoadMatrixd(view.inv());
*/
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(cell[DIVY][0].projection[0]);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixd(cell[DIVY][0].view[0]);
	glCallList(contents.list);
}
void Env::DrawBack(){
/*	glMatrixMode(GL_PROJECTION);
	Vec3d screen(0, front.hOff + front.h/2, front.d);
	Vec2d size(front.w, front.h);
	Affined projection = Affined::ProjectionGL(screen, size, 0.1, 1000);
	glLoadMatrixd(projection);
	glMatrixMode(GL_MODELVIEW);
	Affined view;
	view.LookAtGL(Vec3d(0, 0, -1), Vec3d(0,1,0));
	glLoadMatrixd(view.inv());
*/
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(cell[DIVY][0].projection[1]);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixd(cell[DIVY][0].view[1]);
	glCallList(contents.list);
}

void Env::DrawMirror(int fb){
	glMatrixMode(GL_PROJECTION);
	Vec3d screen(0, config.hOff + config.h/2, config.d);
	Vec2d size(config.w, config.h);
	Affined projection = Affined::ProjectionGL(screen, size, 0.1, 1000);
	glLoadMatrixd(projection);
	glMatrixMode(GL_MODELVIEW);
	Affined view;
	view.LookAtGL(Vec3d(0, 0, 1), Vec3d(0,1,0));
	view = view * Affined::Rot(-config.projectionPitch, 'x');
	glLoadMatrixd(view.inv());
	glDisable(GL_LIGHTING);
	//	仕切りの線
	glLineWidth(10);
	glColor3d(0,0,0);
	glBegin(GL_LINES);
	for(int y=1; y<DIVY; ++y){
		glVertex3dv(cell[y][0].inPos[0]);
		glVertex3dv(cell[y][DIVX-1].inPos[1]);
	}
	for(int x=1; x<DIVX	; ++x){
		glVertex3dv(cell[0][x].inPos[0]);
		glVertex3dv(cell[DIVY-1][x].inPos[2]);
	}
	glEnd();
	//	鏡に写すべき映像を並べる
	glEnable(GL_TEXTURE_2D);
	for(int y=0; y<DIVY; ++y){
		for(int x=0; x<DIVX	; ++x){
			//	表示位置の虚像の表示
			glBindTexture(GL_TEXTURE_2D, cell[y][x].texName[fb]);
			glBegin(GL_TRIANGLE_STRIP);
			for(int i=0; i<4; ++i){
				glTexCoord2dv(cell[y][x].texCoord[fb][i]);
				glVertex3dv(cell[y][x].imagePos[i]);
			}
			glEnd();
		}
	}
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
}
void Env::DrawDesign(){
	//	マウス操作による視点の設定
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, (GLfloat)windowSize.x/(GLfloat)windowSize.y, 0.01, 50.0);
	glMatrixMode(GL_MODELVIEW);	
	glLoadMatrixf(mouseView.inv());

	//	座標軸
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

	//	映像の表示
	glPushMatrix();
	glMultMatrixd(projPose);
	DrawHalf(0);
	glPopMatrix();
	glPushMatrix();
	glMultMatrixd(Affined::Rot(Rad(180), 'y') * projPose);
	DrawHalf(1);
	glPopMatrix();
	
	glPushMatrix();
	DrawHalfFront(1);
	glMultMatrixd(Affined::Rot(Rad(180), 'y'));
	DrawHalfFront(0);
	glPopMatrix();
}
void Env::DrawHalfFront(int fb){
	glDisable(GL_LIGHTING);
	glColor3d(1,1,1);
	glPointSize(4);
	glBegin(GL_POINTS);
	glVertex3dv(cell[DIVY][0].outPosCenter);
	glEnd();
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, cell[DIVY][0].texName[fb]);
	glBegin(GL_TRIANGLE_STRIP);
	for(int i=0; i<4; ++i){
		glTexCoord2dv(cell[DIVY][0].texCoord[fb][i]);
		glVertex3dv(cell[DIVY][0].outPos[i]);
	}
	glEnd();
	glDisable(GL_TEXTURE_2D);
	//	OpenGLのカメラの描画域の表示
	glColor3d(1,1,1);
	glBegin(GL_LINE_LOOP);
	glVertex3dv(cell[DIVY][0].outPos[0]);
	glVertex3dv(cell[DIVY][0].outPos[1]);
	glVertex3dv(cell[DIVY][0].outPos[3]);
	glVertex3dv(cell[DIVY][0].outPos[2]);
	glEnd();				
	glEnable(GL_LIGHTING);
}
void Env::DrawHalf(int fb){
	//	描画
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
			//	鏡の描画
			//	鏡本体の描画
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, Vec4f((float)y/DIVY, (float)x/DIVX, 0, 1));
			glBegin(GL_LINE_LOOP);
			glNormal3dv(cell[y][x].mirror.normal);
			glVertex3dv(cell[y][x].mirror.vertex[0]);
			glVertex3dv(cell[y][x].mirror.vertex[1]);
			glVertex3dv(cell[y][x].mirror.vertex[3]);
			glVertex3dv(cell[y][x].mirror.vertex[2]);
			glEnd();

			//	鏡と表示位置の中心
			glDisable(GL_LIGHTING);
			glColor3dv(Vec3d((float)y/DIVY, (float)x/DIVX, 1.0));
			glPointSize(4);
			glBegin(GL_POINTS);
			glVertex3dv(cell[y][x].mirror.center);
			glVertex3dv(cell[y][x].outPosCenter);
			glEnd();
			//	鏡から、表示位置までの光線
			glColor3dv(Vec3d((float)y/DIVY, (float)x/DIVX, 0.5));
			glBegin(GL_LINES);
			glVertex3dv(cell[y][x].mirror.center);
			glVertex3dv(cell[y][x].outPosCenter);
			glEnd();
			
			//	表示位置の描画
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, cell[y][x].texName[fb]);
			glBegin(GL_TRIANGLE_STRIP);
			for(int i=0; i<4; ++i){
				glTexCoord2dv(cell[y][x].texCoord[fb][i]);
				glVertex3dv(cell[y][x].outPos[i]);
			}
			glEnd();
			//	表示位置の虚像の表示
			glBegin(GL_TRIANGLE_STRIP);
			for(int i=0; i<4; ++i){
				glTexCoord2dv(cell[y][x].texCoord[fb][i]);
				glVertex3dv(cell[y][x].imagePos[i]);
			}
			glEnd();

			glDisable(GL_TEXTURE_2D);
			//	OpenGLのカメラの描画域の表示
			glBegin(GL_LINE_LOOP);
			for(int i=0; i<4; ++i) glVertex3dv(cell[y][x].screen[fb][i]);
			glEnd();				

			glEnable(GL_LIGHTING);
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
	}
	for(int y=0; y<DIVY; ++y){
		glColor3dv(Vec3d(0,1,1));
		glBegin(GL_LINE_LOOP);
		for(std::vector<Vec3d>::iterator it = support.hPlane[y].vertices.begin(); it != support.hPlane[y].vertices.end(); ++it){
			glVertex3dv(*it);
		}
		glEnd();
	}
	glEnable(GL_LIGHTING);
}
void Env::Step(){
	contents.Step(dt);
	glutPostRedisplay();
}