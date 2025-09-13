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
#include "m_dog.h"
#include "m_flash.h"

static cached_soundindex sound_melee;
static cached_soundindex sound_death;
static cached_soundindex sound_pain;
static cached_soundindex sound_sight;
static cached_soundindex sound_search;

// Stand
mframe_t dog_frames_stand[] = {
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
MMOVE_T(dog_move_stand) = { 69, 77, dog_frames_stand, NULL };

MONSTERINFO_STAND(dog_stand) (edict_t* self) -> void
{
	M_SetAnimation(self, &dog_move_stand);
}

// walk
mframe_t dog_frames_walk[] = {
	{ai_walk, 8},
	{ai_walk, 8},
	{ai_walk, 8},
	{ai_walk, 8},
	{ai_walk, 8},
	{ai_walk, 8},
	{ai_walk, 8},
	{ai_walk, 8}
};
MMOVE_T(dog_move_walk) = { 78, 85, dog_frames_walk, NULL };

MONSTERINFO_WALK(dog_walk) (edict_t* self) -> void
{
	M_SetAnimation(self, &dog_move_walk);
}

// Run
mframe_t dog_frames_run[] = {
	{ai_run, 16},
	{ai_run, 32},
	{ai_run, 32},
	{ai_run, 20},
	{ai_run, 64},
	{ai_run, 32},
	{ai_run, 16},
	{ai_run, 32},
	{ai_run, 32},
	{ai_run, 20},
	{ai_run, 64},
	{ai_run, 64}
};
MMOVE_T(dog_move_run) = { 48, 59, dog_frames_run, NULL };

MONSTERINFO_RUN(dog_run) (edict_t* self) -> void
{
	M_SetAnimation(self, &dog_move_run);
}

TOUCH(DogLeapTouch) (edict_t* self, edict_t* other, const trace_t& tr, bool other_touching_self) -> void
{
	vec3_t point;
	vec3_t normal;
	int	   damage;
	if (!other->enemy)
		return;

	if (other->health < 1)
		return;
	normal = self->velocity;
	normal.normalize();
	point = self->s.origin + (normal * self->maxs[0]);
	damage = (int)frandom(0, 10)+ 10;
	T_Damage(other, self, self, self->velocity, point, normal, damage, damage, DAMAGE_NONE, MOD_UNKNOWN);
}

void DogLeaper(edict_t *self)
{
	vec3_t forward;
	float length = (self->s.origin - self->enemy->s.origin).length();
	float fwd_speed = length * 1.95f;

	self->s.origin[2] += 1;
	AngleVectors(self->s.angles, forward, NULL, NULL);
	self->velocity = forward * fwd_speed;
	self->velocity[2] = 250;
	self->groundentity = NULL;
	self->touch = DogLeapTouch;
}

// Leap
mframe_t dog_frames_leap[] = {
	{ai_charge, 0, NULL},
	{ai_charge, 0, DogLeaper},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
	
};
MMOVE_T(dog_move_leap) = { 60, 68, dog_frames_leap, dog_run };

MONSTERINFO_ATTACK(dog_leap) (edict_t* self) -> void
{
	if (!self->enemy)
		return;

	if (self->health < 1)
		return;

	if (range_to(self, self->enemy) > 300)
		return;
	M_SetAnimation(self, &dog_move_leap);	
}

void DogBite(edict_t *self)
{
	static vec3_t aim = {100, 0, -24};
	int damage = frandom()  * 2;
	gi.sound(self, CHAN_VOICE, sound_melee, 1, ATTN_NORM, 0);
	fire_hit(self, aim, damage, damage);
}

// melee
mframe_t dog_frames_melee[] = {
	{ai_charge, 10, NULL},
	{ai_charge, 10, NULL},
	{ai_charge, 10, NULL},
	{ai_charge, 10, DogBite},
	{ai_charge, 10, NULL},
	{ai_charge, 10, NULL},
	{ai_charge, 10, NULL},
	{ai_charge, 10, NULL}
};
MMOVE_T(dog_move_melee) = { 0, 7, dog_frames_melee, dog_run };

MONSTERINFO_MELEE(dog_melee) (edict_t* self) -> void
{
	if (!self->enemy)
		return;
	if (range_to(self, self->enemy) > MELEE_DISTANCE)
		return;
	M_SetAnimation(self, &dog_move_melee);	
}

MONSTERINFO_SETSKIN(dog_setskin) (edict_t* self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}

// Pain (1)
mframe_t dog_frames_pain1[] = {
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
};
MMOVE_T(dog_move_pain1) = { 26, 31, dog_frames_pain1, dog_run };

// Pain (2)
mframe_t dog_frames_pain2[] = {
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, -4,		NULL},
	{ai_move, -12,		NULL},
	{ai_move, -12,		NULL},
	{ai_move, -2,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, -4,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, -10,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
};
MMOVE_T(dog_move_pain2) = { 32, 47, dog_frames_pain2, dog_run };

// Pain
PAIN(dog_pain) (edict_t* self, edict_t* other, float kick, int damage, const mod_t& mod) -> void
{
	float r = frandom();

	if (skill->value == 3)
		return;
	
	if (r < 0.5){
		M_SetAnimation(self, &dog_move_pain1);	
	}else{
		M_SetAnimation(self, &dog_move_pain2);	
	}
	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);			
}

void dog_dead(edict_t* self)
{
	self->mins = { -32, -32, -24 };
	self->maxs = { 32, 32, -8 };
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

// Death (1)
mframe_t dog_frames_death1[] ={
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
MMOVE_T(dog_move_death1) = { 8, 16, dog_frames_death1, dog_dead };

// Death (2)
mframe_t dog_frames_death2[] = {
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,	    NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
};
MMOVE_T(dog_move_death2) = { 17, 25, dog_frames_death2, dog_dead };

// Death
DIE(dog_die) (edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t& mod) -> void
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
		M_SetAnimation(self, &dog_move_death1);
	else
		M_SetAnimation(self, &dog_move_death2);
}

// Sight
MONSTERINFO_SIGHT(dog_sight) (edict_t* self, edict_t* other) -> void
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

// Search
MONSTERINFO_SEARCH(dog_search) (edict_t* self) -> void
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

void SP_monster_dog(edict_t* self)
{
	if (!M_AllowSpawn(self)) {
		G_FreeEdict(self);
		return;
	}

	self->s.modelindex = gi.modelindex("models/monsters/dog/tris.md2");
	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, 40 };
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	sound_melee.assign("dog/dattack1_s.wav");
	sound_death.assign("dog/ddeath_s.wav");
	sound_pain.assign("dog/dpain1_s.wav");
	sound_sight.assign("dog/dsight_s.wav");
	sound_search.assign("dog/idle_s.wav");

	self->health = 50;
	self->gib_health = -35;

	self->mass = 40;

	self->pain = dog_pain;
	self->die = dog_die;
	self->monsterinfo.stand = dog_stand;
	self->monsterinfo.walk = dog_walk;
	self->monsterinfo.run = dog_run;
	self->monsterinfo.melee = dog_melee;
	self->monsterinfo.attack = dog_leap;
	self->monsterinfo.sight = dog_sight;
	self->monsterinfo.search = dog_search;
	self->monsterinfo.setskin = dog_setskin;

	gi.linkentity(self);

	M_SetAnimation(self, &dog_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);
}