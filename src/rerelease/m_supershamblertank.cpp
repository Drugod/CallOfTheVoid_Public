// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

supershamblertank

==============================================================================
*/

#include "g_local.h"
#include "m_supershamblertank.h"
#include "m_flash.h"

constexpr spawnflags_t SPAWNFLAG_supershamblertank_POWERSHIELD = 8_spawnflag;
// n64
constexpr spawnflags_t SPAWNFLAG_supershamblertank_LONG_DEATH = 16_spawnflag;

//tesla
constexpr gtime_t TESLA_TIME_TO_LIVE = 30_sec;
constexpr float	  TESLA_DAMAGE_RADIUS = 128;
constexpr int32_t TESLA_DAMAGE = 3;
constexpr int32_t TESLA_KNOCKBACK = 8;

constexpr gtime_t TESLA_ACTIVATE_TIME = 3_sec;

constexpr int32_t TESLA_EXPLOSION_DAMAGE_MULT = 50; // this is the amount the damage is multiplied by for underwater explosions
constexpr float	  TESLA_EXPLOSION_RADIUS = 200;
//end tesla

static cached_soundindex sound_pain1;
static cached_soundindex sound_pain2;
static cached_soundindex sound_pain3;
static cached_soundindex sound_death;
static cached_soundindex sound_search1;
static cached_soundindex sound_search2;
static cached_soundindex sound_melee;

static cached_soundindex tread_sound_2;

void TreadSound_2(edict_t *self)
{
	gi.sound(self, CHAN_BODY, tread_sound_2, 1, ATTN_NORM, 0);
}

// *************************
// TESLA
// *************************

constexpr gtime_t TESLA2_TIME_TO_LIVE = 30_sec;
constexpr float	  TESLA2_DAMAGE_RADIUS = 128;
constexpr int32_t TESLA2_DAMAGE = 3;
constexpr int32_t TESLA2_KNOCKBACK = 8;

constexpr gtime_t TESLA2_ACTIVATE_TIME = 3_sec;

constexpr int32_t TESLA2_EXPLOSION_DAMAGE_MULT = 50; // this is the amount the damage is multiplied by for underwater explosions
constexpr float	  TESLA2_EXPLOSION_RADIUS = 200;

void tesla2_remove(edict_t* self)
{
	edict_t* cur, * next;

	self->takedamage = false;
	if (self->teamchain)
	{
		cur = self->teamchain;
		while (cur)
		{
			next = cur->teamchain;
			G_FreeEdict(cur);
			cur = next;
		}
	}
	else if (self->air_finished)
		gi.Com_Print("tesla2_mine without a field!\n");

	self->owner = self->teammaster; // Going away, set the owner correctly.
	// PGM - grenade explode does damage to self->enemy
	self->enemy = nullptr;

	// play quad sound if quadded and an underwater explosion
	if ((self->dmg_radius) && (self->dmg > (TESLA2_DAMAGE * TESLA2_EXPLOSION_DAMAGE_MULT)))
		gi.sound(self, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);

	Grenade_Explode(self);
}

DIE(tesla2_die) (edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t& mod) -> void
{
	tesla2_remove(self);
}

void tesla2_blow(edict_t* self)
{
	self->dmg *= TESLA2_EXPLOSION_DAMAGE_MULT;
	self->dmg_radius = TESLA2_EXPLOSION_RADIUS;
	tesla2_remove(self);
}

TOUCH(tesla2_zap) (edict_t* self, edict_t* other, const trace_t& tr, bool other_touching_self) -> void
{
}

static BoxEdictsResult_t tesla2_think_active_BoxFilter(edict_t* check, void* data)
{
	edict_t* self = (edict_t*)data;

	if (!check->inuse)
		return BoxEdictsResult_t::Skip;
	if (check == self)
		return BoxEdictsResult_t::Skip;
	if (check->health < 1)
		return BoxEdictsResult_t::Skip;
	// don't hit teammates
	//if (check->client)
	//{
	//	if (!deathmatch->integer)
	//		return BoxEdictsResult_t::Skip;
	//	else if (CheckTeamDamage(check, self->teammaster))
	//		return BoxEdictsResult_t::Skip;
	//}
	//if (!(check->svflags & SVF_MONSTER) && !(check->flags & FL_DAMAGEABLE) && !check->client)
	//	return BoxEdictsResult_t::Skip;
	if ((check->svflags & SVF_MONSTER))
		return BoxEdictsResult_t::Skip;

	// don't hit other teslas in SP/coop
	if (!deathmatch->integer && check->classname && (check->flags & FL_TRAP))
		return BoxEdictsResult_t::Skip;

	return BoxEdictsResult_t::Keep;
}

