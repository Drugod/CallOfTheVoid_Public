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
#include "m_enforcer.h"
#include "m_flash.h"

static cached_soundindex sound_death;
static cached_soundindex sound_hit;
static cached_soundindex sound_attack;
static cached_soundindex sound_search;
static cached_soundindex sound_pain1;
static cached_soundindex sound_pain2;
static cached_soundindex sound_sight1;
static cached_soundindex sound_sight2;
static cached_soundindex sound_sight3;
static cached_soundindex sound_sight4;

vec3_t SightEndtToDir(edict_t* self, vec3_t orig_dir);

// Stand
mframe_t enforcer_frames_stand[] = {
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand}
};
MMOVE_T(enforcer_move_stand) = { 0, 6, enforcer_frames_stand, NULL };

MONSTERINFO_STAND(enforcer_stand) (edict_t* self) -> void
{
	M_SetAnimation(self, &enforcer_move_stand);
}

// Run
mframe_t enforcer_frames_run[] = {
	{ai_run, 18},
	{ai_run, 14},
	{ai_run, 7},
	{ai_run, 12},
	
	{ai_run, 14},
	{ai_run, 14},
	{ai_run, 7},
	{ai_run, 11}
};
MMOVE_T(enforcer_move_run) = { 23, 30, enforcer_frames_run, NULL };

MONSTERINFO_RUN(enforcer_run) (edict_t* self) -> void
{
	M_SetAnimation(self, &enforcer_move_run);
}

// Run
mframe_t enforcer_frames_walk[] = {
	{ai_walk, 2, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 3, NULL},
	
	{ai_walk, 1, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 1, NULL},
	
	{ai_walk, 2, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 1, NULL},
	
	{ai_walk, 2, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 2, NULL}
	
};
MMOVE_T(enforcer_move_walk) = { 7, 22, enforcer_frames_walk, NULL };


MONSTERINFO_WALK(enforcer_walk) (edict_t* self) -> void
{
	M_SetAnimation(self, &enforcer_move_walk);
}

//attack
TOUCH(enfbolt_touch) (edict_t* self, edict_t* other, const trace_t& tr, bool other_touching_self) -> void
{
	int	mod;

	if (other == self->owner)
		return;

	if (tr.surface && (tr.surface->flags & SURF_SKY))
	{
		G_FreeEdict(self);
		return;
	}
	if (other->takedamage)
		T_Damage(other, self, self->owner, self->velocity, self->s.origin, tr.plane.normal, self->dmg, 1, DAMAGE_ENERGY, MOD_UNKNOWN);
	else
	{
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_WELDING_SPARKS);
		gi.WriteByte(15);
		gi.WritePosition(self->s.origin);
		gi.WriteDir(vec3_origin);
		gi.WriteByte(226);
		gi.multicast(self->s.origin, MULTICAST_PVS, false);
		gi.sound(self, CHAN_WEAPON, sound_hit, 1, ATTN_NORM, 0);
	}
	G_FreeEdict(self);
}

