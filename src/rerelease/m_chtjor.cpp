// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

chtjor

==============================================================================
*/

#include "g_local.h"
#include "m_chtjor.h"
#include "m_flash.h"

static cached_soundindex sound_pain1;
static cached_soundindex sound_pain2;
static cached_soundindex sound_pain3;
static cached_soundindex sound_idle;
static cached_soundindex sound_death;
static cached_soundindex sound_search1;
static cached_soundindex sound_search2;
static cached_soundindex sound_search3;
static cached_soundindex sound_attack1, sound_attack1_loop, sound_attack1_end;
static cached_soundindex sound_attack2, sound_bfg_fire;
static cached_soundindex sound_firegun;
static cached_soundindex sound_step_left;
static cached_soundindex sound_step_right;
static cached_soundindex sound_death_hit;

void monster_muzzleflasher(edict_t* self, const vec3_t& start, monster_muzzleflash_id_t id)
{
	float r = frandom();

	if (r <= 0.5f){
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_EXPLOSION1);
		gi.WritePosition(start);
		gi.multicast(self->s.origin, MULTICAST_PHS, false);
	}
}

void monster_fire_rocket_chtjor(edict_t* self, const vec3_t& start, const vec3_t& dir, int damage, int speed, monster_muzzleflash_id_t flashtype)
{
	monster_fire_rocket(self, start, dir, 50, 650, flashtype);
	monster_muzzleflasher(self, start, flashtype);
}

void monster_fire_bfg_chtjor(edict_t* self, const vec3_t& start, const vec3_t& aimdir, int damage, int speed, int kick,	float damage_radius, monster_muzzleflash_id_t flashtype)
{
	fire_bfg(self, start, aimdir, damage, speed, damage_radius);
}

void chtjor_attack1_end_sound(edict_t *self)
{
	if (self->monsterinfo.weapon_sound)
	{
		gi.sound(self, CHAN_WEAPON, sound_attack1_end, 1, ATTN_NORM, 0);
		self->monsterinfo.weapon_sound = 0;
	}
}

MONSTERINFO_SEARCH(chtjor_search) (edict_t *self) -> void
{
	float r;

	r = frandom();

	if (r <= 0.3f)
		gi.sound(self, CHAN_VOICE, sound_search1, 1, ATTN_NORM, 0);
	else if (r <= 0.6f)
		gi.sound(self, CHAN_VOICE, sound_search2, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_search3, 1, ATTN_NORM, 0);
}

void chtjor_dead(edict_t *self);
void chtjorBFG(edict_t *self);
void chtjor_firebullet(edict_t *self);
void chtjor_reattack1(edict_t *self);
void chtjor_attack1(edict_t *self);
void chtjor_idle(edict_t *self);
void chtjor_step_left(edict_t *self);
void chtjor_step_right(edict_t *self);
void chtjor_death_hit(edict_t *self);

//
// stand
//
mframe_t chtjor_frames_stand[] = {
	{ ai_stand, 0, chtjor_idle },
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
	{ ai_stand, 19 },
	{ ai_stand, 11, chtjor_step_left },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand, 6 },
	{ ai_stand, 9, chtjor_step_right },
	{ ai_stand }, // 40
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand, -2, nullptr },
	{ ai_stand, -17, chtjor_step_left },
	{ ai_stand },
	{ ai_stand, -12 },				   // 50
	{ ai_stand, -14, chtjor_step_right } // 51
};
MMOVE_T(chtjor_move_stand) = { FRAME_stand01, FRAME_stand51, chtjor_frames_stand, nullptr };

void chtjor_idle (edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_NORM, 0);
}

void chtjor_death_hit(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_death_hit, 1, ATTN_NORM, 0);
}

void chtjor_step_left(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_step_left, 1, ATTN_NORM, 0);
}

void chtjor_step_right(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_step_right, 1, ATTN_NORM, 0);
}

MONSTERINFO_STAND(chtjor_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &chtjor_move_stand);

	chtjor_attack1_end_sound(self);
}

