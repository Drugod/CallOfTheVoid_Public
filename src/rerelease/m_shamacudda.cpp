// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

shamacudda

==============================================================================
*/

#include "g_local.h"
#include "m_shamacudda.h"
#include "m_flash.h"

static cached_soundindex sound_pain;
static cached_soundindex sound_idle;
static cached_soundindex sound_die;
static cached_soundindex sound_sight;
static cached_soundindex sound_windup;
static cached_soundindex sound_melee1;
static cached_soundindex sound_melee2;
static cached_soundindex sound_smack;
static cached_soundindex sound_boom;
static cached_soundindex sound_bfg_fire;

//
// misc
//

MONSTERINFO_SIGHT(shamacudda_sight) (edict_t* self, edict_t* other) -> void
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

constexpr vec3_t lightning_left_hand[] = {
	{ 44, 36, 25 },
	{ 10, 44, 57 },
	{ -1, 40, 70 },
	{ -10, 34, 75 },
	{ 7.4f, 24, 89 }
};

constexpr vec3_t lightning_right_hand[] = {
	{ 28, -38, 25 },
	{ 31, -7, 70 },
	{ 20, 0, 80 },
	{ 16, 1.2f, 81 },
	{ 27, -11, 83 }
};

/*static void shamacudda_lightning_update(edict_t* self)
{
	edict_t *lightning = self->beam;

	if (self->s.frame >= FRAME_magic01 + q_countof(lightning_left_hand))
	{
		G_FreeEdict(lightning);
		self->beam = nullptr;
		return;
	}

	vec3_t f, r;
	AngleVectors(self->s.angles, f, r, nullptr);
	lightning->s.origin = M_ProjectFlashSource(self, lightning_left_hand[self->s.frame - FRAME_magic01], f, r);
	lightning->s.old_origin = M_ProjectFlashSource(self, lightning_right_hand[self->s.frame - FRAME_magic01], f, r);
	gi.linkentity(lightning);
}*/

static void shamacudda_lightning_update(edict_t* self)
{
	edict_t* lightningL = self->beam;
	edict_t* lightningR = self->beam2;

	if (self->s.frame >= FRAME_magic01 + q_countof(lightning_left_hand))
	{
		if (lightningL) { G_FreeEdict(lightningL); self->beam = nullptr; }
		if (lightningR) { G_FreeEdict(lightningR); self->beam2 = nullptr; }
		return;
	}

	vec3_t f, r;
	AngleVectors(self->s.angles, f, r, nullptr);

	if (lightningL) {
		lightningL->s.origin = M_ProjectFlashSource(self,
			lightning_left_hand[self->s.frame - FRAME_magic01], f, r);
		gi.linkentity(lightningL);
	}

	if (lightningR) {
		lightningR->s.origin = M_ProjectFlashSource(self,
			lightning_right_hand[self->s.frame - FRAME_magic01], f, r);
		gi.linkentity(lightningR);
	}
}


/*void shamacudda_windup(edict_t* self)
{
	gi.sound(self, CHAN_WEAPON, sound_windup, 1, ATTN_NORM, 0);

	edict_t *lightning = self->beam = G_Spawn();
	lightning->s.scale = 0.5f;
	lightning->s.modelindex = gi.modelindex("sprites/s_bfg1.sp2");
	lightning->owner = self;
	shamacudda_lightning_update(self);
}*/

void shamacudda_windup(edict_t* self)
{
	gi.sound(self, CHAN_WEAPON, sound_windup, 1, ATTN_NORM, 0);

	// bola en mano izquierda
	edict_t* lightningL = self->beam = G_Spawn();
	lightningL->s.scale = 0.5f;
	lightningL->s.modelindex = gi.modelindex("sprites/s_bfx1.sp2");
	lightningL->owner = self;

	// bola en mano derecha
	edict_t* lightningR = self->beam2 = G_Spawn();
	lightningR->s.scale = 0.5f;
	lightningR->s.modelindex = gi.modelindex("sprites/s_bfx1.sp2");
	lightningR->owner = self;

	shamacudda_lightning_update(self);
}

