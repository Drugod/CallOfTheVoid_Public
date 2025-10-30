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
// m_hknight.c

#include "g_local.h"
#include "m_hknightberserker_strogg.h"

constexpr spawnflags_t SPAWNFLAG_BERSERK_NOJUMPING = 8_spawnflag;

static cached_soundindex sound_attack;
static cached_soundindex sound_melee;
static cached_soundindex sound_death;
static cached_soundindex sound_proj_hit;
static cached_soundindex sound_sight;
static cached_soundindex sound_search;
static cached_soundindex sound_pain;
static cached_soundindex sound_thud;
static cached_soundindex sound_explod;
static cached_soundindex sound_jump;

void hknight_bsk_run(edict_t* self);
vec3_t SightEndtToDir(edict_t* self, vec3_t orig_dir);

void SwingSword2(edict_t* self)
{
	vec3_t dir;
	static vec3_t aim = { 100, 0, -24 };
	int damage;

	if (!self->enemy)
		return;
	//VectorSubtract(self->s.origin, self->enemy->s.origin, dir);
	dir = self->s.origin - self->enemy->s.origin;

	if (dir.length() > 100.0)
		return;
	damage = (frandom() + frandom() + frandom()) * 3;

	fire_hit(self, aim, damage, damage);
}


int CheckDistance2(edict_t* self, edict_t* enemy)
{
	vec3_t v;

	//VectorSubtract(self->s.origin, enemy->s.origin, v);
	v = self->s.origin - enemy->s.origin;
	float dist = v.length();
	return dist;
}


// Stand
mframe_t hknight_bsk_frames_stand [] =
{
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
MMOVE_T(hknight_bsk_move_stand) = {0, 8, hknight_bsk_frames_stand, NULL};

MONSTERINFO_STAND(hknight_bsk_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &hknight_bsk_move_stand);	
}

// Charge
mframe_t hknight_bsk_frames_charge [] =
{
	{ai_charge, 20,	NULL},
	{ai_charge, 25,	NULL},
	{ai_charge, 18,	NULL},
	{ai_charge, 16,	NULL},
	{ai_charge, 14,	NULL},
	{ai_charge, 20,	SwingSword2},
	{ai_charge, 21,	SwingSword2},
	{ai_charge, 13,	SwingSword2},
	{ai_charge, 20,	SwingSword2},
	{ai_charge, 20,	SwingSword2},
	{ai_charge, 18,	SwingSword2},
	{ai_charge, 16,	NULL},
	{ai_charge, 20,	NULL},
	{ai_charge, 14,	NULL},
	{ai_charge, 25,	NULL},
	{ai_charge, 21,	NULL}
};
MMOVE_T(hknight_bsk_move_charge) = {63, 78, hknight_bsk_frames_charge, hknight_bsk_run};

bool CheckForCharge2(edict_t *self)
{
	if (!self->enemy)
		return false;

	if (!visible(self, self->enemy))
		return false;

	if (fabs(self->s.origin[2] - self->enemy->s.origin[2]) > 20)
		return false;
	if (CheckDistance2(self, self->enemy) > 320)
		return false;
	return true;
}

// Run
/*mframe_t hknight_bsk_frames_run[] =
{
	{ai_run, 20, NULL},
	{ai_run, 18, NULL},
	{ai_run, 25, NULL},
	{ai_run, 16, NULL},
	{ai_run, 14, NULL},
	{ai_run, 25, NULL},
	{ai_run, 21, NULL},
	{ai_run, 13, NULL}
};*/

void speedframe(edict_t* self)
{
	self->nextthink = level.time + 0.05_sec;
}

mframe_t hknight_bsk_frames_run[] =
{
	{ai_run, 40, speedframe},
	{ai_run, 36, speedframe},
	{ai_run, 50, speedframe},
	{ai_run, 32, speedframe},
	{ai_run, 28, speedframe},
	{ai_run, 50, speedframe},
	{ai_run, 42, speedframe},
	{ai_run, 26, speedframe}
};
MMOVE_T(hknight_bsk_move_run) = {29, 36, hknight_bsk_frames_run, NULL};