/*void chtjorstep(edict_t* self) {
	uint32_t i;
	edict_t* e;

	for (i = 1; i <= MAX_CLIENTS; i++) {
		e = &g_edicts[i];
		if (e->inuse && e->client) {
			float r = range_to(self, e);
			if (r <= RANGE_MELEE){
				r = 1.0;
			}else {
				r = r * 0.0025;
			}

			vec3_t& angles = e->client->ps.kick_angles;
			e->client->v_dmg_pitch = (-100 * 0.10f) / r;
			e->client->v_dmg_roll = 0.10 / r;
			e->client->v_dmg_time = level.time + 0.1_sec;
			e->client->quake_time = (level.time + 100_ms) / r;

			float factor = min(1.0f, (e->client->quake_time.seconds() / level.time.seconds()) * 0.25f);
			factor = factor / r;
			angles.x += crandom_open() * factor * 100;
			angles.z += crandom_open() * factor * 100;
			angles.y += crandom_open() * factor * 100;
		}
	}
}*/

mframe_t chtjor_frames_run[] = {
	{ ai_run, 17, chtjor_step_left },
	{ ai_run, 0},
	{ ai_run },
	{ ai_run, 0},
	{ ai_run, 12 },
	{ ai_run, 8 },
	{ ai_run, 10 },
	{ ai_run, 33, chtjor_step_right },
	{ ai_run, 0},
	{ ai_run },
	{ ai_run, 0},
	{ ai_run, 9 },
	{ ai_run, 9 },
	{ ai_run, 9 }
};
MMOVE_T(chtjor_move_run) = { FRAME_walk06, FRAME_walk19, chtjor_frames_run, nullptr };

//
// walk
//
#if 0
mframe_t chtjor_frames_start_walk[] = {
	{ ai_walk, 5 },
	{ ai_walk, 6 },
	{ ai_walk, 7 },
	{ ai_walk, 9 },
	{ ai_walk, 15 }
};
MMOVE_T(chtjor_move_start_walk) = { FRAME_walk01, FRAME_walk05, chtjor_frames_start_walk, nullptr };
#endif

mframe_t chtjor_frames_walk[] = {
	{ ai_walk, 17 },
	{ ai_walk, 0 },
	{ ai_walk },
	{ ai_walk, 0 },
	{ ai_walk, 12 },
	{ ai_walk, 8 },
	{ ai_walk, 10 },
	{ ai_walk, 33 },
	{ ai_walk, 0 },
	{ ai_walk },
	{ ai_walk, 0 },
	{ ai_walk, 9 },
	{ ai_walk, 9 },
	{ ai_walk, 9 }
};
MMOVE_T(chtjor_move_walk) = { FRAME_walk06, FRAME_walk19, chtjor_frames_walk, nullptr };

#if 0
mframe_t chtjor_frames_end_walk[] = {
	{ ai_walk, 11 },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk, 8 },
	{ ai_walk, -8 }
};
MMOVE_T(chtjor_move_end_walk) = { FRAME_walk20, FRAME_walk25, chtjor_frames_end_walk, nullptr };
#endif

MONSTERINFO_WALK(chtjor_walk) (edict_t *self) -> void
{
	M_SetAnimation(self, &chtjor_move_walk);
}

MONSTERINFO_RUN(chtjor_run) (edict_t *self) -> void
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		M_SetAnimation(self, &chtjor_move_stand);
	else
		M_SetAnimation(self, &chtjor_move_run);

	chtjor_attack1_end_sound(self);
}

mframe_t chtjor_frames_pain3[] = {
	{ ai_move, -28 },
	{ ai_move, -6 },
	{ ai_move, -3, chtjor_step_left },
	{ ai_move, -9 },
	{ ai_move, 0, chtjor_step_right },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, -7 },
	{ ai_move, 1 },
	{ ai_move, -11 },
	{ ai_move, -4 },
	{ ai_move },
	{ ai_move },
	{ ai_move, 10 },
	{ ai_move, 11 },
	{ ai_move },
	{ ai_move, 10 },
	{ ai_move, 3 },
	{ ai_move, 10 },
	{ ai_move, 7, chtjor_step_left },
	{ ai_move, 17 },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, chtjor_step_right }
};
MMOVE_T(chtjor_move_pain3) = { FRAME_pain301, FRAME_pain325, chtjor_frames_pain3, chtjor_run };

