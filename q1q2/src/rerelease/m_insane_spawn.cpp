// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

insane_spawn

==============================================================================
*/
#include "g_local.h"
#include "m_insane_spawn.h"

constexpr spawnflags_t SPAWNFLAG_insane_spawn_CRAWL = 4_spawnflag;
constexpr spawnflags_t SPAWNFLAG_insane_spawn_STAND_GROUND = 16_spawnflag;
constexpr spawnflags_t SPAWNFLAG_insane_spawn_ALWAYS_STAND = 32_spawnflag;
constexpr spawnflags_t SPAWNFLAG_BERSERK_NOJUMPING = 8_spawnflag;

static cached_soundindex sound_fist;
static cached_soundindex sound_shake;
static cached_soundindex sound_moan;
static cached_soundindex sound_scream[8];
static cached_soundindex sound_death;
static cached_soundindex sound_hit;
static cached_soundindex sound_jump;
static cached_soundindex sound_land;
static cached_soundindex sound_pain;
static cached_soundindex sound_sight;
static cached_soundindex sound_search;
static cached_soundindex sound_thud;
static cached_soundindex sound_explod;

void insane_spawn_fist(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_fist, 1, ATTN_IDLE, 0);
}

void insane_spawn_shake(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_shake, 1, ATTN_IDLE, 0);
}

extern const mmove_t insane_spawn_move_cross, insane_spawn_move_struggle_cross;

void insane_spawn_moan(edict_t *self)
{
	// Paril: don't moan every second
	if (self->monsterinfo.attack_finished < level.time)
	{
		gi.sound(self, CHAN_VOICE, sound_moan, 1, ATTN_IDLE, 0);
		self->monsterinfo.attack_finished = level.time + random_time(1_sec, 3_sec);
	}
}

void insane_spawn_scream(edict_t *self)
{
	// Paril: don't moan every second
	if (self->monsterinfo.attack_finished < level.time)
	{
		gi.sound(self, CHAN_VOICE, random_element(sound_scream), 1, ATTN_IDLE, 0);
		self->monsterinfo.attack_finished = level.time + random_time(1_sec, 3_sec);
	}
}

void insane_spawn_stand(edict_t *self);
void insane_spawn_walk(edict_t *self);
void insane_spawn_run(edict_t *self);
void insane_spawn_checkdown(edict_t *self);
void insane_spawn_checkup(edict_t *self);
void insane_spawn_onground(edict_t *self);

// Attack
void T_SlamRadiusDamageSpawn(vec3_t point, edict_t* inflictor, edict_t* attacker, float damage, float kick, edict_t* ignore, float radius, mod_t mod)
{
	float	 points;
	edict_t* ent = nullptr;
	vec3_t	 v;
	vec3_t	 dir;

	while ((ent = findradius(ent, inflictor->s.origin, radius * 2.f)) != nullptr)
	{
		if (ent == ignore)
			continue;
		if (!strcmp(ent->classname, "monster_insane_spawn"))
			continue;
		if (!ent->takedamage)
			continue;
		if (!CanDamage(ent, inflictor))
			continue;
		// don't hit players in mid air
		if (ent->client && !ent->groundentity)
			continue;

		v = closest_point_to_box(point, ent->s.origin + ent->mins, ent->s.origin + ent->maxs) - point;

		// calculate contribution amount
		float amount = min(1.f, 1.f - (v.length() / radius));

		// too far away
		if (amount <= 0.f)
			continue;

		amount *= amount;

		// damage & kick are exponentially scaled
		points = max(1.f, damage * amount);

		dir = (ent->s.origin - point).normalized();

		// keep the point at their feet so they always get knocked up
		point[2] = ent->absmin[2];
		T_Damage(ent, inflictor, attacker, dir, point, dir, (int)points, (int)(kick * amount),
			DAMAGE_RADIUS, mod);

		if (ent->client)
			ent->velocity.z = max(270.f, ent->velocity.z);
	}
}


static void insane_spawn_attack_slam(edict_t* self)
{
	gi.sound(self, CHAN_WEAPON, sound_thud, 1, ATTN_NORM, 0);
	gi.sound(self, CHAN_AUTO, sound_explod, 0.75f, ATTN_NORM, 0);
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_BERSERK_SLAM);
	vec3_t f, r, start;
	AngleVectors(self->s.angles, f, r, nullptr);
	start = M_ProjectFlashSource(self, { 20.f, -14.3f, -21.f }, f, r);
	trace_t tr = gi.traceline(self->s.origin, start, self, MASK_SOLID);
	gi.WritePosition(tr.endpos);
	gi.WriteDir({ 0.f, 0.f, 1.f });
	gi.multicast(tr.endpos, MULTICAST_PHS, false);
	self->gravity = 1.0f;
	self->velocity = {};
	self->flags |= FL_KILL_VELOCITY;

	T_SlamRadiusDamageSpawn(tr.endpos, self, self, 10, 300.f, self, 165, MOD_UNKNOWN);
}