MONSTERINFO_RUN (hknight_bsk_run) (edict_t *self) -> void
{
	M_SetAnimation(self, &hknight_bsk_move_run);	
}

// walk
mframe_t hknight_bsk_frames_walk[] =
{
	{ai_walk, 2, NULL},
	{ai_walk, 5, NULL},
	{ai_walk, 5, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 6, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 2, NULL}
};
MMOVE_T(hknight_bsk_move_walk) = { 9, 28, hknight_bsk_frames_walk, NULL };

MONSTERINFO_WALK(hknight_bsk_walk) (edict_t* self) -> void
{
	M_SetAnimation(self, &hknight_bsk_move_walk);
}

// Attack
void T_SlamRadiusDamage2(vec3_t point, edict_t* inflictor, edict_t* attacker, float damage, float kick, edict_t* ignore, float radius, mod_t mod)
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

static void hellberserk_attack_slam(edict_t* self)
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

	T_SlamRadiusDamage2(tr.endpos, self, self, 50, 300.f, self, 165, MOD_UNKNOWN);
}

TOUCH(hellberserk_jump_touch) (edict_t* self, edict_t* other, const trace_t& tr, bool other_touching_self) -> void
{
	if (self->health <= 0)
	{
		self->touch = nullptr;
		return;
	}

	if (self->groundentity)
	{
		self->s.frame = 165;

		if (self->touch)
			hellberserk_attack_slam(self);

		self->touch = nullptr;
	}
}

static void hellberserk_high_gravity(edict_t* self)
{
	if (self->velocity[2] < 0)
		self->gravity = 2.25f * (800.f / level.gravity);
	else
		self->gravity = 5.25f * (800.f / level.gravity);
}

void hellberserk_jump_takeoff(edict_t* self)
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
	self->touch = hellberserk_jump_touch;
	hellberserk_high_gravity(self);
}

void hellberserk_check_landing(edict_t* self)
{
	hellberserk_high_gravity(self);

	if (self->groundentity)
	{
		self->monsterinfo.attack_finished = 0_ms;
		self->monsterinfo.unduck(self);
		self->s.frame = 165;
		if (self->touch)
		{
			hellberserk_attack_slam(self);
			self->touch = nullptr;
		}
		self->flags &= ~FL_KILL_VELOCITY;
		return;
	}

	if (level.time > self->monsterinfo.attack_finished)
		self->monsterinfo.nextframe = 158;
	else
		self->monsterinfo.nextframe = 160;
}

mframe_t hknight_bsk_frames_attack[] = {
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_move, 0, hellberserk_jump_takeoff },
	{ai_move, 0, hellberserk_high_gravity },
	{ai_move, 0, hellberserk_check_landing },
	{ai_move, 0, monster_footstep },
	{ai_move},
	{ai_move, 0, monster_footstep },
	{ai_charge, 0, monster_footstep },
	{ai_move},
	{ai_move}
};
MMOVE_T(hknight_bsk_move_attack) = {155, 165, hknight_bsk_frames_attack, hknight_bsk_run };

// Slice
mframe_t hknight_bsk_frames_slice [] =
{
	{ai_charge, 9,	NULL},
	{ai_charge, 6,	NULL},
	{ai_charge, 13,	NULL},
	{ai_charge, 4,	NULL},
	{ai_charge, 7,	SwingSword2},
	{ai_charge, 15,	SwingSword2},
	{ai_charge, 8,	SwingSword2},
	{ai_charge, 2,	SwingSword2},
	{ai_charge, 0,	SwingSword2},
	{ai_charge, 3,	NULL}
};
MMOVE_T(hknight_bsk_move_slice) = {112, 121, hknight_bsk_frames_slice, hknight_bsk_run};

