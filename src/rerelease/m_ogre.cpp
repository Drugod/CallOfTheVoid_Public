/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// m_ogre.c

#include "g_local.h"
#include "m_ogre.h"
#include "m_flash.h"

static cached_soundindex sound_death;
static cached_soundindex sound_attack;
static cached_soundindex sound_drag;
static cached_soundindex sound_melee;
static cached_soundindex sound_sight;
static cached_soundindex sound_search;
static cached_soundindex sound_idle;
static cached_soundindex sound_idle2;
static cached_soundindex sound_pain;

// just in case you do't want to use the macros
typedef float vec_t;

void ogre_idle(edict_t* self)
{
	if (frandom() < 0.2)
		gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_NORM, 0);
}

void ogre_idle2(edict_t* self)
{
	if (frandom() < 0.2)
		gi.sound(self, CHAN_VOICE, sound_idle2, 1, ATTN_NORM, 0);
}

void ogre_drag(edict_t* self)
{
	if (frandom() < 0.2)
		gi.sound(self, CHAN_VOICE, sound_drag, 1, ATTN_NORM, 0);
}

// Stand
mframe_t ogre_frames_stand[] = {
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand}
};
MMOVE_T(ogre_move_stand) = { 0, 8, ogre_frames_stand, NULL };

MONSTERINFO_STAND(ogre_stand) (edict_t* self) -> void
{
	M_SetAnimation(self, &ogre_move_stand);
}

// Run
mframe_t ogre_frames_run[] = {
	{ai_run, 9,ogre_idle2},
	{ai_run, 12},
	{ai_run, 8},
	{ai_run, 22},
	{ai_run, 16},
	{ai_run, 4},
	{ai_run, 13},
	{ai_run, 24}
};
MMOVE_T(ogre_move_run) = { 25, 32, ogre_frames_run, NULL };

MONSTERINFO_RUN(ogre_run) (edict_t* self) -> void
{
	M_SetAnimation(self, &ogre_move_run);
}

// Run
mframe_t ogre_frames_walk[] = {
	{ai_walk, 3},
	{ai_walk, 2},
	{ai_walk, 2, ogre_idle},
	{ai_walk, 2},
	{ai_walk, 2},
	{ai_walk, 5, ogre_drag},
	{ai_walk, 3},
	{ai_walk, 2},
	{ai_walk, 3},
	{ai_walk, 1},
	{ai_walk, 2},
	{ai_walk, 3},
	{ai_walk, 3},
	{ai_walk, 3},
	{ai_walk, 3},
	{ai_walk, 4}
};
MMOVE_T(ogre_move_walk) = { 9, 24, ogre_frames_walk, NULL };

MONSTERINFO_WALK(ogre_walk) (edict_t* self) -> void
{
	M_SetAnimation(self, &ogre_move_walk);
}

void OgreChainsaw(edict_t* self)
{
	vec3_t dir;
	static vec3_t aim = { 100, 0, -24 };
	int damage;

	if (!self->enemy)
		return;
	//VectorSubtract(self->s.origin, self->enemy->s.origin, dir);
	dir = self->enemy->s.origin - self->s.origin;

	if (dir.length() > 100.0)
		return;
	damage = (frandom() + frandom() + frandom()) * 4;

	fire_hit(self, aim, damage, damage);
}

// Smash
mframe_t ogre_frames_smash[] = {
	{ai_charge, 0},
	{ai_charge, 0},
	{ai_charge, 1},
	{ai_charge, 4},
	{ai_charge, 14,OgreChainsaw},
	{ai_charge, 14,OgreChainsaw},
	{ai_charge, 20,OgreChainsaw},
	{ai_charge, 23,OgreChainsaw},
	{ai_charge, 10,OgreChainsaw},
	{ai_charge, 12,OgreChainsaw},
	{ai_charge, 1},
	{ai_charge, 4},
	{ai_charge, 12},
	{ai_charge, 0 }
};
MMOVE_T(ogre_move_smash) = { 47, 60, ogre_frames_smash, ogre_run };

