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
// m_hknight.c

#include "g_local.h"
#include "m_hknight_prototype.h"

static cached_soundindex sound_attack;
static cached_soundindex sound_melee;
static cached_soundindex sound_death;
static cached_soundindex sound_proj_hit;
static cached_soundindex sound_sight;
static cached_soundindex sound_search;
static cached_soundindex sound_pain;

void hknight_prototype_run(edict_t* self);
vec3_t* SightEndtToDir(edict_t* self, vec3_t orig_dir);

void SwingSword_Prototype(edict_t* self)
{
	vec3_t dir;
	static vec3_t aim = { 100, 0, -24 };
	int damage;

	if (!self->enemy)
		return;
	dir = self->s.origin - self->enemy->s.origin;

	if (dir.length() > 100.0)
		return;
	damage = (frandom() + frandom() + frandom()) * 3;

	fire_hit(self, aim, damage, damage);
}

int CheckDistance_Prototype(edict_t* self, edict_t* enemy)
{
	vec3_t v;
	v = self->s.origin - enemy->s.origin;
	float dist = v.length();
	return dist;
}

// Stand
mframe_t hknight_prototype_frames_stand [] =
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
MMOVE_T(hknight_prototype_move_stand) = {0, 8, hknight_prototype_frames_stand, NULL};

MONSTERINFO_STAND(hknight_prototype_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &hknight_prototype_move_stand);	
}

// Charge
mframe_t hknight_prototype_frames_charge [] =
{
	{ai_charge, 20,	NULL},
	{ai_charge, 25,	NULL},
	{ai_charge, 18,	NULL},
	{ai_charge, 16,	NULL},
	{ai_charge, 14,	NULL},
	{ai_charge, 20,	SwingSword_Prototype},
	{ai_charge, 21,	SwingSword_Prototype},
	{ai_charge, 13,	SwingSword_Prototype},
	{ai_charge, 20,	SwingSword_Prototype},
	{ai_charge, 20,	SwingSword_Prototype},
	{ai_charge, 18,	SwingSword_Prototype},
	{ai_charge, 16,	NULL},
	{ai_charge, 20,	NULL},
	{ai_charge, 14,	NULL},
	{ai_charge, 25,	NULL},
	{ai_charge, 21,	NULL}
};
MMOVE_T(hknight_prototype_move_charge) = {63, 78, hknight_prototype_frames_charge, hknight_prototype_run};

bool CheckForCharge_Prototype(edict_t *self)
{
	if (!self->enemy)
		return false;

	if (!visible(self, self->enemy))
		return false;

	if (level.time < self->monsterinfo.attack_finished)
		return false;

	if (fabs(self->s.origin[2] - self->enemy->s.origin[2]) > 20)
		return false;

	if (CheckDistance_Prototype(self, self->enemy) < 80)
		return false;
	
	self->monsterinfo.attack_finished = level.time + 2.0_sec;
	return true;
}

// Run
void checkerCharge_Prototype(edict_t* self)
{
	if (CheckForCharge_Prototype(self))
		M_SetAnimation(self, &hknight_prototype_move_charge);	
}

mframe_t hknight_prototype_frames_run [] =
{
	{ai_run, 20, checkerCharge_Prototype},
	{ai_run, 18, NULL},
	{ai_run, 25, NULL},
	{ai_run, 16, NULL},
	{ai_run, 14, NULL},
	{ai_run, 25, NULL},
	{ai_run, 21, NULL},
	{ai_run, 13, NULL}
};
MMOVE_T(hknight_prototype_move_run) = {29, 36, hknight_prototype_frames_run, NULL};

MONSTERINFO_RUN (hknight_prototype_run) (edict_t *self) -> void
{
	M_SetAnimation(self, &hknight_prototype_move_run);	
}

// walk
mframe_t hknight_prototype_frames_walk[] =
{
	{ai_walk, 2, NULL},
	{ai_walk, 5, NULL},
	{ai_walk, 5, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 6, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 2, NULL}
};
MMOVE_T(hknight_prototype_move_walk) = { 9, 28, hknight_prototype_frames_walk, NULL };

