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
// m_shalrath_strogg.cpp

#include "g_local.h"
#include "m_shalrath.h"
#include "m_flash.h"

static cached_soundindex sound_death;
static cached_soundindex sound_search;
static cached_soundindex sound_pain;
static cached_soundindex sound_attack;
static cached_soundindex sound_fire;
static cached_soundindex sound_sight;

vec3_t SightEndtToDir(edict_t* self, vec3_t orig_dir);

// Stand
mframe_t shalrath_frames_stand [] =
{
	{ai_stand, 0, NULL}
};
MMOVE_T(shalrath_move_stand) = {0, 0, shalrath_frames_stand, NULL};

MONSTERINFO_STAND(shalrath_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &shalrath_move_stand);
}

// Walk
mframe_t shalrath_frames_walk[] = {
	{ai_run, 6, NULL},
	{ai_run, 4, NULL},
	{ai_run, 0, NULL},
	{ai_run, 0, NULL},
	{ai_run, 0, NULL},
	{ai_run, 0, NULL},
	{ai_run, 5, NULL},
	{ai_run, 6, NULL},
	{ai_run, 5, NULL},
	{ai_run, 0, NULL},
	{ai_run, 4, NULL},
	{ai_run, 5, NULL}
};
MMOVE_T(shalrath_move_walk) = { 23, 34, shalrath_frames_walk, NULL };

MONSTERINFO_WALK(shalrath_walk) (edict_t* self) -> void
{
	M_SetAnimation(self, &shalrath_move_walk);
}

// Run
mframe_t shalrath_frames_run [] = {
	{ai_run, 6, NULL},
	{ai_run, 4, NULL},
	{ai_run, 0, NULL},
	{ai_run, 0, NULL},
	{ai_run, 0, NULL},
	{ai_run, 0, NULL},
	{ai_run, 5, NULL},
	{ai_run, 6, NULL},
	{ai_run, 5, NULL},
	{ai_run, 0, NULL},
	{ai_run, 4, NULL},
	{ai_run, 5, NULL}
};
MMOVE_T(shalrath_move_run) = {23, 34, shalrath_frames_run, NULL};

MONSTERINFO_RUN(shalrath_run) (edict_t *self) -> void
{
	M_SetAnimation(self, &shalrath_move_run);
}

void shalrath_roar(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_attack, 1, ATTN_NORM, 0);
}

TOUCH (shalrath_pod_touch) (edict_t* self, edict_t* other, const trace_t& tr, bool other_touching_self) -> void
{
	if (other == self->owner)
		return;
	if (strcmp(other->classname, "SP_monster_zombie_strogg") == 0) // decino: According to shalrath.qc
		//T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, 110, 0, 0, 0);
		T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, 110, 110, DAMAGE_NONE, MOD_UNKNOWN);
	//T_RadiusDamage(self, self->owner, self->dmg, NULL, self->dmg + 40, 0);
	T_RadiusDamage(self, self->owner, self->dmg, NULL, self->dmg + 40, DAMAGE_NONE, MOD_EXPLOSIVE);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_ROCKET_EXPLOSION);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS, false);

	G_FreeEdict(self);
}

THINK(shalrath_pod_home) (edict_t* self) -> void
{
	vec3_t			end;
	vec3_t			dir;

	if (self->enemy && self->enemy->health < 1) {
		G_FreeEdict(self);
		return;
	}

	//VectorCopy(self->enemy->s.origin, end);
	end = self->enemy->s.origin;
	end[2] += self->enemy->viewheight; 
			
	//VectorSubtract(end, self->s.origin, dir);
	dir = end - self->s.origin;
			
	//VectorNormalize(dir);
	dir.normalize();

	/*VectorScale(dir, (skill->value >= 3) ? 350 : 250, self->velocity);*/
	if (skill->value >= 3) {
		self->velocity[0] = 350 * dir[0];
		self->velocity[1] = 350 * dir[1];
		self->velocity[2] = 350 * dir[2];
	}else{
		self->velocity[0] = 250 * dir[0];
		self->velocity[1] = 250 * dir[1];
		self->velocity[2] = 250 * dir[2];
	}
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_TUNNEL_SPARKS);
	gi.WriteByte(15);
	gi.WritePosition(self->s.origin);
	gi.WriteDir(vec3_origin);
	gi.WriteByte(255);
	gi.multicast(self->s.origin, MULTICAST_PVS, false);

	self->nextthink = level.time + 0.2_ms;
	self->think = shalrath_pod_home;
}

DIE(fire_shalrath_pod_die) (edict_t* self, edict_t* other, edict_t* inflictor, int damage, const vec3_t& point, const mod_t& mod) -> void
{
	if (mod.id == MOD_CRUSH)
		CTFResetGrapple(self);
}

