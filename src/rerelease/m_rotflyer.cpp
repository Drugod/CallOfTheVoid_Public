// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

rotflyer

==============================================================================
*/

#include "g_local.h"
#include "m_rotflyer.h"
#include "m_flash.h"

static cached_soundindex sound_sight;
static cached_soundindex sound_idle;
static cached_soundindex sound_pain1;
static cached_soundindex sound_pain2;
static cached_soundindex sound_slash;
static cached_soundindex sound_sproing;
static cached_soundindex sound_die;

void rotflyer_check_melee(edict_t *self);
void rotflyer_loop_melee(edict_t *self);
void rotflyer_setstart(edict_t *self);

// ROGUE - kamikaze stuff
void rotflyer_kamikaze(edict_t *self);
void rotflyer_kamikaze_check(edict_t *self);
void rotflyer_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod);

MONSTERINFO_SIGHT(rotflyer_sight) (edict_t *self, edict_t *other) -> void
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

MONSTERINFO_IDLE(rotflyer_idle) (edict_t *self) -> void
{
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

void rotflyer_pop_blades(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_sproing, 1, ATTN_NORM, 0);
}

mframe_t rotflyer_frames_stand[] = {
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
MMOVE_T(rotflyer_move_stand) = { FRAME_stand01, FRAME_stand45, rotflyer_frames_stand, nullptr };

mframe_t rotflyer_frames_walk[] = {
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 }
};
MMOVE_T(rotflyer_move_walk) = { FRAME_stand01, FRAME_stand45, rotflyer_frames_walk, nullptr };

mframe_t rotflyer_frames_run[] = {
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 }
};
MMOVE_T(rotflyer_move_run) = { FRAME_stand01, FRAME_stand45, rotflyer_frames_run, nullptr };

mframe_t rotflyer_frames_kamizake[] = {
	{ ai_charge, 40, rotflyer_kamikaze_check },
	{ ai_charge, 40, rotflyer_kamikaze_check },
	{ ai_charge, 40, rotflyer_kamikaze_check },
	{ ai_charge, 40, rotflyer_kamikaze_check },
	{ ai_charge, 40, rotflyer_kamikaze_check }
};
MMOVE_T(rotflyer_move_kamikaze) = { FRAME_rollr02, FRAME_rollr06, rotflyer_frames_kamizake, rotflyer_kamikaze };

MONSTERINFO_RUN(rotflyer_run) (edict_t *self) -> void
{
	if (self->mass > 50)
		M_SetAnimation(self, &rotflyer_move_kamikaze);
	else if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		M_SetAnimation(self, &rotflyer_move_stand);
	else
		M_SetAnimation(self, &rotflyer_move_run);
}

MONSTERINFO_WALK(rotflyer_walk) (edict_t *self) -> void
{
	if (self->mass > 50)
		rotflyer_run(self);
	else
		M_SetAnimation(self, &rotflyer_move_walk);
}

MONSTERINFO_STAND(rotflyer_stand) (edict_t *self) -> void
{
	if (self->mass > 50)
		rotflyer_run(self);
	else
		M_SetAnimation(self, &rotflyer_move_stand);
}

// ROGUE - kamikaze stuff

void rotflyer_kamikaze_explode(edict_t *self)
{
	vec3_t dir;

	if (self->monsterinfo.commander && self->monsterinfo.commander->inuse &&
		!strcmp(self->monsterinfo.commander->classname, "monster_carrier"))
		self->monsterinfo.commander->monsterinfo.monster_slots++;

	if (self->enemy)
	{
		dir = self->enemy->s.origin - self->s.origin;
		T_Damage(self->enemy, self, self, dir, self->s.origin, vec3_origin, (int) 50, (int) 50, DAMAGE_RADIUS, MOD_UNKNOWN);
	}

	rotflyer_die(self, nullptr, nullptr, 0, dir, MOD_EXPLOSIVE);
}

void rotflyer_kamikaze(edict_t *self)
{
	M_SetAnimation(self, &rotflyer_move_kamikaze);
}

