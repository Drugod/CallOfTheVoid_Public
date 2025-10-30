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
// m_demon.c

#include "g_local.h"
#include "m_fiend_strogg.h"

constexpr spawnflags_t SPAWNFLAG_BERSERK_NOJUMPING = 8_spawnflag;

static cached_soundindex sound_death;
static cached_soundindex sound_hit;
static cached_soundindex sound_jump;
static cached_soundindex sound_land;
static cached_soundindex sound_pain;
static cached_soundindex sound_sight;
static cached_soundindex sound_search;
static cached_soundindex sound_thud;
static cached_soundindex sound_explod;

// just in case you do't want to use the macros
typedef float vec_t;
// Stand
mframe_t demon_frames_stand [] =
{
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand, 0, NULL}
};
MMOVE_T(demon_move_stand) = {0, 12, demon_frames_stand, NULL};

MONSTERINFO_STAND(demon_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &demon_move_stand);
}

// Run
mframe_t demon_frames_run [] =
{
	{ai_run, 20, NULL},
	{ai_run, 15, NULL},
	{ai_run, 36, NULL},
	{ai_run, 20, NULL},
	{ai_run, 15, NULL},
	{ai_run, 36, NULL}
};
MMOVE_T(demon_move_run) = {21, 26, demon_frames_run, NULL};

MONSTERINFO_RUN(demon_run) (edict_t *self) -> void
{
	M_SetAnimation(self, &demon_move_run);	
}

// Walk
mframe_t demon_frames_walk[] =
{
	{ai_walk, 8, NULL},
	{ai_walk, 6, NULL},
	{ai_walk, 6, NULL},
	{ai_walk, 7, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 6, NULL},
	{ai_walk, 10, NULL},
	{ai_walk, 10, NULL}
};
MMOVE_T(demon_move_walk) = { 13, 20, demon_frames_walk, NULL };

MONSTERINFO_WALK(demon_walk) (edict_t* self) -> void
{
	M_SetAnimation(self, &demon_move_walk);
}

bool CheckDemonJump(edict_t *self)
{
	vec3_t	dir;
	float	distance;

	if (!self->enemy)
		return false;
	if (self->s.origin[2] + self->mins[2] > self->enemy->s.origin[2] + self->enemy->mins[2] + 0.75 * self->enemy->size[2])
		return false;
	if (self->s.origin[2] + self->mins[2] < self->enemy->s.origin[2] + self->enemy->mins[2] + 0.25 * self->enemy->size[2])
		return false;
		
	dir = self->enemy->s.origin - self->s.origin;
	dir[2] = 0;
	distance = dir.length();
	
	if (distance < 100)
		return false;
		
	if (distance > 200)
	{
		if (frandom() < 0.9)
			return false;
	}
	return true;
};


TOUCH (DemonJumpTouch) (edict_t* self, edict_t* other, const trace_t& tr, bool other_touching_self) -> void
{
	vec3_t vel;
	vec3_t point;
	vec3_t normal;
	int	   damage;

	if (self->health < 1)
		return;
	if (other->takedamage)
	{
		vel = self->velocity;
		if (vel.length() > 400)
		{
			normal = self->velocity;
			normal.normalize();
			point = self->s.origin + (normal * self->maxs[0]);
			damage = (int)frandom(0, 10) + 10;
			T_Damage(other, self, self, self->velocity, point, normal, damage, damage, DAMAGE_NONE, MOD_UNKNOWN);
		}
	}
	else
		gi.sound(self, CHAN_WEAPON, sound_land, 1, ATTN_NORM, 0);
	self->touch = NULL;

	if (!M_CheckBottom(self))
	{
		if (self->groundentity)
		{
			M_SetAnimation(self, &demon_move_run);
			self->movetype = MOVETYPE_STEP;
		}
		return;
	}
}

void DemonJump(edict_t *self)
{
	vec3_t forward;
	float length = (self->s.origin - self->enemy->s.origin).length();
	float fwd_speed = length * 1.95f;


	AngleVectors(self->s.angles, forward, NULL, NULL);
	self->s.origin[2] += 1;
	self->velocity[0] = 600 * forward[0];
	self->velocity[1] = 600 * forward[1];
	self->velocity[2] = 250;

	self->groundentity = NULL;
	self->touch = DemonJumpTouch;
}

static void demon_roar(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_jump, 1, ATTN_NORM, 0);
}