// Smash
mframe_t hknight_bsk_frames_smash [] =
{
	{ai_charge, 1,	NULL},
	{ai_charge, 13,	NULL},
	{ai_charge, 9,	NULL},
	{ai_charge, 11,	NULL},
	{ai_charge, 10,	SwingSword2},
	{ai_charge, 7,	SwingSword2},
	{ai_charge, 12,	SwingSword2},
	{ai_charge, 2,	SwingSword2},
	{ai_charge, 3,	SwingSword2},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL}
};
MMOVE_T(hknight_bsk_move_smash) = {122, 132, hknight_bsk_frames_smash, hknight_bsk_run};

// Watk
mframe_t hknight_bsk_frames_watk [] =
{
	{ai_charge, 2, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, SwingSword2},
	{ai_charge, 0, SwingSword2},
	{ai_charge, 0, SwingSword2},
	{ai_charge, 1, NULL},
	{ai_charge, 4, NULL},
	{ai_charge, 5, NULL},
	{ai_charge, 3, SwingSword2},
	{ai_charge, 2, SwingSword2},
	{ai_charge, 2, SwingSword2},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 1, NULL},
	{ai_charge, 1, SwingSword2},
	{ai_charge, 3, SwingSword2},
	{ai_charge, 4, SwingSword2},
	{ai_charge, 6, NULL},
	{ai_charge, 7, NULL},
	{ai_charge, 3, NULL}
};
MMOVE_T(hknight_bsk_move_watk) = {133, 154, hknight_bsk_frames_watk, hknight_bsk_run};

// Melee
MONSTERINFO_MELEE (hknight_bsk_melee) (edict_t *self) -> void
{
	self->dmg_radius++;
	
	if (self->dmg_radius == 1)
		M_SetAnimation(self, &hknight_bsk_move_slice);	
	else if (self->dmg_radius == 2)
		M_SetAnimation(self, &hknight_bsk_move_smash);		
	else if (self->dmg_radius == 3)
	{
		M_SetAnimation(self, &hknight_bsk_move_watk);				
		self->dmg_radius = 0;
	}
	gi.sound(self, CHAN_WEAPON, sound_melee, 1, ATTN_NORM, 0);
}

MONSTERINFO_ATTACK(hknight_bsk_attack) (edict_t* self) -> void
{
	if (self->monsterinfo.melee_debounce_time <= level.time && (range_to(self, self->enemy) < MELEE_DISTANCE))
		hknight_bsk_melee(self);
	else if (!self->spawnflags.has(SPAWNFLAG_BERSERK_NOJUMPING) && (self->timestamp < level.time) && range_to(self, self->enemy) > 50.f)
	{
		M_SetAnimation(self, &hknight_bsk_move_attack);
		gi.sound(self, CHAN_WEAPON, sound_jump, 1, ATTN_NORM, 0);
		self->timestamp = level.time + 5_sec;
	}
}

// Pain
mframe_t hknight_bsk_frames_pain [] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
MMOVE_T(hknight_bsk_move_pain)= {37, 41, hknight_bsk_frames_pain, hknight_bsk_run};

PAIN (hknight_bsk_pain)(edict_t* self, edict_t* other, float kick, int damage, const mod_t& mod) -> void
{
	if (skill->value == 3)
		return;
	if (level.time < self->pain_debounce_time)
		return;
	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

	self->pain_debounce_time = level.time + 1_sec;
	M_SetAnimation(self, &hknight_bsk_move_pain);					
}

