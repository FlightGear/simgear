// 3D cloud class
//
// Written by Harald JOHNSEN, started April 2005.
//
// Copyright (C) 2005  Harald JOHNSEN - hjohnsen@evc.net
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
//
//

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include <plib/sg.h>
#include <plib/ssg.h>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sg_path.hxx>

#include STL_ALGORITHM
#include SG_GLU_H

#include "cloudfield.hxx"
#include "newcloud.hxx"


/*
*/

static ssgTexture *cloudTextures[SGNewCloud::CLTexture_max];


bool SGNewCloud::useAnisotropic = true;
SGBbCache *SGNewCloud::cldCache = 0;
static bool texturesLoaded = false;
float SGNewCloud::nearRadius = 3500.0f;
bool SGNewCloud::lowQuality = false;
sgVec3 SGNewCloud::sunlight = {0.5f, 0.5f, 0.5f};
sgVec3 SGNewCloud::ambLight = {0.5f, 0.5f, 0.5f};
sgVec3 SGNewCloud::modelSunDir = {0,1,0};


// constructor
SGNewCloud::SGNewCloud() :
	bbId(-1),
//	rank(-1),
	minx(999), miny(999), minz(999), maxx(-999), maxy(-999), maxz(-999)

{
	cloudId = (int) this;
	sgSetVec3(center, 0.0f, 0.0f, 0.0f);
	sgSetVec3(cloudpos, 0.0f, 0.0f, 0.0f);
	list_spriteContainer.reserve(8);
	list_spriteDef.reserve(40);
//	if( ! texturesLoaded ) {}
	if( cldCache == 0 ) {
		cldCache = new SGBbCache;
		cldCache->init( 64 );
	}
}

SGNewCloud::~SGNewCloud() {
	list_spriteDef.clear();
	list_spriteContainer.clear();
	cldCache->free( bbId, cloudId );
}


// load all textures used to draw cloud sprites
void SGNewCloud::loadTextures(const string &tex_path) {
	if( texturesLoaded )
		return;
	texturesLoaded = true;

	SGPath cloud_path;

    cloud_path.set(tex_path);
    cloud_path.append("cl_cumulus.rgb");
    cloudTextures[ CLTexture_cumulus ] = new ssgTexture( cloud_path.str().c_str(), false, false, false );
    cloudTextures[ CLTexture_cumulus ]->ref();

    cloud_path.set(tex_path);
    cloud_path.append("cl_stratus.rgb");
    cloudTextures[ CLTexture_stratus ] = new ssgTexture( cloud_path.str().c_str(), false, false, false );
    cloudTextures[ CLTexture_stratus ]->ref();

}

void SGNewCloud::startFade(bool direction, float duration, float pauseLength) {
}
void SGNewCloud::setFade(float howMuch) {
}


static float rayleighCoeffAngular(float cosAngle) {
	return 3.0f / 4.0f * (1.0f + cosAngle * cosAngle);
}

// cp is normalized (len==1)
static void CartToPolar3d(sgVec3 cp, sgVec3 polar) {
    polar[0] = atan2(cp[1], cp[0]);
    polar[1] = SG_PI / 2.0f - atan2(sqrtf (cp[0] * cp[0] + cp[1] * cp[1]), cp[2]);
	polar[2] = 1.0f;
}

static void PolarToCart3d(sgVec3 p, sgVec3 cart) {
    float tmp = cos(p[1]);
    cart[0] = cos(p[0]) * tmp;
    cart[1] = sin(p[0]) * tmp;
    cart[2] = sin(p[1]);
}


// compute the light for a cloud sprite corner
// from the normal and the sun, scaled by the Rayleigh factor
// and finaly added to the ambient light
static void lightFunction(sgVec3 normal, sgVec4 light, float pf) {
	float cosAngle = sgScalarProductVec3( normal, SGNewCloud::modelSunDir);
	float vl = (1.0f - 0.1f + cosAngle / 10.0f) * pf;
	sgScaleVec3( light, SGNewCloud::sunlight, vl );
	sgAddVec3( light, SGNewCloud::ambLight );
	// we need to clamp or else the light will bug when adding transparency
	if( light[0] > 1.0 )	light[0] = 1.0;
	if( light[1] > 1.0 )	light[1] = 1.0;
	if( light[2] > 1.0 )	light[2] = 1.0;
	light[3] = 1.0;
}

