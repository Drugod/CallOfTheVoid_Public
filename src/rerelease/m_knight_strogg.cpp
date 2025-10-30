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
// m_knight.c

#include "g_local.h"
#include "m_knight_strogg.h"

static cached_soundindex sound_death;
static cached_soundindex sound_pain;
static cached_soundindex sound_sight;
static cached_soundindex sound_search;
static cached_soundindex sound_melee1;
static cached_soundindex sound_melee2;

void knight_strogg_attack(edict_t *self);
void SwingSword(edict_t* self);
int CheckDistance(edict_t* self, edict_t* enemy);

// Stand
mframe_t knight_strogg_frames_stand [] =
{
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL}
};
MMOVE_T(knight_strogg_move_stand) = {0, 8, knight_strogg_frames_stand, NULL};

MONSTERINFO_STAND (knight_strogg_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &knight_strogg_move_stand);	
}

// Run
mframe_t knight_strogg_frames_run [] =
{
	{ai_run, 16, NULL},
	{ai_run, 20, NULL},
	{ai_run, 13, NULL},
	{ai_run, 7,  NULL},
	{ai_run, 16, NULL},
	{ai_run, 20, NULL},
	{ai_run, 14, NULL},
	{ai_run, 6,	knight_strogg_attack}
};
MMOVE_T(knight_strogg_move_run) = {9, 16, knight_strogg_frames_run, NULL};

MONSTERINFO_RUN (knight_strogg_run) (edict_t *self) -> void
{
	M_SetAnimation(self, &knight_strogg_move_run);		
}

void knight_strogg_attack_swing(edict_t *self)
{
	if (frandom() > 0.5)
		gi.sound(self, CHAN_WEAPON, sound_melee1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_WEAPON, sound_melee2, 1, ATTN_NORM, 0);
}

// Walk
mframe_t knight_strogg_frames_walk [] =
{
	{ai_walk, 3, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 3, NULL}
};
MMOVE_T(knight_strogg_move_walk) = {53, 66, knight_strogg_frames_walk, NULL};

MONSTERINFO_WALK (knight_strogg_walk) (edict_t *self) -> void
{
	M_SetAnimation(self, &knight_strogg_move_walk);		
}

// Attack
mframe_t knight_strogg_frames_attack [] =
{
	{ai_charge, 16,	knight_strogg_attack_swing},
	{ai_charge, 20,	NULL},
	{ai_charge, 13,	NULL},
	{ai_charge, 7,	NULL},
	{ai_charge, 16,	SwingSword},
	{ai_charge, 20,	SwingSword},
	{ai_charge, 14,	SwingSword},
	{ai_charge, 6,	SwingSword},
	{ai_charge, 14,	SwingSword},
	{ai_charge, 10,	NULL},
	{ai_charge, 7,	NULL}
};
MMOVE_T(knight_strogg_move_attack) = {17, 27, knight_strogg_frames_attack, knight_strogg_run};

void knight_strogg_attack(edict_t *self)
{
	if (self->enemy && CheckDistance(self, self->enemy) < (MELEE_DISTANCE * 4))
		M_SetAnimation(self, &knight_strogg_move_attack);		
	else
		M_SetAnimation(self, &knight_strogg_move_run);			
}

void knight_strogg_melee_swing(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_melee1, 1, ATTN_NORM, 0);
}

// Melee
mframe_t knight_strogg_frames_melee [] =
{
	{ai_charge, 0,	knight_strogg_melee_swing},
	{ai_charge, 7,	NULL},
	{ai_charge, 4,	NULL},
	{ai_charge, 0,	NULL},
	{ai_charge, 3,	NULL},
	{ai_charge, 4,	SwingSword},
	{ai_charge, 1,	SwingSword},
	{ai_charge, 3,	SwingSword},
	{ai_charge, 1,	NULL},
	{ai_charge, 5,	NULL}
};
MMOVE_T(knight_strogg_move_melee) = {42, 51, knight_strogg_frames_melee, knight_strogg_run};

MONSTERINFO_MELEE (knight_strogg_melee) (edict_t *self) -> void
{
	M_SetAnimation(self, &knight_strogg_move_melee);				
}

// Pain (1)
mframe_t knight_strogg_frames_pain1 [] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
MMOVE_T(knight_strogg_move_pain1) = {28, 30, knight_strogg_frames_pain1, knight_strogg_run};

