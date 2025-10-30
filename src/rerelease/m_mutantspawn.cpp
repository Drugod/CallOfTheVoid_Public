// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

mutantspawn

==============================================================================
*/

#include "g_local.h"
#include "m_mutantspawn.h"

constexpr spawnflags_t SPAWNFLAG_MUTANT_NOJUMPING = 8_spawnflag;

static cached_soundindex sound_swing;
static cached_soundindex sound_hit;
static cached_soundindex sound_hit2;
static cached_soundindex sound_death;
static cached_soundindex sound_idle;
static cached_soundindex sound_pain1;
static cached_soundindex sound_pain2;
static cached_soundindex sound_sight;
static cached_soundindex sound_search;
static cached_soundindex sound_step1;
static cached_soundindex sound_step2;
static cached_soundindex sound_step3;
static cached_soundindex sound_thud;

//
// SOUNDS
//

void mutantspawn_step(edict_t *self)
{
	int n = irandom(3);
	if (n == 0)
		gi.sound(self, CHAN_BODY, sound_step1, 1, ATTN_NORM, 0);
	else if (n == 1)
		gi.sound(self, CHAN_BODY, sound_step2, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_BODY, sound_step3, 1, ATTN_NORM, 0);
}

MONSTERINFO_SIGHT(mutantspawn_sight) (edict_t *self, edict_t *other) -> void
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

MONSTERINFO_SEARCH(mutantspawn_search) (edict_t *self) -> void
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

void mutantspawn_swing(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_swing, 1, ATTN_NORM, 0);
}

//
// STAND
//

mframe_t mutantspawn_frames_stand[] = {
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }, // 10

	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }, // 20

	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }, // 30

	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }, // 40

	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }, // 50

	{ ai_stand }
};
MMOVE_T(mutantspawn_move_stand) = { FRAME_stand101, FRAME_stand151, mutantspawn_frames_stand, nullptr };

MONSTERINFO_STAND(mutantspawn_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &mutantspawn_move_stand);
}

//
// IDLE
//

void mutantspawn_idle_loop(edict_t *self)
{
	if (frandom() < 0.75f)
		self->monsterinfo.nextframe = FRAME_stand155;
}

mframe_t mutantspawn_frames_idle[] = {
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }, // scratch loop start
	{ ai_stand },
	{ ai_stand },
	{ ai_stand, 0, mutantspawn_idle_loop }, // scratch loop end
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }
};
MMOVE_T(mutantspawn_move_idle) = { FRAME_stand152, FRAME_stand164, mutantspawn_frames_idle, mutantspawn_stand };

MONSTERINFO_IDLE(mutantspawn_idle) (edict_t *self) -> void
{
	M_SetAnimation(self, &mutantspawn_move_idle);
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

//
// WALK
//

mframe_t mutantspawn_frames_walk[] = {
	{ ai_walk, 3 },
	{ ai_walk, 1 },
	{ ai_walk, 5 },
	{ ai_walk, 10 },
	{ ai_walk, 13 },
	{ ai_walk, 10 },
	{ ai_walk },
	{ ai_walk, 5 },
	{ ai_walk, 6 },
	{ ai_walk, 16 },
	{ ai_walk, 15 },
	{ ai_walk, 6 }
};
MMOVE_T(mutantspawn_move_walk) = { FRAME_walk05, FRAME_walk16, mutantspawn_frames_walk, nullptr };

void mutantspawn_walk_loop(edict_t *self)
{
	M_SetAnimation(self, &mutantspawn_move_walk);
}

mframe_t mutantspawn_frames_start_walk[] = {
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, -2 },
	{ ai_walk, 1 }
};
MMOVE_T(mutantspawn_move_start_walk) = { FRAME_walk01, FRAME_walk04, mutantspawn_frames_start_walk, mutantspawn_walk_loop };

MONSTERINFO_WALK(mutantspawn_walk) (edict_t *self) -> void
{
	M_SetAnimation(self, &mutantspawn_move_start_walk);
}

//
// RUN
//

mframe_t mutantspawn_frames_run[] = {
	{ ai_run, 40 },
	{ ai_run, 40, mutantspawn_step },
	{ ai_run, 24 },
	{ ai_run, 5, mutantspawn_step },
	{ ai_run, 17 },
	{ ai_run, 10 }
};
MMOVE_T(mutantspawn_move_run) = { FRAME_run03, FRAME_run08, mutantspawn_frames_run, nullptr };