MONSTERINFO_IDLE(shamacudda_idle) (edict_t* self) -> void
{
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

void shamacudda_maybe_idle(edict_t* self)
{
	if (frandom() > 0.8)
		gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

//
// stand
//

mframe_t shamacudda_frames_stand[] = {
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
MMOVE_T(shamacudda_move_stand) = { FRAME_stand01, FRAME_stand17, shamacudda_frames_stand, nullptr };

MONSTERINFO_STAND(shamacudda_stand) (edict_t* self) -> void
{
	M_SetAnimation(self, &shamacudda_move_stand);
}

//
// walk
//

void shamacudda_walk(edict_t* self);

mframe_t shamacudda_frames_walk[] = {
	{ ai_walk, 10 }, // FIXME: add footsteps?
	{ ai_walk, 9 },
	{ ai_walk, 9 },
	{ ai_walk, 5 },
	{ ai_walk, 6 },
	{ ai_walk, 12 },
	{ ai_walk, 8 },
	{ ai_walk, 3 },
	{ ai_walk, 13 },
	{ ai_walk, 9 },
	{ ai_walk, 7, shamacudda_maybe_idle },
	{ ai_walk, 5 },
};
MMOVE_T(shamacudda_move_walk) = { FRAME_walk01, FRAME_walk12, shamacudda_frames_walk, nullptr };

MONSTERINFO_WALK(shamacudda_walk) (edict_t* self) -> void
{
	M_SetAnimation(self, &shamacudda_move_walk);
}

//
// run
//

void shamacudda_run(edict_t* self);

mframe_t shamacudda_frames_run[] = {
	{ ai_run, 20 }, // FIXME: add footsteps?
	{ ai_run, 24 },
	{ ai_run, 20 },
	{ ai_run, 20 },
	{ ai_run, 24 },
	{ ai_run, 20, shamacudda_maybe_idle },
};
MMOVE_T(shamacudda_move_run) = { FRAME_run01, FRAME_run06, shamacudda_frames_run, nullptr };

MONSTERINFO_RUN(shamacudda_run) (edict_t* self) -> void
{
	if (self->enemy && self->enemy->client)
		self->monsterinfo.aiflags |= AI_BRUTAL;
	else
		self->monsterinfo.aiflags &= ~AI_BRUTAL;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		M_SetAnimation(self, &shamacudda_move_stand);
		return;
	}

	M_SetAnimation(self, &shamacudda_move_run);
}

//
// pain
//

// FIXME: needs halved explosion damage

mframe_t shamacudda_frames_pain[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
};
MMOVE_T(shamacudda_move_pain) = { FRAME_pain01, FRAME_pain06, shamacudda_frames_pain, shamacudda_run };

PAIN(shamacudda_pain) (edict_t* self, edict_t* other, float kick, int damage, const mod_t &mod) -> void
{
	if (level.time < self->timestamp)
		return;

	self->timestamp = level.time + 1_ms;
	gi.sound(self, CHAN_AUTO, sound_pain, 1, ATTN_NORM, 0);

	if (mod.id != MOD_CHAINFIST && damage <= 30 && frandom() > 0.2f)
		return;

	// If hard or nightmare, don't go into pain while attacking
	if (skill->integer >= 2)
	{
		if ((self->s.frame >= FRAME_smash01) && (self->s.frame <= FRAME_smash12))
			return;

		if ((self->s.frame >= FRAME_swingl01) && (self->s.frame <= FRAME_swingl09))
			return;

		if ((self->s.frame >= FRAME_swingr01) && (self->s.frame <= FRAME_swingr09))
			return;
	}
	
	if (!M_ShouldReactToPain(self, mod))
		return; // no pain anims in nightmare

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 2_sec;
	M_SetAnimation(self, &shamacudda_move_pain);
}

MONSTERINFO_SETSKIN(shamacudda_setskin) (edict_t* self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}

//
// attacks
//
void shamacuddaSaveLoc(edict_t* self)
{
	self->pos1 = self->enemy->s.origin; // save for aiming the shot
	self->pos1[2] += self->enemy->viewheight;
	self->monsterinfo.nextframe = FRAME_magic09;

	gi.sound(self, CHAN_WEAPON, sound_boom, 1, ATTN_NORM, 0);
	shamacudda_lightning_update(self);
}

constexpr spawnflags_t SPAWNFLAG_SHAMBLER_PRECISE = 1_spawnflag;

vec3_t FindshamacuddaOffset(edict_t *self)
{
	vec3_t offset = { 0, 0, 48.f };

	for (int i = 0; i < 8; i++)
	{
		if (M_CheckClearShot(self, offset))
			return offset;

		offset.z -= 4.f;
	}

	return { 0, 0, 48.f };
}

void monster_fire_bfg_shama(edict_t* self, const vec3_t& start, const vec3_t& aimdir, int damage, int speed, int kick, float damage_radius, monster_muzzleflash_id_t flashtype)
{
	fire_bfg_shama(self, start, aimdir, damage, speed, damage_radius);
}


void shamacuddaCastLightning(edict_t* self)
{
	if (!self->enemy)
		return;

	vec3_t start;
	vec3_t dir;
	vec3_t forward, right;
	vec3_t offset = FindshamacuddaOffset(self);

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, offset, forward, right);

	// calc direction to where we targted
	PredictAim(self, self->enemy, start, 0, false, self->spawnflags.has(SPAWNFLAG_SHAMBLER_PRECISE) ? 0.f : 0.1f, &dir, nullptr);

	vec3_t end = start + (dir * 8192);
	trace_t tr = gi.traceline(start, end, self, MASK_PROJECTILE | CONTENTS_SLIME | CONTENTS_LAVA);

	/*gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_LIGHTNING);
	gi.WriteEntity(self);	// source entity
	gi.WriteEntity(world); // destination entity
	gi.WritePosition(start);
	gi.WritePosition(tr.endpos);
	gi.multicast(start, MULTICAST_PVS, false);*/

	//fire_bullet(self, start, dir, irandom(8, 12), 15, 0, 0, MOD_TESLA);

	gi.sound(self, CHAN_WEAPON, sound_bfg_fire, 1, ATTN_NORM, 0);
	monster_fire_bfg_shama(self, start, dir, 50, 300, 100, 200, MZ2_JORG_BFG_1);
}

mframe_t shamacudda_frames_magic[] = {
	{ ai_charge, 0, shamacudda_windup },
	{ ai_charge, 0, shamacudda_lightning_update },
	{ ai_charge, 0, shamacudda_lightning_update },
	{ ai_move, 0, shamacudda_lightning_update },
	{ ai_move, 0, shamacudda_lightning_update },
	{ ai_move, 0, shamacuddaSaveLoc},
	{ ai_move },
	{ ai_charge },
	{ ai_move, 0, shamacuddaCastLightning },
	{ ai_move },
	{ ai_move },
	{ ai_move },
};

MMOVE_T(shamacudda_attack_magic) = { FRAME_magic01, FRAME_magic12, shamacudda_frames_magic, shamacudda_run };

MONSTERINFO_ATTACK(shamacudda_attack) (edict_t* self) -> void
{
	M_SetAnimation(self, &shamacudda_attack_magic);
}

//
// melee
//

void shamacudda_melee1(edict_t* self)
{
	gi.sound(self, CHAN_WEAPON, sound_melee1, 1, ATTN_NORM, 0);
}

void shamacudda_melee2(edict_t* self)
{
	gi.sound(self, CHAN_WEAPON, sound_melee2, 1, ATTN_NORM, 0);
}

void sham_swingl9(edict_t* self);
void sham_swingr9(edict_t* self);
void sham_smash10(edict_t* self);

void ShamClaw(edict_t* self);
mframe_t shamacudda_frames_smash[] = {
	{ ai_charge, 2, shamacudda_melee1 },
	{ ai_charge, 6 },
	{ ai_charge, 6 },
	{ ai_charge, 5 },
	{ ai_charge, 4 },
	{ ai_charge, 1 },
	{ ai_charge, 0 },
	{ ai_charge, 0 },
	{ ai_charge, 0 },
	{ ai_charge, 0, sham_smash10 },
	{ ai_charge, 5 },
	{ ai_charge, 4 },
};

MMOVE_T(shamacudda_attack_smash) = { FRAME_smash01, FRAME_smash12, shamacudda_frames_smash, shamacudda_run };

mframe_t shamacudda_frames_swingl[] = {
	{ ai_charge, 5, shamacudda_melee1 },
	{ ai_charge, 3 },
	{ ai_charge, 7 },
	{ ai_charge, 3 },
	{ ai_charge, 7 },
	{ ai_charge, 9 },
	{ ai_charge, 5, ShamClaw },
	{ ai_charge, 4 },
	{ ai_charge, 8, sham_swingl9 },
};

MMOVE_T(shamacudda_attack_swingl) = { FRAME_swingl01, FRAME_swingl09, shamacudda_frames_swingl, shamacudda_run };

mframe_t shamacudda_frames_swingr[] = {
	{ ai_charge, 1, shamacudda_melee2 },
	{ ai_charge, 8 },
	{ ai_charge, 14 },
	{ ai_charge, 7 },
	{ ai_charge, 3 },
	{ ai_charge, 6 },
	{ ai_charge, 6, ShamClaw },
	{ ai_charge, 3 },
	{ ai_charge, 8, sham_swingr9 },
};

MMOVE_T(shamacudda_attack_swingr) = { FRAME_swingr01, FRAME_swingr09, shamacudda_frames_swingr, shamacudda_run };

void sham_swingl9(edict_t* self);
void sham_swingr9(edict_t* self);

MONSTERINFO_MELEE(shamacudda_melee) (edict_t* self) -> void
{
	float chance = frandom();
	if (chance > 0.6 || self->health == 600)
		M_SetAnimation(self, &shamacudda_attack_smash);
	else if (chance > 0.3)
		M_SetAnimation(self, &shamacudda_attack_swingl);
	else
		M_SetAnimation(self, &shamacudda_attack_swingr);
}

//
// death
//

void shamacudda_dead(edict_t* self)
{
	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, -0 };
	monster_dead(self);
}