MONSTERINFO_WALK(hknight_prototype_walk) (edict_t* self) -> void
{
	M_SetAnimation(self, &hknight_prototype_move_walk);
}

void hknight_prototype_reset_magic(edict_t *self)
{
	self->radius_dmg = -2;
	if (self->enemy && CheckDistance_Prototype(self, self->enemy) < 320 && (frandom() < 0.75))
		self->monsterinfo.attack_finished = level.time + 1.0_sec;
}

TOUCH(prototype_magic_touch) (edict_t* self, edict_t* other, const trace_t& tr, bool other_touching_self) -> void
{
	if (other == self->owner)
		return;
	if (tr.surface && (tr.surface->flags & SURF_SKY))
	{
		G_FreeEdict (self);
		return;
	}
	if (other->takedamage)
		T_Damage (other, self, self->owner, self->velocity, self->s.origin, tr.plane.normal, self->dmg, 1, DAMAGE_ENERGY, MOD_UNKNOWN);
	else
	{
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_WELDING_SPARKS);
		gi.WriteByte(15);
		gi.WritePosition(self->s.origin);
		gi.WriteDir(vec3_origin);
		gi.WriteByte(66);
		gi.multicast(self->s.origin, MULTICAST_PVS, false);

		gi.sound (self, CHAN_WEAPON, sound_proj_hit, 1, ATTN_NORM, 0);
	}
	G_FreeEdict(self);
}

void fire_magic_prototype(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed)
{
	edict_t	*magic;

	if (!self->enemy || self->enemy == self)
		return;

	dir = SightEndtToDir(self, dir)[0];
	dir.normalize();

	magic = G_Spawn();
	magic->s.origin = start;
	
	magic->s.old_origin = start;	
	
	magic->s.angles = vectoangles(dir);
	
	magic->velocity[0] = speed * dir[0];
	magic->velocity[1] = speed * dir[1];
	magic->velocity[2] = speed * dir[2];

	magic->movetype = MOVETYPE_FLYMISSILE;
	magic->clipmask = MASK_PROJECTILE;
	magic->solid = SOLID_BBOX;
	magic->s.effects |= EF_IONRIPPER;

	magic->mins = {0, 0, 0};
	magic->maxs = {0, 0, 0};

	magic->s.modelindex = gi.modelindex ("models/monsters/spikestrogg/tris.md2");
	magic->owner = self;
	magic->touch = prototype_magic_touch;
	magic->dmg = damage;
	magic->enemy = self->enemy;
	gi.linkentity(magic);
	gi.sound(self, CHAN_WEAPON, sound_attack, 1, ATTN_NORM, 0);
}	

void FireMagic_Prototype(edict_t *self)
{
	vec3_t		dir;

	dir = self->enemy->s.origin - self->s.origin;
	dir[1] = dir[1] + self->radius_dmg * 60;
	
	dir.normalize();

	fire_magic_prototype(self, self->s.origin, dir, 9, 300);
	self->radius_dmg++;
}

// Attack
mframe_t hknight_prototype_frames_attack [] =
{
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, hknight_prototype_reset_magic},
	{ai_charge, 0, FireMagic_Prototype},
	{ai_charge, 0, FireMagic_Prototype},
	{ai_charge, 0, FireMagic_Prototype},
	{ai_charge, 0, FireMagic_Prototype},
	{ai_charge, 0, FireMagic_Prototype},
	{ai_charge, 0, FireMagic_Prototype}
};
MMOVE_T(hknight_prototype_move_attack) = {155, 165, hknight_prototype_frames_attack, hknight_prototype_run};

MONSTERINFO_ATTACK (hknight_prototype_attack) (edict_t *self) -> void
{
		M_SetAnimation(self, &hknight_prototype_move_attack);		
}