mframe_t chtjor_frames_pain2[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(chtjor_move_pain2) = { FRAME_pain201, FRAME_pain203, chtjor_frames_pain2, chtjor_run };

mframe_t chtjor_frames_pain1[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(chtjor_move_pain1) = { FRAME_pain101, FRAME_pain103, chtjor_frames_pain1, chtjor_run };

mframe_t chtjor_frames_death1[] = {
	{ ai_move, 0, BossExplode },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, -2 },
	{ ai_move, -5 },
	{ ai_move, -8 },
	{ ai_move, -15, chtjor_step_left },
	{ ai_move }, // 10
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, -11 },
	{ ai_move, -25 },
	{ ai_move, -10, chtjor_step_right },
	{ ai_move },
	{ ai_move }, // 20
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, -21 },
	{ ai_move, -10 },
	{ ai_move, -16, chtjor_step_left },
	{ ai_move },
	{ ai_move },
	{ ai_move }, // 30
	{ ai_move },
	{ ai_move },
	{ ai_move, 22 },
	{ ai_move, 33, chtjor_step_left },
	{ ai_move },
	{ ai_move },
	{ ai_move, 28 },
	{ ai_move, 28, chtjor_step_right },
	{ ai_move },
	{ ai_move }, // 40
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, -19 },
	{ ai_move, 0, chtjor_death_hit },
	{ ai_move },
	{ ai_move } // 50
};
MMOVE_T(chtjor_move_death) = { FRAME_death01, FRAME_death50, chtjor_frames_death1, chtjor_dead };

mframe_t chtjor_frames_attack2[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, chtjorBFG },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(chtjor_move_attack2) = { FRAME_attak201, FRAME_attak213, chtjor_frames_attack2, chtjor_run };

mframe_t chtjor_frames_start_attack1[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }
};
MMOVE_T(chtjor_move_start_attack1) = { FRAME_attak101, FRAME_attak108, chtjor_frames_start_attack1, chtjor_attack1 };

mframe_t chtjor_frames_attack1[] = {
	{ ai_charge, 0, chtjor_firebullet },
	{ ai_charge},
	{ ai_charge, 0, chtjor_firebullet },
	{ ai_charge},
	{ ai_charge, 0, chtjor_firebullet },
	{ ai_charge}
};
MMOVE_T(chtjor_move_attack1) = { FRAME_attak109, FRAME_attak114, chtjor_frames_attack1, chtjor_reattack1 };