THINK(tesla2_think_active) (edict_t* self) -> void
{
	int		 i, num;
	static edict_t* touch[MAX_EDICTS];
	edict_t* hit;
	vec3_t	 dir, start;
	trace_t	 tr;

	if (level.time > self->air_finished)
	{
		tesla2_remove(self);
		return;
	}

	start = self->s.origin;
	start[2] += 16;

	num = gi.BoxEdicts(self->teamchain->absmin, self->teamchain->absmax, touch, MAX_EDICTS, AREA_SOLID, tesla2_think_active_BoxFilter, self);
	for (i = 0; i < num; i++)
	{
		// if the tesla died while zapping things, stop zapping.
		if (!(self->inuse))
			break;

		hit = touch[i];
		if (!hit->inuse)
			continue;
		if (hit == self)
			continue;
		if (hit->health < 1)
			continue;
		// don't hit teammates
		//if (hit->client)
		//{
		//	if (!deathmatch->integer)
		//		continue;
		//	else if (CheckTeamDamage(hit, self->teamchain->owner))
		//		continue;
		//}
		//if (!(hit->svflags & SVF_MONSTER) && !(hit->flags & FL_DAMAGEABLE) && !hit->client)
		//	continue;

		tr = gi.traceline(start, hit->s.origin, self, MASK_PROJECTILE);
		if (tr.fraction == 1 || tr.ent == hit)
		{
			dir = hit->s.origin - start;

			// PMM - play quad sound if it's above the "normal" damage
			if (self->dmg > TESLA2_DAMAGE)
				gi.sound(self, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);

			// PGM - don't do knockback to walking monsters
			//if ((hit->svflags & SVF_MONSTER) && !(hit->flags & (FL_FLY | FL_SWIM)))
			if ((hit->client) && !(hit->flags & (FL_FLY | FL_SWIM)))
				T_Damage(hit, self, self->teammaster, dir, tr.endpos, tr.plane.normal,
					self->dmg, 0, DAMAGE_NONE, MOD_TESLA);
			else
				T_Damage(hit, self, self->teammaster, dir, tr.endpos, tr.plane.normal,
					self->dmg, TESLA2_KNOCKBACK, DAMAGE_NONE, MOD_TESLA);

			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_LIGHTNING);
			gi.WriteEntity(self);	// source entity
			gi.WriteEntity(hit); // destination entity
			gi.WritePosition(start);
			gi.WritePosition(tr.endpos);
			gi.multicast(start, MULTICAST_PVS, false);
		}
	}

	if (self->inuse)
	{
		self->think = tesla2_think_active;
		self->nextthink = level.time + 10_hz;
	}
}

THINK(tesla2_activate) (edict_t* self) -> void
{
	edict_t* trigger;
	edict_t* search;

	if (gi.pointcontents(self->s.origin) & (CONTENTS_SLIME | CONTENTS_LAVA | CONTENTS_WATER))
	{
		tesla2_blow(self);
		return;
	}

	// only check for spawn points in deathmatch
	if (deathmatch->integer)
	{
		search = nullptr;
		while ((search = findradius(search, self->s.origin, 1.5f * TESLA2_DAMAGE_RADIUS)) != nullptr)
		{
			// [Paril-KEX] don't allow traps to be placed near flags or teleporters
			// if it's a monster or player with health > 0
			// or it's a player start point
			// and we can see it
			// blow up
			if (search->classname && ((deathmatch->integer &&
				((!strncmp(search->classname, "info_player_", 12)) ||
					(!strcmp(search->classname, "misc_teleporter_dest")) ||
					(!strncmp(search->classname, "item_flag_", 10))))) &&
				(visible(search, self)))
			{
				BecomeExplosion1(self);
				return;
			}
		}
	}

	trigger = G_Spawn();
	trigger->s.origin = self->s.origin;
	trigger->mins = { -TESLA2_DAMAGE_RADIUS, -TESLA2_DAMAGE_RADIUS, self->mins[2] };
	trigger->maxs = { TESLA2_DAMAGE_RADIUS, TESLA2_DAMAGE_RADIUS, TESLA2_DAMAGE_RADIUS };
	trigger->movetype = MOVETYPE_NONE;
	trigger->solid = SOLID_TRIGGER;
	trigger->owner = self;
	trigger->touch = tesla2_zap;
	trigger->classname = "tesla trigger";
	// doesn't need to be marked as a teamslave since the move code for bounce looks for teamchains
	gi.linkentity(trigger);

	self->s.angles = {};
	// clear the owner if in deathmatch
	if (deathmatch->integer)
		self->owner = nullptr;
	self->teamchain = trigger;
	self->think = tesla2_think_active;
	self->nextthink = level.time + FRAME_TIME_S;
	self->air_finished = level.time + TESLA2_TIME_TO_LIVE;
}

