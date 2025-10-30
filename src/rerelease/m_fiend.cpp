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
// m_demon.c

#include "g_local.h"
#include "m_fiend.h"

static cached_soundindex sound_death;
static cached_soundindex sound_hit;
static cached_soundindex sound_jump;
static cached_soundindex sound_land;
static cached_soundindex sound_pain;
static cached_soundindex sound_sight;
static cached_soundindex sound_search;
static cached_soundindex sound_thud;
static cached_soundindex sound_explod;

// just in case you do't want to use the macros
typedef float vec_t;
// Stand
mframe_t fiend_frames_stand [] =
{
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand, 0, NULL}
};
MMOVE_T(fiend_move_stand) = {0, 12, fiend_frames_stand, NULL};

MONSTERINFO_STAND(fiend_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &fiend_move_stand);
}

// Run
mframe_t fiend_frames_run [] =
{
	{ai_run, 20, NULL},
	{ai_run, 15, NULL},
	{ai_run, 36, NULL},
	{ai_run, 20, NULL},
	{ai_run, 15, NULL},
	{ai_run, 36, NULL}
};
MMOVE_T(fiend_move_run) = {21, 26, fiend_frames_run, NULL};

MONSTERINFO_RUN(fiend_run) (edict_t *self) -> void
{
	M_SetAnimation(self, &fiend_move_run);	
}

// Walk
mframe_t fiend_frames_walk[] =
{
	{ai_walk, 8, NULL},
	{ai_walk, 6, NULL},
	{ai_walk, 6, NULL},
	{ai_walk, 7, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 6, NULL},
	{ai_walk, 10, NULL},
	{ai_walk, 10, NULL}
};
MMOVE_T(fiend_move_walk) = { 13, 20, fiend_frames_walk, NULL };

MONSTERINFO_WALK(fiend_walk) (edict_t* self) -> void
{
	M_SetAnimation(self, &fiend_move_walk);
}

bool CheckfiendJump(edict_t *self)
{
	vec3_t	dir;
	float	distance;

	if (!self->enemy)
		return false;
	if (self->s.origin[2] + self->mins[2] > self->enemy->s.origin[2] + self->enemy->mins[2] + 0.75 * self->enemy->size[2])
		return false;
	if (self->s.origin[2] + self->mins[2] < self->enemy->s.origin[2] + self->enemy->mins[2] + 0.25 * self->enemy->size[2])
		return false;
		
	dir = self->enemy->s.origin - self->s.origin;
	dir[2] = 0;
	distance = dir.length();
	
	if (distance < 100)
		return false;
		
	if (distance > 200)
	{
		if (frandom() < 0.9)
			return false;
	}
	return true;
};

TOUCH (fiendJumpTouch) (edict_t* self, edict_t* other, const trace_t& tr, bool other_touching_self) -> void
{
	vec3_t vel;
	vec3_t point;
	vec3_t normal;
	int	   damage;

	if (self->health < 1)
		return;
	if (other->takedamage)
	{
		vel = self->velocity;
		if (vel.length() > 400)
		{
			normal = self->velocity;
			normal.normalize();
			point = self->s.origin + (normal * self->maxs[0]);
			damage = (int)frandom(0, 10) + 10;
			T_Damage(other, self, self, self->velocity, point, normal, damage, damage, DAMAGE_NONE, MOD_UNKNOWN);
		}
	}else{
		gi.sound(self, CHAN_WEAPON, sound_land, 1, ATTN_NORM, 0);
	}

	self->touch = NULL;

	if (!M_CheckBottom(self))
	{
		if (self->groundentity)
		{
			M_SetAnimation(self, &fiend_move_run);
			self->movetype = MOVETYPE_STEP;
		}
		return;
	}
}

void fiendJump(edict_t *self)
{
	vec3_t forward;
	float length = (self->s.origin - self->enemy->s.origin).length();
	float fwd_speed = length * 1.95f;


	AngleVectors(self->s.angles, forward, NULL, NULL);
	self->s.origin[2] += 1;
	//VectorScale(forward, 600, self->velocity);
	self->velocity[0] = 600 * forward[0];
	self->velocity[1] = 600 * forward[1];
	self->velocity[2] = 250;

	self->groundentity = NULL;
	self->touch = fiendJumpTouch;
}

void fiend_roar(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_jump, 1, ATTN_NORM, 0);
}

// Attack
mframe_t fiend_frames_jump [] =
{
	{ai_charge,	0,	NULL},
	{ai_charge,	0,	fiend_roar},
	{ai_charge,	0,	NULL},
	{ai_charge,	0,	fiendJump},
	{ai_move,	0,	NULL},
	{ai_move,	0,	NULL},
	{ai_move,	0,	NULL},
	{ai_move,	0,	NULL},
	{ai_move,	0,	NULL},
	{ai_move,	0,	NULL},
	{ai_move,	0,	NULL},
	{ai_move,	0,	NULL}
};
MMOVE_T(fiend_move_jump) = {27, 38, fiend_frames_jump, fiend_run};