void rotflyer_kamikaze_check(edict_t *self)
{
	float dist;

	// PMM - this needed because we could have gone away before we get here (blocked code)
	if (!self->inuse)
		return;

	if ((!self->enemy) || (!self->enemy->inuse))
	{
		rotflyer_kamikaze_explode(self);
		return;
	}

	self->s.angles[0] = vectoangles(self->enemy->s.origin - self->s.origin).x;

	self->goalentity = self->enemy;

	dist = realrange(self, self->enemy);

	if (dist < 90)
		rotflyer_kamikaze_explode(self);
}

#if 0
mframe_t rotflyer_frames_rollright[] = {
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
MMOVE_T(rotflyer_move_rollright) = { FRAME_rollr01, FRAME_rollr09, rotflyer_frames_rollright, nullptr };

mframe_t rotflyer_frames_rollleft[] = {
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
MMOVE_T(rotflyer_move_rollleft) = { FRAME_rollf01, FRAME_rollf09, rotflyer_frames_rollleft, nullptr };
#endif

mframe_t rotflyer_frames_pain3[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(rotflyer_move_pain3) = { FRAME_pain301, FRAME_pain304, rotflyer_frames_pain3, rotflyer_run };

mframe_t rotflyer_frames_pain2[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(rotflyer_move_pain2) = { FRAME_pain201, FRAME_pain204, rotflyer_frames_pain2, rotflyer_run };

mframe_t rotflyer_frames_pain1[] = {
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
MMOVE_T(rotflyer_move_pain1) = { FRAME_pain101, FRAME_pain109, rotflyer_frames_pain1, rotflyer_run };

#if 0
mframe_t rotflyer_frames_defense[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move }, // Hold this frame
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(rotflyer_move_defense) = { FRAME_defens01, FRAME_defens06, rotflyer_frames_defense, nullptr };

mframe_t rotflyer_frames_bankright[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(rotflyer_move_bankright) = { FRAME_bankr01, FRAME_bankr07, rotflyer_frames_bankright, nullptr };

mframe_t rotflyer_frames_bankleft[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(rotflyer_move_bankleft) = { FRAME_bankl01, FRAME_bankl07, rotflyer_frames_bankleft, nullptr };
#endif


void rotflyer_fireleft(edict_t *self)
{
	if (!self->enemy || !self->enemy->inuse) return;

	vec3_t start, forward, right, aim, end;

	AngleVectors(self->s.angles, forward, right, nullptr);

	start = self->s.origin;
	start[0] += forward[0] * 12.72f;
	start[1] += forward[1] * 12.72f;
	start[2] += -11.0f;
	start[0] += right[0] * 12.0f;
	start[1] += right[1] * 12.0f;

	end = self->enemy->s.origin;
	end[2] += self->enemy->viewheight;
	aim = end - start;
	aim.normalize();

	monster_fire_blueblaster(self, start, aim, 1, 600, MZ2_FLYER_BLASTER_1, EF_BLUEHYPERBLASTER);
}

void rotflyer_fireright(edict_t *self)
{
	if (!self->enemy || !self->enemy->inuse) return;

	vec3_t start, forward, right, aim, end;

	AngleVectors(self->s.angles, forward, right, nullptr);

	start = self->s.origin;
	start[0] += forward[0] * 12.72f;
	start[1] += forward[1] * 12.72f;
	start[2] += -11.0f;
	start[0] += right[0] * -12.0f;
	start[1] += right[1] * -12.0f;

	end = self->enemy->s.origin;
	end[2] += self->enemy->viewheight;
	aim = end - start;
	aim.normalize();

	monster_fire_blueblaster(self, start, aim, 1, 600, MZ2_FLYER_BLASTER_2, EF_BLUEHYPERBLASTER);
}

mframe_t rotflyer_frames_attack2[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, -10, rotflyer_fireleft },	 // left gun
	{ ai_charge, -10, rotflyer_fireright }, // right gun
	{ ai_charge, -10, rotflyer_fireleft },	 // left gun
	{ ai_charge, -10, rotflyer_fireright }, // right gun
	{ ai_charge, -10, rotflyer_fireleft },	 // left gun
	{ ai_charge, -10, rotflyer_fireright }, // right gun
	{ ai_charge, -10, rotflyer_fireleft },	 // left gun
	{ ai_charge, -10, rotflyer_fireright }, // right gun
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }
};
MMOVE_T(rotflyer_move_attack2) = { FRAME_attak201, FRAME_attak217, rotflyer_frames_attack2, rotflyer_run };

// PMM
// circle strafe frames

mframe_t rotflyer_frames_attack3[] = {
	{ ai_charge, 10 },
	{ ai_charge, 10 },
	{ ai_charge, 10 },
	{ ai_charge, 10, rotflyer_fireleft },	// left gun
	{ ai_charge, 10, rotflyer_fireright }, // right gun
	{ ai_charge, 10, rotflyer_fireleft },	// left gun
	{ ai_charge, 10, rotflyer_fireright }, // right gun
	{ ai_charge, 10, rotflyer_fireleft },	// left gun
	{ ai_charge, 10, rotflyer_fireright }, // right gun
	{ ai_charge, 10, rotflyer_fireleft },	// left gun
	{ ai_charge, 10, rotflyer_fireright }, // right gun
	{ ai_charge, 10 },
	{ ai_charge, 10 },
	{ ai_charge, 10 },
	{ ai_charge, 10 },
	{ ai_charge, 10 },
	{ ai_charge, 10 }
};
MMOVE_T(rotflyer_move_attack3) = { FRAME_attak201, FRAME_attak217, rotflyer_frames_attack3, rotflyer_run };
// pmm

void rotflyer_slash_left(edict_t *self)
{
	vec3_t aim = { MELEE_DISTANCE, self->mins[0], 0 };
	if (!fire_hit(self, aim, 5, 0))
		self->monsterinfo.melee_debounce_time = level.time + 1.5_sec;
	gi.sound(self, CHAN_WEAPON, sound_slash, 1, ATTN_NORM, 0);
}

void rotflyer_slash_right(edict_t *self)
{
	vec3_t aim = { MELEE_DISTANCE, self->maxs[0], 0 };
	if (!fire_hit(self, aim, 5, 0))
		self->monsterinfo.melee_debounce_time = level.time + 1.5_sec;
	gi.sound(self, CHAN_WEAPON, sound_slash, 1, ATTN_NORM, 0);
}

mframe_t rotflyer_frames_start_melee[] = {
	{ ai_charge, 0, rotflyer_pop_blades },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }
};
MMOVE_T(rotflyer_move_start_melee) = { FRAME_attak101, FRAME_attak106, rotflyer_frames_start_melee, rotflyer_loop_melee };

mframe_t rotflyer_frames_end_melee[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }
};
MMOVE_T(rotflyer_move_end_melee) = { FRAME_attak119, FRAME_attak121, rotflyer_frames_end_melee, rotflyer_run };

mframe_t rotflyer_frames_loop_melee[] = {
	{ ai_charge }, // Loop Start
	{ ai_charge },
	{ ai_charge, 0, rotflyer_slash_left }, // Left Wing Strike
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, rotflyer_slash_right }, // Right Wing Strike
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge } // Loop Ends

};
MMOVE_T(rotflyer_move_loop_melee) = { FRAME_attak107, FRAME_attak118, rotflyer_frames_loop_melee, rotflyer_check_melee };

void rotflyer_loop_melee(edict_t *self)
{
	M_SetAnimation(self, &rotflyer_move_loop_melee);
}

static void rotflyer_set_fly_parameters(edict_t *self, bool melee)
{
	if (melee)
	{
		// engage thrusters for a slice
		self->monsterinfo.fly_pinned = false;
		self->monsterinfo.fly_thrusters = true;
		self->monsterinfo.fly_position_time = 0_sec;
		self->monsterinfo.fly_acceleration = 20.f;
		self->monsterinfo.fly_speed = 210.f;
		self->monsterinfo.fly_min_distance = 0.f;
		self->monsterinfo.fly_max_distance = 10.f;
	}
	else
	{
		self->monsterinfo.fly_thrusters = false;
		self->monsterinfo.fly_acceleration = 15.f;
		self->monsterinfo.fly_speed = 165.f;
		self->monsterinfo.fly_min_distance = 45.f;
		self->monsterinfo.fly_max_distance = 200.f;
	}
}

MONSTERINFO_ATTACK(rotflyer_attack) (edict_t *self) -> void
{
	if (self->mass > 50)
	{
		rotflyer_run(self);
		return;
	}

	float range = range_to(self, self->enemy);

	if (self->enemy && visible(self, self->enemy) && range <= 225.f && frandom() > (range / 225.f) * 0.35f)
	{
		// fly-by slicing!
		self->monsterinfo.attack_state = AS_STRAIGHT;
		M_SetAnimation(self, &rotflyer_move_start_melee);
		rotflyer_set_fly_parameters(self, true);
	}
	else
	{
		self->monsterinfo.attack_state = AS_STRAIGHT;
		M_SetAnimation(self, &rotflyer_move_attack2);
	}

	// [Paril-KEX] for alternate fly mode, sometimes we'll pin us
	// down, kind of like a pseudo-stand ground
	if (!self->monsterinfo.fly_pinned && brandom() && self->enemy && visible(self, self->enemy))
	{
		self->monsterinfo.fly_pinned = true;
		self->monsterinfo.fly_position_time = max(self->monsterinfo.fly_position_time, self->monsterinfo.fly_position_time + 1.7_sec); // make sure there's enough time for attack2/3

		if (brandom())
			self->monsterinfo.fly_ideal_position = self->s.origin + (self->velocity * frandom()); // pin to our current position
		else
			self->monsterinfo.fly_ideal_position += self->enemy->s.origin; // make un-relative
	}

	// if we're currently pinned, fly_position_time will unpin us eventually
}

MONSTERINFO_MELEE(rotflyer_melee) (edict_t *self) -> void
{
	if (self->mass > 50)
		rotflyer_run(self);
	else
	{
		M_SetAnimation(self, &rotflyer_move_start_melee);
		rotflyer_set_fly_parameters(self, true);
	}
}

void rotflyer_check_melee(edict_t *self)
{
	if (range_to(self, self->enemy) <= RANGE_MELEE)
	{
		if (self->monsterinfo.melee_debounce_time <= level.time)
		{
			M_SetAnimation(self, &rotflyer_move_loop_melee);
			return;
		}
	}

	M_SetAnimation(self, &rotflyer_move_end_melee);
	rotflyer_set_fly_parameters(self, false);
}

PAIN(rotflyer_pain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
{
	int n;

	//	pmm	 - kamikaze's don't feel pain
	if (self->mass != 50)
		return;
	// pmm

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3_sec;

	n = irandom(3);
	if (n == 0)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else if (n == 1)
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);

	if (!M_ShouldReactToPain(self, mod))
		return; // no pain anims in nightmare

	rotflyer_set_fly_parameters(self, false);

	if (n == 0)
		M_SetAnimation(self, &rotflyer_move_pain1);
	else if (n == 1)
		M_SetAnimation(self, &rotflyer_move_pain2);
	else
		M_SetAnimation(self, &rotflyer_move_pain3);
}

MONSTERINFO_SETSKIN(rotflyer_setskin) (edict_t *self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}

DIE(rotflyer_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PHS, false);

	self->s.skinnum /= 2;

	ThrowGibs(self, 55, {
		{ 2, "models/objects/gibs/sm_metal/tris.md2" },
		{ 2, "models/objects/gibs/sm_meat/tris.md2" },
		{ "models/monsters/rotflyer/gibs/base.md2", GIB_SKINNED },
		{ 2, "models/monsters/rotflyer/gibs/gun.md2", GIB_SKINNED },
		{ 2, "models/monsters/rotflyer/gibs/wing.md2", GIB_SKINNED },
		{ "models/monsters/rotflyer/gibs/head.md2", GIB_SKINNED | GIB_HEAD }
	});
	
	self->touch = nullptr;
}

