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
#include "m_ogrestrogg.h"
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

typedef float vec_t;

void ogre2_idle(edict_t* self)
{
	if (frandom() < 0.2)
		gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_NORM, 0);
}

void ogre2_idle2(edict_t* self)
{
	if (frandom() < 0.2)
		gi.sound(self, CHAN_VOICE, sound_idle2, 1, ATTN_NORM, 0);
}

void ogre2_drag(edict_t* self)
{
	if (frandom() < 0.2)
		gi.sound(self, CHAN_VOICE, sound_drag, 1, ATTN_NORM, 0);
}

mframe_t ogre2_frames_stand[] = {
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
MMOVE_T(ogre2_move_stand) = { 0, 8, ogre2_frames_stand, NULL };

MONSTERINFO_STAND(ogre2_stand) (edict_t* self) -> void
{
	M_SetAnimation(self, &ogre2_move_stand);
}

mframe_t ogre2_frames_run[] = {
	{ai_run, 9,ogre2_idle2},
	{ai_run, 12},
	{ai_run, 8},
	{ai_run, 22},
	{ai_run, 16},
	{ai_run, 4},
	{ai_run, 13},
	{ai_run, 24}
};
MMOVE_T(ogre2_move_run) = { 25, 32, ogre2_frames_run, NULL };

MONSTERINFO_RUN(ogre2_run) (edict_t* self) -> void
{
	M_SetAnimation(self, &ogre2_move_run);
}

mframe_t ogre2_frames_walk[] = {
	{ai_walk, 3},
	{ai_walk, 2},
	{ai_walk, 2, ogre2_idle},
	{ai_walk, 2},
	{ai_walk, 2},
	{ai_walk, 5, ogre2_drag},
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
MMOVE_T(ogre2_move_walk) = { 9, 24, ogre2_frames_walk, NULL };

MONSTERINFO_WALK(ogre2_walk) (edict_t* self) -> void
{
	M_SetAnimation(self, &ogre2_move_walk);
}

void OgreStroggChainsaw(edict_t* self)
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

mframe_t ogre2_frames_smash[] = {
	{ai_charge, 0},
	{ai_charge, 0},
	{ai_charge, 1},
	{ai_charge, 4},
	{ai_charge, 14,OgreStroggChainsaw},
	{ai_charge, 14,OgreStroggChainsaw},
	{ai_charge, 20,OgreStroggChainsaw},
	{ai_charge, 23,OgreStroggChainsaw},
	{ai_charge, 10,OgreStroggChainsaw},
	{ai_charge, 12,OgreStroggChainsaw},
	{ai_charge, 1},
	{ai_charge, 4},
	{ai_charge, 12},
	{ai_charge, 0 }
};
MMOVE_T(ogre2_move_smash) = { 47, 60, ogre2_frames_smash, ogre2_run };

mframe_t ogre2_frames_swing[] = {
	{ai_charge, 11,	NULL},
	{ai_charge, 1,	NULL},
	{ai_charge, 4,	NULL},
	{ai_charge, 19,	OgreStroggChainsaw},
	{ai_charge, 13,	OgreStroggChainsaw},
	{ai_charge, 10,	OgreStroggChainsaw},
	{ai_charge, 10,	OgreStroggChainsaw},
	{ai_charge, 10,	OgreStroggChainsaw},
	{ai_charge, 10,	OgreStroggChainsaw},
	{ai_charge, 10,	OgreStroggChainsaw},
	{ai_charge, 3,	NULL},
	{ai_charge, 8,	NULL},
	{ai_charge, 9,	NULL},
	{ai_charge, 0,	NULL}
};
MMOVE_T(ogre2_move_swing) = { 33, 46, ogre2_frames_swing, ogre2_run };

MONSTERINFO_MELEE(ogre2_melee) (edict_t* self) -> void
{
	if (frandom() > 0.5)
		M_SetAnimation(self, &ogre2_move_smash);
	else
		M_SetAnimation(self, &ogre2_move_swing);
	gi.sound(self, CHAN_WEAPON, sound_melee, 1, ATTN_NORM, 0);
}

void soldier_fire2(edict_t* self);

mframe_t ogre2_frames_attack[] ={
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	soldier_fire2},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL}
};
MMOVE_T(ogre2_move_attack) = { 61, 66, ogre2_frames_attack, ogre2_run };

MONSTERINFO_ATTACK(ogre2_attack) (edict_t* self) -> void
{
	M_SetAnimation(self, &ogre2_move_attack);
}

static void ogre2_pain_sound(edict_t* self)
{
	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
}

mframe_t ogre2_frames_pain1[] = {
	{ai_move, 0,		NULL},
	{ai_move, 0,		ogre2_pain_sound},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
};
MMOVE_T(ogre2_move_pain1) = { 67, 71, ogre2_frames_pain1, ogre2_run };

mframe_t ogre2_frames_pain2[] = {
	{ai_move, 0,		NULL},
	{ai_move, 0,		ogre2_pain_sound},
	{ai_move, 0,		NULL}
};
MMOVE_T(ogre2_move_pain2) = { 72, 74, ogre2_frames_pain2, ogre2_run };

mframe_t ogre2_frames_pain3[] = {
	{ai_move, 0,		NULL},
	{ai_move, 0,		ogre2_pain_sound},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
};
MMOVE_T(ogre2_move_pain3) = { 75, 80, ogre2_frames_pain3, ogre2_run };