mframe_t chtjor_frames_end_attack1[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(chtjor_move_end_attack1) = { FRAME_attak115, FRAME_attak118, chtjor_frames_end_attack1, chtjor_run };

void chtjor_reattack1(edict_t *self)
{
	if (visible(self, self->enemy))
	{
		if (frandom() < 0.9f)
			M_SetAnimation(self, &chtjor_move_attack1);
		else
		{
			M_SetAnimation(self, &chtjor_move_end_attack1);
			chtjor_attack1_end_sound(self);
		}
	}
	else
	{
		M_SetAnimation(self, &chtjor_move_end_attack1);
		chtjor_attack1_end_sound(self);
	}
}

void chtjor_attack1(edict_t *self)
{
	M_SetAnimation(self, &chtjor_move_attack1);
}

PAIN(chtjor_pain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
{
	if (level.time < self->pain_debounce_time)
		return;

	// Lessen the chance of him going into his pain frames if he takes little damage
	if (mod.id != MOD_CHAINFIST)
	{
		if (damage <= 40)
			if (frandom() <= 0.6f)
				return;

		if ((self->s.frame >= FRAME_attak101) && (self->s.frame <= FRAME_attak108))
			if (frandom() <= 0.005f)
				return;

		if ((self->s.frame >= FRAME_attak109) && (self->s.frame <= FRAME_attak114))
			if (frandom() <= 0.00005f)
				return;

		if ((self->s.frame >= FRAME_attak201) && (self->s.frame <= FRAME_attak208))
			if (frandom() <= 0.005f)
				return;
	}

	self->pain_debounce_time = level.time + 3_sec;

	bool do_pain3 = false;

	if (damage > 50)
	{
		if (damage <= 100)
		{
			gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
		}
		else
		{
			if (frandom() <= 0.3f)
			{
				do_pain3 = true;
				gi.sound(self, CHAN_VOICE, sound_pain3, 1, ATTN_NORM, 0);
			}
		}
	}

	if (!M_ShouldReactToPain(self, mod))
		return; // no pain anims in nightmare
	
	chtjor_attack1_end_sound(self);

	if (damage <= 50)
		M_SetAnimation(self, &chtjor_move_pain1);
	else if (damage <= 100)
		M_SetAnimation(self, &chtjor_move_pain2);
	else if (do_pain3)
		M_SetAnimation(self, &chtjor_move_pain3);
}

MONSTERINFO_SETSKIN(chtjor_setskin) (edict_t* self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}

void chtjorBFG(edict_t *self)
{
	vec3_t forward, right;
	vec3_t start;
	vec3_t dir;
	vec3_t vec;

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, { -40.3f, 230.f, 300.2f }, forward, right);

	vec = self->enemy->s.origin;
	vec[2] += self->enemy->viewheight;
	dir = vec - start;
	dir.normalize();
	gi.sound(self, CHAN_WEAPON, sound_bfg_fire, 1, ATTN_NORM, 0);
	monster_fire_bfg_chtjor(self, start, dir, 50, 300, 100, 200, MZ2_JORG_BFG_1);
}

void chtjor_firebullet_right(edict_t *self)
{
	vec3_t forward, right, start;
	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, { 158.5f, 86.7f, 150.f }, forward, right);
	PredictAim(self, self->enemy, start, 0, false, -0.2f, &forward, nullptr);
	monster_fire_rocket_chtjor(self, start, forward, 50, 650, MZ2_UNUSED_0);
}

void chtjor_firebullet_left(edict_t *self)
{
	vec3_t forward, right, start;
	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, { 158.5f, -86.7f, 150.f }, forward, right);
	PredictAim(self, self->enemy, start, 0, false, 0.2f, &forward, nullptr);
	monster_fire_rocket_chtjor(self, start, forward, 50, 650, MZ2_UNUSED_0);
}

void chtjor_firebullet(edict_t *self)
{
	chtjor_firebullet_left(self);
	chtjor_firebullet_right(self);
};

MONSTERINFO_ATTACK(chtjor_attack) (edict_t *self) -> void
{
	if (frandom() <= 0.75f) {
		gi.sound(self, CHAN_WEAPON, sound_attack1, 1, ATTN_NORM, 0);
		self->monsterinfo.weapon_sound = gi.soundindex("boss3/w_loop.wav");
		M_SetAnimation(self, &chtjor_move_start_attack1);
	}else{
		gi.sound(self, CHAN_VOICE, sound_attack2, 1, ATTN_NORM, 0);
		M_SetAnimation(self, &chtjor_move_attack2);
	}
}

void chtjor_dead(edict_t *self)
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1_BIG);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PHS, false);

	self->s.sound = 0;

	ThrowGibs(self, 500, {
		{ 2, "models/objects/gibs/sm_meat/tris.md2" },
		{ 2, "models/objects/gibs/sm_metal/tris.md2", GIB_METALLIC },
		{ "models/monsters/chtjor/gibs/chest.md2", GIB_SKINNED },
		{ 2, "models/monsters/chtjor/gibs/foot.md2", GIB_SKINNED },
		{ 2, "models/monsters/chtjor/gibs/gun.md2", GIB_SKINNED | GIB_UPRIGHT },
		{ 2, "models/monsters/chtjor/gibs/thigh.md2", GIB_SKINNED | GIB_UPRIGHT },
		{ "models/monsters/chtjor/gibs/spine.md2", GIB_SKINNED | GIB_UPRIGHT },
		{ 4, "models/monsters/chtjor/gibs/tube.md2", GIB_SKINNED },
		{ 6, "models/monsters/chtjor/gibs/spike.md2", GIB_SKINNED },
		{ "models/monsters/chtjor/gibs/head.md2", GIB_SKINNED | GIB_METALLIC | GIB_HEAD }
	});
}

