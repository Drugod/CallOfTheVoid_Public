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
// m_zombie.c

#include "g_local.h"
#include "m_zombie_strogg.h"

static cached_soundindex sound_sight;
static cached_soundindex sound_search;
static cached_soundindex sound_fling;
static cached_soundindex sound_pain1;
static cached_soundindex sound_pain2;
static cached_soundindex sound_fall;
static cached_soundindex sound_miss;
static cached_soundindex sound_hit;
static cached_soundindex sound_gib;

void zombie_strogg_down(edict_t *self);
void zombie_strogg_get_up_attempt(edict_t *self);
vec3_t SightEndtToDir(edict_t* self, vec3_t orig_dir);

// Stand
mframe_t zombie_strogg_frames_stand [] =
{
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
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
MMOVE_T(zombie_strogg_move_stand) = {0, 14, zombie_strogg_frames_stand, NULL};

MONSTERINFO_STAND(zombie_strogg_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &zombie_strogg_move_stand);	
}

void zombie_strogg_reset_state(edict_t *self)
{
	self->zombie_state = 0;
}

// Run
mframe_t zombie_strogg_frames_run [] =
{
	{ai_run, 1, zombie_strogg_reset_state},
	{ai_run, 1, NULL},
	{ai_run, 0, NULL},
	{ai_run, 1, NULL},
	{ai_run, 2, NULL},
	{ai_run, 3, NULL},
	{ai_run, 4, NULL},
	{ai_run, 4, NULL},
	{ai_run, 2, NULL},
	{ai_run, 0, NULL},
	{ai_run, 0, NULL},
	{ai_run, 0, NULL},
	{ai_run, 2, NULL},
	{ai_run, 4, NULL},
	{ai_run, 6, NULL},
	{ai_run, 7, NULL},
	{ai_run, 3, NULL},
	{ai_run, 8, NULL}
};
MMOVE_T(zombie_strogg_move_run) = {34, 51, zombie_strogg_frames_run, NULL};

MONSTERINFO_RUN(zombie_strogg_run) (edict_t *self) -> void
{
	M_SetAnimation(self, &zombie_strogg_move_run);	
}

// walk
mframe_t zombie_strogg_frames_walk [] =
{
	{ai_walk, 0, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 1, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 1, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL}
};
MMOVE_T(zombie_strogg_move_walk) = {15, 33, zombie_strogg_frames_walk, NULL};

MONSTERINFO_WALK(zombie_strogg_walk) (edict_t *self) -> void
{
	M_SetAnimation(self, &zombie_strogg_move_walk);	
}

// Sight
MONSTERINFO_SIGHT(zombie_strogg_sight) (edict_t* self, edict_t* other) -> void
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

/*
=================
fire_zombie_strogg_grenade
=================
*/
THINK(Grenade_zombie_strogg_Explode) (edict_t* ent) -> void
{
	vec3_t origin;
	mod_t  mod;

	if (ent->owner->client)
		PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

	// FIXME: if we are onground then raise our Z just a bit since we are a point?
	if (ent->enemy) {
		float  points;
		vec3_t v;
		vec3_t dir;

		v = ent->enemy->mins + ent->enemy->maxs;
		v = ent->enemy->s.origin + (v * 0.5f);
		v = ent->s.origin - v;
		points = ent->dmg - 0.5f * v.length();
		dir = ent->enemy->s.origin - ent->s.origin;
		mod = MOD_GRENADE;
		T_Damage(ent->enemy, ent, ent->owner, dir, ent->s.origin, vec3_origin, (int)points, (int)points, DAMAGE_RADIUS, mod);
	}

	mod = MOD_G_SPLASH;
	T_RadiusDamage(ent, ent->owner, (float)ent->dmg, ent->enemy, ent->dmg_radius, DAMAGE_NONE, mod);

	origin = ent->s.origin + (ent->velocity * -0.02f);
	gi.WriteByte(svc_temp_entity);
	if (ent->waterlevel) {
		if (ent->groundentity)
			gi.WriteByte(TE_GRENADE_EXPLOSION_WATER);
		else
			gi.WriteByte(TE_ROCKET_EXPLOSION_WATER);
	}
	else {
		if (ent->groundentity)
			gi.WriteByte(TE_GRENADE_EXPLOSION);
		else
			gi.WriteByte(TE_ROCKET_EXPLOSION);
	}
	gi.WritePosition(origin);
	gi.multicast(ent->s.origin, MULTICAST_PHS, false);

	G_FreeEdict(ent);
}