static void shamacudda_shrink(edict_t* self)
{
	self->maxs[2] = 0;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

mframe_t shamacudda_frames_death[] = {
	{ ai_move, 0 },
	{ ai_move, 0 },
	{ ai_move, 0, shamacudda_shrink },
	{ ai_move, 0 },
	{ ai_move, 0 },
	{ ai_move, 0 },
	{ ai_move, 0 },
	{ ai_move, 0 },
	{ ai_move, 0 },
	{ ai_move, 0 },
	{ ai_move, 0 }, // FIXME: thud?
};
MMOVE_T(shamacudda_move_death) = { FRAME_death01, FRAME_death11, shamacudda_frames_death, shamacudda_dead };

DIE(shamacudda_die) (edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t &mod) -> void
{
	if (self->beam)
	{
		G_FreeEdict(self->beam);
		self->beam = nullptr;
	}

	if (self->beam2)
	{
		G_FreeEdict(self->beam2);
		self->beam2 = nullptr;
	}

	// check for gib
	if (M_CheckGib(self, mod))
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		ThrowGibs(self, damage, {
			{ "models/monsters/shamacudda/gibs/arm/tris.md2", GIB_ACID },
			{ "models/monsters/shamacudda/gibs/torso/tris.md2", GIB_ACID },
			{ "models/monsters/shamacudda/gibs/leg/tris.md2", GIB_ACID },
			{ "models/monsters/shamacudda/gibs/head/tris.md2", GIB_ACID | GIB_HEAD }
		});
		self->deadflag = true;
		return;
	}

	if (self->deadflag)
		return;

	// regular death
	gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	self->deadflag = true;
	self->takedamage = true;

	M_SetAnimation(self, &shamacudda_move_death);
}