MONSTERINFO_RUN(mutantspawn_run) (edict_t *self) -> void
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		M_SetAnimation(self, &mutantspawn_move_stand);
	else
		M_SetAnimation(self, &mutantspawn_move_run);
}

//
// MELEE
//

void mutantspawn_hit_left(edict_t *self)
{
	vec3_t aim = { MELEE_DISTANCE, self->mins[0], 8 };
	if (fire_hit(self, aim, irandom(5, 15), 100))
		gi.sound(self, CHAN_WEAPON, sound_hit, 1, ATTN_NORM, 0);
	else
	{
		gi.sound(self, CHAN_WEAPON, sound_swing, 1, ATTN_NORM, 0);
		self->monsterinfo.melee_debounce_time = level.time + 1.5_sec;
	}
}

void mutantspawn_hit_right(edict_t *self)
{
	vec3_t aim = { MELEE_DISTANCE, self->maxs[0], 8 };
	if (fire_hit(self, aim, irandom(5, 15), 100))
		gi.sound(self, CHAN_WEAPON, sound_hit2, 1, ATTN_NORM, 0);
	else
	{
		gi.sound(self, CHAN_WEAPON, sound_swing, 1, ATTN_NORM, 0);
		self->monsterinfo.melee_debounce_time = level.time + 1.5_sec;
	}
}

void mutantspawn_check_refire(edict_t *self)
{
	if (!self->enemy || !self->enemy->inuse || self->enemy->health <= 0)
		return;

	if ((self->monsterinfo.melee_debounce_time <= level.time) && ((frandom() < 0.5f) || (range_to(self, self->enemy) <= RANGE_MELEE)))
		self->monsterinfo.nextframe = FRAME_attack09;
}

mframe_t mutantspawn_frames_attack[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, mutantspawn_hit_left },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, mutantspawn_hit_right },
	{ ai_charge, 0, mutantspawn_check_refire }
};
MMOVE_T(mutantspawn_move_attack) = { FRAME_attack09, FRAME_attack15, mutantspawn_frames_attack, mutantspawn_run };

MONSTERINFO_MELEE(mutantspawn_melee) (edict_t *self) -> void
{
	M_SetAnimation(self, &mutantspawn_move_attack);
}

//
// ATTACK
//

TOUCH(mutantspawn_jump_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	if (self->health <= 0)
	{
		self->touch = nullptr;
		return;
	}

	if (self->style == 1 && other->takedamage)
	{
		// [Paril-KEX] only if we're actually moving fast enough to hurt
		if (self->velocity.length() > 30)
		{
			vec3_t point;
			vec3_t normal;
			int	   damage;

			normal = self->velocity;
			normal.normalize();
			point = self->s.origin + (normal * self->maxs[0]);
			damage = (int) frandom(40, 50);
			T_Damage(other, self, self, self->velocity, point, normal, damage, damage, DAMAGE_NONE, MOD_UNKNOWN);
			self->style = 0;
		}
	}

	if (!M_CheckBottom(self))
	{
		if (self->groundentity)
		{
			self->monsterinfo.nextframe = FRAME_attack02;
			self->touch = nullptr;
		}
		return;
	}

	self->touch = nullptr;
}

void mutantspawn_jump_takeoff(edict_t *self)
{
	vec3_t forward;

	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
	AngleVectors(self->s.angles, forward, nullptr, nullptr);
	self->s.origin[2] += 1;
	self->velocity = forward * 425;
	self->velocity[2] = 160;
	self->groundentity = nullptr;
	self->monsterinfo.aiflags |= AI_DUCKED;
	self->monsterinfo.attack_finished = level.time + 3_sec;
	self->style = 1;
	self->touch = mutantspawn_jump_touch;
}

void mutantspawn_check_landing(edict_t *self)
{
	monster_jump_finished(self);

	if (self->groundentity)
	{
		gi.sound(self, CHAN_WEAPON, sound_thud, 1, ATTN_NORM, 0);
		self->monsterinfo.attack_finished = level.time + random_time(500_ms, 1.5_sec);

		if (self->monsterinfo.unduck)
			self->monsterinfo.unduck(self);

		if (range_to(self, self->enemy) <= RANGE_MELEE * 2.f)
			self->monsterinfo.melee(self);

		return;
	}

	if (level.time > self->monsterinfo.attack_finished)
		self->monsterinfo.nextframe = FRAME_attack02;
	else
		self->monsterinfo.nextframe = FRAME_attack05;
}