mframe_t ogre2_frames_pain4[] ={
	{ai_move, 0,		NULL},
	{ai_move, 10,	    ogre2_pain_sound},
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
MMOVE_T(ogre2_move_pain4) = { 81, 96, ogre2_frames_pain4, ogre2_run };

mframe_t ogre2_frames_pain5[] = {
	{ai_move, 0,		NULL},
	{ai_move, 10,	    ogre2_pain_sound},
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
MMOVE_T(ogre2_move_pain5) = { 97, 111, ogre2_frames_pain5, ogre2_run };

PAIN(ogre2_pain) (edict_t* self, edict_t* other, float kick, int damage, const mod_t& mod) -> void
{
	if (skill->value == 3)
		return;
	if (self->pain_debounce_time > level.time)
		return;

	float r = frandom();

	if (r < 0.25)
	{
		M_SetAnimation(self, &ogre2_move_pain1);
		self->pain_debounce_time = level.time + 1_sec;
	}
	else if (r < 0.5)
	{
		M_SetAnimation(self, &ogre2_move_pain2);
		self->pain_debounce_time = level.time + 1_sec;
	}
	else if (r < 0.75)
	{
		M_SetAnimation(self, &ogre2_move_pain3);
		self->pain_debounce_time = level.time + 1_sec;
	}
	else if (r < 0.88)
	{
		M_SetAnimation(self, &ogre2_move_pain4);
		self->pain_debounce_time = level.time + 2_sec;
	}
	else
	{
		M_SetAnimation(self, &ogre2_move_pain5);
		self->pain_debounce_time = level.time + 2_sec;
	}
}

MONSTERINFO_SETSKIN(ogre2_setskin) (edict_t* self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}

void ogre2_dead(edict_t* self)
{
	self->mins = { -32, -32, -24 };
	self->maxs = { 32, 32, -8 };
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

static void ogre2_dead_sound(edict_t* self)
{
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
}

mframe_t ogre2_frames_death1[] ={
	{ai_move, 0,		NULL},
	{ai_move, 0,		ogre2_dead_sound},
	{ai_move, 0,		ogre2_dead},
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
MMOVE_T(ogre2_move_death1) = { 112, 125, ogre2_frames_death1, ogre2_dead };

mframe_t ogre2_frames_death2[] = {
	{ai_move, 0,		NULL},
	{ai_move, 5,		ogre2_dead_sound},
	{ai_move, 0,		ogre2_dead},
	{ai_move, 1,		NULL},
	{ai_move, 3,		NULL},
	{ai_move, 7,		NULL},
	{ai_move, 25,	    NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
};
MMOVE_T(ogre2_move_death2) = { 126, 135, ogre2_frames_death2, ogre2_dead };

DIE(ogre2_die) (edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t& mod) -> void
{
	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		ThrowGibs(self, damage, {
			{ "models/objects/gibs/bone/tris.md2" },
			{ "models/objects/gibs/sm_meat/tris.md2" },
			{ "models/monsters/ogre2strogg/gibs/g_arm.md2" },
			{ "models/monsters/ogre2strogg/gibs/g_leg.md2" },
			{ "models/monsters/ogre2strogg/gibs/g_leg.md2" },
			{ "models/monsters/ogre2strogg/gibs/g_head.md2", GIB_HEAD }
			});
		self->deadflag = true;
		return;
	}
	if (self->deadflag == true)
		return;

	self->deadflag = true;
	self->takedamage = true;

	if (frandom() < 0.5) {
		gitem_t* item2 = FindItemByClassname("ammo_mini_shells");
		if (item2 != NULL) {
			Drop_Item(self, item2);
		}
	}
	
	if (frandom() < 0.5)
		M_SetAnimation(self, &ogre2_move_death1);
	else
		M_SetAnimation(self, &ogre2_move_death2);
}

MONSTERINFO_SIGHT(ogre2_sight) (edict_t* self, edict_t* other) -> void
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

MONSTERINFO_SEARCH(ogre2_search) (edict_t* self) -> void
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

void SP_monster_ogre2_strogg(edict_t* self)
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

	self->health = 220 * st.health_multiplier;
	self->gib_health = -80;
	self->count = 3;//weapon shot type
	self->mass = 200;

	self->pain = ogre2_pain;
	self->die = ogre2_die;
	self->monsterinfo.stand = ogre2_stand;
	self->monsterinfo.walk = ogre2_walk;
	self->monsterinfo.run = ogre2_run;
	self->monsterinfo.attack = ogre2_attack;
	self->monsterinfo.melee = ogre2_melee;
	self->monsterinfo.sight = ogre2_sight;
	self->monsterinfo.search = ogre2_search;
	self->monsterinfo.setskin = ogre2_setskin;

	self->s.modelindex = gi.modelindex("models/monsters/ogre2strogg/tris.md2");
	gi.modelindex("models/monsters/ogre2strogg/gibs/g_arm.md2");
	gi.modelindex("models/monsters/ogre2strogg/gibs/g_leg.md2");
	gi.modelindex("models/monsters/ogre2strogg/gibs/g_head.md2");

	gi.linkentity(self);

	M_SetAnimation(self, &ogre2_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);
}