TOUCH(Grenade_zombie_strogg_Touch) (edict_t* ent, edict_t* other, const trace_t& tr, bool other_touching_self) -> void
{
	if (other == ent->owner)
		return;

	if (tr.surface && (tr.surface->flags & SURF_SKY)) {
		G_FreeEdict(ent);
		return;
	}

	if (!other->takedamage) {
		gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/grenlb1b.wav"), 1, ATTN_NORM, 0);
		return;
	}
	ent->enemy = other;
	Grenade_Explode(ent);
}

void fire_zombie_strogg_grenade(edict_t* self, const vec3_t& start, const vec3_t& aimdir, int damage, int speed, gtime_t timer, float damage_radius, float right_adjust, float up_adjust, bool monster)
{
	edict_t* grenade;
	vec3_t	 dir;
	vec3_t	 forward, right, up;

	dir = vectoangles(aimdir);
	AngleVectors(dir, forward, right, up);

	grenade = G_Spawn();
	grenade->s.origin = start;
	grenade->velocity = aimdir * speed;

	if (up_adjust) {
		float gravityAdjustment = level.gravity / 800.f;
		grenade->velocity += up * up_adjust * gravityAdjustment;
	}

	if (right_adjust)
		grenade->velocity += right * right_adjust;

	grenade->movetype = MOVETYPE_BOUNCE;
	grenade->clipmask = MASK_PROJECTILE;
	// [Paril-KEX]
	if (self->client && !G_ShouldPlayersCollide(true))
		grenade->clipmask &= ~CONTENTS_PLAYER;
	grenade->solid = SOLID_BBOX;
	grenade->svflags |= SVF_PROJECTILE;
	grenade->flags |= (FL_DODGE | FL_TRAP);
	grenade->s.effects |= EF_GRENADE;
	grenade->speed = speed;
	grenade->avelocity = { crandom() * 360, crandom() * 360, crandom() * 360 };
	grenade->s.modelindex = gi.modelindex("models/objects/grenade_zombie/tris.md2");
	grenade->nextthink = level.time + timer;
	grenade->think = Grenade_zombie_strogg_Explode;
	grenade->s.effects |= EF_GRENADE_LIGHT;
	grenade->owner = self;
	grenade->touch = Grenade_zombie_strogg_Touch;
	grenade->dmg = damage;
	grenade->dmg_radius = damage_radius;
	grenade->classname = "grenade_zombie";
	gi.linkentity(grenade);
}

