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

#ifndef _NEWCLOUD_HXX
#define _NEWCLOUD_HXX

#include <plib/sg.h>
#include <simgear/compiler.h>
#include STL_STRING
#include <vector>

#include "bbcache.hxx"

SG_USING_STD(string);
SG_USING_STD(vector);

/**
 * 3D cloud class.
 */
class SGNewCloud {

public:
	enum CLFamilly_type {
		CLFamilly_cu = 0,
		CLFamilly_cb,
		CLFamilly_st,
		CLFamilly_ns,
		CLFamilly_sc,
		CLFamilly_as,
		CLFamilly_ac,
		CLFamilly_ci,
		CLFamilly_cc,
		CLFamilly_cs,
		CLFamilly_nn
	};
	SGNewCloud(CLFamilly_type classification=CLFamilly_nn);
	SGNewCloud(string classification);
	~SGNewCloud();

	enum CLbox_type {
		CLbox_standard = 0,
		CLbox_sc = 1,
		CLbox_cumulus = 2,
		CLbox_stratus = 3
	};

	enum CLTexture_type {
		CLTexture_cumulus = 1,
		CLTexture_stratus = 2,
		CLTexture_max
	};

private:

	class spriteDef {
	public:
		sgVec3		pos;
		float		r;
		CLbox_type	sprite_type;
		sgVec4		l0, l1, l2, l3;
		sgVec3		normal, n0, n1, n2, n3;
		int			rank;
		int			box;
		float		dist;		// distance used during sort
		bool operator<(const spriteDef &b) const {
			return (this->dist < b.dist);
		}
	};

	typedef struct {
		sgVec3		pos;
		float		r;
		// the type defines how the sprites can be positioned inside the box, their size, etc
		CLbox_type	cont_type;
		sgVec3		center;
	} spriteContainer;

	typedef vector<spriteDef>		list_of_spriteDef;
	typedef vector<spriteContainer>	list_of_spriteContainer;

	void init(void);

	void computeSimpleLight(sgVec3 eyePos);
	void addSprite(float x, float y, float z, float r, CLbox_type type, int box);

	// sort on distance to eye because of transparency
	void sortSprite( sgVec3 eyePos );

	// render the cloud on screen or on the RTT texture to build the impostor
	void Render3Dcloud( bool drawBB, sgVec3 eyePos, sgVec3 deltaPos, float dist_center );

	// compute rotations so that a quad is facing the camera
	void CalcAngles(sgVec3 refpos, sgVec3 eyePos, float *angleY, float *angleX);

	// draw a cloud but this time we use the impostor texture
	void RenderBB(sgVec3 deltaPos, bool first_time, float dist_center);

	// determine if it is a good idea to use an impostor to render the cloud
	bool isBillboardable(float dist);

	int		cloudId, bbId;
	sgVec3	rotX, rotY;

//	int		rank;
	sgVec3	cloudpos, center;
	float	delta_base;
	list_of_spriteDef		list_spriteDef;
	list_of_spriteContainer	list_spriteContainer;
	float radius;
	CLFamilly_type familly;

	// fading data
	bool direction, fadeActive;
	float duration, pauseLength, fadetimer;
	float last_step;

public:
	// add a new box to the cloud
	void addContainer(float x, float y, float z, float r, CLbox_type type);

	// generate all sprite with defined boxes
	void genSprites(void);

	// debug only, define a cumulus
	void new_cu(void);

	// debug only
	void drawContainers(void);

	// define the new position of the cloud (inside the cloud field, not on sphere)
	void SetPos(sgVec3 newPos);

	// render the cloud, fakepos is a relative position inside the cloud field
	void Render(sgVec3 fakepos);

	// 
	void startFade(bool direction, float duration, float pauseLength);
	void setFade(float howMuch);

	inline float getRadius() { return radius; }
	inline sgVec3 *getCenter() { return &center; }

	inline CLFamilly_type getFamilly(void) { return familly; }

	// load all textures used to draw cloud sprites
	static void loadTextures( const string &tex_path );
	static sgVec3 modelSunDir;
	static sgVec3 sunlight, ambLight;
	static bool useAnisotropic;
	static float nearRadius;
	static bool lowQuality;
	static SGBbCache	*cldCache;
};

#endif // _NEWCLOUD_HXX
