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
// m_spawn.c

#include "g_local.h"
#include "m_spawn.h"

static cached_soundindex sound_death;
static cached_soundindex sound_hit;
static cached_soundindex sound_land;
static cached_soundindex sound_sight;

void spawn_rejump(edict_t *self);
void spawn_unbounce(edict_t *self)
{
	self->movetype = MOVETYPE_STEP;
}

// Stand
mframe_t spawn_frames_stand [] =
{
	//{ai_stand, 0, spawn_unbounce}
	{ai_stand, 0, NULL}
};
MMOVE_T(spawn_move_stand) = {0, 0, spawn_frames_stand, nullptr};

MONSTERINFO_STAND(spawn_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &spawn_move_stand);			
}

// Run
mframe_t spawn_frames_run [] =
{
		{ai_run, 0, NULL},
		{ai_run, 0, NULL},
		{ai_run, 0, NULL},
		{ai_run, 0, NULL},
		{ai_run, 0, NULL},
		{ai_run, 0, NULL},
		{ai_run, 0, NULL},
		{ai_run, 0, NULL},
		{ai_run, 0, NULL},
		{ai_run, 0, NULL},
		{ai_run, 2, NULL},
		{ai_run, 2, NULL},
		{ai_run, 2, NULL},
		{ai_run, 2, NULL},
		{ai_run, 2, NULL},
		{ai_run, 2, NULL},
		{ai_run, 2, NULL},
		{ai_run, 2, NULL},
		{ai_run, 2, NULL},
		{ai_run, 2, NULL},
		{ai_run, 2, NULL},
		{ai_run, 2, NULL},
		{ai_run, 2, NULL},
		{ai_run, 2, NULL},
		{ai_run, 2, NULL}
};
MMOVE_T(spawn_move_run) = {25, 49, spawn_frames_run, nullptr};

MONSTERINFO_RUN(spawn_run) (edict_t *self) -> void
{
	M_SetAnimation(self, &spawn_move_run);		
}

// WALK
mframe_t spawn_frames_walk [] =
{
		{ai_walk, 0, NULL},
		{ai_walk, 0, NULL},
		{ai_walk, 0, NULL},
		{ai_walk, 0, NULL},
		{ai_walk, 0, NULL},
		{ai_walk, 0, NULL},
		{ai_walk, 0, NULL},
		{ai_walk, 0, NULL},
		{ai_walk, 0, NULL},
		{ai_walk, 0, NULL},
		{ai_walk, 2, NULL},
		{ai_walk, 2, NULL},
		{ai_walk, 2, NULL},
		{ai_walk, 2, NULL},
		{ai_walk, 2, NULL},
		{ai_walk, 2, NULL},
		{ai_walk, 2, NULL},
		{ai_walk, 2, NULL},
		{ai_walk, 2, NULL},
		{ai_walk, 2, NULL},
		{ai_walk, 2, NULL},
		{ai_walk, 2, NULL},
		{ai_walk, 2, NULL},
		{ai_walk, 2, NULL},
		{ai_walk, 2, NULL}
};
MMOVE_T(spawn_move_walk) = {25, 49, spawn_frames_walk, nullptr};

MONSTERINFO_WALK(spawn_walk) (edict_t *self) -> void
{
	M_SetAnimation(self, &spawn_move_walk);		
}

// Sight
MONSTERINFO_SIGHT(spawn_sight) (edict_t* self, edict_t* other) -> void
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

// Jump
void prespawnJump(edict_t* self);
void spawnJump(edict_t* self);
void spawn_rejump(edict_t* self);

mframe_t spawn_frames_jump[] =
{
		{ai_charge, 0, NULL},
		{ai_charge, 0, prespawnJump},
		{ai_charge, 0, NULL},
		{ai_charge, 0, NULL},
		{ai_charge, 0, spawnJump},
		{ai_charge, 0, NULL},
		{ai_charge, 0, NULL},
		{ai_charge, 0, NULL},
		{ai_charge, 0, NULL},
		{ai_charge, 0, spawn_rejump}
};
MMOVE_T(spawn_move_jump) = { 50, 59, spawn_frames_jump, spawn_run};