DIE(chtjor_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	chtjor_attack1_end_sound(self);
	self->deadflag = true;
	self->takedamage = false;
	self->count = 0;
	M_SetAnimation(self, &chtjor_move_death);
}

MONSTERINFO_CHECKATTACK(chtjor_CheckAttack) (edict_t *self) -> bool
{
	return M_CheckAttack_Base(self, 0.4f, 0.8f, 0.6f, 0.4f, 0.2f, 0.f);
}

/*QUAKED monster_chtjor (1 .5 0) (-80 -80 0) (90 90 140) Ambush Trigger_Spawn Sight */
void SP_monster_chtjor(edict_t *self)
{
	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict( self );
		return;
	}

	sound_pain1.assign("boss3/bs3pain1.wav");
	sound_pain2.assign("boss3/bs3pain2.wav");
	sound_pain3.assign("boss3/bs3pain3.wav");
	sound_death.assign("boss3/bs3deth1.wav");
	sound_attack1.assign("boss3/bs3atck1.wav");
	sound_attack1_loop.assign("boss3/bs3atck1_loop.wav");
	sound_attack1_end.assign("boss3/bs3atck1_end.wav");
	sound_attack2.assign("boss3/bs3atck2.wav");
	sound_search1.assign("boss3/bs3srch1.wav");
	sound_search2.assign("boss3/bs3srch2.wav");
	sound_search3.assign("boss3/bs3srch3.wav");
	sound_idle.assign("boss3/bs3idle1.wav");
	sound_step_left.assign("boss3/step1.wav");
	sound_step_right.assign("boss3/step2.wav");
	sound_firegun.assign("boss3/xfire.wav");
	sound_death_hit.assign("boss3/d_hit.wav");
	sound_bfg_fire.assign("makron/bfg_fire.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/chtjor/tris.md2");
	
	gi.modelindex("models/monsters/chtjor/gibs/chest.md2");
	gi.modelindex("models/monsters/chtjor/gibs/foot.md2");
	gi.modelindex("models/monsters/chtjor/gibs/gun.md2");
	gi.modelindex("models/monsters/chtjor/gibs/head.md2");
	gi.modelindex("models/monsters/chtjor/gibs/spike.md2");
	gi.modelindex("models/monsters/chtjor/gibs/spine.md2");
	gi.modelindex("models/monsters/chtjor/gibs/thigh.md2");
	gi.modelindex("models/monsters/chtjor/gibs/tube.md2");

	self->mins = {-100, -100, 0};
	self->maxs = { 100, 100, 350};

	self->health = 12000 * st.health_multiplier;
	self->gib_health = -2000;
	self->mass = 1000;

	self->pain = chtjor_pain;
	self->die = chtjor_die;
	self->monsterinfo.stand = chtjor_stand;
	self->monsterinfo.walk = chtjor_walk;
	self->monsterinfo.run = chtjor_run;
	self->monsterinfo.dodge = nullptr;
	self->monsterinfo.attack = chtjor_attack;
	self->monsterinfo.search = chtjor_search;
	self->monsterinfo.melee = nullptr;
	self->monsterinfo.sight = nullptr;
	self->monsterinfo.checkattack = chtjor_CheckAttack;
	self->monsterinfo.setskin = chtjor_setskin;
	gi.linkentity(self);

	M_SetAnimation(self, &chtjor_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);
	self->monsterinfo.aiflags |= AI_IGNORE_SHOTS;
	self->monsterinfo.aiflags |= AI_DOUBLE_TROUBLE;
}