// Swing
mframe_t ogre_frames_swing[] = {
	{ai_charge, 11,	NULL},
	{ai_charge, 1,	NULL},
	{ai_charge, 4,	NULL},
	{ai_charge, 19,	OgreChainsaw},
	{ai_charge, 13,	OgreChainsaw},
	{ai_charge, 10,	OgreChainsaw},
	{ai_charge, 10,	OgreChainsaw},
	{ai_charge, 10,	OgreChainsaw},
	{ai_charge, 10,	OgreChainsaw},
	{ai_charge, 10,	OgreChainsaw},
	{ai_charge, 3,	NULL},
	{ai_charge, 8,	NULL},
	{ai_charge, 9,	NULL},
	{ai_charge, 0,	NULL}
};
MMOVE_T(ogre_move_swing) = { 33, 46, ogre_frames_swing, ogre_run };

// Melee
MONSTERINFO_MELEE(ogre_melee) (edict_t* self) -> void
{
	if (frandom() > 0.5)
		M_SetAnimation(self, &ogre_move_smash);
	else
		M_SetAnimation(self, &ogre_move_swing);
	gi.sound(self, CHAN_WEAPON, sound_melee, 1, ATTN_NORM, 0);
}

// Grenade
void GunnerGrenade(edict_t* self);
mframe_t ogre_frames_attack[] ={
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	GunnerGrenade},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL}
};
MMOVE_T(ogre_move_attack) = { 61, 66, ogre_frames_attack, ogre_run };

MONSTERINFO_ATTACK(ogre_attack) (edict_t* self) -> void
{
	M_SetAnimation(self, &ogre_move_attack);
}


// Pain (1)
mframe_t ogre_frames_pain1[] = {
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
};
MMOVE_T(ogre_move_pain1) = { 67, 71, ogre_frames_pain1, ogre_run };

// Pain (2)
mframe_t ogre_frames_pain2[] = {
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
};
MMOVE_T(ogre_move_pain2) = { 72, 74, ogre_frames_pain2, ogre_run };

// Pain (3)
mframe_t ogre_frames_pain3[] = {
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
};
MMOVE_T(ogre_move_pain3) = { 75, 80, ogre_frames_pain3, ogre_run };