// PMM - kamikaze code .. blow up if blocked
MONSTERINFO_BLOCKED(rotflyer_blocked) (edict_t *self, float dist) -> bool
{
	// kamikaze = 100, normal = 50
	if (self->mass == 100)
	{
		rotflyer_kamikaze_check(self);

		// if the above didn't blow us up (i.e. I got blocked by the player)
		if (self->inuse)
			T_Damage(self, self, self, vec3_origin, self->s.origin, vec3_origin, 9999, 100, DAMAGE_NONE, MOD_UNKNOWN);

		return true;
	}

	return false;
}

TOUCH(rotflyer_kamikaze_touch) (edict_t *ent, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	T_Damage(ent, ent, ent, ent->velocity.normalized(), ent->s.origin, ent->velocity.normalized(), 9999, 100, DAMAGE_NONE, MOD_UNKNOWN);
}

TOUCH(rotflyer_touch) (edict_t *ent, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	if ((other->monsterinfo.aiflags & AI_ALTERNATE_FLY) && (other->flags & FL_FLY) &&
		(ent->monsterinfo.duck_wait_time < level.time))
	{
		ent->monsterinfo.duck_wait_time = level.time + 1_sec;
		ent->monsterinfo.fly_thrusters = false;

		vec3_t dir = (ent->s.origin - other->s.origin).normalized();
		ent->velocity = dir * 500.f;

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_SPLASH);
		gi.WriteByte(32);
		gi.WritePosition(tr.endpos);
		gi.WriteDir(dir);
		gi.WriteByte(SPLASH_SPARKS);
		gi.multicast(tr.endpos, MULTICAST_PVS, false);
	}
}