// compute the light for a cloud sprite
// we use ambient light and orientation versus sun position
// TODO:check sun pos and check code
void SGNewCloud::computeSimpleLight(sgVec3 FakeEyePos) {
	// constant Rayleigh factor if we are not doing Anisotropic lighting
	float pf = 1.0f;
	const float ang = 45.0f * SG_PI / 180.0f;
	list_of_spriteDef::iterator iSprite;
	for( iSprite = list_spriteDef.begin() ; iSprite != list_spriteDef.end() ; iSprite++ ) {
		if( useAnisotropic ) {
			sgVec3 eyeDir;
            sgSubVec3(eyeDir, iSprite->pos, FakeEyePos);
            sgNormaliseVec3(eyeDir);
            float cosAngle = sgScalarProductVec3(eyeDir, modelSunDir);
            pf = rayleighCoeffAngular(cosAngle);
		}
		// compute the vector going from the container box center to the sprite
		// TODO : this is a constant except for cloudpos, compute the normal in setpos function
		sgVec3 normal;
		spriteContainer *thisSpriteContainer = &list_spriteContainer[iSprite->box];
        sgSubVec3(normal, iSprite->pos, thisSpriteContainer->pos);
        sgSubVec3(normal, thisSpriteContainer->center);
        sgSubVec3(normal, cloudpos);
        sgNormaliseVec3(normal);
		if( lowQuality ) {
			// juste use the traditional normal to compute some lightning
			sgVec4 centerColor;
			lightFunction(normal, centerColor, pf);
			sgCopyVec4(iSprite->l0, centerColor);
			sgCopyVec4(iSprite->l1, centerColor);
			sgCopyVec4(iSprite->l2, centerColor);
			sgCopyVec4(iSprite->l3, centerColor);

		} else {
			// use exotic lightning function, this will give more 'relief' to the clouds
			// compute a normal for each vextex this will simulate a smooth shading for a round shape
			sgVec3 polar, cart, pt;
			// I suspect this code to be bugged...
            CartToPolar3d(normal, polar);

			// offset the normal vector by some angle for each vertex
            sgSetVec3(pt, polar[0] - ang, polar[1] - ang, polar[2]);
            PolarToCart3d(pt, cart);
            lightFunction(cart, iSprite->l0, pf);
            sgSetVec3(pt, polar[0] + ang, polar[1] - ang, polar[2]);
            PolarToCart3d(pt, cart);
            lightFunction(cart, iSprite->l1, pf);
            sgSetVec3(pt, polar[0] + ang, polar[1] + ang, polar[2]);
            PolarToCart3d(pt, cart);
            lightFunction(cart, iSprite->l2, pf);
            sgSetVec3(pt, polar[0] - ang, polar[1] + ang, polar[2]);
            PolarToCart3d(pt, cart);
            lightFunction(cart, iSprite->l3, pf);
		}
	}
}


// add a new box to the cloud
void SGNewCloud::addContainer (float x, float y, float z, float r, CLbox_type type) {
	spriteContainer cont;
	sgSetVec3( cont.pos, x, y, z );
	cont.r = r;
	cont.cont_type = type;
	sgSetVec3( cont.center, 0.0f, 0.0f, 0.0f);
	list_spriteContainer.push_back( cont );
}

// add a sprite inside a box
void SGNewCloud::addSprite(float x, float y, float z, float r, CLbox_type type, int box) {
	spriteDef newSpriteDef;
	int rank = list_spriteDef.size();
	sgSetVec3( newSpriteDef.pos, x, y, z);
	newSpriteDef.box = box;
	newSpriteDef.sprite_type = type;
	newSpriteDef.rank = rank;
	newSpriteDef.r = r;
	list_spriteDef.push_back( newSpriteDef );
	spriteContainer *thisBox = &list_spriteContainer[box];
	sgVec3 deltaPos;
	sgSubVec3( deltaPos, newSpriteDef.pos, thisBox->pos );
	sgAddVec3( thisBox->center, deltaPos );

	r = r * 0.6f;	// 0.5 * 1.xxx
    if( x - r < minx )
		minx = x - r;
    if( y - r < miny )
		miny = y - r;
    if( z - r < minz )
		minz = z - r;
    if( x + r > maxx )
		maxx = x + r;
    if( y + r > maxy )
		maxy = y + r;
    if( z + r > maxz )
		maxz = z + r;

}

// return a random number between -n/2 and n/2
static float Rnd(float n) {
	return n * (-0.5f + sg_random());
}