THINK(tesla2_think) (edict_t* ent) -> void
{
	if (gi.pointcontents(ent->s.origin) & (CONTENTS_SLIME | CONTENTS_LAVA))
	{
		tesla2_remove(ent);
		return;
	}

	ent->s.angles = {};

	if (!(ent->s.frame))
		gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/teslaopen.wav"), 1, ATTN_NORM, 0);

	ent->s.frame++;
	if (ent->s.frame > 14)
	{
		ent->s.frame = 14;
		ent->think = tesla2_activate;
		ent->nextthink = level.time + 10_hz;
	}
	else
	{
		if (ent->s.frame > 9)
		{
			if (ent->s.frame == 10)
			{
				if (ent->owner && ent->owner->client)
				{
					PlayerNoise(ent->owner, ent->s.origin, PNOISE_WEAPON); // PGM
				}
				ent->s.skinnum = 1;
			}
			else if (ent->s.frame == 12)
				ent->s.skinnum = 2;
			else if (ent->s.frame == 14)
				ent->s.skinnum = 3;
		}
		ent->think = tesla2_think;
		ent->nextthink = level.time + 10_hz;
	}
}

TOUCH(tesla2_lava) (edict_t* ent, edict_t* other, const trace_t& tr, bool other_touching_self) -> void
{
	if (tr.contents & (CONTENTS_SLIME | CONTENTS_LAVA))
	{
		tesla2_blow(ent);
		return;
	}

	if (ent->velocity)
	{
		if (frandom() > 0.5f)
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/hgrenb1a.wav"), 1, ATTN_NORM, 0);
		else
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/hgrenb2a.wav"), 1, ATTN_NORM, 0);
	}
}

void fire_tesla2(edict_t* self, const vec3_t& start, const vec3_t& aimdir, int tesla_damage_multiplier, int speed, bool monster)
{
	edict_t* tesla;
	vec3_t	 dir;
	vec3_t	 forward, right, up;

	dir = vectoangles(aimdir);
	AngleVectors(dir, forward, right, up);

	tesla = G_Spawn();
	tesla->s.origin = start;
	tesla->velocity = aimdir * speed;

	float gravityAdjustment = level.gravity / 800.f;

	tesla->velocity += up * (200 + crandom() * 10.0f) * gravityAdjustment;
	tesla->velocity += right * (crandom() * 10.0f);

	tesla->s.angles = {};
	tesla->movetype = MOVETYPE_BOUNCE;
	tesla->solid = SOLID_BBOX;
	tesla->s.effects |= EF_GRENADE;
	tesla->s.renderfx |= RF_IR_VISIBLE;
	tesla->mins = { -12, -12, 0 };
	tesla->maxs = { 12, 12, 20 };
	tesla->s.modelindex = gi.modelindex("models/weapons/g_tesla/tris.md2");

	tesla->owner = self; // PGM - we don't want it owned by self YET.
	tesla->teammaster = self;

	tesla->wait = (level.time + TESLA2_TIME_TO_LIVE).seconds();
	tesla->think = tesla2_think;
	tesla->nextthink = level.time + TESLA2_ACTIVATE_TIME;

	// blow up on contact with lava & slime code
	tesla->touch = tesla2_lava;

	if (deathmatch->integer)
		// PMM - lowered from 50 - 7/29/1998
		tesla->health = 20;
	else
		tesla->health = 50; // FIXME - change depending on skill?

	tesla->takedamage = true;
	tesla->die = tesla2_die;
	tesla->dmg = TESLA2_DAMAGE * tesla_damage_multiplier;
	tesla->classname = "tesla_mine";
	tesla->flags |= (FL_DAMAGEABLE | FL_TRAP);
	tesla->clipmask = (MASK_PROJECTILE | CONTENTS_SLIME | CONTENTS_LAVA) & ~CONTENTS_DEADMONSTER;

	// [Paril-KEX]
	if (self->client && !G_ShouldPlayersCollide(true))
		tesla->clipmask &= ~CONTENTS_PLAYER;

	tesla->flags |= FL_MECHANICAL;

	gi.linkentity(tesla);
}