// Grenade
void zombie_strogg_fire_grenade(edict_t* self, const vec3_t& start, const vec3_t& aimdir, int damage, int speed,
	monster_muzzleflash_id_t flashtype, float right_adjust, float up_adjust)
{
	fire_zombie_strogg_grenade(self, start, aimdir, damage, speed, 2.5_sec, damage + 40.f, right_adjust, up_adjust, true);
	monster_muzzleflash(self, start, flashtype);
}
void ZombieGrenade(edict_t* self)
{
	vec3_t					 start;
	vec3_t					 forward, right, up;
	vec3_t					 aim;
	monster_muzzleflash_id_t flash_number;
	float					 spread;
	float					 pitch = 0;
	// PMM
	vec3_t target;
	bool   blindfire = false;

	if (!self->enemy || !self->enemy->inuse) // PGM
		return;								 // PGM

	// pmm
	if (self->monsterinfo.aiflags & AI_MANUAL_STEERING)
		blindfire = true;

	self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
	spread = 0.10f;
	//flash_number = MZ2_GUNNER_GRENADE_4;
	flash_number = MZ2_UNUSED_0;

	if ((blindfire) && (!visible(self, self->enemy))){
		// and we have a valid blind_fire_target
		if (!self->monsterinfo.blind_fire_target)
			return;

		target = self->monsterinfo.blind_fire_target;
	}else
		target = self->enemy->s.origin;
	// pmm

	AngleVectors(self->s.angles, forward, right, up); // PGM
	start = M_ProjectFlashSource(self, { 16, 0, 8 }, forward, right);

	// PGM
	if (self->enemy){
		float dist;
		aim = target - self->s.origin;
		dist = aim.length();

		// aim up if they're on the same level as me and far away.
		if ((dist > 512) && (aim[2] < 64) && (aim[2] > -64)){
			aim[2] += (dist - 512);
		}

		aim.normalize();
		pitch = aim[2];
		if (pitch > 0.4f)
			pitch = 0.4f;
		else if (pitch < -0.5f)
			pitch = -0.5f;
	}
	float right_adjustment = -0.1f; // This value determines how much you want to adjust to the right
	aim = forward + (right * (spread + right_adjustment)); // Added right_adjustment to the right vector
	//aim = forward + (right * spread);
	aim += (up * pitch);

	// try search for best pitch
	if (M_CalculatePitchToFire(self, target, start, aim, 600, 2.5f, false))
		zombie_strogg_fire_grenade(self, start, aim, 25, 600, flash_number, (crandom_open() * 10.0f), frandom() * 10.f);
	else
		// normal shot
		zombie_strogg_fire_grenade(self, start, aim, 50, 600, flash_number, (crandom_open() * 10.0f), 200.f + (crandom_open() * 10.0f));
}

// Attack (1)
mframe_t zombie_strogg_frames_attack1 [] =
{
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, ZombieGrenade}
};
MMOVE_T (zombie_strogg_move_attack1) = {52, 64, zombie_strogg_frames_attack1, zombie_strogg_run};

// Attack (2)
mframe_t zombie_strogg_frames_attack2 [] =
{
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, ZombieGrenade}
};
MMOVE_T (zombie_strogg_move_attack2) = {65, 78, zombie_strogg_frames_attack2, zombie_strogg_run};

// Attack (3)
mframe_t zombie_strogg_frames_attack3 [] =
{
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, ZombieGrenade}
};
MMOVE_T (zombie_strogg_move_attack3) = {79, 90, zombie_strogg_frames_attack3, zombie_strogg_run};

// Attack
MONSTERINFO_ATTACK (zombie_strogg_attack)(edict_t *self) ->void
{
	float r = frandom();
	if (r < 0.3)
		M_SetAnimation(self, &zombie_strogg_move_attack1);	
	else if (r < 0.6)
		M_SetAnimation(self, &zombie_strogg_move_attack2);			
	else
		M_SetAnimation(self, &zombie_strogg_move_attack3);	
}

mframe_t zombie_strogg_frames_get_up [] =
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
	{ai_move, 0, NULL}
};
MMOVE_T (zombie_strogg_move_get_up) = {173, 191, zombie_strogg_frames_get_up, zombie_strogg_run};

void zombie_strogg_pain1(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
}

void zombie_strogg_pain2(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
}

void zombie_strogg_hit_floor(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_fall, 1, ATTN_NORM, 0);
}

void zombie_strogg_get_up(edict_t *self)
{
	self->maxs = { 16,16,40 };
	self->takedamage = true;
	self->health = 60;
	zombie_strogg_sight(self, self->enemy);

	if (!M_walkmove(self, 0, 0))
	{
		zombie_strogg_get_up_attempt(self);
		return;
	}
	M_SetAnimation(self, &zombie_strogg_move_get_up);
}