// Slice
mframe_t hknight_prototype_frames_slice [] =
{
	{ai_charge, 9,	NULL},
	{ai_charge, 6,	NULL},
	{ai_charge, 13,	NULL},
	{ai_charge, 4,	NULL},
	{ai_charge, 7,	SwingSword_Prototype},
	{ai_charge, 15,	SwingSword_Prototype},
	{ai_charge, 8,	SwingSword_Prototype},
	{ai_charge, 2,	SwingSword_Prototype},
	{ai_charge, 0,	SwingSword_Prototype},
	{ai_charge, 3,	NULL}
};
MMOVE_T(hknight_prototype_move_slice) = {112, 121, hknight_prototype_frames_slice, hknight_prototype_run};

// Smash
mframe_t hknight_prototype_frames_smash [] =
{
	{ai_charge, 1,	NULL},
	{ai_charge, 13,	NULL},
	{ai_charge, 9,	NULL},
	{ai_charge, 11,	NULL},
	{ai_charge, 10,	SwingSword_Prototype},
	{ai_charge, 7,	SwingSword_Prototype},
	{ai_charge, 12,	SwingSword_Prototype},
	{ai_charge, 2,	SwingSword_Prototype},
	{ai_charge, 3,	SwingSword_Prototype},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL}
};
MMOVE_T(hknight_prototype_move_smash) = {122, 132, hknight_prototype_frames_smash, hknight_prototype_run};

// Watk
mframe_t hknight_prototype_frames_watk [] =
{
	{ai_charge, 2, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, SwingSword_Prototype},
	{ai_charge, 0, SwingSword_Prototype},
	{ai_charge, 0, SwingSword_Prototype},
	{ai_charge, 1, NULL},
	{ai_charge, 4, NULL},
	{ai_charge, 5, NULL},
	{ai_charge, 3, SwingSword_Prototype},
	{ai_charge, 2, SwingSword_Prototype},
	{ai_charge, 2, SwingSword_Prototype},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 1, NULL},
	{ai_charge, 1, SwingSword_Prototype},
	{ai_charge, 3, SwingSword_Prototype},
	{ai_charge, 4, SwingSword_Prototype},
	{ai_charge, 6, NULL},
	{ai_charge, 7, NULL},
	{ai_charge, 3, NULL}
};
MMOVE_T(hknight_prototype_move_watk) = {133, 154, hknight_prototype_frames_watk, hknight_prototype_run};

// Melee
MONSTERINFO_MELEE (hknight_prototype_melee) (edict_t *self) -> void
{
	self->dmg_radius++;
	
	if (self->dmg_radius == 1)
		M_SetAnimation(self, &hknight_prototype_move_slice);	
	else if (self->dmg_radius == 2)
		M_SetAnimation(self, &hknight_prototype_move_smash);		
	else if (self->dmg_radius == 3)
	{
		M_SetAnimation(self, &hknight_prototype_move_watk);				
		self->dmg_radius = 0;
	}
	gi.sound(self, CHAN_WEAPON, sound_melee, 1, ATTN_NORM, 0);
}

// Pain
mframe_t hknight_prototype_frames_pain [] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
MMOVE_T(hknight_prototype_move_pain)= {37, 41, hknight_prototype_frames_pain, hknight_prototype_run};

PAIN (hknight_prototype_pain)(edict_t* self, edict_t* other, float kick, int damage, const mod_t& mod) -> void
{
	if (skill->value == 3)
		return;
	if (level.time < self->pain_debounce_time)
		return;
	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

	self->pain_debounce_time = level.time + 1_sec;
	M_SetAnimation(self, &hknight_prototype_move_pain);					
}