void monster_fire_tesla(edict_t* self, const vec3_t& start, const vec3_t& aimdir, int damage, int speed,
	monster_muzzleflash_id_t flashtype, float right_adjust, float up_adjust)
{
	fire_tesla2(self, start, aimdir, damage, speed, true);
	monster_muzzleflash(self, start, flashtype);
}

MONSTERINFO_SEARCH(supershamblertank_search) (edict_t *self) -> void
{
	if (frandom() < 0.5f)
		gi.sound(self, CHAN_VOICE, sound_search1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_search2, 1, ATTN_NORM, 0);
}

void supershamblertank_dead(edict_t *self);
void supershamblertankRocket(edict_t *self);
void supershamblertankMachineGun1(edict_t* self);
void supershamblertankMachineGun2(edict_t* self);
void supershamblertankMachineGun3(edict_t* self);
void supershamblertankMachineGun4(edict_t* self);
void supershamblertankMachineGunner(edict_t *self, int type);
void supershamblertank_reattack1(edict_t *self);

//
// stand
//

mframe_t supershamblertank_frames_stand[] = {
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }
};
MMOVE_T(supershamblertank_move_stand) = { FRAME_stand_1, FRAME_stand_60, supershamblertank_frames_stand, nullptr };

MONSTERINFO_STAND(supershamblertank_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &supershamblertank_move_stand);
}

mframe_t supershamblertank_frames_run[] = {
	{ ai_run, 12, TreadSound_2 },
	{ ai_run, 12 },
	{ ai_run, 12 },
	{ ai_run, 12 },
	{ ai_run, 12 },
	{ ai_run, 12 },
	{ ai_run, 12 },
	{ ai_run, 12 },
	{ ai_run, 12 },
	{ ai_run, 12 },
	{ ai_run, 12 },
	{ ai_run, 12 },
	{ ai_run, 12 },
	{ ai_run, 12 },
	{ ai_run, 12 },
	{ ai_run, 12 },
	{ ai_run, 12 },
	{ ai_run, 12 }
};
MMOVE_T(supershamblertank_move_run) = { FRAME_forwrd_1, FRAME_forwrd_18, supershamblertank_frames_run, nullptr };

//
// walk
//

mframe_t supershamblertank_frames_forward[] = {
	{ ai_walk, 4, TreadSound_2 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 }
};
MMOVE_T(supershamblertank_move_forward) = { FRAME_forwrd_1, FRAME_forwrd_18, supershamblertank_frames_forward, nullptr };

void supershamblertank_forward(edict_t *self)
{
	M_SetAnimation(self, &supershamblertank_move_forward);
}

MONSTERINFO_WALK(supershamblertank_walk) (edict_t *self) -> void
{
	M_SetAnimation(self, &supershamblertank_move_forward);
}

MONSTERINFO_RUN(supershamblertank_run) (edict_t *self) -> void
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		M_SetAnimation(self, &supershamblertank_move_stand);
	else
		M_SetAnimation(self, &supershamblertank_move_run);
}