// generate all sprite with defined boxes
void SGNewCloud::genSprites( void ) {
	float x, y, z, r;
    int N, sc;
    N = list_spriteContainer.size();
	for(int i = 0 ; i < N ; i++ ) {
		spriteContainer *thisBox = & list_spriteContainer[i];
		// the type defines how the sprites can be positioned inside the box, their size, etc
		switch(thisBox->cont_type) {
			case CLbox_sc:
				for( sc = 0 ; sc <= 4 ; sc ++ ) {
					r = thisBox->r + Rnd(0.2f);
					x = thisBox->pos[SG_X] + Rnd(thisBox->r);
					y = thisBox->pos[SG_Y] + Rnd(thisBox->r * 0.2f);
					z = thisBox->pos[SG_Z] + Rnd(thisBox->r);
					addSprite(x, y, z, r, thisBox->cont_type, i);
				}
				break;
			case CLbox_stratus:
				sc = 1;
				r = thisBox->r;
				x = thisBox->pos[SG_X];
				y = thisBox->pos[SG_Y];
				z = thisBox->pos[SG_Z];
				addSprite(x, y, z, r, thisBox->cont_type, i);
				break;
			case CLbox_cumulus:
				for( sc = 0 ; sc <= 4 ; sc ++ ) {
					r = thisBox->r + Rnd(0.2f);
					x = thisBox->pos[SG_X] + Rnd(thisBox->r * 0.75f);
					y = thisBox->pos[SG_Y] + Rnd(thisBox->r * 0.5f);
					z = thisBox->pos[SG_Z] + Rnd(thisBox->r * 0.75f);
					if ( y < thisBox->pos[SG_Y] - thisBox->r / 10.0f )
						y = thisBox->pos[SG_Y] - thisBox->r / 10.0f;
					addSprite(x, y, z, r, thisBox->cont_type, i);
				}
				break;
			default:
				for( sc = 0 ; sc <= 4 ; sc ++ ) {
					r = thisBox->r + Rnd(0.2f);
					x = thisBox->pos[SG_X] + Rnd(thisBox->r);
					y = thisBox->pos[SG_Y] + Rnd(thisBox->r);
					z = thisBox->pos[SG_Z] + Rnd(thisBox->r);
					addSprite(x, y, z, r, thisBox->cont_type, i);
				}
				break;
		}
        sgScaleVec3(thisBox->center, 1.0f / sc);
	}

	radius = maxx - minx;
    if ( (maxy - miny) > radius )
		radius = (maxy - miny);
    if ( (maxz - minz) > radius )
		radius = (maxz - minz);
    radius /= 2.0f;
    sgSetVec3( center, (maxx + minx) / 2.0f, (maxy + miny) / 2.0f, (maxz + minz) / 2.0f );

/*    fadingrank = 0
'    fadingrank = UBound(tbSpriteDef()) * 10
    fadingdir = 0*/
	// TODO : compute initial sprite normals for lighting function
}


// definition of a cu cloud, only for testing
void SGNewCloud::new_cu(void) {
	float s = 250.0f;
	float r = Rnd(1.0) + 0.5;
	if( r < 0.5f ) {
		addContainer(0.0f, 0.0f, 0.0f, s, CLbox_cumulus);
		addContainer(s, 0, 0, s, CLbox_cumulus);
		addContainer(0, 0, 2 * s, s, CLbox_cumulus);
		addContainer(s, 0, 2 * s, s, CLbox_cumulus);

		addContainer(-1.2f * s, 0.2f * s, s, s * 1.4f, CLbox_cumulus);
		addContainer(0.2f * s, 0.2f * s, s, s * 1.4f, CLbox_cumulus);
		addContainer(1.6f * s, 0.2f * s, s, s * 1.4f, CLbox_cumulus);
	} else if ( r < 0.90f ) {
		addContainer(0, 0, 0, s * 1.2, CLbox_cumulus);
		addContainer(s, 0, 0, s, CLbox_cumulus);
		addContainer(0, 0, s, s, CLbox_cumulus);
		addContainer(s * 1.1, 0, s, s * 1.2, CLbox_cumulus);

		addContainer(-1.2 * s, 1 + 0.2 * s, s * 0.5, s * 1.4, CLbox_standard);
		addContainer(0.2 * s, 1 + 0.25 * s, s * 0.5, s * 1.5, CLbox_standard);
		addContainer(1.6 * s, 1 + 0.2 * s, s * 0.5, s * 1.4, CLbox_standard);

	} else {
		// cb
		s = 675.0f;
		addContainer(0, 0, 0, s, CLbox_cumulus);
		addContainer(0, 0, s, s, CLbox_cumulus);
		addContainer(s, 0, s, s, CLbox_cumulus);
		addContainer(s, 0, 0, s, CLbox_cumulus);

		addContainer(s / 2, s, s / 2, s * 1.5, CLbox_standard);

		addContainer(0, 2 * s, 0, s, CLbox_standard);
		addContainer(0, 2 * s, s, s, CLbox_standard);
		addContainer(s, 2 * s, s, s, CLbox_standard);
		addContainer(s, 2 * s, 0, s, CLbox_standard);

	}
	genSprites();
}