// Pain (2)
mframe_t knight_strogg_frames_pain2 [] =
{
	{ai_move, 0, NULL},
	{ai_move, 3, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 2, NULL},
	{ai_move, 4, NULL},
	{ai_move, 2, NULL},
	{ai_move, 5, NULL},
	{ai_move, 5, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
MMOVE_T(knight_strogg_move_pain2) = {31, 41, knight_strogg_frames_pain2, knight_strogg_run};

PAIN (knight_strogg_pain)(edict_t* self, edict_t* other, float kick, int damage, const mod_t& mod) -> void
{
	if (skill->value == 3)
		return;
	if (level.time < self->pain_debounce_time)
		return;
	gi.sound (self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

	self->pain_debounce_time = level.time + 1_sec;
	if (frandom() < 0.85)
		M_SetAnimation(self, &knight_strogg_move_pain1);
	else
		M_SetAnimation(self, &knight_strogg_move_pain2);
}

void knight_strogg_dead(edict_t *self)
{
	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, -8 };
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

// Death (1)
mframe_t knight_strogg_frames_die1 [] =
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
	{ai_move, 0, NULL}
};
MMOVE_T(knight_strogg_move_die1) = {76, 85, knight_strogg_frames_die1, knight_strogg_dead};

// Death (2)
mframe_t knight_strogg_frames_die2 [] =
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
	{ai_move, 0, NULL}
};
MMOVE_T(knight_strogg_move_die2) = {86, 96, knight_strogg_frames_die2, knight_strogg_dead};

DIE (knight_strogg_die)(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t& mod) -> void
{
	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		ThrowGibs(self, damage, {
			{ "models/objects/gibs/bone/tris.md2" },
			{ "models/objects/gibs/sm_meat/tris.md2" },
			{ "models/monsters/knightstrogg/gibs/g_arm.md2" },
			{ "models/monsters/knightstrogg/gibs/g_arm.md2" },
			{ "models/monsters/knightstrogg/gibs/g_leg.md2" },
			{ "models/monsters/knightstrogg/gibs/g_leg.md2" },
			{ "models/monsters/knightstrogg/gibs/g_head.md2", GIB_HEAD }
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
		M_SetAnimation(self, &knight_strogg_move_die1);	
	else
		M_SetAnimation(self, &knight_strogg_move_die2);		
}

// Sight
MONSTERINFO_SIGHT (knight_strogg_sight) (edict_t* self, edict_t* other) -> void
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

// Search
MONSTERINFO_SEARCH(knight_strogg_search) (edict_t* self) -> void
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

MONSTERINFO_SETSKIN(knight_strogg_setskin) (edict_t* self) -> void
{
	if (self->health < (self->max_health / 2)) {
		if (self->s.skinnum == 0)
			self->s.skinnum = 1;
		else if (self->s.skinnum == 2)
			self->s.skinnum = 3;
	}
}

void SP_monster_knight_strogg(edict_t *self)
{
	if (!M_AllowSpawn(self)) {
		G_FreeEdict(self);
		return;
	}	

	sound_melee1.assign("knight/sword1.wav");
	sound_melee2.assign("knight/sword2.wav");
	sound_death.assign("knight/kdeath_s.wav");
	sound_pain.assign("knight/khurt_s.wav");
	sound_sight.assign("knight/ksight_s.wav");
	sound_search.assign("knight/idle_s.wav");
	
	self->mins = {-16,-16,-24};
	self->maxs = {16,16,40};	
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->s.modelindex = gi.modelindex("models/monsters/knightstrogg/tris.md2");
	gi.modelindex("models/monsters/knightstrogg/gibs/g_arm.md2");
	gi.modelindex("models/monsters/knightstrogg/gibs/g_leg.md2");
	gi.modelindex("models/monsters/knightstrogg/gibs/g_head.md2");


	self->health = 100 * st.health_multiplier;
	self->gib_health = -40;

	self->mass = 75;

	self->pain = knight_strogg_pain;
	self->die = knight_strogg_die;
	self->monsterinfo.stand = knight_strogg_stand;
	self->monsterinfo.walk = knight_strogg_walk;
	self->monsterinfo.run = knight_strogg_run;
	self->monsterinfo.melee = knight_strogg_melee;
	self->monsterinfo.sight = knight_strogg_sight;
	self->monsterinfo.search = knight_strogg_search;
	self->monsterinfo.setskin = knight_strogg_setskin;
	if (irandom(2) == 0) //appear random skin
		self->s.skinnum = 0;
	else
		self->s.skinnum = 2;


	gi.linkentity(self);
	M_SetAnimation(self, &knight_strogg_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;	
	
	walkmonster_start(self);
}