#if 0
mframe_t supershamblertank_frames_turn_right[] = {
	{ ai_move, 0, TreadSound_2 },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(supershamblertank_move_turn_right) = { FRAME_right_1, FRAME_right_18, supershamblertank_frames_turn_right, supershamblertank_run };

mframe_t supershamblertank_frames_turn_left[] = {
	{ ai_move, 0, TreadSound_2 },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(supershamblertank_move_turn_left) = { FRAME_left_1, FRAME_left_18, supershamblertank_frames_turn_left, supershamblertank_run };
#endif

mframe_t supershamblertank_frames_pain3[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(supershamblertank_move_pain3) = { FRAME_pain3_9, FRAME_pain3_12, supershamblertank_frames_pain3, supershamblertank_run };

mframe_t supershamblertank_frames_pain2[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(supershamblertank_move_pain2) = { FRAME_pain2_5, FRAME_pain2_8, supershamblertank_frames_pain2, supershamblertank_run };

mframe_t supershamblertank_frames_pain1[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(supershamblertank_move_pain1) = { FRAME_pain1_1, FRAME_pain1_4, supershamblertank_frames_pain1, supershamblertank_run };

static void BossLoop(edict_t *self)
{
	if (!(self->spawnflags & SPAWNFLAG_supershamblertank_LONG_DEATH))
		return;

	if (self->count)
		self->count--;
	else
		self->spawnflags &= ~SPAWNFLAG_supershamblertank_LONG_DEATH;

	self->monsterinfo.nextframe = FRAME_death_19;
}

static void supershamblertankGrenade(edict_t *self)
{
	vec3_t					 forward, right;
	vec3_t					 start;
	monster_muzzleflash_id_t flash_number;

	if (!self->enemy || !self->enemy->inuse) // PGM
		return;								 // PGM

	if (self->s.frame == FRAME_attak4_1)
		flash_number = MZ2_SUPERTANK_GRENADE_1;
	else
		flash_number = MZ2_SUPERTANK_GRENADE_2;

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, monster_flash_offset[flash_number], forward, right);

	vec3_t aim_point;
	PredictAim(self, self->enemy, start, 0, false, crandom_open() * 0.1f, &forward, &aim_point);

	for (float speed = 500.f; speed < 1000.f; speed += 100.f)
	{
		if (!M_CalculatePitchToFire(self, aim_point, start, forward, speed, 2.5f, true))
			continue;

		monster_fire_tesla(self, start, forward, 50, speed, flash_number, 0.f, 0.f);
		break;
	}
}

mframe_t supershamblertank_frames_death1[] = {
	{ ai_move, 0, BossExplode },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, BossLoop }
};
MMOVE_T(supershamblertank_move_death) = { FRAME_death_1, FRAME_death_24, supershamblertank_frames_death1, supershamblertank_dead };

mframe_t supershamblertank_frames_attack4[] = {
	{ ai_move, 0, supershamblertankGrenade },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, supershamblertankGrenade },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(supershamblertank_move_attack4) = { FRAME_attak4_1, FRAME_attak4_6, supershamblertank_frames_attack4, supershamblertank_run };

mframe_t supershamblertank_frames_attack2[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, supershamblertankRocket },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, supershamblertankRocket },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, supershamblertankRocket },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(supershamblertank_move_attack2) = { FRAME_attak2_1, FRAME_attak2_27, supershamblertank_frames_attack2, supershamblertank_run };

mframe_t supershamblertank_frames_attack1[] = {
	{ ai_charge, 0, supershamblertankMachineGun1},
	{ ai_charge, 0, supershamblertankMachineGun2},
	{ ai_charge, 0, supershamblertankMachineGun3},
	{ ai_charge, 0, supershamblertankMachineGun4},
	{ ai_charge, 0, supershamblertankMachineGun1},
	{ ai_charge, 0, supershamblertankMachineGun2}
};
MMOVE_T(supershamblertank_move_attack1) = { FRAME_attak1_1, FRAME_attak1_6, supershamblertank_frames_attack1, supershamblertank_reattack1 };

mframe_t supershamblertank_frames_end_attack1[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(supershamblertank_move_end_attack1) = { FRAME_attak1_7, FRAME_attak1_20, supershamblertank_frames_end_attack1, supershamblertank_run };

void supershamblertank_reattack1(edict_t *self)
{
	if (visible(self, self->enemy))
	{
		if (self->timestamp >= level.time || frandom() < 0.3f)
			M_SetAnimation(self, &supershamblertank_move_attack1);
		else
			M_SetAnimation(self, &supershamblertank_move_end_attack1);
	}
	else
		M_SetAnimation(self, &supershamblertank_move_end_attack1);
}

PAIN(supershamblertank_pain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
{
	if (level.time < self->pain_debounce_time)
		return;

	// Lessen the chance of him going into his pain frames
	if (mod.id != MOD_CHAINFIST)
	{
		if (damage <= 25)
			if (frandom() < 0.2f)
				return;

		// Don't go into pain if he's firing his rockets
		if ((self->s.frame >= FRAME_attak2_1) && (self->s.frame <= FRAME_attak2_14))
			return;
	}

	if (damage <= 10)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else if (damage <= 25)
		gi.sound(self, CHAN_VOICE, sound_pain3, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

	self->pain_debounce_time = level.time + 3_sec;
	
	if (!M_ShouldReactToPain(self, mod))
		return; // no pain anims in nightmare

	if (damage <= 10)
		M_SetAnimation(self, &supershamblertank_move_pain1);
	else if (damage <= 25)
		M_SetAnimation(self, &supershamblertank_move_pain2);
	else
		M_SetAnimation(self, &supershamblertank_move_pain3);
}

MONSTERINFO_SETSKIN(supershamblertank_setskin) (edict_t *self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;
	else
		self->s.skinnum &= ~1;
}

void supershamblertankRocket(edict_t *self)
{
	vec3_t					 forward, right;
	vec3_t					 start;
	vec3_t					 dir;
	vec3_t					 vec;
	monster_muzzleflash_id_t flash_number;

	if (!self->enemy || !self->enemy->inuse) // PGM
		return;								 // PGM

	if (self->s.frame == FRAME_attak2_8)
		flash_number = MZ2_SUPERTANK_ROCKET_1;
	else if (self->s.frame == FRAME_attak2_11)
		flash_number = MZ2_SUPERTANK_ROCKET_2;
	else // (self->s.frame == FRAME_attak2_14)
		flash_number = MZ2_SUPERTANK_ROCKET_3;

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, monster_flash_offset[flash_number], forward, right);

	if (self->spawnflags.has(SPAWNFLAG_supershamblertank_POWERSHIELD))
	{
		vec = self->enemy->s.origin;
		vec[2] += self->enemy->viewheight;
		dir = vec - start;
		dir.normalize();
		monster_fire_heat(self, start, dir, 40, 500, flash_number, 0.075f);
	}
	else
	{
		PredictAim(self, self->enemy, start, 750, false, 0.f, &forward, nullptr);
		monster_fire_rocket(self, start, forward, 50, 750, flash_number);
	}
}

//void supershamblertankMachineGun(edict_t *self)
//{
//	vec3_t					 dir;
//	vec3_t					 start;
//	vec3_t					 forward, right;
//	monster_muzzleflash_id_t flash_number;
//
//	if (!self->enemy || !self->enemy->inuse) // PGM
//		return;								 // PGM
//
//	flash_number = static_cast<monster_muzzleflash_id_t>(MZ2_SUPERTANK_MACHINEGUN_1 + (self->s.frame - FRAME_attak1_1));
//
//	dir[0] = 0;
//	dir[1] = self->s.angles[1];
//	dir[2] = 0;
//
//	AngleVectors(dir, forward, right, nullptr);
//	start = M_ProjectFlashSource(self, monster_flash_offset[flash_number], forward, right);
//	PredictAim(self, self->enemy, start, 0, true, -0.1f, &forward, nullptr);
//	monster_fire_bullet(self, start, forward, 6, 4, DEFAULT_BULLET_HSPREAD * 3, DEFAULT_BULLET_VSPREAD * 3, flash_number);
//}

void supershamblertankMachineGun1(edict_t* self)
{
	supershamblertankMachineGunner(self, 1);
}

void supershamblertankMachineGun2(edict_t* self)
{
	supershamblertankMachineGunner(self, 2);
}

void supershamblertankMachineGun3(edict_t* self)
{
	supershamblertankMachineGunner(self, 3);
}

void supershamblertankMachineGun4(edict_t* self)
{
	supershamblertankMachineGunner(self, 4);
}

void supershamblertankMachineGunner(edict_t* self, int type)
{
	vec3_t start;
	vec3_t dir;
	vec3_t forward, right;
	monster_muzzleflash_id_t flash_number;

	AngleVectors(self->s.angles, forward, right, nullptr);
	flash_number = static_cast<monster_muzzleflash_id_t>(MZ2_SUPERTANK_MACHINEGUN_1 + (self->s.frame - FRAME_attak1_1));
	start = M_ProjectFlashSource(self, monster_flash_offset[flash_number], forward, right);
	
	float y_adjustment, z_adjustment;

	switch (type)
	{
	case 1:
		y_adjustment = 15.0f; //izquierdo arriba
		z_adjustment = -3.0f;
		break;
	case 2:
		y_adjustment = 2.0f; //derecho arriba
		z_adjustment = -3.0f;
		break;
	case 3:
		y_adjustment = 15.0f; //izquierdo abajo
		z_adjustment = -12.0f;
		break;
	case 4:
	default:
		y_adjustment = 2.0f; //derecho abajo
		z_adjustment = -12.0f;
		break;
	}

	vec3_t right_adjustment = right * y_adjustment;
	vec3_t up_adjustment = vec3_t{0.0f, 0.0f, z_adjustment };
	vec3_t total_adjustment = right_adjustment + up_adjustment;
	start += total_adjustment;

	// calc direction to where we targeted
	dir = self->pos1 - start;
	dir.normalize();

	int damage = 35;
	int radius_damage = 45;

	if (self->s.frame > FRAME_attak1_3)
	{
		damage /= 2;
		radius_damage /= 2;
	}

	fire_plasma(self, start, dir, damage, 725, radius_damage, radius_damage);

	// save for aiming the shot
	self->pos1 = self->enemy->s.origin;
	self->pos1[2] += self->enemy->viewheight;
}

void StroggTankChainsaw(edict_t* self)
{
	vec3_t dir;
	static vec3_t aim = { 100, 0, -24 };
	int damage;

	if (!self->enemy)
		return;
	//VectorSubtract(self->s.origin, self->enemy->s.origin, dir);
	dir = self->enemy->s.origin - self->s.origin;

	if (dir.length() > 100.0)
		return;
	damage = (frandom() + frandom() + frandom()) * 4;

	fire_hit(self, aim, damage, damage);
}

// Swing
mframe_t supershamblertank_frames_melee[] = {
	{ai_charge, 11,	NULL},
	{ai_charge, 1,	NULL},
	{ai_charge, 4,	NULL},
	{ai_charge, 19,	StroggTankChainsaw},
	{ai_charge, 13,	StroggTankChainsaw},
	{ai_charge, 10,	StroggTankChainsaw},
	{ai_charge, 10,	StroggTankChainsaw},
	{ai_charge, 10,	StroggTankChainsaw},
	{ai_charge, 10,	StroggTankChainsaw},
	{ai_charge, 10,	StroggTankChainsaw},
	{ai_charge, 3,	NULL},
	{ai_charge, 8,	NULL},
	{ai_charge, 9,	NULL},
	{ai_charge, 0,	NULL}
};
MMOVE_T(supershamblertank_move_melee) = { FRAME_forwrd_1, FRAME_forwrd_14, supershamblertank_frames_melee, supershamblertank_run };

// Melee
MONSTERINFO_MELEE(supershamblertank_melee) (edict_t* self) -> void
{
	M_SetAnimation(self, &supershamblertank_move_melee);
	gi.sound(self, CHAN_WEAPON, sound_melee, 1, ATTN_NORM, 0);
}

MONSTERINFO_ATTACK(supershamblertank_attack) (edict_t *self) -> void
{
	vec3_t vec;
	float  range;

	vec = self->enemy->s.origin - self->s.origin;
	range = range_to(self, self->enemy);

	// Attack 1 == Chaingun
	// Attack 2 == Rocket Launcher
	// Attack 3 == Grenade Launcher
	bool chaingun_good = M_CheckClearShot(self, monster_flash_offset[MZ2_SUPERTANK_MACHINEGUN_1]);
	bool rocket_good = M_CheckClearShot(self, monster_flash_offset[MZ2_SUPERTANK_ROCKET_1]);
	bool grenade_good = M_CheckClearShot(self, monster_flash_offset[MZ2_SUPERTANK_GRENADE_1]);

	// fire rockets more often at distance
	if (chaingun_good && (!rocket_good || range <= 540 || frandom() < 0.3f))
	{
		// prefer grenade if the enemy is above us
		if (grenade_good && (range >= 350 || vec.z > 120.f || frandom() < 0.2f))
			M_SetAnimation(self, &supershamblertank_move_attack4);
		else
		{
			M_SetAnimation(self, &supershamblertank_move_attack1);
			self->timestamp = level.time + random_time(1500_ms, 2700_ms);
		}
	}
	else if (rocket_good)
	{
		// prefer grenade if the enemy is above us
		if (grenade_good && (vec.z > 120.f || frandom() < 0.2f))
			M_SetAnimation(self, &supershamblertank_move_attack4);
		else
			M_SetAnimation(self, &supershamblertank_move_attack2);
	}
	else if (grenade_good)
		M_SetAnimation(self, &supershamblertank_move_attack4);
}

//
// death
//

static void supershamblertank_gib(edict_t *self)
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1_BIG);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PHS, false);

	self->s.sound = 0;
	self->s.skinnum /= 2;

	ThrowGibs(self, 500, {
		{ 2, "models/objects/gibs/sm_meat/tris.md2" },
		{ 2, "models/objects/gibs/sm_metal/tris.md2", GIB_METALLIC },
		{ "models/monsters/boss1/gibs/cgun.md2", GIB_SKINNED | GIB_METALLIC },
		{ "models/monsters/boss1/gibs/chest.md2", GIB_SKINNED },
		{ "models/monsters/boss1/gibs/core.md2", GIB_SKINNED },
		{ "models/monsters/boss1/gibs/ltread.md2", GIB_SKINNED | GIB_UPRIGHT },
		{ "models/monsters/boss1/gibs/rgun.md2", GIB_SKINNED | GIB_UPRIGHT },
		{ "models/monsters/boss1/gibs/tube.md2", GIB_SKINNED | GIB_UPRIGHT },
		{ "models/monsters/shamblertank/gibs/g_arm.md2", GIB_SKINNED | GIB_UPRIGHT },
		{ "models/monsters/shamblertank/gibs/g_leg.md2", GIB_SKINNED | GIB_UPRIGHT },
		{ "models/monsters/shamblertank/gibs/g_head.md2", GIB_SKINNED | GIB_METALLIC | GIB_HEAD }
	});
}