/*QUAKED monster_rotflyer (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
 */
void SP_monster_rotflyer(edict_t *self)
{
	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict( self );
		return;
	}

	sound_sight.assign("flyer/flysght1.wav");
	sound_idle.assign("flyer/flysrch1.wav");
	sound_pain1.assign("flyer/flypain1.wav");
	sound_pain2.assign("flyer/flypain2.wav");
	sound_slash.assign("flyer/flyatck2.wav");
	sound_sproing.assign("flyer/flyatck1.wav");
	sound_die.assign("flyer/flydeth1.wav");

	gi.soundindex("flyer/flyatck3.wav");

	self->s.modelindex = gi.modelindex("models/monsters/rotflyer/tris.md2");
	
	gi.modelindex("models/monsters/rotflyer/gibs/base.md2");
	gi.modelindex("models/monsters/rotflyer/gibs/wing.md2");
	gi.modelindex("models/monsters/rotflyer/gibs/gun.md2");
	gi.modelindex("models/monsters/rotflyer/gibs/head.md2");

	self->mins = { -16, -16, -24 };
	// PMM - shortened to 16 from 32
	self->maxs = { 16, 16, 16 };
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->viewheight = 12;

	self->monsterinfo.engine_sound = gi.soundindex("flyer/flyidle1.wav");

	self->health = 50 * st.health_multiplier;
	self->mass = 50;

	self->pain = rotflyer_pain;
	self->die = rotflyer_die;

	self->monsterinfo.stand = rotflyer_stand;
	self->monsterinfo.walk = rotflyer_walk;
	self->monsterinfo.run = rotflyer_run;
	self->monsterinfo.attack = rotflyer_attack;
	self->monsterinfo.melee = rotflyer_melee;
	self->monsterinfo.sight = rotflyer_sight;
	self->monsterinfo.idle = rotflyer_idle;
	self->monsterinfo.blocked = rotflyer_blocked;
	self->monsterinfo.setskin = rotflyer_setskin;

	gi.linkentity(self);

	M_SetAnimation(self, &rotflyer_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	if (self->s.effects & EF_ROCKET)
	{
		// PMM - normal rotflyer has mass of 50
		self->mass = 100;
		self->yaw_speed = 5;
		self->touch = rotflyer_kamikaze_touch;
	}
	else
	{
		self->monsterinfo.aiflags |= AI_ALTERNATE_FLY;
		self->monsterinfo.fly_buzzard = true;
		rotflyer_set_fly_parameters(self, false);
		self->touch = rotflyer_touch;
	}

	flymonster_start(self);
}