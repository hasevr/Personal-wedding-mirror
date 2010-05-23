#include "Decal.h"
#include "Env.h"
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
	sheetSize = Vec2d(4, 3)*4;
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
	int rv = gluBuild2DMipmaps(GL_TEXTURE_2D, (nc==1||nc==3) ? GL_RGB : GL_RGBA, tx, ty, pxfm[nc - 1], GL_UNSIGNED_BYTE, texbuf);
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

double randd(){
	return 2*(double)rand()/(double)RAND_MAX - 1;
}

Vec3d Fairy::NewGoal(){
	Vec3d offset(0, 3.5, 0);
	Vec3d area(6, 0.1, 18);
	goal = pos;
	if (abs(goal.x) < 4) goal.x *= 2;
	if (abs(goal.y) < 16) goal.y *= 2;
	Vec3d rv = goal + 0.5*Vec3d(area.x*randd(), area.y*randd(), area.z*randd());
	for(int i=0; i<3; ++i){
		rv[i] = max(rv[i], offset[i]-area[i]);
		rv[i] = min(rv[i], offset[i]+area[i]);
	}
	return rv;
}

Fairy::Fairy(){
	dir = Vec3d(randd(),0,randd()).unit();
	pos = Vec3d(3*randd(), 5, 8*randd());
	pos = NewGoal();
	goal = NewGoal();
	count = 0; 
	decal = NULL;
}

//	壁の時は、中心向き、天井の時は任意の向き。向きの変化はゆっくりと
void Fairy::Update(double dt){
	//	位置・速度の更新
	count ++;
	Vec3d force = (goal-pos);
	if (force.norm() < 0.5){
		goal = NewGoal();
		count = 0;
	}
	pos += vel * dt;
	vel += force.unit() * 0.1 * dt;
	vel *= 0.998;
	
	//	向き
	static Vec2d vtx[4] = {	//	部屋の頂点
		Vec2d(-6, -16), 
		Vec2d( 6, -16), 
		Vec2d( 6,  16), 
		Vec2d(-6,  16), 
	};
	static Vec3d normal[4] = {
		Vec3d( 0, 0,  1), 
		Vec3d(-1, 0,  0), 
		Vec3d( 0, 0, -1), 
		Vec3d( 1, 0,  0), 
	};
	Vec2d pos2(pos.x, pos.z);
	int area;
	for(area=0; area<4; ++area){
		if (vtx[area]%pos2 > 0 && vtx[(area+1)%4]%pos2 < 0) break;
	}
	if (abs(pos.z) < 15  && abs(pos.x) < 4.5) onCeil = true;
	else onCeil = false;
	
	if (onCeil){
		dir = vel;
		dir.y = 0;
		dir.unitize();
	}else{
		dir = Matrix3d::Rot(Rad(90), 'y') * normal[area];
		if (dir * vel < 0) dir *= -1;
	}
}
void Fairy::Draw(){
	
	Affinef pose;
	pose.Ex() = -dir;
	if (onCeil){
		pose.Ez() = Vec3d(0,-1,0.8).unit();
		pose.Ey() = pose.Ez()%pose.Ex();
	}else{
		pose.Ey() = Vec3d(0,1,0);
		pose.Ez() = pose.Ex()%pose.Ey();
	}
	oriGoal.FromMatrix(pose.Rot());
	Quaterniond delta = oriGoal * ori.Inv();
	delta = Quaterniond::Rot(delta.RotationHalf() * 0.1);
	ori = delta * ori;
	ori.ToMatrix(pose.Rot());
	pose.Pos() = pos;
	glPushMatrix();
	glMultMatrixf(pose);
	glDisable(GL_DEPTH_TEST);
	decal->Draw();
	glEnable(GL_DEPTH_TEST);
	glPopMatrix();
}

void Fairies::Update(double dt){
	for(unsigned i=0; i<size(); ++i){
		at(i).Update(dt);
	}
}