// Melee
void DemonMelee(edict_t* self)
{
	vec3_t dir;
	static vec3_t aim = { 100, 0, -24 };
	int damage;

	if (!self->enemy)
		return;
	dir = self->enemy->s.origin - self->s.origin;

	if (dir.length() > 100.0)
		return;
	damage = 10 + 5 * frandom();

	if (fire_hit(self, aim, damage, damage))
		gi.sound(self, CHAN_WEAPON, sound_hit, 1, ATTN_NORM, 0);
}

mframe_t demon_frames_melee[] =
{
	{ai_charge, 4,	NULL},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL},
	{ai_charge, 1,	NULL},
	{ai_charge, 14,	DemonMelee},
	{ai_charge, 1,	NULL},
	{ai_charge, 6,	NULL},
	{ai_charge, 8,	NULL},
	{ai_charge, 4,	NULL},
	{ai_charge, 2,	NULL},
	{ai_charge, 12,	DemonMelee},
	{ai_charge, 5,	NULL},
	{ai_charge, 8,	NULL},
	{ai_charge, 4,	NULL},
	{ai_charge, 4,	NULL}
};
MMOVE_T(demon_move_melee) = { 54, 68, demon_frames_melee, demon_run };

MONSTERINFO_MELEE(demon_melee) (edict_t* self) -> void
{
	M_SetAnimation(self, &demon_move_melee);
}

// Attack
void T_SlamRadiusDamage3(vec3_t point, edict_t* inflictor, edict_t* attacker, float damage, float kick, edict_t* ignore, float radius, mod_t mod)
{
	float	 points;
	edict_t* ent = nullptr;
	vec3_t	 v;
	vec3_t	 dir;

	while ((ent = findradius(ent, inflictor->s.origin, radius * 2.f)) != nullptr)
	{
		if (ent == ignore)
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

static void demon_attack_slam(edict_t* self)
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

	T_SlamRadiusDamage3(tr.endpos, self, self, 100, 300.f, self, 165, MOD_UNKNOWN);
}

TOUCH(demon_jump_touch) (edict_t* self, edict_t* other, const trace_t& tr, bool other_touching_self) -> void
{
	if (self->health <= 0)
	{
		self->touch = nullptr;
		return;
	}

	if (self->groundentity)
	{
		self->s.frame = 38;

		if (self->touch)
			demon_attack_slam(self);

		self->touch = nullptr;
	}
}

static void demon_high_gravity(edict_t* self)
{
	if (self->velocity[2] < 0)
		self->gravity = 2.25f * (800.f / level.gravity);
	else
		self->gravity = 5.25f * (800.f / level.gravity);
}

void demon_jump_takeoff(edict_t* self)
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
	self->touch = demon_jump_touch;
	demon_high_gravity(self);
}

void demon_check_landing(edict_t* self)
{
	demon_high_gravity(self);

	if (self->groundentity)
	{
		self->monsterinfo.attack_finished = 0_ms;
		self->monsterinfo.unduck(self);
		self->s.frame = 38;
		if (self->touch)
		{
			demon_attack_slam(self);
			self->touch = nullptr;
		}
		self->flags &= ~FL_KILL_VELOCITY;
		return;
	}

	if (level.time > self->monsterinfo.attack_finished)
		self->monsterinfo.nextframe = 31;
	else
		self->monsterinfo.nextframe = 33;
}


mframe_t demon_frames_jump [] =
{
	{ai_charge,	0,	NULL},
	{ai_charge,	0,	demon_roar},
	{ai_charge,	0,	NULL},
	{ai_charge,	0,	demon_jump_takeoff}, //DemonJump},
	{ai_move,	0,	demon_high_gravity},
	{ai_move,	0,	demon_check_landing},
	{ai_move,	0,	monster_footstep},
	{ai_move,	0,	NULL},
	{ai_move,	0,	monster_footstep},
	{ai_move,	0,	monster_footstep},
	{ai_move,	0,	NULL},
	{ai_move,	0,	NULL}
};
MMOVE_T(demon_move_jump) = {27, 38, demon_frames_jump, demon_run};

MONSTERINFO_ATTACK(demon_attack) (edict_t *self) -> void
{
	//M_SetAnimation(self, &demon_move_jump);	
	/*if (self->monsterinfo.melee_debounce_time <= level.time && (range_to(self, self->enemy) < MELEE_DISTANCE))
		demon_melee(self);
	else*/ 
	//if (!self->spawnflags.has(SPAWNFLAG_BERSERK_NOJUMPING) && (self->timestamp < level.time && brandom()) && range_to(self, self->enemy) > 50.f)
	if (!self->spawnflags.has(SPAWNFLAG_BERSERK_NOJUMPING) && (self->timestamp < level.time) && range_to(self, self->enemy) > 50.f)
	{
		M_SetAnimation(self, &demon_move_jump);
		gi.sound(self, CHAN_WEAPON, sound_jump, 1, ATTN_NORM, 0);
		self->timestamp = level.time + 2_sec;
	}
}