// define the new position of the cloud (inside the cloud field, not on sphere)
void SGNewCloud::SetPos(sgVec3 newPos) {
    int N = list_spriteDef.size();
    sgVec3 deltaPos;
	sgSubVec3( deltaPos, newPos, cloudpos );

    // for each particle
	for(int i = 0 ; i < N ; i ++) {
		sgAddVec3( list_spriteDef[i].pos, deltaPos );
	}
	sgAddVec3( center, deltaPos );
    sgSetVec3( cloudpos, newPos[SG_X], newPos[SG_Y], newPos[SG_Z]);
	// TODO : recompute sprite normal so we don't have to redo that each frame
}




void SGNewCloud::drawContainers() {


}



// sort on distance to eye because of transparency
void SGNewCloud::sortSprite( sgVec3 eye ) {
	list_of_spriteDef::iterator iSprite;

	// compute distance from sprite to eye
	for( iSprite = list_spriteDef.begin() ; iSprite != list_spriteDef.end() ; iSprite++ ) {
		sgVec3 dist;
		sgSubVec3( dist, iSprite->pos, eye );
		iSprite->dist = -(dist[0]*dist[0] + dist[1]*dist[1] + dist[2]*dist[2]);
	}
	std::sort( list_spriteDef.begin(), list_spriteDef.end() );
}