void fire_enfbolt(edict_t* self, vec3_t start, vec3_t dir, int damage, int speed)
{
	edict_t* bolt;

	if (!self->enemy || self->enemy == self)
		return;

	/*//VectorCopy(SightEndtToDir(self, dir)[0], dir);
	dir = SightEndtToDir(self, dir)[0];
	
	//VectorNormalize(dir);
	dir.normalize();

	bolt = G_Spawn();

	//VectorCopy(start, bolt->s.origin);
	bolt->s.origin = start;

	//VectorCopy(start, bolt->s.old_origin);
	bolt->s.old_origin = start;

	//vectoangles(dir, bolt->s.angles);
	bolt->s.angles = vectoangles(dir);

	//VectorScale(dir, speed, bolt->velocity);
	bolt->velocity[0] = speed * dir[0];
	bolt->velocity[1] = speed * dir[1];
	bolt->velocity[2] = speed * dir[2];

	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->clipmask = MASK_PROJECTILE;
	bolt->solid = SOLID_BBOX;
	bolt->s.effects |= EF_HYPERBLASTER;
	
	//VectorClear(bolt->mins);
	bolt->mins = { 0, 0, 0 };

	//VectorClear(bolt->maxs);
	bolt->maxs = { 0, 0, 0 };

	bolt->s.modelindex = gi.modelindex("models/monsters/laserstrogg/tris.md2");
	bolt->owner = self;
	bolt->touch = enfbolt_touch;
	//bolt->nextthink = level.time + 5_ms;
	//bolt->think = G_FreeEdict;
	bolt->dmg = damage;
	gi.linkentity(bolt);
	gi.sound(self, CHAN_WEAPON, sound_attack, 1, ATTN_NORM, 0);
	*/	

	bolt = G_Spawn();
	bolt->s.origin = start;
	bolt->s.old_origin = start;
	bolt->s.angles = vectoangles(dir);
	bolt->velocity = dir * speed;
	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->flags |= FL_DODGE;
	bolt->clipmask = MASK_PROJECTILE;
	bolt->solid = SOLID_BBOX;
	bolt->s.effects |= EF_HYPERBLASTER;
	bolt->s.modelindex = gi.modelindex("models/monsters/laserstrogg/tris.md2");
	bolt->owner = self;
	bolt->touch = enfbolt_touch;
	bolt->nextthink = level.time + 2_sec;
	bolt->think = G_FreeEdict;
	bolt->dmg = damage;
	gi.linkentity(bolt);
	gi.sound(self, CHAN_WEAPON, sound_attack, 1, ATTN_NORM, 0);

}

void FireEnforcerBolt(edict_t* self)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;
	vec3_t	vec;
	//vec3_t	offset = { 30, 8.5, 16 };

	AngleVectors(self->s.angles, forward, right, NULL);
	//G_ProjectSource(self->s.origin, offset, forward, right, start);
	//start = G_ProjectSource(self->s.origin, offset, forward, right);

	start = self->s.origin;
	start[0] += forward[0] * 12.72f;
	start[1] += forward[1] * 12.72f;
	start[2] += 11.36f;
	start[0] += right[0] * 9.24f;
	start[1] += right[1] * 9.24f;

	//VectorCopy(self->enemy->s.origin, vec);
	vec = self->enemy->s.origin;
	vec[2] += self->enemy->viewheight;

	//VectorSubtract(vec, start, dir);
	dir = vec - start;

	//VectorNormalize(dir);
	dir.normalize();

	fire_enfbolt(self, start, dir, 15, 600);
}

mframe_t enforcer_frames_attack2[] =
{
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	FireEnforcerBolt},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL}
};
MMOVE_T(enforcer_move_attack2) = { 35, 38, enforcer_frames_attack2, enforcer_run };

MONSTERINFO_ATTACK (enforcer_attack_again) (edict_t* self) ->void
{
	self->s.frame = 35;
	M_SetAnimation(self, &enforcer_move_attack2);
}

mframe_t enforcer_frames_attack1[] ={
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL},
	
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	FireEnforcerBolt},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL}
	
};
MMOVE_T(enforcer_move_attack1) = { 31, 38, enforcer_frames_attack1, enforcer_attack_again };

MONSTERINFO_ATTACK(enforcer_attack) (edict_t* self) -> void
{
	M_SetAnimation(self, &enforcer_move_attack1);
}

// Pain (1)
mframe_t enforcer_frames_pain1[] = {
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
	
};
MMOVE_T(enforcer_move_pain1) = { 66, 69, enforcer_frames_pain1, enforcer_run };

// Pain (2)
mframe_t enforcer_frames_pain2[] = {
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	
	{ai_move, 0,		NULL}
};
MMOVE_T(enforcer_move_pain2) = { 70, 74, enforcer_frames_pain2, enforcer_run };

// Pain (3)
mframe_t enforcer_frames_pain3[] = {
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
	
};
MMOVE_T(enforcer_move_pain3) = { 75, 82, enforcer_frames_pain3, enforcer_run };


// Pain (4)
mframe_t enforcer_frames_pain4[] = {
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 2,		NULL},
	
	{ai_move, 1,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 1,		NULL},
	{ai_move, 1,		NULL},
	
	{ai_move, 1,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 1,		NULL},
	
	{ai_move, 1,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
	
};
MMOVE_T(enforcer_move_pain4) = { 83, 101, enforcer_frames_pain4, enforcer_run };

