#include "Mirror.h"
#include "Env.h"

void Front::Init(){
	w = 2;
	h = 1.5;
	d = 15.5;
	hOff = 1;
}
void Config::Init(){
	outXRad[0] = Rad(75);
	outXRad[1] = Rad(-75);
	outY[0] = 1;		//	31m だから、片側 13m と考える
	outY[1] = -12.0;		//	
	ceil = 4.5-0.9;	//	机の高さが90cm?
	wall = 4.6;		//	幅13mだが、天井付近は細くなっているので、9mと見た
	d = 1.2;
	hOff = 0.05;
	h = 0.53 - hOff;
	w = h*4/3;
	projectionPitch = Rad(30);
	double mul = (1.0/d) * 0.4;
	d *= mul;	hOff *= mul;	h *= mul;
	w *= mul;
	
	hSupportDepth = 0.0192;		//	hSupportの長さは2cm
	hSupportGroove = 0.003;		//	hSupport側の溝 3mm
	t = 0.002 - 0.001*0.06;
}


void Cell::Init(int xIn, int yIn){
	Config& config = env.config;
	texSize = 256;
	double ySum = 0;
	double yInterval[DIVY];
	double yPos[DIVY+1];
	double alpha = 0.8;
	for(int y=0; y<DIVY; ++y){
		if (y==0) yInterval[y] = alpha;
		else yInterval[y] = pow(alpha, y);
		ySum += yInterval[y];
	}
	for(int y=0; y<DIVY; ++y) yInterval[y] /= ySum;
	yPos[0] = 0;
	for(int y=0; y<DIVY; ++y) yPos[y+1] = yPos[y] + yInterval[y];
	//	プロジェクタが原点。上がy、プロジェクタから出る光の向きがz、プロジェクタに向かって右がx
	//	inDirの計算：プロジェクタの仕様と分割数で決まる
	x = xIn;
	y = yIn;
	for(int i=0; i<4; ++i){
		inPos[i].x = -config.w/2 + (config.w/DIVX) * (x+i%2);
		inPos[i].y = config.hOff + config.h * yPos[y+i/2];
		inPos[i].z = config.d;
	}
	inDirCenter.x = (inPos[0].x+inPos[1].x) / 2;	//-config.w/2 + (config.w/DIVX) * (x+0.5);
	inDirCenter.y = (inPos[0].y+inPos[2].y) / 2; //config.hOff + config.h * (yPos[y]+yPos[y+1])/2;
	inDirCenter.z = config.d;
	for(int i=0; i<4; ++i) inDir[i] = inPos[i].unit();
	inDirCenter.unitize();

	//	全体をx軸回転
	Quaterniond rot = Quaterniond::Rot(config.projectionPitch, 'x');
	for(int i=0; i<4; ++i){
		inDir[i] = rot * inDir[i];
		inPos[i] = rot * inPos[i];
	}
	inDirCenter = rot * inDirCenter;		

	//	outDirの計算
	//  dist(天井高)=ceil  H=16m のスクリーン。 outY[0]m ～ outY[1]m
	//	横は、-80度～80度で角度等間隔
	double radX = config.outXRad[0] + (config.outXRad[1]-config.outXRad[0]) * (x + (y%2)/2.0) / (DIVX-0.5);
	outDirCenter.x = tan(radX) * config.ceil;
	if (-config.wall<outDirCenter.x && outDirCenter.x < config.wall){
		outDirCenter.y = config.ceil;
	}else{
		outDirCenter.x = outDirCenter.x>0 ? config.wall : -config.wall;
		outDirCenter.y = tan(Rad(90)-radX) * config.wall;
		if (outDirCenter.y < 0) outDirCenter.y *= -1;
	}
	outDirCenter.z = config.outY[0]+(config.outY[1]-config.outY[0])*y/(DIVY-1);


	outDirCenter.unitize();


	mirror.normal = (-inDirCenter + outDirCenter).unit();
	for(int i=0; i<4; ++i){
		outDir[i] = -inDir[i] + 2*(inDir[i] - (inDir[i]*mirror.normal)*mirror.normal);
	}
}
void Cell::CalcPosition(double depth, int id){	//	頂点id がdepthになるようにする
	Config& config = env.config;
	//	一番真ん中下に近いものの奥行きをdepthに設定する
	//	頂点は、-x -y, +x, -y, -x +y, +x +y の順
	mirror.vertex[id] = inDir[id] * (depth / inDir[id].z);
	double mirrorOff = mirror.normal * mirror.vertex[id];
	for(int i=0; i<4; ++i){
		mirror.vertex[i] = mirrorOff / (inDir[i] * mirror.normal) * inDir[i];
	}
	mirror.center = mirrorOff / (inDirCenter * mirror.normal) * inDirCenter;
	
	Vec3d oc = outDirCenter;
	double ceilDist = config.ceil - mirror.center.y;
	oc *= ceilDist / oc.y;
	double wallLeft = -config.wall - mirror.center.x;
	double wallRight = config.wall - mirror.center.x;
	if (oc.x > wallRight){
		oc *= wallRight/oc.x;
		outPlace = 1;
	}else if (oc.x < wallLeft){
		oc *= wallLeft/oc.x;
		outPlace = -1;
	}else{
		outPlace = 0;
	}
	outPosCenter = mirror.center + oc;

	for(int i=0; i<4; ++i){
		Vec3d oc = outDir[i];
		double ceilDist = config.ceil - mirror.vertex[i].y;
		oc *= ceilDist / oc.y;
		double wallLeft = -config.wall - mirror.vertex[i].x;
		double wallRight = config.wall - mirror.vertex[i].x;
		if (outPlace == 1) oc *= wallRight/oc.x;
		else if (outPlace == -1) oc *= wallLeft/oc.x;
		outPos[i] = mirror.vertex[i] + oc;
	}

	//	imagePos
	for(int i=0; i<4; ++i){
		imagePos[i] = outPos[i] - (2*mirror.normal*(outPos[i] - mirror.center)) * mirror.normal;
	}
}
void Cell::InitFrontCamera(int fb){
	Config& config = env.config;

	if (fb==0) view.LookAtGL(Vec3d(0, 0, env.front.d), Vec3d(0, 1, 0));
	else view.LookAtGL(Vec3d(0, 0, -env.front.d), Vec3d(0, 1, 0));
	screenCenter = Vec3d(0, env.front.hOff + env.front.h/2, env.front.d);
	screenSize = Vec2d(env.front.w, env.front.h);

	texCoord[0] = Vec2d(0,0);
	texCoord[1] = Vec2d(1,0);
	texCoord[2] = Vec2d(0,1);
	texCoord[3] = Vec2d(1,1);
	projection = Affined::ProjectionGL(screenCenter, screenSize, 1, 10000);
	

	localScreen[0] = Vec3d(-env.front.w/2, env.front.hOff, -env.front.d);
	localScreen[1] = Vec3d( env.front.w/2, env.front.hOff, -env.front.d);
	localScreen[2] = Vec3d(-env.front.w/2, env.front.hOff+env.front.h, -env.front.d);
	localScreen[3] = Vec3d( env.front.w/2, env.front.hOff+env.front.h, -env.front.d);
	outPosCenter = Vec3d();
	for(int i=0; i<4; ++i){
		localOutPos[i] = localScreen[i];
		screen[i] = view * localScreen[i];
		outPos[i] = screen[i];
		outPosCenter += outPos[i];
	}
	outPosCenter /= 4;
	if (env.cameraMode == Env::CM_TILE){
		view = Affined();
	}
}
void Cell::InitCamera(int fb){
	Config& config = env.config;

	if (env.cameraMode == Env::CM_WINDOW){
		view.Pos() = env.projPose[fb].inv().Pos();
		if (outPlace){	//	壁だったら
			view.LookAtGL(Vec3d(outPlace*config.wall, view.Pos().y, view.Pos().z), Vec3d(0, 1, 0));
		}else{
			view.LookAtGL(Vec3d(view.Pos().x, config.ceil, view.Pos().z), Vec3d(0, 0, 1));
		}
	}else if (env.cameraMode == Env::CM_TILE){
		if (outPlace){	//	壁だったら
			view.Pos() = Vec3d(outPlace*(config.wall-1), outPosCenter.y, outPosCenter.z);
			view.LookAtGL(Vec3d(outPlace*config.wall, view.Pos().y, view.Pos().z), Vec3d(0, 1, 0));
		}else{
			view.Pos() = Vec3d(outPosCenter.x, env.config.ceil-1, outPosCenter.z);
			Vec3d head = (Vec3d(view.Pos().x, 0, view.Pos().z) - env.projPose[fb].inv() * env.centerSeat).unit();
			view.LookAtGL(Vec3d(view.Pos().x, env.config.ceil, view.Pos().z), head);
		}
	}
	for(int i=0; i<4; ++i) localOutPos[i] = view.inv() * outPos[i];
	Vec3d localOutPosU[4];
	for(int i=0; i<4; ++i) localOutPosU[i] = localOutPos[i] / -localOutPos[i].z;

	Vec2d limit[2] = {Vec2d(DBL_MAX, DBL_MAX), Vec2d(-DBL_MAX, -DBL_MAX)};
	for(int i=0; i<4; ++i){
		if (localOutPosU[i].x < limit[0].x) limit[0].x = localOutPosU[i].x;
		if (localOutPosU[i].x > limit[1].x) limit[1].x = localOutPosU[i].x;
		if (localOutPosU[i].y < limit[0].y) limit[0].y = localOutPosU[i].y;
		if (localOutPosU[i].y > limit[1].y) limit[1].y = localOutPosU[i].y;
	}
	screenCenter.x = (limit[0].x + limit[1].x)/2;
	screenCenter.y = (limit[0].y + limit[1].y)/2;
	screenCenter.z = 1;
	screenSize.x = limit[1].x - limit[0].x;
	screenSize.y = limit[1].y - limit[0].y;
	for(int i=0; i<4; ++i){
		texCoord[i].x = (localOutPosU[i].x - limit[0].x) / screenSize.x;
		texCoord[i].y = (localOutPosU[i].y - limit[0].y) / screenSize.y;
	}
	projection = Affined::ProjectionGL(screenCenter, screenSize, 1, 10000);
	localScreen[0] = Vec3d(limit[0].x, limit[0].y, -1);
	localScreen[1] = Vec3d(limit[1].x, limit[0].y, -1);
	localScreen[2] = Vec3d(limit[1].x, limit[1].y, -1);
	localScreen[3] = Vec3d(limit[0].x, limit[1].y, -1);
	Vec3d localNormal = ((localOutPos[2]-localOutPos[0]) ^ (localOutPos[1]-localOutPos[0])).unit();
	for(int i=0; i<4; ++i){
		//	k * localScreen[i] * localNormal = localOutPos[0] * localNormal;
		localScreen[i] *= (localOutPos[0] * localNormal) / (localScreen[i] * localNormal);
		screen[i] = view * localScreen[i];
	}

	if (env.cameraMode == Env::CM_WINDOW){
		view = env.projPose[fb] * view;
	}else if (env.cameraMode == Env::CM_TILE){
		view = Affined();
	}
}
void Cell::InitGL(){
	char* tmp = new char[texSize*texSize*4];
	//	texBuf
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	glGenTextures( 2, texName);
	for(int i=0; i<2; ++i){
		glBindTexture( GL_TEXTURE_2D, texName[i] );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, texSize, texSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, tmp);
	}
	delete tmp;
#ifdef USE_GLEW
	//	render(depth)Buf	
	glGenRenderbuffersEXT(1, &renderName);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderName);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, texWidth, texHeight);		
	//	freame buf
	glGenFramebuffersEXT(1, &frameName);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, frameName);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, texName, 0);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, renderName);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);	
#endif
}
void Cell::BeforeDrawTex(int tex){
	glViewport(0, 0, texSize, texSize);
	glClearColor(0,0,0,1);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(projection);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixd(view.inv());
}
void Cell::AfterDrawTex(int tex){
	glFlush();
	glBindTexture(GL_TEXTURE_2D, texName[tex]);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, texSize,  texSize);
}