void fire_shalrath_pod(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed)
{
	edict_t	*pod;

	if (!self->enemy || self->enemy == self)
		return;

	//VectorCopy(SightEndtToDir(self, dir)[0], dir);
	dir = SightEndtToDir(self, dir); 
	//VectorNormalize(dir);
	dir.normalize();

	pod = G_Spawn();
	//VectorCopy(start, pod->s.origin);
	pod->s.origin = start;
	
	//VectorCopy(start, pod->s.old_origin);
	pod->s.old_origin = start;
	
	//vectoangles(dir, pod->s.angles);
	pod->s.angles = vectoangles(dir);

	//VectorScale(dir, speed, pod->velocity);
	pod->velocity[0] = speed * dir[0];
	pod->velocity[1] = speed * dir[1];
	pod->velocity[2] = speed * dir[2];

	//VectorSet(pod->avelocity, 300, 300, 300);
	pod->avelocity[0] = 300;
	pod->avelocity[1] = 300;
	pod->avelocity[2] = 300;
	
	pod->movetype = MOVETYPE_FLYMISSILE;
	pod->clipmask = MASK_PROJECTILE;
	pod->solid = SOLID_BBOX;
	//VectorClear(pod->mins);
	pod->mins[0] = 0;
	pod->mins[1] = 0;
	pod->mins[2] = 0;
	//VectorClear(pod->maxs);
	pod->maxs[0] = 0;
	pod->maxs[1] = 0;
	pod->maxs[2] = 0;
	pod->s.modelindex = gi.modelindex("models/monsters/podstrogg/tris.md2");
	pod->owner = self;
	pod->touch = shalrath_pod_touch;
	pod->nextthink = level.time + 0.1_ms;
	pod->think = shalrath_pod_home;
	pod->dmg = damage;
	pod->die = fire_shalrath_pod_die;
	pod->enemy = self->enemy;
	gi.linkentity(pod);
	gi.sound(self, CHAN_WEAPON, sound_fire, 1, ATTN_NORM, 0);
}

void FireShalrathPod(edict_t *self)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;
	vec3_t	vec;
	vec3_t offset = {16, 0, 16};

	AngleVectors (self->s.angles, forward, right, NULL);
	//G_ProjectSource(self->s.origin, offset, forward, right, start);
	start = G_ProjectSource(self->s.origin, offset, forward, right);

	//VectorCopy(self->enemy->s.origin, vec);
	vec = self->enemy->s.origin;

	//VectorSubtract(vec, start, dir);
	dir = vec - start;
	
	//VectorNormalize(dir);
	dir.normalize();

	fire_shalrath_pod(self, start, dir, 40, 400);
}

// Attack
mframe_t shalrath_frames_attack [] =
{
	{ai_charge, 0, shalrath_roar},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, FireShalrathPod},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL}
};
MMOVE_T(shalrath_move_attack) = {0, 10, shalrath_frames_attack, shalrath_run};

MONSTERINFO_ATTACK (shalrath_attack) (edict_t *self) -> void
{
	M_SetAnimation(self, &shalrath_move_attack);
}

// Pain
mframe_t shalrath_frames_pain [] ={
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
MMOVE_T(shalrath_move_pain) = {11, 15, shalrath_frames_pain, shalrath_run};

PAIN(shalrath_pain) (edict_t* self, edict_t* other, float kick, int damage, const mod_t& mod) -> void
{
	if (skill->value == 3)
		return;
	if (level.time < self->pain_debounce_time)
		return;
	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
	
	M_SetAnimation(self, &shalrath_move_pain);	
	self->pain_debounce_time = level.time + 3_ms;
}

MONSTERINFO_SETSKIN(shalrath_setskin) (edict_t* self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}

void shalrath_dead(edict_t *self)
{
	self->mins = { -32, -32, -24 };
	self->maxs = { 32, 32, -8 };
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

// Death
mframe_t shalrath_frames_death [] ={
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
MMOVE_T(shalrath_move_death) = {16, 22, shalrath_frames_death, shalrath_dead};

DIE(shalrath_die) (edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t& mod) -> void
{
	int		n;

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
	M_SetAnimation(self, &shalrath_move_death);
}

// Sight
MONSTERINFO_SIGHT(shalrath_sight) (edict_t* self, edict_t* other) -> void
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

// Search
MONSTERINFO_SEARCH (shalrath_search) (edict_t *self) -> void
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

void SP_monster_shalrath(edict_t *self)
{
	if (!M_AllowSpawn(self)) {
		G_FreeEdict(self);
		return;
	}	
	
	self->s.modelindex = gi.modelindex("models/monsters/shalrath/tris.md2");	
	self->mins = { -32, -32, -24 };
	self->maxs = { 32, 32, 48 };	
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;	
	
	sound_death.assign("shalrath/death.wav");
	sound_search.assign("shalrath/idle.wav");
	sound_pain.assign("shalrath/pain.wav");
	sound_attack.assign("shalrath/attack.wav");
	sound_fire.assign("shalrath/attack2.wav");
	sound_sight.assign("shalrath/sight.wav");

	self->health = 400;
	self->gib_health = -90;
	
	self->mass = 400;

	self->pain = shalrath_pain;
	self->die = shalrath_die;
	self->monsterinfo.stand = shalrath_stand;
	self->monsterinfo.walk = shalrath_walk;
	self->monsterinfo.run = shalrath_run;
	self->monsterinfo.attack = shalrath_attack;
	self->monsterinfo.sight = shalrath_sight;
	self->monsterinfo.search = shalrath_search;

	gi.linkentity(self);
	M_SetAnimation(self, &shalrath_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);
}
