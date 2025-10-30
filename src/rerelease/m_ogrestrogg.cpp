// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

ogrestrogg

==============================================================================
*/

#include "g_local.h"
#include "m_ogrestrogg.h"
#include "m_flash.h"

static cached_soundindex sound_pain1;
static cached_soundindex sound_pain2;
static cached_soundindex sound_die;
static cached_soundindex sound_die2;
static cached_soundindex sound_gun;
static cached_soundindex sound_gunb;
static cached_soundindex sound_cleaver_swing;
static cached_soundindex sound_cleaver_hit;
static cached_soundindex sound_cleaver_miss;
static cached_soundindex sound_idle;
static cached_soundindex sound_search;
static cached_soundindex sound_sight;

MONSTERINFO_IDLE(ogrestrogg_idle) (edict_t* self) -> void
{
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

MONSTERINFO_SIGHT(ogrestrogg_sight) (edict_t* self, edict_t* other) -> void
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

MONSTERINFO_SEARCH(ogrestrogg_search) (edict_t* self) -> void
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

void ogrestrogg_cleaver_swing(edict_t* self)
{
	gi.sound(self, CHAN_WEAPON, sound_cleaver_swing, 1, ATTN_NORM, 0);
}

mframe_t ogrestrogg_frames_stand[] = {
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }
};
MMOVE_T(ogrestrogg_move_stand) = { FRAME_stand1, FRAME_stand7, ogrestrogg_frames_stand, nullptr };

MONSTERINFO_STAND(ogrestrogg_stand) (edict_t* self) -> void
{
	M_SetAnimation(self, &ogrestrogg_move_stand);
}

mframe_t ogrestrogg_frames_walk[] = {
	{ ai_walk, 15 },
	{ ai_walk, 7 },
	{ ai_walk, 6 },
	{ ai_walk, 5 },
	{ ai_walk, 2, monster_footstep },
	{ ai_walk },
	{ ai_walk, 2 },
	{ ai_walk, 8 },
	{ ai_walk, 12 },
	{ ai_walk, 8 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 2, monster_footstep },
	{ ai_walk, 2 },
	{ ai_walk, 1 },
	{ ai_walk, 8 }
};
MMOVE_T(ogrestrogg_move_walk) = { FRAME_walk1, FRAME_walk16, ogrestrogg_frames_walk, nullptr };

MONSTERINFO_WALK(ogrestrogg_walk) (edict_t* self) -> void
{
	M_SetAnimation(self, &ogrestrogg_move_walk);
}

mframe_t ogrestrogg_frames_run[] = {
	{ ai_run, 23 },
	{ ai_run, 14 },
	{ ai_run, 14, monster_footstep },
	{ ai_run, 21 },
	{ ai_run, 12 },
	{ ai_run, 13, monster_footstep }
};
MMOVE_T(ogrestrogg_move_run) = { FRAME_run1, FRAME_run6, ogrestrogg_frames_run, nullptr };

MONSTERINFO_RUN(ogrestrogg_run) (edict_t* self) -> void
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		M_SetAnimation(self, &ogrestrogg_move_stand);
	else
		M_SetAnimation(self, &ogrestrogg_move_run);
}

void ogrestroggMelee(edict_t* self)
{
	vec3_t aim = { MELEE_DISTANCE, self->mins[0], -4 };
	if (fire_hit(self, aim, irandom(20, 25), 300))
		gi.sound(self, CHAN_AUTO, sound_cleaver_hit, 1, ATTN_NORM, 0);
	else
	{
		gi.sound(self, CHAN_AUTO, sound_cleaver_miss, 1, ATTN_NORM, 0);
		self->monsterinfo.melee_debounce_time = level.time + 1.5_sec;
	}
}

mframe_t ogrestrogg_frames_attack_melee[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, ogrestrogg_cleaver_swing },
	{ ai_charge },
	{ ai_charge, 0, ogrestroggMelee },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, ogrestrogg_cleaver_swing },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, ogrestroggMelee },
	{ ai_charge },
	{ ai_charge }
};
MMOVE_T(ogrestrogg_move_attack_melee) = { FRAME_melee3, FRAME_melee16, ogrestrogg_frames_attack_melee, ogrestrogg_run };