TOUCH(insane_spawn_jump_touch) (edict_t* self, edict_t* other, const trace_t& tr, bool other_touching_self) -> void
{
	if (self->health <= 0)
	{
		self->touch = nullptr;
		return;
	}

	if (self->groundentity)
	{
		self->s.frame = 244;

		if (self->touch)
			insane_spawn_attack_slam(self);

		self->touch = nullptr;
	}
}

static void insane_spawn_high_gravity(edict_t* self)
{
	if (self->velocity[2] < 0)
		self->gravity = 2.25f * (800.f / level.gravity);
	else
		self->gravity = 5.25f * (800.f / level.gravity);
}

void insane_spawn_jump_takeoff(edict_t* self)
{
	vec3_t forward;

	if (!self->enemy)
		return;

	// immediately turn to where we need to go
	float length = (self->s.origin - self->enemy->s.origin).length();
	float fwd_speed = length * 1.95f;
	vec3_t dir;
	PredictAim(self, self->enemy, self->s.origin, fwd_speed, false, 0.f, &dir, nullptr);
	self->s.angles[1] = vectoyaw(dir);
	AngleVectors(self->s.angles, forward, nullptr, nullptr);
	self->s.origin[2] += 1;
	self->velocity = forward * fwd_speed;
	self->velocity[2] = 450;
	self->groundentity = nullptr;
	self->monsterinfo.aiflags |= AI_DUCKED;

	self->monsterinfo.attack_finished = level.time + 3_sec;

	self->touch = insane_spawn_jump_touch;
	insane_spawn_high_gravity(self);
}

void insane_spawn_check_landing(edict_t* self)
{
	insane_spawn_high_gravity(self);

	if (self->groundentity)
	{
		self->monsterinfo.attack_finished = 0_ms;
		self->monsterinfo.unduck(self);
		self->s.frame = 244;
		if (self->touch)
		{
			insane_spawn_attack_slam(self);
			self->touch = nullptr;
		}
		self->flags &= ~FL_KILL_VELOCITY;
		return;
	}

	if (level.time > self->monsterinfo.attack_finished)
		self->monsterinfo.nextframe = 240;
	else
		self->monsterinfo.nextframe = 242;
}

mframe_t insane_spawn_frames_jump[] =
{
	{ai_charge,	0,	NULL},
	{ai_charge,	0,	NULL},
	{ai_charge,	0,	NULL},
	{ai_charge,	0,	insane_spawn_jump_takeoff},
	{ai_move,	0,	insane_spawn_high_gravity},
	{ai_move,	0,	insane_spawn_check_landing},
	{ai_move,	0,	monster_footstep},
	{ai_move,	0,	NULL},
	{ai_move,	0,	NULL}
};
MMOVE_T(insane_spawn_move_jump) = { 236, 244, insane_spawn_frames_jump, insane_spawn_run };
MMOVE_T(insane_spawn_move_jump2) = { 236, 244, insane_spawn_frames_jump, NULL };

MONSTERINFO_ATTACK(insane_spawn_attack) (edict_t* self) -> void
{
	if (!self->spawnflags.has(SPAWNFLAG_BERSERK_NOJUMPING) && (self->timestamp < level.time) && range_to(self, self->enemy) > 50.f)
	{

		gi.sound(self, CHAN_WEAPON, sound_jump, 1, ATTN_NORM, 0);
		if (self->health < 50)
		{
			M_SetAnimation(self, &insane_spawn_move_jump2);
			self->timestamp = 0_sec;
		}
		else
		{
			M_SetAnimation(self, &insane_spawn_move_jump);
			self->timestamp = level.time + 1_sec;
		}
	}
}

MONSTERINFO_MELEE(insane_spawn_attack_2) (edict_t* self) -> void
{
	gi.sound(self, CHAN_WEAPON, sound_jump, 1, ATTN_NORM, 0);
	if (self->health < 50)
	{
		M_SetAnimation(self, &insane_spawn_move_jump2);
		self->timestamp = 0_sec;
	}
	else
	{
			M_SetAnimation(self, &insane_spawn_move_jump);
			self->timestamp = level.time + 1_sec;
	}
}