// render the cloud on screen or on the RTT texture to build the impostor
void SGNewCloud::Render3Dcloud( bool drawBB, sgVec3 FakeEyePos, sgVec3 deltaPos, float dist_center ) {

/*    int clrank		 = fadingrank / 10;
	int clfadeinrank = fadingrank - clrank * 10;*/
	float CloudVisFade = 1.0 / (1.5 * SGCloudField::get_CloudVis());

    computeSimpleLight( FakeEyePos );

    // view point sort, we sort because of transparency
	sortSprite( FakeEyePos );

	GLint previousTexture = -1, thisTexture;
	list_of_spriteDef::iterator iSprite;
	for( iSprite = list_spriteDef.begin() ; iSprite != list_spriteDef.end() ; iSprite++ ) {
		// choose texture to use depending on sprite type
		switch(iSprite->sprite_type) {
			case CLbox_stratus:
				thisTexture = CLTexture_stratus;
				break;
			default:
				thisTexture = CLTexture_cumulus;
				break;
		}
		// in practice there is no texture switch (atm)
		if( previousTexture != thisTexture ) {
			previousTexture = thisTexture;
			glBindTexture(GL_TEXTURE_2D, cloudTextures[thisTexture]->getHandle());
		}

			sgVec3 translate;
			if( drawBB ) {
				sgCopyVec3( translate, iSprite->pos);
				sgSubVec3( translate, iSprite->pos, deltaPos );
			}
			else
				sgSubVec3( translate, iSprite->pos, deltaPos);


			// flipx and flipy are random texture flip flags, this gives more random clouds
			float flipx = (float) ( iSprite->rank & 1 );
			float flipy = (float) ( (iSprite->rank >> 1) & 1 );
			// cu texture have a flat bottom so we can't do a vertical flip
			if( iSprite->sprite_type == CLbox_cumulus || iSprite->sprite_type == CLbox_stratus )
				flipy = 0.0f;
			if( iSprite->sprite_type == CLbox_stratus )
				flipx = 0.0f;
			// adjust colors depending on cloud type
			// TODO : rewrite that later, still experimental
			switch(iSprite->sprite_type) {
				case CLbox_cumulus:
					// dark bottom
                    sgScaleVec3(iSprite->l0, 0.6f);
                    sgScaleVec3(iSprite->l1, 0.6f);
					break;
				case CLbox_stratus:
					// usually dark grey
                    sgScaleVec3(iSprite->l0, 0.8f);
                    sgScaleVec3(iSprite->l1, 0.8f);
                    sgScaleVec3(iSprite->l2, 0.8f);
                    sgScaleVec3(iSprite->l3, 0.8f);
					break;
				default:
					// darker bottom than top
                    sgScaleVec3(iSprite->l0, 0.8f);
                    sgScaleVec3(iSprite->l1, 0.8f);
					break;
			}
			float r = iSprite->r * 0.5f;

			sgVec4 l0, l1, l2, l3;
			sgCopyVec4 ( l0, iSprite->l0 );
			sgCopyVec4 ( l1, iSprite->l1 );
			sgCopyVec4 ( l2, iSprite->l2 );
			sgCopyVec4 ( l3, iSprite->l3 );
			if( ! drawBB ) {
				// blend clouds with sky based on distance to limit the contrast of distant cloud
				float t = 1.0f - dist_center * CloudVisFade;
				if ( t < 0.0f ) 
					t = 0.0f;	// no, it should have been culled
				// now clouds at the far plane are half blended
				sgScaleVec4( l0, t );
				sgScaleVec4( l1, t );
				sgScaleVec4( l2, t );
				sgScaleVec4( l3, t );
			}
			// compute the rotations so that the quad is facing the camera
			sgVec3 pos;
			sgSetVec3( pos, translate[SG_X], translate[SG_Z], translate[SG_Y] );
			sgCopyVec3( translate, pos );
			sgNormaliseVec3( translate );
			sgVec3 x, y, up = {0.0f, 0.0f, 1.0f};
			sgVectorProductVec3(x, translate, up);
			sgNormaliseVec3(x);
			sgScaleVec3(x, r);
			sgVectorProductVec3(y, x, translate);
			sgNormaliseVec3(y);
			sgScaleVec3(y, r);
 
			sgVec3 left, right;
			if( drawBB )
				sgSetVec3( left, iSprite->pos[SG_X], iSprite->pos[SG_Z], iSprite->pos[SG_Y]);
			else
				sgCopyVec3( left, pos );
			sgSubVec3 (left, y);
			sgAddVec3 (right, left, x);
			sgSubVec3 (left, x);

			glBegin(GL_QUADS);
				glColor4fv(l0);
                glTexCoord2f(flipx, 1.0f - flipy);
                glVertex3fv(left);
                glColor4fv(l1);
                glTexCoord2f(1.0f - flipx, 1.0f - flipy);
                glVertex3fv(right);
				sgScaleVec3( y, 2.0 );
				sgAddVec3( left, y);
				sgAddVec3( right, y);
                glColor4fv(l2);
                glTexCoord2f(1.0f - flipx, flipy);
                glVertex3fv(right);
                glColor4fv(l3);
                glTexCoord2f(flipx, flipy);
                glVertex3fv(left);

			glEnd();	

	}
}


// compute rotations so that a quad is facing the camera
// TODO:change obsolete code because we dont use glrotate anymore
void SGNewCloud::CalcAngles(sgVec3 refpos, sgVec3 FakeEyePos, float *angleY, float *angleX) {
    sgVec3 upAux, lookAt, objToCamProj, objToCam;
	float angle, angle2;

    sgSetVec3(objToCamProj, -FakeEyePos[SG_X] + refpos[SG_X], -FakeEyePos[SG_Z] + refpos[SG_Z], 0.0f);
    sgNormaliseVec3(objToCamProj);

    sgSetVec3(lookAt, 0.0f, 1.0f, 0.0f);
    sgVectorProductVec3(upAux, lookAt, objToCamProj);
    angle = sgScalarProductVec3(lookAt, objToCamProj);
	if( (angle < 0.9999f) && (angle > -0.9999f) ) {
        angle = acos(angle) * 180.0f / SG_PI;
        if( upAux[2] < 0.0f )
            angle = -angle;
	} else
        angle = 0.0f;

    sgSetVec3(objToCam, -FakeEyePos[SG_X] + refpos[SG_X], -FakeEyePos[SG_Z] + refpos[SG_Z], -FakeEyePos[SG_Y] + refpos[SG_Y]);
	sgNormaliseVec3(objToCam);

    angle2 = sgScalarProductVec3(objToCamProj, objToCam);
	if( (angle2 < 0.9999f) && (angle2 > -0.9999f) ) {
        angle2 = -acos(angle2) * 180.0f / SG_PI;
        if(  objToCam[2] > 0.0f )
            angle2 = -angle2;
	} else
        angle2 = 0.0f;

	angle2 += 90.0f;

	*angleY = angle;
    *angleX = angle2;
}