void zombie_strogg_start_fall(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
}

// Down
mframe_t zombie_strogg_frames_get_up_attempt [] =
{
	{ai_move, 0,		zombie_strogg_get_up_attempt}
};
MMOVE_T (zombie_strogg_move_get_up_attempt) = {173, 173, zombie_strogg_frames_get_up_attempt, NULL};

void zombie_strogg_get_up_attempt(edict_t *self)
{
	static int down = 0;
	zombie_strogg_down(self);

	// Try getting up in 5 seconds
	if (down >= 250){
		down = 0;
		zombie_strogg_get_up(self);
		return;
	}
	self->s.frame = 172;
	M_SetAnimation(self, &zombie_strogg_move_get_up_attempt);	
	down++;
}

void zombie_strogg_down(edict_t *self)
{
	self->takedamage = false;
	self->health = 60;
	self->maxs = { 16, 16, 0 };
}

// Pain (1)
mframe_t zombie_strogg_frames_pain1 [] =
{
	{ai_move, 0, zombie_strogg_pain1},
	{ai_move, 3, NULL},
	{ai_move, 1, NULL},
	{ai_move, 1, NULL},
	{ai_move, 3, NULL},
	{ai_move, 1, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
MMOVE_T (zombie_strogg_move_pain1) = {91, 102, zombie_strogg_frames_pain1, zombie_strogg_run};

// Pain (2)
mframe_t zombie_strogg_frames_pain2 [] =
{
	{ai_move, 0, zombie_strogg_pain2},
	{ai_move, 2, NULL},
	{ai_move, 8, NULL},
	{ai_move, 6, NULL},
	{ai_move, 2, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, zombie_strogg_hit_floor},
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
	{ai_move, 1, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
MMOVE_T (zombie_strogg_move_pain2) = {103, 130, zombie_strogg_frames_pain2, zombie_strogg_run};

// Pain (3)
mframe_t zombie_strogg_frames_pain3 [] =
{
	{ai_move, 0, zombie_strogg_pain2},
	{ai_move, 0, NULL},
	{ai_move, 3, NULL},
	{ai_move, 1, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 1, NULL},
	{ai_move, 1, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
MMOVE_T (zombie_strogg_move_pain3) = {131, 148, zombie_strogg_frames_pain3, zombie_strogg_run};

// Pain (4)
mframe_t zombie_strogg_frames_pain4 [] =
{
	{ai_move, 0, zombie_strogg_pain1},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 1, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
MMOVE_T (zombie_strogg_move_pain4) = {149, 161, zombie_strogg_frames_pain4, zombie_strogg_run};

// Pain (5)
mframe_t zombie_strogg_frames_fall_start [] =
{
	{ai_move, 0,	zombie_strogg_start_fall},
	{ai_move, -8,	NULL},
	{ai_move, -5,	NULL},
	{ai_move, -3,	NULL},
	{ai_move, -1,	NULL},
	{ai_move, -2,	NULL},
	{ai_move, -1,	NULL},
	{ai_move, -1,	NULL},
	{ai_move, -2,	NULL},
	{ai_move, 0,	zombie_strogg_hit_floor},
	{ai_move, 0,	zombie_strogg_down}
};
MMOVE_T (zombie_strogg_move_fall_start) = {162, 172, zombie_strogg_frames_fall_start, zombie_strogg_get_up_attempt};

// Pain
PAIN (zombie_strogg_pain) (edict_t* self, edict_t* other, float kick, int damage, const mod_t& mod) -> void
{
	float r;
	self->health = 60;

	if (damage < 9)
		return;
	if (self->zombie_state == 2)
		return;
	if (damage >= 25)
	{
		self->zombie_state = 2;
		M_SetAnimation(self, &zombie_strogg_move_fall_start);
		return;
	}
	if (self->pain_debounce_time > level.time)
	{
		self->zombie_state = 2;
		M_SetAnimation(self, &zombie_strogg_move_fall_start);		
		return;
	}
	if (self->zombie_state)
	{
		self->pain_debounce_time = level.time + 3_sec;
		return;
	}
	self->zombie_state = 1;

	// decino: No pain animations in Nightmare mode
	if (skill->value >= 3)
		return;
	r = frandom();

	if (r < 0.25)
		M_SetAnimation(self, &zombie_strogg_move_pain1);	
	else if (r <  0.5)
		M_SetAnimation(self, &zombie_strogg_move_pain2);	
	else if (r <  0.75)
		M_SetAnimation(self, &zombie_strogg_move_pain3);	
	else
		M_SetAnimation(self, &zombie_strogg_move_pain4);	
}

// Death
DIE (zombie_strogg_die) (edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t& mod) -> void
{
	if (self->deadflag == true)
		return;

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_ROCKET_EXPLOSION);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS, false);
	T_RadiusClassDamage(self, attacker, 500.0f, (char*)"monster_zombie_strogg", 100.0f, MOD_EXPLOSIVE);
	
	gi.sound(self, CHAN_VOICE, sound_gib, 1, ATTN_NORM, 0);

	ThrowGibs(self, damage, {
		{ "models/objects/gibs/bone/tris.md2" },
		{ "models/objects/gibs/sm_meat/tris.md2" },
		{ "models/monsters/zombiestrogg/gibs/g_arm.md2" },
		{ "models/monsters/zombiestrogg/gibs/g_arm.md2" },
		{ "models/monsters/zombiestrogg/gibs/g_leg.md2" },
		{ "models/monsters/zombiestrogg/gibs/g_leg.md2" },
		{ "models/monsters/zombiestrogg/gibs/g_head.md2", GIB_HEAD }
		});
	self->deadflag = true;

	if (frandom() < 0.5) {
		gitem_t* item2 = FindItemByClassname("ammo_mini_grenades");
		if (item2 != NULL) {
			Drop_Item(self, item2);
		}
	}
}

// Search
MONSTERINFO_SEARCH (zombie_strogg_search) (edict_t *self) -> void
{
	if (frandom() < 0.2)
		gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

void SP_monster_zombie_strogg(edict_t *self)
{
	self->s.modelindex = gi.modelindex("models/monsters/zombiestrogg/tris.md2");
	gi.modelindex("models/monsters/zombiestrogg/gibs/g_arm.md2");
	gi.modelindex("models/monsters/zombiestrogg/gibs/g_leg.md2");
	gi.modelindex("models/monsters/zombiestrogg/gibs/g_head.md2");

	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, 40 };
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;	
	
	self->health = 60 * st.health_multiplier;

	sound_sight.assign("zombie/z_idle_s.wav");
	sound_search.assign("zombie/idle_w2_s.wav");
	sound_fling.assign("zombie/z_shot1.wav");
	sound_pain1.assign("zombie/z_pain_s.wav");
	sound_pain2.assign("zombie/z_pain1_s.wav");
	sound_fall.assign("zombie/z_fall.wav");
	sound_miss.assign("zombie/z_miss.wav");
	sound_hit.assign("zombie/z_hit.wav");
	sound_gib.assign("zombie/z_gib_s.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->gib_health = -5;
	self->mass = 60;
	self->flags |= FL_DEEPONE;

	self->monsterinfo.stand = zombie_strogg_stand;
	self->monsterinfo.walk = zombie_strogg_walk;
	self->monsterinfo.run = zombie_strogg_run;
	self->monsterinfo.attack = zombie_strogg_attack;
	self->monsterinfo.sight = zombie_strogg_sight;
	self->monsterinfo.search = zombie_strogg_search;

	self->pain = zombie_strogg_pain;
	self->die = zombie_strogg_die;

	gi.linkentity(self);

	M_SetAnimation(self, &zombie_strogg_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);
}