void hknight_bsk_dead(edict_t *self)
{
	self->mins = { -16, -16, -24 };	
	self->maxs = { 16, 16, -8 };	
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

// Death (1)
mframe_t hknight_bsk_frames_die1 [] =
{
	{ai_move, 0,		NULL},
	{ai_move, 10,	    NULL},
	{ai_move, 8,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 10,	    NULL},
	{ai_move, 11,	    NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
};
MMOVE_T(hknight_bsk_move_die1) = {42, 53, hknight_bsk_frames_die1, hknight_bsk_dead};

// Death (2)
mframe_t hknight_bsk_frames_die2 [] =
{
	{ai_move, 0,		NULL },
	{ai_move, 0,		NULL },
	{ai_move, 0,		NULL },
	{ai_move, 0,		NULL },
	{ai_move, 0,		NULL },
	{ai_move, 0,		NULL },
	{ai_move, 0,		NULL },
	{ai_move, 0,		NULL },
	{ai_move, 0,		NULL }
};
MMOVE_T(hknight_bsk_move_die2) = {54, 62, hknight_bsk_frames_die2, hknight_bsk_dead};

DIE (hknight_bsk_die)(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t& mod) -> void
{
	if (self->health <= self->gib_health){
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		ThrowGibs(self, damage, {
			{ "models/objects/gibs/bone/tris.md2" },
			{ "models/objects/gibs/sm_meat/tris.md2" },
			{ "models/monsters/hellberserkerstrogg/gibs/g_arm.md2" },
			{ "models/monsters/hellberserkerstrogg/gibs/g_arm.md2" },
			{ "models/monsters/hellberserkerstrogg/gibs/g_leg.md2" },
			{ "models/monsters/hellberserkerstrogg/gibs/g_leg.md2" },
			{ "models/monsters/hellberserkerstrogg/gibs/g_head.md2", GIB_HEAD }
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
		M_SetAnimation(self, &hknight_bsk_move_die1);
	else
		M_SetAnimation(self, &hknight_bsk_move_die2);	
}

// Sight
MONSTERINFO_SIGHT (hknight_bsk_sight) (edict_t* self, edict_t* other) -> void
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

// Search
MONSTERINFO_SEARCH(hknight_bsk_search) (edict_t *self) -> void
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

MONSTERINFO_SETSKIN(hknight_bsk_setskin) (edict_t* self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}

void SP_monster_hknight_bsk_strogg(edict_t *self)
{
	if (!M_AllowSpawn(self)) {
		G_FreeEdict(self);
		return;
	}

	sound_attack.assign("hknight/attack1.wav");
	sound_melee.assign("hknight/slash1.wav");
	sound_death.assign("hknight/death1_s.wav");
	sound_proj_hit.assign("hknight/hit.wav");
	sound_sight.assign("hknight/sight1_s.wav");
	sound_search.assign("hknight/idle_s.wav");
	sound_pain.assign("hknight/pain1_s.wav");
	sound_thud.assign("mutant/thud1.wav");
	sound_explod.assign("world/explod2.wav");
	sound_jump.assign("berserk/jump.wav");

	self->mins = {-16,-16,-24};
	self->maxs = {16,16,40};	
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->s.modelindex = gi.modelindex("models/monsters/hellberserkerstrogg/tris.md2");
	gi.modelindex("models/monsters/hellberserkerstrogg/gibs/g_arm.md2");
	gi.modelindex("models/monsters/hellberserkerstrogg/gibs/g_leg.md2");
	gi.modelindex("models/monsters/hellberserkerstrogg/gibs/g_head.md2");


	self->health = 250 * st.health_multiplier;
	self->gib_health = -40;

	self->mass = 250;

	self->pain = hknight_bsk_pain;
	self->die = hknight_bsk_die;
	self->monsterinfo.stand = hknight_bsk_stand;
	self->monsterinfo.walk = hknight_bsk_walk;
	self->monsterinfo.run = hknight_bsk_run;
	self->monsterinfo.attack = hknight_bsk_attack;
	self->monsterinfo.melee = hknight_bsk_melee;
	self->monsterinfo.sight = hknight_bsk_sight;
	self->monsterinfo.search = hknight_bsk_search;
	self->monsterinfo.setskin = hknight_bsk_setskin;
	self->monsterinfo.dodge = M_MonsterDodge;
	
	M_SetAnimation(self, &hknight_bsk_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;	

	self->monsterinfo.combat_style = COMBAT_MELEE;
	self->monsterinfo.can_jump = !self->spawnflags.has(SPAWNFLAG_BERSERK_NOJUMPING);
	self->monsterinfo.drop_height = 256;
	self->monsterinfo.jump_height = 40;

	gi.linkentity(self);

	walkmonster_start(self);
}