mframe_t mutantspawn_frames_jump[] = {
	{ ai_charge },
	{ ai_charge, 17 },
	{ ai_charge, 15, mutantspawn_jump_takeoff },
	{ ai_charge, 15 },
	{ ai_charge, 15, mutantspawn_check_landing },
	{ ai_charge },
	{ ai_charge, 3 },
	{ ai_charge }
};
MMOVE_T(mutantspawn_move_jump) = { FRAME_attack01, FRAME_attack08, mutantspawn_frames_jump, mutantspawn_run };

MONSTERINFO_ATTACK(mutantspawn_jump) (edict_t *self) -> void
{
	M_SetAnimation(self, &mutantspawn_move_jump);
}

//
// CHECKATTACK
//

bool mutantspawn_check_melee(edict_t *self)
{
	return range_to(self, self->enemy) <= RANGE_MELEE && self->monsterinfo.melee_debounce_time <= level.time;
}

bool mutantspawn_check_jump(edict_t *self)
{
	vec3_t v;
	float  distance;

	// Paril: no harm in letting them jump down if you're below them
	// if (self->absmin[2] > (self->enemy->absmin[2] + 0.75 * self->enemy->size[2]))
	//	return false;

	// don't jump if there's no way we can reach standing height
	if (self->absmin[2] + 125 < self->enemy->absmin[2])
		return false;

	v[0] = self->s.origin[0] - self->enemy->s.origin[0];
	v[1] = self->s.origin[1] - self->enemy->s.origin[1];
	v[2] = 0;
	distance = v.length();

	// if we're not trying to avoid a melee, then don't jump
	if (distance < 100 && self->monsterinfo.melee_debounce_time <= level.time)
		return false;
	// only use it to close distance gaps
	if (distance > 265)
		return false;

	return self->monsterinfo.attack_finished < level.time && brandom();
}

MONSTERINFO_CHECKATTACK(mutantspawn_checkattack) (edict_t *self) -> bool
{
	if (!self->enemy || self->enemy->health <= 0)
		return false;

	if (mutantspawn_check_melee(self))
	{
		self->monsterinfo.attack_state = AS_MELEE;
		return true;
	}

	if (!self->spawnflags.has(SPAWNFLAG_MUTANT_NOJUMPING) && mutantspawn_check_jump(self))
	{
		self->monsterinfo.attack_state = AS_MISSILE;
		return true;
	}

	return false;
}

//
// PAIN
//

mframe_t mutantspawn_frames_pain1[] = {
	{ ai_move, 4 },
	{ ai_move, -3 },
	{ ai_move, -8 },
	{ ai_move, 2 },
	{ ai_move, 5 }
};
MMOVE_T(mutantspawn_move_pain1) = { FRAME_pain101, FRAME_pain105, mutantspawn_frames_pain1, mutantspawn_run };

mframe_t mutantspawn_frames_pain2[] = {
	{ ai_move, -24 },
	{ ai_move, 11 },
	{ ai_move, 5 },
	{ ai_move, -2 },
	{ ai_move, 6 },
	{ ai_move, 4 }
};
MMOVE_T(mutantspawn_move_pain2) = { FRAME_pain201, FRAME_pain206, mutantspawn_frames_pain2, mutantspawn_run };

mframe_t mutantspawn_frames_pain3[] = {
	{ ai_move, -22 },
	{ ai_move, 3 },
	{ ai_move, 3 },
	{ ai_move, 2 },
	{ ai_move, 1 },
	{ ai_move, 1 },
	{ ai_move, 6 },
	{ ai_move, 3 },
	{ ai_move, 2 },
	{ ai_move },
	{ ai_move, 1 }
};
MMOVE_T(mutantspawn_move_pain3) = { FRAME_pain301, FRAME_pain311, mutantspawn_frames_pain3, mutantspawn_run };