// Pain (4)
mframe_t ogre_frames_pain4[] ={
	{ai_move, 0,		NULL},
	{ai_move, 10,	    NULL},
	{ai_move, 9,		NULL},
	{ai_move, 4,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
};
MMOVE_T(ogre_move_pain4) = { 81, 96, ogre_frames_pain4, ogre_run };

// Pain (5)
mframe_t ogre_frames_pain5[] = {
	{ai_move, 0,		NULL},
	{ai_move, 10,	    NULL},
	{ai_move, 9,		NULL},
	{ai_move, 4,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
};
MMOVE_T(ogre_move_pain5) = { 97, 111, ogre_frames_pain5, ogre_run };

// Pain
//void ogre_pain(edict_t* self)
PAIN(ogre_pain) (edict_t* self, edict_t* other, float kick, int damage, const mod_t& mod) -> void
{
	float r;

	// decino: No pain animations in Nightmare mode
	if (skill->value == 3)
		return;
	if (self->pain_debounce_time > level.time)
		return;
	r = frandom();
	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

	if (r < 0.25)
	{
		M_SetAnimation(self, &ogre_move_pain1);
		self->pain_debounce_time = level.time + 1_sec;
	}
	else if (r < 0.5)
	{
		M_SetAnimation(self, &ogre_move_pain2);
		self->pain_debounce_time = level.time + 1_sec;
	}
	else if (r < 0.75)
	{
		M_SetAnimation(self, &ogre_move_pain3);
		self->pain_debounce_time = level.time + 1_sec;
	}
	else if (r < 0.88)
	{
		M_SetAnimation(self, &ogre_move_pain4);
		self->pain_debounce_time = level.time + 2_sec;
	}
	else
	{
		M_SetAnimation(self, &ogre_move_pain5);
		self->pain_debounce_time = level.time + 2_sec;
	}
}

MONSTERINFO_SETSKIN(ogre_setskin) (edict_t* self) -> void
{
	if (self->health < (self->max_health / 2)){
		if (self->s.skinnum == 2)
			self->s.skinnum = 3;
		else if (self->s.skinnum == 0)
			self->s.skinnum = 1;
	}
}

void ogre_dead(edict_t* self)
{
	self->mins = { -32, -32, -24 };
	self->maxs = { 32, 32, -8 };
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

// Death (1)
mframe_t ogre_frames_death1[] ={
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		ogre_dead},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
};
MMOVE_T(ogre_move_death1) = { 112, 125, ogre_frames_death1, ogre_dead };

// Death (2)
mframe_t ogre_frames_death2[] = {
	{ai_move, 0,		NULL},
	{ai_move, 5,		NULL},
	{ai_move, 0,		ogre_dead},
	{ai_move, 1,		NULL},
	{ai_move, 3,		NULL},
	{ai_move, 7,		NULL},
	{ai_move, 25,	    NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
};
MMOVE_T(ogre_move_death2) = { 126, 135, ogre_frames_death2, ogre_dead };

// Death
DIE(ogre_die) (edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t& mod) -> void
{
	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		ThrowGibs(self, damage, {
			{ "models/objects/gibs/bone/tris.md2" },
			{ "models/objects/gibs/sm_meat/tris.md2" },
			{ "models/objects/gibs/head2/tris.md2", GIB_HEAD }
			});
		self->deadflag = true;
		return;
	}
	if (self->deadflag == true)
		return;
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);

	self->deadflag = true;
	self->takedamage = true;

	if (frandom() < 0.5)
		M_SetAnimation(self, &ogre_move_death1);
	else
		M_SetAnimation(self, &ogre_move_death2);
}

// Sight
MONSTERINFO_SIGHT(ogre_sight) (edict_t* self, edict_t* other) -> void
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

// Search
MONSTERINFO_SEARCH(ogre_search) (edict_t* self) -> void
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

void SP_monster_ogre(edict_t* self)
{

	if (!M_AllowSpawn(self)) {
		G_FreeEdict(self);
		return;
	}

	
	self->mins = { -32, -32, -24 };
	self->maxs = { 32, 32, 64 };
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	sound_death.assign("ogre/ogdth_s.wav");
	sound_attack.assign("ogre/grenade.wav");
	sound_melee.assign("ogre/ogsawatk.wav");
	sound_sight.assign("ogre/ogwake_s.wav");
	sound_search.assign("ogre/ogidle2_s.wav");
	sound_idle2.assign("ogre/ogidle2_s.wav");
	sound_idle.assign("ogre/ogidle_s.wav");
	sound_pain.assign("ogre/ogpain1_s.wav");
	sound_drag.assign("ogre/ogdrag.wav");

	self->health = 200;
	self->gib_health = -80;

	self->mass = 200;

	self->pain = ogre_pain;
	self->die = ogre_die;

	self->monsterinfo.stand = ogre_stand;
	self->monsterinfo.walk = ogre_walk;
	self->monsterinfo.run = ogre_run;
	self->monsterinfo.attack = ogre_attack;
	self->monsterinfo.melee = ogre_melee;
	self->monsterinfo.sight = ogre_sight;
	self->monsterinfo.search = ogre_search;
	self->monsterinfo.setskin = ogre_setskin;

	self->s.modelindex = gi.modelindex("models/monsters/ogre/tris.md2");

	gi.linkentity(self);

	M_SetAnimation(self, &ogre_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);
}