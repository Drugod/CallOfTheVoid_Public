// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_misc.c

#include "g_local.h"
//#include "g_combat.cpp"

extern edict_t* g_edicts;
extern game_export_t globals; // o "extern game_export_t globals;" según tu cabecera

void T_DamageChthon(edict_t* targ, edict_t* inflictor, edict_t* attacker, const vec3_t& dir, const vec3_t& point, const vec3_t& normal, int damage, int knockback, damageflags_t dflags, mod_t mod);

/*QUAKED misc_chtondead (1 .5 0) (-176 -120 -24) (176 120 72)
Just the chthon model dead on the ground
*/

void SP_misc_chtondead(edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->mins = { -176, -120, -24 };
	ent->maxs = { 176, 120, 72 };
	ent->s.modelindex = gi.modelindex("models/props/chtondead/tris.md2");
	gi.linkentity(ent);
}

/*QUAKED event_lighting (1 .5 0) (-176 -120 -24) (176 120 72)
Sparks shoot out between two targeted points.
*/

bool GetTwoPointsForTarget(const char* tag, vec3_t& outA, vec3_t& outB)
{
	//gi.Com_PrintFmt("Entro en GetTwoPointsForTarget \n");
	vec3_t pts[2];
	int count = 0;

	for (int i = 1; i < globals.num_edicts && count < 2; i++)
	{
		edict_t* e = &g_edicts[i];
		if (!e->inuse)
			continue;
		if (e->target && strcmp(e->target, tag) == 0)
		{
			if (e->solid == SOLID_BSP) {
				pts[count++] = (e->absmin + e->absmax) * 0.5f;
			}
			else {
				pts[count++] = e->s.origin;
			}
		}
	}

	if (count == 2)
	{
		outA = pts[0];
		outB = pts[1];
		return true;
	}
	return false;
}

void fire_lightning(edict_t* self, const vec3_t& start, const vec3_t& dir, int damage, int speed, int kick)
{
	float range;
	bool cached = false;
	vec3_t A, B;
	if (!cached) {
		if (GetTwoPointsForTarget("lighting", A, B)) {
			cached = true;
			range = (B - A).length();
		}
	}
	vec3_t end = A + dir * range;

	trace_t tr = gi.traceline(A, end, self, MASK_PROJECTILE | CONTENTS_SLIME | CONTENTS_LAVA);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_LIGHTNING);
	gi.WriteEntity(self);
	gi.WriteEntity(world);
	gi.WritePosition(A);
	gi.WritePosition(tr.endpos);
	gi.multicast(start, MULTICAST_PVS, false);

	//if (tr.ent && tr.ent->takedamage)
	if (tr.ent)
	{
		vec3_t hitdir = dir;
		hitdir.normalize();
		//T_Damage(tr.ent, self, self, hitdir, tr.endpos, vec3_t{ 0,0,0 }, damage, kick, DAMAGE_ENERGY, MOD_TESLA);
		T_DamageChthon(tr.ent, self, self, hitdir, tr.endpos, vec3_t{ 0,0,0 }, 1, kick, DAMAGE_ENERGY, MOD_TESLA);
	}
	gi.sound(self, CHAN_WEAPON, gi.soundindex("shambler/lightning.wav"), 1, ATTN_NORM, 0);
}