// Paril: unused atm because it breaks N64.
// may fix later
void insane_spawn_shrink(edict_t *self)
{
	self->maxs[2] = 0;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

mframe_t insane_spawn_frames_stand_normal[] = {
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand, 0, insane_spawn_checkdown }
};
MMOVE_T(insane_spawn_move_stand_normal) = { FRAME_stand60, FRAME_stand65, insane_spawn_frames_stand_normal, insane_spawn_stand };

mframe_t insane_spawn_frames_stand_insane_spawn[] = {
	{ ai_stand, 0, insane_spawn_shake },
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
	{ ai_stand, 0, insane_spawn_checkdown }
};
MMOVE_T(insane_spawn_move_stand_insane_spawn) = { FRAME_stand65, FRAME_stand94, insane_spawn_frames_stand_insane_spawn, insane_spawn_stand };

mframe_t insane_spawn_frames_uptodown[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, insane_spawn_moan },
	{ ai_move },//, 0, monster_duck_down },
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

	{ ai_move, 2.7f },
	{ ai_move, 4.1f },
	{ ai_move, 6 },
	{ ai_move, 7.6f },
	{ ai_move, 3.6f },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, insane_spawn_fist },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, insane_spawn_fist },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(insane_spawn_move_uptodown) = { FRAME_stand1, FRAME_stand40, insane_spawn_frames_uptodown, insane_spawn_onground };

mframe_t insane_spawn_frames_downtoup[] = {
	{ ai_move, -0.7f }, // 41
	{ ai_move, -1.2f }, // 42
	{ ai_move, -1.5f }, // 43
	{ ai_move, -4.5f }, // 44
	{ ai_move, -3.5f }, // 45
	{ ai_move, -0.2f }, // 46
	{ ai_move },		// 47
	{ ai_move, -1.3f }, // 48
	{ ai_move, -3 },	// 49
	{ ai_move, -2 },	// 50
	{ ai_move  },//, 0, monster_duck_up },		// 51
	{ ai_move },		// 52
	{ ai_move },		// 53
	{ ai_move, -3.3f }, // 54
	{ ai_move, -1.6f }, // 55
	{ ai_move, -0.3f }, // 56
	{ ai_move },		// 57
	{ ai_move },		// 58
	{ ai_move }			// 59
};
MMOVE_T(insane_spawn_move_downtoup) = { FRAME_stand41, FRAME_stand59, insane_spawn_frames_downtoup, insane_spawn_stand };

mframe_t insane_spawn_frames_jumpdown[] = {
	{ ai_move, 0.2f },
	{ ai_move, 11.5f },
	{ ai_move, 5.1f },
	{ ai_move, 7.1f },
	{ ai_move }
};
MMOVE_T(insane_spawn_move_jumpdown) = { FRAME_stand96, FRAME_stand100, insane_spawn_frames_jumpdown, insane_spawn_onground };

mframe_t insane_spawn_frames_down[] = {
	{ ai_move }, // 100
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }, // 110
	{ ai_move, -1.7f },
	{ ai_move, -1.6f },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, insane_spawn_fist },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }, // 120
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }, // 130
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, insane_spawn_moan },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }, // 140
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }, // 150
	{ ai_move, 0.5f },
	{ ai_move },
	{ ai_move, -0.2f, insane_spawn_scream },
	{ ai_move },
	{ ai_move, 0.2f },
	{ ai_move, 0.4f },
	{ ai_move, 0.6f },
	{ ai_move, 0.8f },
	{ ai_move, 0.7f },
	{ ai_move, 0, insane_spawn_checkup } // 160
};
MMOVE_T(insane_spawn_move_down) = { FRAME_stand100, FRAME_stand160, insane_spawn_frames_down, insane_spawn_onground };

mframe_t insane_spawn_frames_walk_normal[] = {
	{ ai_walk, 0, insane_spawn_scream },
	{ ai_walk, 2.5f },
	{ ai_walk, 3.5f },
	{ ai_walk, 1.7f },
	{ ai_walk, 2.3f },
	{ ai_walk, 2.4f },
	{ ai_walk, 2.2f, monster_footstep },
	{ ai_walk, 4.2f },
	{ ai_walk, 5.6f },
	{ ai_walk, 3.3f },
	{ ai_walk, 2.4f },
	{ ai_walk, 0.9f },
	{ ai_walk, 0, monster_footstep }
};
MMOVE_T(insane_spawn_move_walk_normal) = { FRAME_walk27, FRAME_walk39, insane_spawn_frames_walk_normal, insane_spawn_walk };