MONSTERINFO_MELEE(ogrestrogg_melee) (edict_t* self) -> void
{
	M_SetAnimation(self, &ogrestrogg_move_attack_melee);
}

// Grenade
void monster_fire_ogregrenade(edict_t* self, const vec3_t& start, const vec3_t& aimdir, int damage, int speed,
	monster_muzzleflash_id_t flashtype, float right_adjust, float up_adjust)
{
	fire_grenade(self, start, aimdir, damage, speed, 2.5_sec, damage + 40.f, right_adjust, up_adjust, true);
	monster_muzzleflash(self, start, flashtype);
}

void OgreGrenade(edict_t* self)
{
	float range;
	vec3_t start, forward, right, aim, v;
	vec3_t offset = { 4.6f * 1.15f, 16.8f * 1.15f, 19.3f * 1.15f };

	AngleVectors(self->s.angles, forward, right, NULL);
	start = G_ProjectSource(self->s.origin, offset, forward, right);
	aim = forward;

	v = self->s.origin - self->enemy->s.origin;
	range = v.length();

	if (range < 300.0f)
		range = 300.0f;

	monster_fire_ogregrenade(self, start, aim, 50, 2.0*range, MZ2_GUNNER_GRENADE_1, -50.f, 0.f);
}

mframe_t ogrestrogg_frames_attack_gun[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, OgreGrenade },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, monster_footstep },
	{ ai_charge }
};
MMOVE_T(ogrestrogg_move_attack_gun) = { FRAME_attack1, FRAME_attack9, ogrestrogg_frames_attack_gun, ogrestrogg_run };

MONSTERINFO_ATTACK(ogrestrogg_attack) (edict_t* self) -> void
{
	M_SetAnimation(self, &ogrestrogg_move_attack_gun);
}

static void ogrestrogg_pain_sound(edict_t* self)
{
	if (self->health < (self->max_health / 2))
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
}