static void fiend_strogg_pain_sound(edict_t* self)
{
	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
}

mframe_t demon_frames_pain [] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, fiend_strogg_pain_sound},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
MMOVE_T(demon_move_pain) = {39, 44, demon_frames_pain, demon_run};

PAIN (demon_pain)  (edict_t* self, edict_t* other, float kick, int damage, const mod_t& mod) -> void
{
	if (skill->value == 3)
		return;
	if (self->touch = DemonJumpTouch)
		return;
	if (self->pain_debounce_time > level.time)
		return;
	self->pain_debounce_time = level.time + 1_sec;

	if (frandom() * 200 > damage)
		return;
	M_SetAnimation(self, &demon_move_pain);
}

MONSTERINFO_SETSKIN(demon_setskin) (edict_t* self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}

void demon_dead(edict_t *self)
{
	self->mins = { -32, -32, -24 };
	self->maxs = { 32, 32, -8 };
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

static void demon_death_sound(edict_t* self)
{
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
}

mframe_t demon_frames_die [] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, demon_death_sound},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
MMOVE_T(demon_move_die) = {45, 53, demon_frames_die, demon_dead};

DIE(demon_die) (edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t& mod) -> void
{
	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		ThrowGibs(self, damage, {
			{ "models/objects/gibs/bone/tris.md2" },
			{ "models/objects/gibs/sm_meat/tris.md2" },
			{ "models/monsters/fiendstrogg/gibs/g_arm.md2" },
			{ "models/monsters/fiendstrogg/gibs/g_arm.md2" },
			{ "models/monsters/fiendstrogg/gibs/g_leg.md2" },
			{ "models/monsters/fiendstrogg/gibs/g_leg.md2" },
			{ "models/monsters/fiendstrogg/gibs/g_head.md2", GIB_HEAD }
			});
		self->deadflag = true;
		return;
	}
	if (self->deadflag == true)
		return;

	self->deadflag = true;
	self->takedamage = true;

	M_SetAnimation(self, &demon_move_die);
}

// Sight
MONSTERINFO_SIGHT(demon_sight) (edict_t* self, edict_t* other) -> void
{
	
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

// Search
MONSTERINFO_SEARCH(demon_search) (edict_t *self) -> void
{
	gi.sound (self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

void SP_monster_fiend_strogg(edict_t *self)
{

	if (!M_AllowSpawn(self)) {
		G_FreeEdict(self);
		return;
	}

	self->mins = { -32, -32, -24 };
	self->maxs = { 32, 32, 64 };
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->s.modelindex = gi.modelindex("models/monsters/fiendstrogg/tris.md2");
	gi.modelindex("models/monsters/fiendstrogg/gibs/g_arm.md2");
	gi.modelindex("models/monsters/fiendstrogg/gibs/g_leg.md2");
	gi.modelindex("models/monsters/fiendstrogg/gibs/g_head.md2");

	self->health = 300 * st.health_multiplier;

	sound_death.assign("demon/ddeath_s.wav");
	sound_hit.assign("demon/dhit2.wav");
	sound_jump.assign("demon/djump_s.wav");
	sound_land.assign("demon/dland2.wav");
	sound_pain.assign("demon/dpain1_s.wav");
	sound_search.assign("demon/idle1_s.wav");
	sound_sight.assign("demon/sight2_s.wav");
	sound_thud.assign("mutant/thud1.wav");
	sound_explod.assign("world/explod2.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->gib_health = -80;
	self->mass = 300;

	self->monsterinfo.stand = demon_stand;
	self->monsterinfo.walk = demon_walk;
	self->monsterinfo.run = demon_run;
	self->monsterinfo.attack = demon_attack;
	self->monsterinfo.melee = demon_melee;
	self->monsterinfo.sight = demon_sight;
	self->monsterinfo.search = demon_search;
	self->monsterinfo.setskin = demon_setskin;

	self->pain = demon_pain;
	self->die = demon_die;

	gi.linkentity(self);

	M_SetAnimation(self, &demon_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);
}