mframe_t insane_spawn_frames_walk_insane_spawn[] = {
	{ ai_walk, 0, insane_spawn_scream }, // walk 1
	{ ai_walk, 3.4f },			   // walk 2
	{ ai_walk, 3.6f },			   // 3
	{ ai_walk, 2.9f },			   // 4
	{ ai_walk, 2.2f },			   // 5
	{ ai_walk, 2.6f, monster_footstep },			   // 6
	{ ai_walk },				   // 7
	{ ai_walk, 0.7f },			   // 8
	{ ai_walk, 4.8f },			   // 9
	{ ai_walk, 5.3f },			   // 10
	{ ai_walk, 1.1f },			   // 11
	{ ai_walk, 2, monster_footstep },				   // 12
	{ ai_walk, 0.5f },			   // 13
	{ ai_walk },				   // 14
	{ ai_walk },				   // 15
	{ ai_walk, 4.9f },			   // 16
	{ ai_walk, 6.7f },			   // 17
	{ ai_walk, 3.8f },			   // 18
	{ ai_walk, 2, monster_footstep },				   // 19
	{ ai_walk, 0.2f },			   // 20
	{ ai_walk },				   // 21
	{ ai_walk, 3.4f },			   // 22
	{ ai_walk, 6.4f },			   // 23
	{ ai_walk, 5 },				   // 24
	{ ai_walk, 1.8f, monster_footstep },			   // 25
	{ ai_walk }					   // 26
};
MMOVE_T(insane_spawn_move_walk_insane_spawn) = { FRAME_walk1, FRAME_walk26, insane_spawn_frames_walk_insane_spawn, insane_spawn_walk };

mframe_t insane_spawn_frames_stand_pain[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, monster_footstep },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, monster_footstep }
};
MMOVE_T(insane_spawn_move_stand_pain) = { FRAME_st_pain2, FRAME_st_pain12, insane_spawn_frames_stand_pain, insane_spawn_run };

mframe_t insane_spawn_frames_crawl[] = {
	{ ai_walk, 0, insane_spawn_scream },
	{ ai_walk, 1.5f },
	{ ai_walk, 2.1f },
	{ ai_walk, 3.6f },
	{ ai_walk, 2, monster_footstep },
	{ ai_walk, 0.9f },
	{ ai_walk, 3 },
	{ ai_walk, 3.4f },
	{ ai_walk, 2.4f, monster_footstep }
};
MMOVE_T(insane_spawn_move_crawl) = { FRAME_crawl1, FRAME_crawl9, insane_spawn_frames_crawl, nullptr };