MONSTERINFO_ATTACK(fiend_attack) (edict_t *self) -> void
{
	M_SetAnimation(self, &fiend_move_jump);	
}

void fiendMelee(edict_t *self)
{
	vec3_t dir;
	static vec3_t aim = {100, 0, -24};
	int damage;

	if (!self->enemy)
		return;
	dir = self->enemy->s.origin - self->s.origin;

	if (dir.length() > 100.0)
		return;
	damage = 10 + 5 * frandom();

	if (fire_hit(self, aim, damage, damage))
		gi.sound(self, CHAN_WEAPON, sound_hit, 1, ATTN_NORM, 0);
}

// Melee
mframe_t fiend_frames_melee [] =
{
	{ai_charge, 4,	NULL},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL},
	{ai_charge, 1,	NULL},
	{ai_charge, 14,	fiendMelee},
	{ai_charge, 1,	NULL},
	{ai_charge, 6,	NULL},
	{ai_charge, 8,	NULL},
	{ai_charge, 4,	NULL},
	{ai_charge, 2,	NULL},
	{ai_charge, 12,	fiendMelee},
	{ai_charge, 5,	NULL},
	{ai_charge, 8,	NULL},
	{ai_charge, 4,	NULL},
	{ai_charge, 4,	NULL}
};
MMOVE_T(fiend_move_melee) = {54, 68, fiend_frames_melee, fiend_run};

MONSTERINFO_MELEE(fiend_melee) (edict_t *self) -> void
{
	M_SetAnimation(self, &fiend_move_melee);
}

static void fiendPainSound(edict_t* self)
{
	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
}

// Pain
mframe_t fiend_frames_pain [] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, fiendPainSound},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
MMOVE_T(fiend_move_pain) = {39, 44, fiend_frames_pain, fiend_run};

PAIN (fiend_pain)  (edict_t* self, edict_t* other, float kick, int damage, const mod_t& mod) -> void
{
	if (skill->value == 3)
		return;
	if (self->touch = fiendJumpTouch)
		return;
	if (self->pain_debounce_time > level.time)
		return;
	self->pain_debounce_time = level.time + 1_sec;

	if (frandom() * 200 > damage)
		return;
	M_SetAnimation(self, &fiend_move_pain);
}

MONSTERINFO_SETSKIN(fiend_setskin) (edict_t* self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}

void fiend_dead(edict_t *self)
{
	self->mins = { -32, -32, -24 };
	self->maxs = { 32, 32, -8 };
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

// Death
mframe_t fiend_frames_die [] =
{
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
MMOVE_T(fiend_move_die) = {45, 53, fiend_frames_die, fiend_dead};

DIE(fiend_die) (edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t& mod) -> void
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

	M_SetAnimation(self, &fiend_move_die);
}

MONSTERINFO_SIGHT(fiend_sight) (edict_t* self, edict_t* other) -> void
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

MONSTERINFO_SEARCH(fiend_search) (edict_t *self) -> void
{
	gi.sound (self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

void SP_monster_fiend(edict_t *self)
{
	if (!M_AllowSpawn(self)) {
		G_FreeEdict(self);
		return;
	}

	self->mins = { -32, -32, -24 };
	self->maxs = { 32, 32, 64 };
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->s.modelindex = gi.modelindex("models/monsters/fiend/tris.md2");
	self->health = 300;

	if (self->solid == SOLID_NOT)
		return;

	sound_death.assign("demon/ddeath.wav");
	sound_hit.assign("demon/dhit2.wav");
	sound_jump.assign("demon/djump.wav");
	sound_land.assign("demon/dland2.wav");
	sound_pain.assign("demon/dpain1.wav");
	sound_search.assign("demon/idle1.wav");
	sound_sight.assign("demon/sight2.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->gib_health = -80;
	self->mass = 300;

	self->monsterinfo.stand = fiend_stand;
	self->monsterinfo.walk = fiend_walk;
	self->monsterinfo.run = fiend_run;
	self->monsterinfo.attack = fiend_attack;
	self->monsterinfo.melee = fiend_melee;
	self->monsterinfo.sight = fiend_sight;
	self->monsterinfo.search = fiend_search;
	self->monsterinfo.setskin = fiend_setskin;

	self->pain = fiend_pain;
	self->die = fiend_die;

	gi.linkentity(self);

	M_SetAnimation(self, &fiend_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);
}