void SP_monster_shamacudda(edict_t* self)
{
	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict( self );
		return;
	}

	self->s.modelindex = gi.modelindex("models/monsters/shamacudda/tris.md2");
	self->mins = { -32, -32, -24 };
	self->maxs = { 32, 32, 64 };
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	gi.modelindex("models/proj/lightning/tris.md2");
	gi.modelindex("sprites/s_bfx1.sp2");
	gi.modelindex("sprites/s_bfx2.sp2");
	gi.modelindex("sprites/s_bfx3.sp2");

	sound_bfg_fire.assign("makron/bfg_fire.wav");
	sound_pain.assign("shambler/shurt2_s.wav");
	sound_idle.assign("shambler/sidle_s.wav");
	sound_die.assign("shambler/sdeath_s.wav");
	sound_windup.assign("shambler/sattck1.wav");
	sound_melee1.assign("shambler/melee1_s.wav");
	sound_melee2.assign("shambler/melee2_s.wav");
	sound_sight.assign("shambler/ssight_s.wav");
	sound_smack.assign("shambler/smack.wav");
	sound_boom.assign("shambler/sboom.wav");

	self->health = 600 * st.health_multiplier;
	self->gib_health = -60;
	self->mass = 500;
	self->flags |= FL_DEEPONE;

	self->pain = shamacudda_pain;
	self->die = shamacudda_die;
	self->monsterinfo.stand = shamacudda_stand;
	self->monsterinfo.walk = shamacudda_walk;
	self->monsterinfo.run = shamacudda_run;
	self->monsterinfo.dodge = nullptr;
	self->monsterinfo.attack = shamacudda_attack;
	self->monsterinfo.melee = shamacudda_melee;
	self->monsterinfo.sight = shamacudda_sight;
	self->monsterinfo.idle = shamacudda_idle;
	self->monsterinfo.blocked = nullptr;
	self->monsterinfo.setskin = shamacudda_setskin;

	gi.linkentity(self);

	if (self->spawnflags.has(SPAWNFLAG_SHAMBLER_PRECISE))
		self->monsterinfo.aiflags |= AI_IGNORE_SHOTS;

	M_SetAnimation(self, &shamacudda_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);
}