mframe_t insane_spawn_frames_crawl_pain[] = {
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
MMOVE_T(insane_spawn_move_crawl_pain) = { FRAME_cr_pain2, FRAME_cr_pain10, insane_spawn_frames_crawl_pain, insane_spawn_run };

MONSTERINFO_WALK(insane_spawn_walk) (edict_t *self) -> void
{
	if (self->spawnflags.has(SPAWNFLAG_insane_spawn_STAND_GROUND)) // Hold Ground?
		if (self->s.frame == FRAME_cr_pain10)
		{
			M_SetAnimation(self, &insane_spawn_move_down);
			return;
		}
	if (self->spawnflags.has(SPAWNFLAG_insane_spawn_CRAWL))
		M_SetAnimation(self, &insane_spawn_move_crawl);
	else if (frandom() <= 0.5f)
		M_SetAnimation(self, &insane_spawn_move_walk_normal);
	else
		M_SetAnimation(self, &insane_spawn_move_walk_insane_spawn);
}

mframe_t insane_spawn_frames_run[] = {
	{ ai_run, 30 ,insane_spawn_scream },
	{ ai_run, 15 },
	{ ai_run, 12 },
	{ ai_run, 15 },
	{ ai_run, 25 },
	{ ai_run, 15 },
	{ ai_run, 12, monster_footstep },
	{ ai_run, 15 },
	{ ai_run, 25 }
};
MMOVE_T(insane_spawn_move_run) = { 227, 235, insane_spawn_frames_run, NULL};

MONSTERINFO_RUN(insane_spawn_run) (edict_t* self) -> void
{
	if (self->health < 50)
	{
		self->monsterinfo.attack_finished = 0_sec;
		M_SetAnimation(self, &insane_spawn_move_jump2);
	}
	else
	{
		M_SetAnimation(self, &insane_spawn_move_run);
	}
}

PAIN(insane_spawn_pain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
{
	int l, r;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3_sec;

	r = 1 + brandom();
	if (self->health < 25)
		l = 25;
	else if (self->health < 50)
		l = 50;
	else if (self->health < 75)
		l = 75;
	else
		l = 100;
	gi.sound(self, CHAN_VOICE, gi.soundindex(G_Fmt("player/male/pain{}_{}.wav", l, r).data()), 1, ATTN_IDLE, 0);

	if (((self->s.frame >= FRAME_crawl1) && (self->s.frame <= FRAME_crawl9)) || ((self->s.frame >= FRAME_stand99) && (self->s.frame <= FRAME_stand160)) || ((self->s.frame >= FRAME_stand1 && self->s.frame <= FRAME_stand40)))
	{
		M_SetAnimation(self, &insane_spawn_move_crawl_pain);
	}
	else
	{
		M_SetAnimation(self, &insane_spawn_move_stand_pain);
	}
}

void insane_spawn_onground(edict_t *self)
{
	M_SetAnimation(self, &insane_spawn_move_down);
}

void insane_spawn_checkdown(edict_t *self)
{
	if (self->spawnflags.has(SPAWNFLAG_insane_spawn_ALWAYS_STAND)) // Always stand
		return;
	if (frandom() < 0.3f)
	{
		if (frandom() < 0.5f)
			M_SetAnimation(self, &insane_spawn_move_uptodown);
		else
			M_SetAnimation(self, &insane_spawn_move_jumpdown);
	}
}

void insane_spawn_checkup(edict_t *self)
{
	if (self->spawnflags.has_all(SPAWNFLAG_insane_spawn_CRAWL | SPAWNFLAG_insane_spawn_STAND_GROUND))
		return;
	if (frandom() < 0.5f)
	{
		M_SetAnimation(self, &insane_spawn_move_downtoup);
	}
}

MONSTERINFO_STAND(insane_spawn_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &insane_spawn_move_stand_normal);
}

DIE(insane_spawn_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_ROCKET_EXPLOSION);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS, false);
	T_RadiusClassDamage(self, attacker, 500.0f, (char*)"monster_insane_spawn", 100.0f, MOD_EXPLOSIVE);

	gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_IDLE, 0);
	ThrowGibs(self, damage, {
		{ 2, "models/monsters/insane_spawn/gibs/g_arm.md2" },
		{ 2, "models/monsters/insane_spawn/gibs/g_leg.md2" },
		{ "models/monsters/insane_spawn/gibs/g_head.md2", GIB_HEAD }
	});
	self->deadflag = true;
	return;
}

MONSTERINFO_SETSKIN(insane_spawn_setskin) (edict_t* self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}

void SP_monster_insane_spawn(edict_t *self)
{
	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict( self );
		return;
	}

	sound_fist.assign("insane/insane11.wav");
	sound_shake.assign("insane/insane5.wav");
	sound_moan.assign("insane/insane7.wav");
	sound_scream[0].assign("insane/insane1.wav");
	sound_scream[1].assign("insane/insane2.wav");
	sound_scream[2].assign("insane/insane3.wav");
	sound_scream[3].assign("insane/insane4.wav");
	sound_scream[4].assign("insane/insane6.wav");
	sound_scream[5].assign("insane/insane8.wav");
	sound_scream[6].assign("insane/insane9.wav");
	sound_scream[7].assign("insane/insane10.wav");

	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, 32 };
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->s.modelindex = gi.modelindex("models/monsters/insane_spawn/tris.md2");

	self->health = 100 * st.health_multiplier;

	if (self->solid == SOLID_NOT)
		return;

	self->gib_health = -50;
	self->mass = 300;

	self->monsterinfo.stand = insane_spawn_stand;
	self->monsterinfo.walk = insane_spawn_walk;
	self->monsterinfo.run = insane_spawn_run;
	self->monsterinfo.dodge = nullptr;
	self->monsterinfo.attack = insane_spawn_attack;
	self->monsterinfo.melee = insane_spawn_attack_2;
	self->monsterinfo.sight = nullptr;
	self->monsterinfo.setskin = insane_spawn_setskin;

	self->pain = insane_spawn_pain;
	self->die = insane_spawn_die;

	gi.linkentity(self);

	M_SetAnimation(self, &insane_spawn_move_stand_normal);
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);
}
