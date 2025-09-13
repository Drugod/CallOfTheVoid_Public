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
// m_army.c

#include "g_local.h"
#include "m_soldier_grunt_prototype.h"
#include "m_flash.h"

static cached_soundindex sound_death;
static cached_soundindex sound_search;
static cached_soundindex sound_pain1;
static cached_soundindex sound_pain2;
static cached_soundindex sound_attack;
static cached_soundindex sound_sight;

// Stand
mframe_t grunt_prototype_frames_stand[] = {
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand}
};
MMOVE_T(grunt_prototype_move_stand) = { 0, 7, grunt_prototype_frames_stand, NULL };

MONSTERINFO_STAND(grunt_prototype_stand) (edict_t* self) -> void
{
	M_SetAnimation(self, &grunt_prototype_move_stand);
}

// Run
mframe_t grunt_prototype_frames_run[] = {
	{ai_run, 11},
	{ai_run, 15},
	{ai_run, 10},
	{ai_run, 10},
	{ai_run, 8},
	{ai_run, 15},
	{ai_run, 10},
	{ai_run, 8}
};
MMOVE_T(grunt_prototype_move_run) = { 73, 80, grunt_prototype_frames_run, NULL };

MONSTERINFO_RUN(grunt_prototype_run) (edict_t* self) -> void
{
	M_SetAnimation(self, &grunt_prototype_move_run);
}

// Run
mframe_t grunt_prototype_frames_walk[] = {
	{ai_walk, 1, NULL},
	{ai_walk, 1, NULL},
	{ai_walk, 1, NULL},
	{ai_walk, 1, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 1, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 1, NULL},
	{ai_walk, 1, NULL},
	{ai_walk, 1, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 1, NULL},
	{ai_walk, 1, NULL},
	{ai_walk, 1, NULL}
};
MMOVE_T(grunt_prototype_move_walk) = { 90, 113, grunt_prototype_frames_walk, NULL };


MONSTERINFO_WALK(grunt_prototype_walk) (edict_t* self) -> void
{
	M_SetAnimation(self, &grunt_prototype_move_walk);
}

void soldier_fire_vanilla(edict_t* self, int flash_number, bool angle_limited);
void grunt_prototype_fire(edict_t* self)
{
	self->count = 3;
	soldier_fire_vanilla(self, 0, false);
}

mframe_t grunt_prototype_frames_attack[] ={
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL},
	
	{ai_charge, 0,	grunt_prototype_fire},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL},
	
	{ai_charge, 0,	NULL}
};
MMOVE_T(grunt_prototype_move_attack) = { 81, 89, grunt_prototype_frames_attack, grunt_prototype_run };

MONSTERINFO_ATTACK(grunt_prototype_attack) (edict_t* self) -> void
{
	M_SetAnimation(self, &grunt_prototype_move_attack);
}

MONSTERINFO_SETSKIN(grunt_prototype_setskin) (edict_t* self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}

// Pain (1)
mframe_t grunt_prototype_frames_pain1[] = {
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
};
MMOVE_T(grunt_prototype_move_pain1) = { 40, 45, grunt_prototype_frames_pain1, grunt_prototype_run };

// Pain (2)
mframe_t grunt_prototype_frames_pain2[] = {
	{ai_move, 0,		NULL},
	{ai_move, 13,		NULL},
	{ai_move, 9,		NULL},
	{ai_move, 0,		NULL},
	
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	
	{ai_move, 2,		NULL},
	{ai_move, 0,		NULL}
};
MMOVE_T(grunt_prototype_move_pain2) = { 46, 59, grunt_prototype_frames_pain2, grunt_prototype_run };

// Pain (3)
mframe_t grunt_prototype_frames_pain3[] = {
	{ai_move, 0,		NULL},
	{ai_move, 1,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	
	{ai_move, 1,		NULL},
	{ai_move, 1,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 1,		NULL},
	
	{ai_move, 4,		NULL},
	{ai_move, 3,		NULL},
	{ai_move, 6,		NULL},
	{ai_move, 8,		NULL},
	
	{ai_move, 2,		NULL}
};
MMOVE_T(grunt_prototype_move_pain3) = { 60, 72, grunt_prototype_frames_pain3, grunt_prototype_run };

// Pain
PAIN(grunt_prototype_pain) (edict_t* self, edict_t* other, float kick, int damage, const mod_t& mod) -> void
{
	float r;

	if (skill->value == 3)
		return;
	if (self->pain_debounce_time > level.time)
		return;
	r = frandom();

	if (r < 0.2)
	{
		M_SetAnimation(self, &grunt_prototype_move_pain1);
		self->pain_debounce_time = level.time + 6_sec;
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	}
	else if (r < 0.6)
	{
		M_SetAnimation(self, &grunt_prototype_move_pain2);
		self->pain_debounce_time = level.time + 1_sec;
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);		
	}
	else
	{
		M_SetAnimation(self, &grunt_prototype_move_pain3);
		self->pain_debounce_time = level.time + 1_sec;
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);				
	}
}

void grunt_prototype_dead(edict_t* self)
{
	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, -8 };
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

// Death (1)
mframe_t grunt_prototype_frames_death1[] ={
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
MMOVE_T(grunt_prototype_move_death1) = { 8, 17, grunt_prototype_frames_death1, grunt_prototype_dead };

// Death (2)
mframe_t grunt_prototype_frames_death2[] = {
	{ai_move, 0,		NULL},
	{ai_move, -5,		NULL},
	{ai_move, -4,		NULL},
	{ai_move, -13,		NULL},
	
	{ai_move, -3,		NULL},
	{ai_move, -4,		NULL},
	{ai_move, 0,	    NULL},
	{ai_move, 0,		NULL},
	
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
};
MMOVE_T(grunt_prototype_move_death2) = { 18, 28, grunt_prototype_frames_death2, grunt_prototype_dead };

// Death
DIE(grunt_prototype_die) (edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t& mod) -> void
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
		M_SetAnimation(self, &grunt_prototype_move_death1);
	else
		M_SetAnimation(self, &grunt_prototype_move_death2);
}

// Sight
MONSTERINFO_SIGHT(grunt_prototype_sight) (edict_t* self, edict_t* other) -> void
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

// Search
MONSTERINFO_SEARCH(grunt_prototype_search) (edict_t* self) -> void
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

void SP_monster_soldier_grunt_prototype(edict_t* self)
{

	if (!M_AllowSpawn(self)) {
		G_FreeEdict(self);
		return;
	}

	self->s.modelindex = gi.modelindex("models/monsters/grunt_prototype/tris.md2");
	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, 40 };
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	sound_death.assign("army/death1_s.wav");
	sound_search.assign("army/idle_s.wav");
	sound_pain1.assign("army/pain1_s.wav");
	sound_pain2.assign("army/pain2_s.wav");
	sound_attack.assign("army/sattck1.wav");
	sound_sight.assign("army/sight1_s.wav");

	self->health = 30 * st.health_multiplier;
	self->gib_health = -35;

	self->mass = 30;

	self->pain = grunt_prototype_pain;
	self->die = grunt_prototype_die;
	self->monsterinfo.stand = grunt_prototype_stand;
	self->monsterinfo.walk = grunt_prototype_walk;
	self->monsterinfo.run = grunt_prototype_run;
	self->monsterinfo.attack = grunt_prototype_attack;
	self->monsterinfo.sight = grunt_prototype_sight;
	self->monsterinfo.search = grunt_prototype_search;
	self->monsterinfo.setskin = grunt_prototype_setskin;
	self->style = 1;
	gi.linkentity(self);

	M_SetAnimation(self, &grunt_prototype_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);
}