// draw a cloud but this time we use the impostor texture
void SGNewCloud::RenderBB(sgVec3 deltaPos, float angleY, float angleX, float dist_center) {
	// TODO:glrotate is not needed
    glPushMatrix();
        glTranslatef(center[SG_X] - deltaPos[SG_X], center[SG_Z] - deltaPos[SG_Z], center[SG_Y] - deltaPos[SG_Y]);
        glRotatef(angleY, 0.0f, 0.0f, 1.0f);
        glRotatef(angleX, 1.0f, 0.0f, 0.0f);
 
		// blend clouds with sky based on distance to limit the contrast of distant cloud
		float CloudVisFade = (1.5 * SGCloudField::get_CloudVis());

		float t = 1.0f - dist_center / CloudVisFade;
		// err the alpha value is not good for impostor, debug that
		t *= 1.65;
		if ( t < 0.0f ) 
			t = 0.0f;
  
        glColor4f(t, t, t, t);
        float r = radius;
		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 1.0f);
			glVertex2f(-r, r);
			glTexCoord2f(1.0f, 1.0f);
			glVertex2f(r, r);
			glTexCoord2f(1.0f, 0.0f);
			glVertex2f(r, -r);
			glTexCoord2f(0.0f, 0.0f);
			glVertex2f(-r, -r);
		glEnd();

#if 0	// debug only
		int age = cldCache->queryImpostorAge(bbId);
		// draw a red border for the newly generated BBs else draw a white border
        if( age < 200 )
            glColor3f(1, 0, 0);
		else
            glColor3f(1, 1, 1);

        glBindTexture(GL_TEXTURE_2D, 0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glBegin(GL_QUADS);
            glVertex2f(-r, -r);
            glVertex2f(r, -r);
            glVertex2f(r, r);
            glVertex2f(-r, r);
        glEnd();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

#endif

	glPopMatrix();

}

// determine if it is a good idea to use an impostor to render the cloud
bool SGNewCloud::isBillboardable(float dist) {

	if( dist <= ( 2.1f * radius ) ) {
        // inside cloud
        return false;
	}
	if( (dist-radius) <= nearRadius ) {
        // near clouds we don't want to use BB
        return false;
	}
	return true;
}



// render the cloud, fakepos is a relative position inside the cloud field
void SGNewCloud::Render(sgVec3 FakeEyePos) {
	sgVec3 dist;


	sgVec3 deltaPos;
	sgCopyVec3( deltaPos, FakeEyePos);
    sgSubVec3( dist, center, FakeEyePos);
    float dist_center = sgLengthVec3(dist);



	if( !isBillboardable(dist_center) ) {
		// not a good candidate for impostors, draw a real cloud
		Render3Dcloud(false, FakeEyePos, deltaPos, dist_center);
	} else {
		GLuint texID = 0;
			// lets use our impostor
			if( bbId >= 0)
				texID = cldCache->QueryTexID(cloudId, bbId);

			// ok someone took our impostor, so allocate a new one
			if( texID == 0 ) {
                // allocate a new Impostor
                bbId = cldCache->alloc(cloudId);
				texID = cldCache->QueryTexID(cloudId, bbId);
			}
			if( texID == 0 ) {
                // no more free texture in the pool
                Render3Dcloud(false, FakeEyePos, deltaPos, dist_center);
			} else {
                float angleX, angleY;
                CalcAngles(center, FakeEyePos, &angleY, &angleX);
				if( ! cldCache->isBbValid( cloudId, bbId, angleY, angleX) ) {
                    // we must build or rebuild this billboard
					// start render to texture
                    cldCache->beginCapture();
					// set transformation matrices
                    cldCache->setRadius(radius, dist_center);
					gluLookAt(FakeEyePos[SG_X], FakeEyePos[SG_Z], FakeEyePos[SG_Y], center[SG_X], center[SG_Z], center[SG_Y], 0.0, 0.0, 1.0);
					// draw into texture
                    Render3Dcloud(true, FakeEyePos, deltaPos, dist_center);
					// save rotation angles for later use
					// TODO:this is not ok
					cldCache->setReference(cloudId, bbId, angleY, angleX);
					// save the rendered cloud into the cache
					cldCache->setTextureData( bbId );
					// finish render to texture and go back into standard context
                    cldCache->endCapture();
				}
                // draw the newly built BB or an old one
                glBindTexture(GL_TEXTURE_2D, texID);
                RenderBB(deltaPos, angleY, angleX, dist_center);
			}
	}

}

