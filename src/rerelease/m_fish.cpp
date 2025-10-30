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


#include "g_local.h"
#include "m_fish.h"

static cached_soundindex sound_search;
static cached_soundindex sound_death;
static cached_soundindex sound_melee;

// Stand
mframe_t fish_frames_stand[] = {
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
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand}
};
MMOVE_T(fish_move_stand) = {39, 56, fish_frames_stand, nullptr};

MONSTERINFO_STAND(fish_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &fish_move_stand);
}

// Run
mframe_t fish_frames_run [] =
{
	{ai_run, 12},
	{ai_run, 12},
	{ai_run, 12},
	{ai_run, 12},
	{ai_run, 12},
	{ai_run, 12},
	{ai_run, 12},
	{ai_run, 12},
	{ai_run, 12},
	{ai_run, 12},
	{ai_run, 12},
	{ai_run, 12},
	{ai_run, 12},
	{ai_run, 12},
	{ai_run, 12},
	{ai_run, 12},
	{ai_run, 12},
	{ai_run, 12}
};
MMOVE_T(fish_move_run) = {39, 56, fish_frames_run, nullptr};

MONSTERINFO_RUN(fish_run) (edict_t *self) -> void
{
	M_SetAnimation(self, &fish_move_run);
}

mframe_t fish_walk_run [] =
{
	{ai_walk, 4},
	{ai_walk, 4},
	{ai_walk, 4},
	{ai_walk, 4},
	{ai_walk, 4},
	{ai_walk, 4},
	{ai_walk, 4},
	{ai_walk, 4},
	{ai_walk, 4},
	{ai_walk, 4},
	{ai_walk, 4},
	{ai_walk, 4},
	{ai_walk, 4},
	{ai_walk, 4},
	{ai_walk, 4},
	{ai_walk, 4},
	{ai_walk, 4},
	{ai_walk, 4}
};
MMOVE_T(fish_move_walk) = {39, 56, fish_walk_run, nullptr};

MONSTERINFO_WALK(fish_walk) (edict_t *self) -> void
{
	M_SetAnimation(self, &fish_move_walk);
}

void FishBite(edict_t* self)
{
	if (!self->enemy)
		return;

	vec3_t aim = { MELEE_DISTANCE, 0, 0 };
	fire_hit(self, aim, 6, 0);

	gi.sound(self, CHAN_WEAPON, sound_melee, 1, ATTN_NORM, 0);
}

// Melee
mframe_t fish_frames_melee [] =
{
	{ai_charge, 10},
	{ai_charge, 10},
	{ai_charge, 0,  FishBite},
	{ai_charge, 10},
	{ai_charge, 10},
	{ai_charge, 10},
	{ai_charge, 10},
	{ai_charge, 10},
	{ai_charge, 0,  FishBite},
	{ai_charge, 10},
	{ai_charge, 10},
	{ai_charge, 10},
	{ai_charge, 10},
	{ai_charge, 10},
	{ai_charge, 0,  FishBite},
	{ai_charge, 10},
	{ai_charge, 10},
	{ai_charge, 10}
};
MMOVE_T(fish_move_melee) = {0, 17, fish_frames_melee, fish_run};

MONSTERINFO_MELEE(fish_melee) (edict_t *self) -> void
{
	M_SetAnimation(self, &fish_move_melee);
}

MONSTERINFO_SEARCH(fish_search) (edict_t *self) -> void
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

void fish_dead(edict_t *self)
{
	self->mins = {-16, -16, -24};
	self->maxs = { 16, 16, -8 };
	monster_dead(self);
}

// Death
mframe_t fish_frames_death [] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
MMOVE_T(fish_move_death) = {18, 38, fish_frames_death, fish_dead};

DIE(fish_die) (edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t& mod) -> void
{
	if (M_CheckGib(self, mod))
	{
		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
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
	M_SetAnimation(self, &fish_move_death);
}

// Pain
mframe_t fish_frames_pain [] =
{
	{ai_move},
	{ai_move},
	{ai_move},
	{ai_move},
	{ai_move},
	{ai_move},
	{ai_move},
	{ai_move},
	{ai_move}
};
MMOVE_T(fish_move_pain) = {57, 65, fish_frames_pain, fish_run};

PAIN(fish_pain)  (edict_t* self, edict_t* other, float kick, int damage, const mod_t& mod) -> void
{
	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3_sec;

	M_SetAnimation(self, &fish_move_pain);
}

static void fish_set_fly_parameters(edict_t* self)
{
	self->monsterinfo.fly_thrusters = false;
	self->monsterinfo.fly_acceleration = 30.f;
	self->monsterinfo.fly_speed = 110.f;
	self->monsterinfo.fly_min_distance = 10.f;
	self->monsterinfo.fly_max_distance = 10.f;
}

void SP_monster_fish(edict_t *self)
{
	if (!M_AllowSpawn(self)) {
		G_FreeEdict(self);
		return;
	}

	sound_search.assign("fish/idle.wav");
	sound_death.assign("fish/death.wav");
	sound_melee.assign("fish/bite.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/fish/tris.md2");
	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, 64 };

	self->health = 25 * st.health_multiplier;
	self->gib_health = -25;
	self->mass = 25;

	self->pain = fish_pain;
	self->die = fish_die;

	self->monsterinfo.stand = fish_stand;
	self->monsterinfo.walk = fish_walk;
	self->monsterinfo.run = fish_run;
	self->monsterinfo.melee = fish_melee;
	self->monsterinfo.search = fish_search;

	gi.linkentity(self);

	M_SetAnimation(self, &fish_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	self->monsterinfo.aiflags |= AI_ALTERNATE_FLY;
	fish_set_fly_parameters(self);

	swimmonster_start(self);
}
