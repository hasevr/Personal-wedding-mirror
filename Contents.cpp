#include "Contents.h"
#include "Env.h"


Contents contents;
Contents::Contents():list(0){
	cvTex = 0;
	ResetShip();
}
void Contents::LoadPhoto(){
	if (decals.size() || backs.size()) return;
	decals.folderName = "texs";
	decals.Load();
	backs.folderName = "backs";
	backs.Load();
	fairies.decals.folderName = "fairies";
	fairies.decals.Load();
	for(unsigned i=0; i<fairies.decals.size(); ++i){
		fairies.decals[i].sheetSize = Vec2d(0.5, 0.5);
	}
	for(unsigned i=0; i<fairies.size(); ++i){
		fairies[i].decal = &fairies.decals[i%fairies.decals.size()];
	}
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

void Contents::Step(double dt){
	fairies.Update(dt);
	
	if (startCount < decals.size() && decals[startCount-1].time > 4){
		startCount++;
	}
	for(unsigned i=0; i< startCount && i<decals.size(); ++i){
		decals[i].time += dt;
		decals[i].posture = paths[i%paths.size()].GetPose(decals[i].time);
		decals[i].color.w = paths[i%paths.size()].GetAlpha(decals[i].time);
	}
	if (backs.size()){
		backs.front().texOffset.y += 0.02 * dt;
	}
}
void Contents::ResetShip(){
	startCount = 1;	
	for(unsigned i=0; i<decals.size(); ++i){
		decals[i].time = 0;
	}
}

void Contents::DrawShip(){
	glDisable(GL_LIGHTING);
	for(unsigned i=0; i<startCount && i<decals.size(); ++i){
		decals[i].Draw();
	}
	if (backs.size()){
		double size = 200;
		backs.front().sheetSize = Vec2d(size, size*100)*2;
		if (env.drawMode == Env::DM_FRONT || env.drawMode == Env::DM_BACK){
			backs.front().texScale = Vec2d(3, 100) * 0.1;
		}else{
			backs.front().texScale = Vec2d(1, 100) * 3;
		}
		backs.front().posture = Affined::Trn(0,0,size);
		backs.front().posture = Affined::Rot(Rad(-90), 'x') * backs.front().posture;
		backs.front().Draw();
		backs.front().posture = Affined::Rot(Rad(90), 'z') * backs.front().posture;
		backs.front().Draw();
		backs.front().posture = Affined::Rot(Rad(180), 'z') * backs.front().posture;
		backs.front().Draw();
	}
	glEnable(GL_LIGHTING);
}

void Contents::DrawFairy(double alpha){
	if (env.drawMode == Env::DM_FRONT) return;	//	フロントスクリーンには妖精は表示しない。

	for(unsigned i=0; i<fairies.decals.size(); ++i) fairies.decals[i].color.A() = alpha;
	glDisable(GL_LIGHTING);
	for(unsigned i=0; i<fairies.size(); ++i){
		fairies[i].Draw();
	}
	glEnable(GL_LIGHTING);
}


void Contents::DrawRandom(){
	srand(0);
	glClearColor(0.2,0.4,0.7,1);
	glClear( GL_COLOR_BUFFER_BIT);
	static Affined af;
	af = Affined::Rot(Rad(0.1), 'y') * af;
	glMultMatrixd(af);
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
	glColor4f(1,1,1,1);
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
void Contents::DrawColor(Vec4d c){
	glClearColor(c.R()*255, c.G()*255, c.B()*255, c.A()*255);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

void Contents::DrawCam(){
	glColor4f(1,1,1,1);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, cvTex);
	if (env.cameraMode == Env::CM_TILE){
		//	カメラ映像の拡大率（映像テクスチャの距離）
//		double d = -3.5;
		double d = -2;
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2dv(cvTexCoord[0]);
		glVertex3d( 1,-1*0.75, d);
		glTexCoord2dv(cvTexCoord[1]);
		glVertex3d(-1,-1*0.75, d);
		glTexCoord2dv(cvTexCoord[2]);
		glVertex3d( 1, 1*0.75, d);
		glTexCoord2dv(cvTexCoord[3]);
		glVertex3d(-1, 1*0.75, d);
		glEnd();
	}else{
		double d = 5;
		double x = 15.0/2;
		double z = 20.0/2;
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2dv(cvTexCoord[0]);
		glVertex3d( x, d,  z);
		glTexCoord2dv(cvTexCoord[1]);
		glVertex3d( x, d, -z);
		glTexCoord2dv(cvTexCoord[2]);
		glVertex3d(-x, d,  z);
		glTexCoord2dv(cvTexCoord[3]);
		glVertex3d(-x, d, -z);
		glEnd();
	}
	glDisable(GL_TEXTURE_2D);		
}

void Contents::Draw(bool isInit){
	if (!isInit && (/*mode==CO_RANDOM || */mode==CO_CEIL || mode==CO_TILE) ) 
		return;

	glNewList(list, GL_COMPILE);
	glDisable(GL_LIGHTING);
	glLineWidth(5);
	static int fairyCount=0;
	const double fadeLen = 20;
	switch(mode){
		case CO_CAM:
			DrawCam();
			break;
		case CO_BLACK:
			if (fairyCount > 0){
				fairyCount --;
				DrawFairy(fairyCount/fadeLen);
			}else{
				DrawColor(Vec4d(0,0,0,1));
			}
			break;
		case CO_WHITE:
			DrawColor(Vec4d(1,1,1,1));
			break;
		case CO_FAIRY:
			if (fairyCount < fadeLen){
				fairyCount ++;
				DrawFairy(fairyCount/fadeLen);
			}
			else DrawFairy(1);
			break;
		case CO_SHIP:
			DrawShip();
			break;

		//	使わないモード
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
	mode = CO_BLACK;
	//	Fairyの生成
	for(int i=0; i<100; ++i){
		fairies.push_back(Fairy());
	}

	//	パスの設定
	Vec3d frontDir(0, env.front.hOff + env.front.h/2, env.front.d);
	frontDir /= frontDir.z;
	Vec3d backDir = frontDir;
	backDir.z *= -1;
	//	遠くから
#if 0
	double alpha = 1.2;
	double delta = 0.3;
	for(int i=6; i>=0; --i){
		pose.Pos() = frontDir * 20*pow(alpha, i);
		paths.back().push_back(Key(0, delta, pose));
	}
#endif
	double speed = 0.5;
	for(int i=0; i<2; ++i){
		Posed pose;
		paths.push_back(Path());
		double scale = 63;
//		pose.Pos() = frontDir * 4*scale + Vec3d(i?10:-10, 0, 0) ;
//		paths.back().push_back(Key(0, 12/speed, pose, 0));
		pose.Pos() = frontDir * 2.5*scale + Vec3d(i?5:-5, 0, 0) ;
		paths.back().push_back(Key(0, 12/speed, pose, 0));
		pose.Pos() = frontDir * 1*scale + Vec3d(i?-1:1, 0, 0) ;
		paths.back().push_back(Key(0, 0.4/speed, pose));
		static double yOff = 14;	//13;
		double xOff = (i==0 ? 1 : -1)*6.2;
		Affined af;
		double lookDiff = -std::abs(xOff) * 10000000;	//	実は完全に水平=無限倍で良い

		af.Pos() = Vec3d(xOff, yOff, 45);
		af.LookAtGL(Vec3d(0, yOff+lookDiff, 45), Vec3d(0,1,0));
		paths.back().push_back(Key(0, 6/speed, Posed(af)));

		af.Pos() = Vec3d(xOff, yOff, 15);
		af.LookAtGL(Vec3d(0, yOff+lookDiff, 15), Vec3d(0,1,0));
		paths.back().push_back(Key(0, 6/speed, Posed(af)));
		
		//-------------------------対称線-----------------------

		af.Pos() = Vec3d(xOff, yOff, -15);
		af.LookAtGL(Vec3d(0, yOff+lookDiff, -15), Vec3d(0,1,0));
		paths.back().push_back(Key(0, 6/speed, Posed(af)));

		af.Pos() = Vec3d(xOff, yOff, -45);
		af.LookAtGL(Vec3d(0, yOff+lookDiff, -45), Vec3d(0,1,0));
		paths.back().push_back(Key(0, 0.4/speed, Posed(af)));

		if (i==0) pose.Ori() = Quaterniond::Rot(Rad(180), 'y');
		else pose.Ori() = Quaterniond::Rot(Rad(-180), 'y');
		pose.Pos() = backDir * 1*scale;
		paths.back().push_back(Key(0, 12/speed, pose));
		pose.Pos() = backDir * 2.5*scale;
		paths.back().push_back(Key(0, 12/speed, pose));
		pose.Pos() = backDir * 4*scale;
		paths.back().push_back(Key(1, 0, pose, 0));
	}

	glGenTextures( 1, &cvTex);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	//	初期コンテンツの描画
	list = glGenLists(1);
	Draw(true);
}
void Contents::Release(){
	decals.Release();
}
void Contents::Capture(unsigned char* src, unsigned len){
	if (!cvTex) return;
	static int wIn=0, hIn=0, w, h, nc;
	if (wIn==0){
		nc = 3;
		if (len==640*480*nc){ wIn=640; hIn=480;}
		else if (len==320*240*nc){ wIn=320; hIn=240;}
		else if (len==160*120*nc){ wIn=160; hIn=120;}
		else {
			DSTR << "unknown size: len: " << len << " = 640*" << len/640 
				<< " = 400*" << len/400 << 
				" = 768*" << len/768 << std::endl;
		}
		w = min(wIn, (int)CVTEX_SIZE);
		h = min(hIn, (int)CVTEX_SIZE);
		double tx = (double)w/CVTEX_SIZE;
		double ty =(double)h/CVTEX_SIZE;
		cvTexCoord[3] = Vec2d(0, ty);
		cvTexCoord[2] = Vec2d(tx, ty);
		cvTexCoord[1] = Vec2d(0, 0);
		cvTexCoord[0] = Vec2d(tx, 0);
		DSTR << "size:" << wIn << "x" << hIn << std::endl;
	}
	for(int y=0; y<h; ++y){
		memcpy(cameraTexBuf[y], src + (y*wIn*nc), w*nc);
	}
}
void Contents::UpdateCameraTex(){
	//	texBuf
	glBindTexture( GL_TEXTURE_2D, cvTex );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, CVTEX_SIZE, CVTEX_SIZE, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, cameraTexBuf);
}