TOUCH(spawn_touch) (edict_t* self, edict_t* other, const trace_t& tr, bool other_touching_self) -> void
{
	vec3_t vel;
	vec3_t point;
	vec3_t normal;
	int	   damage;

	if (other->takedamage)
	{
		vel = self->velocity;
		if (vel.length() > 400)
		{
			normal = self->velocity;
			normal.normalize();
			point = self->s.origin + (normal * self->maxs[0]);
			damage = irandom(0, 10) + 10;
			T_Damage(other, self, self, self->velocity, point, normal, damage, damage, DAMAGE_NONE, MOD_UNKNOWN);

			gi.sound(self, CHAN_WEAPON, sound_hit, 1, ATTN_NORM, 0);
		}
	}
	else {
		gi.sound(self, CHAN_WEAPON, sound_land, 1, ATTN_NORM, 0);
	}

	if (!M_CheckBottom(self))
	{
		if (self->groundentity)
		{
			self->touch = nullptr;
			M_SetAnimation(self, &spawn_move_run);
			self->movetype = MOVETYPE_STEP;
			self->count = 0;
		}
		return;
	}
	self->touch = nullptr;
	M_SetAnimation(self, &spawn_move_jump);
}

void prespawnJump(edict_t* self) {}

void spawnJump(edict_t* self)
{
	vec3_t forward;

	self->movetype = MOVETYPE_BOUNCE;

	AngleVectors(self->s.angles, forward, NULL, NULL);
	self->s.origin[2] += 1;
	self->velocity[0] = 600 * forward[0];
	self->velocity[1] = 600 * forward[1];
	self->velocity[2] = 600 * forward[2];
	self->velocity[2] = 200 + (frandom(0, 1) * 150);
	if (self->groundentity)
	{
		self->groundentity = NULL;
	}
	self->touch = spawn_touch;
}

void spawn_rejump(edict_t* self)
{
	//gi.Com_PrintFmt("entro en rejump valor antes count: {}\n", (int)self->count);
	self->count++;
	//gi.Com_PrintFmt("entro en rejump valor despues count: {}\n", (int)self->count);
	if (self->count < 4) {
		//gi.Com_PrintFmt("entro en if\n");
		self->s.frame = 50;
		return;
	}
}

// Attack
MONSTERINFO_ATTACK(spawn_attack) (edict_t* self) -> void
{
	M_SetAnimation(self, &spawn_move_jump);
}

// Melee
MONSTERINFO_MELEE(spawn_melee) (edict_t* self) -> void
{
	M_SetAnimation(self, &spawn_move_jump);
}

// Death
DIE(spawn_die) (edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t &mod) -> void
{
	self->s.frame = 60;
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_ROCKET_EXPLOSION);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS, false);
	T_RadiusClassDamage(self, attacker, 500.0f, (char*)"monster_spawn", 100.0f, MOD_EXPLOSIVE);
	G_FreeEdict(self);
}

// Pain
PAIN(spawn_pain) (edict_t* self, edict_t* other, float kick, int damage, const mod_t &mod) -> void{}

void SP_monster_spawn(edict_t *self)
{
	if (!M_AllowSpawn(self)) {
		G_FreeEdict(self);
		return;
	}	
	
	self->mins = {-16, -16, -24};
	self->maxs = {16, 16, 40};
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->s.modelindex = gi.modelindex("models/monsters/spawn/tris.md2");	
	self->health = 80;

	sound_death.assign("spawn/death1.wav");
	sound_hit.assign("spawn/hit1.wav");
	sound_land.assign("spawn/land1.wav");
	sound_sight.assign("spawn/sight1.wav");

	self->gib_health = 0;
	self->mass = 80;

	self->die = spawn_die;
	self->pain = spawn_pain;
	self->monsterinfo.stand = spawn_stand;
	self->monsterinfo.walk = spawn_walk;
	self->monsterinfo.run = spawn_run;
	self->monsterinfo.attack = spawn_attack;
	self->monsterinfo.melee = spawn_melee;
	self->monsterinfo.sight = spawn_sight;

	gi.linkentity(self);
	
	M_SetAnimation(self, &spawn_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;	
	
	walkmonster_start(self);
}