PAIN(mutantspawn_pain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
{
	float r;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3_sec;

	r = frandom();
	if (r < 0.33f)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else if (r < 0.66f)
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	
	if (!M_ShouldReactToPain(self, mod))
		return; // no pain anims in nightmare

	if (r < 0.33f)
		M_SetAnimation(self, &mutantspawn_move_pain1);
	else if (r < 0.66f)
		M_SetAnimation(self, &mutantspawn_move_pain2);
	else
		M_SetAnimation(self, &mutantspawn_move_pain3);
}

MONSTERINFO_SETSKIN(mutantspawn_setskin) (edict_t *self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}

//
// DEATH
//

void mutantspawn_shrink(edict_t *self)
{
	self->maxs[2] = 0;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

// [Paril-KEX]
static void ai_move_slide_right(edict_t *self, float dist)
{
	M_walkmove(self, self->s.angles[YAW] + 90, dist);
}

static void ai_move_slide_left(edict_t *self, float dist)
{
	M_walkmove(self, self->s.angles[YAW] - 90, dist);
}

mframe_t mutantspawn_frames_death1[] = {
	{ ai_move_slide_right },
	{ ai_move_slide_right },
	{ ai_move_slide_right },
	{ ai_move_slide_right, 2 },
	{ ai_move_slide_right, 5 },
	{ ai_move_slide_right, 7, mutantspawn_shrink },
	{ ai_move_slide_right, 6 },
	{ ai_move_slide_right, 2 },
	{ ai_move_slide_right }
};
MMOVE_T(mutantspawn_move_death1) = { FRAME_death101, FRAME_death109, mutantspawn_frames_death1, monster_dead };

mframe_t mutantspawn_frames_death2[] = {
	{ ai_move_slide_left },
	{ ai_move_slide_left },
	{ ai_move_slide_left },
	{ ai_move_slide_left, 1 },
	{ ai_move_slide_left, 3, mutantspawn_shrink },
	{ ai_move_slide_left, 6 },
	{ ai_move_slide_left, 8 },
	{ ai_move_slide_left, 5 },
	{ ai_move_slide_left, 2 },
	{ ai_move_slide_left }
};
MMOVE_T(mutantspawn_move_death2) = { FRAME_death201, FRAME_death210, mutantspawn_frames_death2, monster_dead };

DIE(mutantspawn_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	//if (M_CheckGib(self, mod))
	//{
	//	gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);



	//	self->s.skinnum /= 2;

		ThrowGibs(self, damage, {
			{ 2, "models/objects/gibs/bone/tris.md2" },
			{ 4, "models/objects/gibs/sm_meat/tris.md2" },
			{ 2, "models/monsters/mutantspawn/gibs/hand.md2", GIB_SKINNED | GIB_UPRIGHT },
			{ 2, "models/monsters/mutantspawn/gibs/foot.md2", GIB_SKINNED },
			{ "models/monsters/mutantspawn/gibs/chest.md2", GIB_SKINNED },
			{ "models/monsters/mutantspawn/gibs/head.md2", GIB_SKINNED | GIB_HEAD }
		});

		self->deadflag = true;
		//return;
		// 
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_ROCKET_EXPLOSION);
		gi.WritePosition(self->s.origin);
		gi.multicast(self->s.origin, MULTICAST_PVS, false);
		T_RadiusClassDamage(self, attacker, 500.0f, (char*)"monster_mutantspawn", 100.0f, MOD_EXPLOSIVE);
		G_FreeEdict(self);

	//}

	//if (self->deadflag)
	//	return;

	//gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	//self->deadflag = true;
	//self->takedamage = true;

	//if (frandom() < 0.5f)
	//	M_SetAnimation(self, &mutantspawn_move_death1);
	//else
	//	M_SetAnimation(self, &mutantspawn_move_death2);
}

//================
// ROGUE
void mutantspawn_jump_down(edict_t *self)
{
	vec3_t forward, up;

	AngleVectors(self->s.angles, forward, nullptr, up);
	self->velocity += (forward * 100);
	self->velocity += (up * 300);
}

void mutantspawn_jump_up(edict_t *self)
{
	vec3_t forward, up;

	AngleVectors(self->s.angles, forward, nullptr, up);
	self->velocity += (forward * 200);
	self->velocity += (up * 450);
}

void mutantspawn_jump_wait_land(edict_t *self)
{
	if (!monster_jump_finished(self) && self->groundentity == nullptr)
		self->monsterinfo.nextframe = self->s.frame;
	else
		self->monsterinfo.nextframe = self->s.frame + 1;
}

mframe_t mutantspawn_frames_jump_up[] = {
	{ ai_move, -8 },
	{ ai_move, -8, mutantspawn_jump_up },
	{ ai_move, 0, mutantspawn_jump_wait_land },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(mutantspawn_move_jump_up) = { FRAME_jump01, FRAME_jump05, mutantspawn_frames_jump_up, mutantspawn_run };

mframe_t mutantspawn_frames_jump_down[] = {
	{ ai_move },
	{ ai_move, 0, mutantspawn_jump_down },
	{ ai_move, 0, mutantspawn_jump_wait_land },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(mutantspawn_move_jump_down) = { FRAME_jump01, FRAME_jump05, mutantspawn_frames_jump_down, mutantspawn_run };

void mutantspawn_jump_updown(edict_t *self, blocked_jump_result_t result)
{
	if (!self->enemy)
		return;

	if (result == blocked_jump_result_t::JUMP_JUMP_UP)
		M_SetAnimation(self, &mutantspawn_move_jump_up);
	else
		M_SetAnimation(self, &mutantspawn_move_jump_down);
}

/*
===
Blocked
===
*/
MONSTERINFO_BLOCKED(mutantspawn_blocked) (edict_t *self, float dist) -> bool
{
	if (auto result = blocked_checkjump(self, dist); result != blocked_jump_result_t::NO_JUMP)
	{
		if (result != blocked_jump_result_t::JUMP_TURN)
			mutantspawn_jump_updown(self, result);
		return true;
	}

	if (blocked_checkplat(self, dist))
		return true;

	return false;
}
// ROGUE
//================

//
// SPAWN
//

/*QUAKED monster_mutantspawn (1 .5 0) (-32 -32 -24) (32 32 32) Ambush Trigger_Spawn Sight NoJumping
model="models/monsters/mutantspawn/tris.md2"
*/
void SP_monster_mutantspawn(edict_t *self)
{
	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict( self );
		return;
	}

	sound_swing.assign("mutantspawn/mutatck1.wav");
	sound_hit.assign("mutantspawn/mutatck2.wav");
	sound_hit2.assign("mutantspawn/mutatck3.wav");
	sound_death.assign("mutantspawn/mutdeth1_r.wav");
	sound_idle.assign("mutantspawn/mutidle1_r.wav");
	sound_pain1.assign("mutantspawn/mutpain1_r.wav");
	sound_pain2.assign("mutantspawn/mutpain2_r.wav");
	sound_sight.assign("mutantspawn/mutsght1_r.wav");
	sound_search.assign("mutantspawn/mutsrch1_r.wav");
	sound_step1.assign("mutantspawn/step1.wav");
	sound_step2.assign("mutantspawn/step2.wav");
	sound_step3.assign("mutantspawn/step3.wav");
	sound_thud.assign("mutantspawn/thud1.wav");

	self->monsterinfo.aiflags |= AI_STINKY;

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/mutantspawn/tris.md2");
	
	gi.modelindex("models/monsters/mutantspawn/gibs/head.md2");
	gi.modelindex("models/monsters/mutantspawn/gibs/chest.md2");
	gi.modelindex("models/monsters/mutantspawn/gibs/hand.md2");
	gi.modelindex("models/monsters/mutantspawn/gibs/foot.md2");

	self->mins = { -18, -18, -24 };
	self->maxs = { 18, 18, 30 };

	self->health = 300 * st.health_multiplier;
	self->gib_health = -120;
	self->mass = 300;

	self->pain = mutantspawn_pain;
	self->die = mutantspawn_die;

	self->monsterinfo.stand = mutantspawn_stand;
	self->monsterinfo.walk = mutantspawn_walk;
	self->monsterinfo.run = mutantspawn_run;
	self->monsterinfo.dodge = nullptr;
	self->monsterinfo.attack = mutantspawn_jump;
	self->monsterinfo.melee = mutantspawn_melee;
	self->monsterinfo.sight = mutantspawn_sight;
	self->monsterinfo.search = mutantspawn_search;
	self->monsterinfo.idle = mutantspawn_idle;
	self->monsterinfo.checkattack = mutantspawn_checkattack;
	self->monsterinfo.blocked = mutantspawn_blocked; // PGM
	self->monsterinfo.setskin = mutantspawn_setskin;

	gi.linkentity(self);

	M_SetAnimation(self, &mutantspawn_move_stand);

	self->monsterinfo.combat_style = COMBAT_MELEE;

	self->monsterinfo.scale = MODEL_SCALE;
	self->monsterinfo.can_jump = !(self->spawnflags & SPAWNFLAG_MUTANT_NOJUMPING);
	self->monsterinfo.drop_height = 256;
	self->monsterinfo.jump_height = 68;

	walkmonster_start(self);
}