// Pain
PAIN(enforcer_pain) (edict_t* self, edict_t* other, float kick, int damage, const mod_t& mod) -> void
{
	float r;

	if (skill->value == 3)
		return;
	if (self->pain_debounce_time > level.time)
		return;
	r = frandom();

	if (r < 0.5)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);		
	if (r < 0.2)
	{
		M_SetAnimation(self, &enforcer_move_pain1);
		self->pain_debounce_time = level.time + 6_sec;
	}
	else if (r < 0.4)
	{
		M_SetAnimation(self, &enforcer_move_pain2);
		self->pain_debounce_time = level.time + 1_sec;
	}
	else if (r < 0.7)
	{
		M_SetAnimation(self, &enforcer_move_pain3);
		self->pain_debounce_time = level.time + 1_sec;
	}
	else
	{
		M_SetAnimation(self, &enforcer_move_pain4);
		self->pain_debounce_time = level.time + 1_sec;
	}
}

MONSTERINFO_SETSKIN(enforcer_setskin) (edict_t* self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}

void enforcer_dead(edict_t* self)
{
	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, -8 };
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

// Death (1)
mframe_t enforcer_frames_death1[] ={
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 14,		NULL},
	
	{ai_move, 2,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	
	{ai_move, 3,		NULL},
	{ai_move, 5,		NULL},
	{ai_move, 5,		NULL},
	{ai_move, 5,		NULL},
	
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
};
MMOVE_T(enforcer_move_death1) = { 41, 54, enforcer_frames_death1, enforcer_dead };

// Death (2)
mframe_t enforcer_frames_death2[] = {
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,	    NULL},
	{ai_move, 0,		NULL},
	
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
};
MMOVE_T(enforcer_move_death2) = { 55, 65, enforcer_frames_death2, enforcer_dead };

// Death
DIE(enforcer_die) (edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t& mod) -> void
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
		M_SetAnimation(self, &enforcer_move_death1);
	else
		M_SetAnimation(self, &enforcer_move_death2);
}

// Sight
MONSTERINFO_SIGHT(enforcer_sight) (edict_t* self, edict_t* other) -> void
{
	int r = irandom(4);

	switch (r)
	{
		case 0: gi.sound(self, CHAN_VOICE, sound_sight1, 1, ATTN_NORM, 0); break;
		case 1: gi.sound(self, CHAN_VOICE, sound_sight2, 1, ATTN_NORM, 0); break;
		case 2: gi.sound(self, CHAN_VOICE, sound_sight3, 1, ATTN_NORM, 0); break;
		case 3: gi.sound(self, CHAN_VOICE, sound_sight4, 1, ATTN_NORM, 0); break;
		default: gi.sound(self, CHAN_VOICE, sound_sight1, 1, ATTN_NORM, 0); break;
	}
}

// Search
MONSTERINFO_SEARCH(enforcer_search) (edict_t* self) -> void
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

void SP_monster_enforcer(edict_t* self)
{
	if (!M_AllowSpawn(self)) {
		G_FreeEdict(self);
		return;
	}

	self->s.modelindex = gi.modelindex("models/monsters/enforcer/tris.md2");
	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, 40 };
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	sound_death.assign("enforcer/death1.wav");
	sound_hit.assign("enforcer/enfstop.wav");
	sound_attack.assign("enforcer/enfire.wav");
	sound_search.assign("enforcer/idle1.wav");
	sound_pain1.assign("enforcer/pain1.wav");
	sound_pain2.assign("enforcer/pain2.wav");
	sound_sight1.assign("enforcer/sight1.wav");
	sound_sight2.assign("enforcer/sight2.wav");
	sound_sight3.assign("enforcer/sight3.wav");
	sound_sight4.assign("enforcer/sight4.wav");

	self->health = 80;
	self->gib_health = -35;

	self->mass = 120;

	self->pain = enforcer_pain;
	self->die = enforcer_die;
	self->monsterinfo.stand = enforcer_stand;
	self->monsterinfo.walk = enforcer_walk;
	self->monsterinfo.run = enforcer_run;
	self->monsterinfo.attack = enforcer_attack;
	self->monsterinfo.sight = enforcer_sight;
	self->monsterinfo.search = enforcer_search;
	self->monsterinfo.setskin = enforcer_setskin;

	gi.linkentity(self);

	M_SetAnimation(self, &enforcer_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);
}