void supershamblertank_dead(edict_t *self)
{
	// no blowy on deady
	if (self->spawnflags.has(SPAWNFLAG_MONSTER_DEAD))
	{
		self->deadflag = false;
		self->takedamage = true;
		return;
	}

	supershamblertank_gib(self);
}

DIE(supershamblertank_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	if (self->spawnflags.has(SPAWNFLAG_MONSTER_DEAD))
	{
		// check for gib
		if (M_CheckGib(self, mod))
		{
			supershamblertank_gib(self);
			self->deadflag = true;
			return;
		}

		if (self->deadflag)
			return;
	}
	else
	{
		gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
		self->deadflag = true;
		self->takedamage = false;
	}

	M_SetAnimation(self, &supershamblertank_move_death);
}

//===========
// PGM
MONSTERINFO_BLOCKED(supershamblertank_blocked) (edict_t *self, float dist) -> bool
{
	if (blocked_checkplat(self, dist))
		return true;

	return false;
}
// PGM
//===========

//
// monster_supershamblertank
//

// RAFAEL (Powershield)

/*QUAKED monster_supershamblertank (1 .5 0) (-64 -64 0) (64 64 72) Ambush Trigger_Spawn Sight Powershield LongDeath
 */
void SP_monster_supershamblertank(edict_t *self)
{
	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict( self );
		return;
	}

	sound_pain1.assign("bosstank/btkpain1.wav");
	sound_pain2.assign("bosstank/btkpain2.wav");
	sound_pain3.assign("bosstank/btkpain3.wav");
	sound_death.assign("bosstank/btkdeth1.wav");
	sound_search1.assign("bosstank/btkunqv1.wav");
	sound_search2.assign("bosstank/btkunqv2.wav");

	tread_sound_2.assign("bosstank/btkengn1.wav");

	gi.soundindex("gunner/gunatck3.wav");
	gi.soundindex("infantry/infatck1.wav");
	gi.soundindex("tank/rocket.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/shamblertank/tris.md2");
	
	gi.modelindex("models/monsters/boss1/gibs/cgun.md2");
	gi.modelindex("models/monsters/boss1/gibs/chest.md2");
	gi.modelindex("models/monsters/boss1/gibs/core.md2");
	gi.modelindex("models/monsters/boss1/gibs/head.md2");
	gi.modelindex("models/monsters/boss1/gibs/ltread.md2");
	gi.modelindex("models/monsters/boss1/gibs/rgun.md2");
	gi.modelindex("models/monsters/boss1/gibs/rtread.md2");
	gi.modelindex("models/monsters/boss1/gibs/tube.md2");
	gi.modelindex("models/monsters/shamblertank/gibs/g_arm.md2");
	gi.modelindex("models/monsters/shamblertank/gibs/g_leg.md2");
	gi.modelindex("models/monsters/shamblertank/gibs/g_head.md2");

	self->mins = { -64, -64, 0 };
	self->maxs = { 64, 64, 112 };

	self->health = 3000 * st.health_multiplier;
	self->gib_health = -500;
	self->mass = 800;

	self->pain = supershamblertank_pain;
	self->die = supershamblertank_die;
	self->monsterinfo.stand = supershamblertank_stand;
	self->monsterinfo.walk = supershamblertank_walk;
	self->monsterinfo.run = supershamblertank_run;
	self->monsterinfo.dodge = nullptr;
	self->monsterinfo.attack = supershamblertank_attack;
	self->monsterinfo.search = supershamblertank_search;
	self->monsterinfo.melee = supershamblertank_melee;
	self->monsterinfo.sight = nullptr;
	self->monsterinfo.blocked = supershamblertank_blocked; // PGM
	self->monsterinfo.setskin = supershamblertank_setskin;

	gi.linkentity(self);

	M_SetAnimation(self, &supershamblertank_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	// RAFAEL
	if (self->spawnflags.has(SPAWNFLAG_supershamblertank_POWERSHIELD))
	{
		if (!st.was_key_specified("power_armor_type"))
			self->monsterinfo.power_armor_type = IT_ITEM_POWER_SHIELD;
		if (!st.was_key_specified("power_armor_power"))
			self->monsterinfo.power_armor_power = 400;
	}
	// RAFAEL

	walkmonster_start(self);

	// PMM
	self->monsterinfo.aiflags |= AI_IGNORE_SHOTS;
	// pmm

	// TODO
	if (level.is_n64)
	{
		self->spawnflags |= SPAWNFLAG_supershamblertank_LONG_DEATH;
		self->count = 10;
	}
}