mframe_t ogrestrogg_frames_pain[] = {
	{ ai_move },
	{ ai_move, 0, ogrestrogg_pain_sound },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(ogrestrogg_move_pain) = { FRAME_pain2, FRAME_pain5, ogrestrogg_frames_pain, ogrestrogg_run };

mframe_t ogrestrogg_frames_pain_air[] = {
	{ ai_move },
	{ ai_move, 0, ogrestrogg_pain_sound },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(ogrestrogg_move_pain_air) = { FRAME_painup2, FRAME_painup6, ogrestrogg_frames_pain_air, ogrestrogg_run };

PAIN(ogrestrogg_pain) (edict_t* self, edict_t* other, float kick, int damage, const mod_t& mod) -> void
{
	if (level.time < self->pain_debounce_time)
	{
		if ((self->velocity[2] > 100) && (self->monsterinfo.active_move == &ogrestrogg_move_pain))
			M_SetAnimation(self, &ogrestrogg_move_pain_air);
		return;
	}

	self->pain_debounce_time = level.time + 3_sec;

	if (!M_ShouldReactToPain(self, mod))
		return;

	if (self->velocity[2] > 100)
		M_SetAnimation(self, &ogrestrogg_move_pain_air);
	else
		M_SetAnimation(self, &ogrestrogg_move_pain);
}

MONSTERINFO_SETSKIN(ogrestrogg_setskin) (edict_t* self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;
	else
		self->s.skinnum &= ~1;
}

void ogrestrogg_dead(edict_t* self)
{
	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, -8 };
	monster_dead(self);
}

static void ogrestrogg_shrink(edict_t* self)
{
	self->maxs[2] = 0;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

static void ogrestrogg_death_sound(edict_t* self)
{
	gi.sound(self, CHAN_BODY, sound_die, 1, ATTN_NORM, 0);
}

mframe_t ogrestrogg_frames_death[] = {
	{ ai_move },
	{ ai_move, 0, ogrestrogg_death_sound },
	{ ai_move, 0, ogrestrogg_shrink },
	{ ai_move, 0, monster_footstep },
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
MMOVE_T(ogrestrogg_move_death) = { FRAME_death2, FRAME_death22, ogrestrogg_frames_death, ogrestrogg_dead };

DIE(ogrestrogg_die) (edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t& mod) -> void
{
	if (M_CheckGib(self, mod))
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		self->s.skinnum /= 2;

		ThrowGibs(self, damage, {
			{ 2, "models/objects/gibs/bone/tris.md2" },
			{ 2, "models/objects/gibs/sm_meat/tris.md2" },
			{ 2, "models/monsters/gladiatr/gibs/thigh.md2", GIB_SKINNED },
			{ "models/monsters/gladiatr/gibs/chest.md2", GIB_SKINNED },
			{ "models/monsters/ogrestrogg/gibs/g_arm.md2", GIB_SKINNED },
			{ "models/monsters/ogrestrogg/gibs/g_arm.md2", GIB_SKINNED },
			{ "models/monsters/ogrestrogg/gibs/g_leg.md2", GIB_SKINNED },
			{ "models/monsters/ogrestrogg/gibs/g_leg.md2", GIB_SKINNED },
			{ "models/monsters/ogrestrogg/gibs/g_head.md2", GIB_SKINNED | GIB_HEAD }
			});
		self->deadflag = true;
		return;
	}

	if (self->deadflag)
		return;

	self->deadflag = true;
	self->takedamage = true;

	if (brandom())
		gi.sound(self, CHAN_VOICE, sound_die2, 1, ATTN_NORM, 0);

	if (frandom() < 0.5) {
		gitem_t* item2 = FindItemByClassname("ammo_mini_grenades");
		if (item2 != NULL) {
			Drop_Item(self, item2);
		}
	}

	M_SetAnimation(self, &ogrestrogg_move_death);
}

MONSTERINFO_BLOCKED(ogrestrogg_blocked) (edict_t* self, float dist) -> bool
{
	if (blocked_checkplat(self, dist)) {
		return true;
	}
	return false;
}

void SP_monster_ogre_strogg(edict_t* self)
{
	if (!M_AllowSpawn(self)) {
		G_FreeEdict(self);
		return;
	}

	sound_pain1.assign("ogre/ogpain1_s.wav");
	sound_pain2.assign("ogre/ogpain1_s.wav");
	sound_die.assign("ogre/ogdth_s.wav");
	sound_die2.assign("ogre/ogdth_s.wav");
	sound_cleaver_swing.assign("gladiator/melee1.wav");
	sound_cleaver_hit.assign("gladiator/melee2.wav");
	sound_cleaver_miss.assign("gladiator/melee3.wav");
	sound_idle.assign("ogre/ogidle_s.wav");
	sound_idle.assign("ogre/ogidle2_s.wav");
	sound_search.assign("ogre/gldsrch1.wav");
	sound_sight.assign("ogre/ogwake_s.wav");

	gi.modelindex("models/monsters/gladiatr/gibs/chest.md2");
	gi.modelindex("models/monsters/gladiatr/gibs/head.md2");
	gi.modelindex("models/monsters/gladiatr/gibs/larm.md2");
	gi.modelindex("models/monsters/gladiatr/gibs/rarm.md2");
	gi.modelindex("models/monsters/gladiatr/gibs/thigh.md2");
	gi.modelindex("models/monsters/ogrestrogg/gibs/g_arm.md2");
	gi.modelindex("models/monsters/ogrestrogg/gibs/g_leg.md2");
	gi.modelindex("models/monsters/ogrestrogg/gibs/g_head.md2");

	self->s.modelindex = gi.modelindex("models/monsters/ogrestrogg/tris.md2");
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->health = 450 * st.health_multiplier;
	self->mass = 400;
	self->gib_health = -175;
	self->mins = { -32, -32, -24 };
	self->maxs = { 32, 32, 42 };

	self->pain = ogrestrogg_pain;
	self->die = ogrestrogg_die;
	self->monsterinfo.stand = ogrestrogg_stand;
	self->monsterinfo.walk = ogrestrogg_walk;
	self->monsterinfo.run = ogrestrogg_run;
	self->monsterinfo.dodge = nullptr;
	self->monsterinfo.attack = ogrestrogg_attack;
	self->monsterinfo.melee = ogrestrogg_melee;
	self->monsterinfo.sight = ogrestrogg_sight;
	self->monsterinfo.idle = ogrestrogg_idle;
	self->monsterinfo.search = ogrestrogg_search;
	self->monsterinfo.blocked = ogrestrogg_blocked;
	self->monsterinfo.setskin = ogrestrogg_setskin;

	gi.linkentity(self);
	M_SetAnimation(self, &ogrestrogg_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);
}