void hknight_prototype_dead(edict_t *self)
{
	//VectorSet(self->mins, -16, -16, -24);
	self->mins = { -16, -16, -24 };	
	//VectorSet(self->maxs, 16, 16, -8);
	self->maxs = { 16, 16, -8 };	
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

// Death (1)
mframe_t hknight_prototype_frames_die1 [] =
{
	{ai_move, 0,		NULL},
	{ai_move, 10,	    NULL},
	{ai_move, 8,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 10,	    NULL},
	{ai_move, 11,	    NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
};
MMOVE_T(hknight_prototype_move_die1) = {42, 53, hknight_prototype_frames_die1, hknight_prototype_dead};

// Death (2)
mframe_t hknight_prototype_frames_die2 [] =
{
	{ai_move, 0,		NULL },
	{ai_move, 0,		NULL },
	{ai_move, 0,		NULL },
	{ai_move, 0,		NULL },
	{ai_move, 0,		NULL },
	{ai_move, 0,		NULL },
	{ai_move, 0,		NULL },
	{ai_move, 0,		NULL },
	{ai_move, 0,		NULL }
};
MMOVE_T(hknight_prototype_move_die2) = {54, 62, hknight_prototype_frames_die2, hknight_prototype_dead};

DIE (hknight_prototype_die)(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t& mod) -> void
{
	if (self->health <= self->gib_health){
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		ThrowGibs(self, damage, {
			{ "models/objects/gibs/bone/tris.md2" },
			{ "models/objects/gibs/sm_meat/tris.md2" },
			{ "models/monsters/hellknight_prototype/gibs/g_arm.md2" },
			{ "models/monsters/hellknight_prototype/gibs/g_arm.md2" },
			{ "models/monsters/hellknight_prototype/gibs/g_leg.md2" },
			{ "models/monsters/hellknight_prototype/gibs/g_leg.md2" },
			{ "models/monsters/hellknight_prototype/gibs/g_head.md2", GIB_HEAD }
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
		M_SetAnimation(self, &hknight_prototype_move_die1);
	else
		M_SetAnimation(self, &hknight_prototype_move_die2);	
}

// Sight
MONSTERINFO_SIGHT (hknight_prototype_sight) (edict_t* self, edict_t* other) -> void
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

// Search
MONSTERINFO_SEARCH(hknight_prototype_search) (edict_t *self) -> void
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

MONSTERINFO_SETSKIN(hknight_prototype_setskin) (edict_t* self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}

void SP_monster_hknight_prototype(edict_t *self)
{
	if (!M_AllowSpawn(self)) {
		G_FreeEdict(self);
		return;
	}

	sound_attack.assign("hknight/attack1.wav");
	sound_melee.assign("hknight/slash1.wav");
	sound_death.assign("hknight/death1.wav");
	sound_proj_hit.assign("hknight/hit.wav");
	sound_sight.assign("hknight/sight1.wav");
	sound_search.assign("hknight/idle.wav");
	sound_pain.assign("hknight/pain1.wav");

	self->mins = {-16,-16,-24};
	self->maxs = {16,16,40};	
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->s.modelindex = gi.modelindex("models/monsters/hellknight_prototype/tris.md2");
	gi.modelindex("models/monsters/hellknight_prototype/gibs/g_arm.md2");
	gi.modelindex("models/monsters/hellknight_prototype/gibs/g_leg.md2");
	gi.modelindex("models/monsters/hellknight_prototype/gibs/g_head.md2");


	self->health = 250 * st.health_multiplier;
	self->gib_health = -40;

	self->mass = 250;

	self->pain = hknight_prototype_pain;
	self->die = hknight_prototype_die;
	self->monsterinfo.stand = hknight_prototype_stand;
	self->monsterinfo.walk = hknight_prototype_walk;
	self->monsterinfo.run = hknight_prototype_run;
	self->monsterinfo.attack = hknight_prototype_attack;
	self->monsterinfo.melee = hknight_prototype_melee;
	self->monsterinfo.sight = hknight_prototype_sight;
	self->monsterinfo.search = hknight_prototype_search;
	self->monsterinfo.setskin = hknight_prototype_setskin;
	
	gi.linkentity(self);
	M_SetAnimation(self, &hknight_prototype_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;	

	walkmonster_start(self);
}