/*THINK(event_lighting_think) (edict_t* self) -> void //version que no atraviesa monstruos
{
	if (level.time >= self->touch_debounce_time) {
		G_FreeEdict(self);
		return;
	}

	edict_t* owner = self->owner ? self->owner : world;

	vec3_t A, B;
	float range = 8192.f;
	bool have = GetTwoPointsForTarget("lighting", A, B);

	if (have) 
		range = (B - A).length();

	vec3_t fwd = self->movedir; fwd.normalize();
	vec3_t rayStart = have ? A : self->s.origin;
	vec3_t end = rayStart + fwd * range;

	trace_t tr = gi.traceline(rayStart, end, owner, MASK_PROJECTILE | CONTENTS_SLIME | CONTENTS_LAVA);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_LIGHTNING);
	gi.WriteEntity(owner);
	gi.WriteEntity(world);
	gi.WritePosition(rayStart);
	gi.WritePosition(tr.endpos);
	gi.multicast(rayStart, MULTICAST_PVS, false);

	const float  DT_SEC = 0.05f;
	const gtime_t DT = gtime_t::from_sec(DT_SEC);
	int dmg_tick = self->dmg;//(int)ceilf(self->dmg * DT_SEC);
	if (tr.ent && tr.ent->takedamage && dmg_tick > 0) {
		vec3_t hitdir = (tr.endpos - rayStart).normalized();
		T_Damage(tr.ent, owner, owner, hitdir, tr.endpos, vec3_t{ 0,0,0 },
			dmg_tick, self->dmg_radius, DAMAGE_ENERGY, MOD_TESLA);
	}

	self->nextthink = level.time + DT;
}*/

THINK(event_lighting_think)(edict_t* self) -> void
{
	if (level.time >= self->touch_debounce_time) {
		G_FreeEdict(self);
		return;
	}

	edict_t* owner = self->owner ? self->owner : world;

	vec3_t A, B;
	if (!GetTwoPointsForTarget("lighting", A, B)) {
		A = self->s.origin;
		B = self->s.origin + self->movedir * 8192.f;
	}
	vec3_t dir = (B - A).normalized();

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_LIGHTNING);
	gi.WriteEntity(owner);
	gi.WriteEntity(world);
	gi.WritePosition(A);
	gi.WritePosition(B);
	gi.multicast(A, MULTICAST_PVS, false);

	trace_t tr = gi.traceline(A, B, owner, MASK_PROJECTILE | CONTENTS_SLIME | CONTENTS_LAVA);
	//if (tr.ent && tr.ent->takedamage && self->dmg > 0) {
	if (tr.ent && self->dmg > 0) {
		T_DamageChthon(tr.ent, owner, owner, dir, tr.endpos, vec3_t{ 0,0,0 },self->dmg, self->dmg_radius, DAMAGE_ENERGY, MOD_TARGET_LASER);
	}
	self->nextthink = level.time + gtime_t::from_sec(0.05f);
}

USE(use_event_lighting) (edict_t* self, edict_t* other, edict_t* activator) -> void
{
	if (self->delay <= 0.0f) {
		fire_lightning(self, self->s.origin, self->movedir, self->dmg, (int)self->speed, 0);
		return;
	}

	edict_t* beam = G_Spawn();
	beam->classname = "event_lighting_beam";
	beam->owner = self;
	beam->svflags |= SVF_NOCLIENT;
	beam->movetype = MOVETYPE_NONE;
	beam->solid = SOLID_NOT;
	beam->s.origin = self->s.origin;
	beam->movedir = self->movedir;
	beam->dmg = 1;//(self->dmg > 0 ? self->dmg : 15);
	beam->dmg_radius = 0;

	float duration = self->delay; // segundos

	if (duration < 0.0f) duration = 0.0f;
	beam->touch_debounce_time = level.time + gtime_t::from_sec(duration);

	beam->think = event_lighting_think;
	beam->nextthink = level.time + gtime_t::from_sec(0.05f);

	gi.linkentity(beam);
	gi.sound(self, CHAN_WEAPON, gi.soundindex("shambler/lightning.wav"), 1, ATTN_NORM, 0);
}

void SP_event_lighting(edict_t* self)
{
	self->use = use_event_lighting;
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	self->svflags |= SVF_NOCLIENT;

	G_SetMovedir(self->s.angles, self->movedir);

	//if (!self->dmg)
		//self->dmg = 500;
		self->dmg = 1;
	if (!self->speed)
		self->speed = 1000;
}