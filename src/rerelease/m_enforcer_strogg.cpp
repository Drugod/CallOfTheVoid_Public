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
// m_army.c

#include "g_local.h"
#include "m_enforcer_strogg.h"
#include "m_flash.h"

static cached_soundindex sound_death;
static cached_soundindex sound_hit;
static cached_soundindex sound_attack;
static cached_soundindex sound_search;
static cached_soundindex sound_pain1;
static cached_soundindex sound_pain2;
static cached_soundindex sound_sight1;
static cached_soundindex sound_sight2;
static cached_soundindex sound_sight3;
static cached_soundindex sound_sight4;

vec3_t SightEndtToDir(edict_t* self, vec3_t orig_dir);

// Stand
mframe_t enforcer_strogg_frames_stand[] = {
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand},
	{ai_stand}
};
MMOVE_T(enforcer_strogg_move_stand) = { 0, 6, enforcer_strogg_frames_stand, NULL };

MONSTERINFO_STAND(enforcer_strogg_stand) (edict_t* self) -> void
{
	M_SetAnimation(self, &enforcer_strogg_move_stand);
}

// Run
mframe_t enforcer_strogg_frames_run[] = {
	{ai_run, 18},
	{ai_run, 14},
	{ai_run, 7},
	{ai_run, 12},
	
	{ai_run, 14},
	{ai_run, 14},
	{ai_run, 7},
	{ai_run, 11}
};
MMOVE_T(enforcer_strogg_move_run) = { 23, 30, enforcer_strogg_frames_run, NULL };

MONSTERINFO_RUN(enforcer_strogg_run) (edict_t* self) -> void
{
	M_SetAnimation(self, &enforcer_strogg_move_run);
}

// Run
mframe_t enforcer_strogg_frames_walk[] = {
	{ai_walk, 2, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 3, NULL},
	
	{ai_walk, 1, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 1, NULL},
	
	{ai_walk, 2, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 1, NULL},
	
	{ai_walk, 2, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 4, NULL},
	{ai_walk, 2, NULL}
	
};
MMOVE_T(enforcer_strogg_move_walk) = { 7, 22, enforcer_strogg_frames_walk, NULL };


MONSTERINFO_WALK(enforcer_strogg_walk) (edict_t* self) -> void
{
	M_SetAnimation(self, &enforcer_strogg_move_walk);
}

//attack
TOUCH(enfbolt_strogg_touch) (edict_t* self, edict_t* other, const trace_t& tr, bool other_touching_self) -> void
{
	//int	mod;

	if (other == self->owner)
		return;

	if (tr.surface && (tr.surface->flags & SURF_SKY))
	{
		G_FreeEdict(self);
		return;
	}
	if (other->takedamage)
		T_Damage(other, self, self->owner, self->velocity, self->s.origin, tr.plane.normal, self->dmg, 1, DAMAGE_ENERGY, MOD_UNKNOWN);
	else
	{
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_WELDING_SPARKS);
		gi.WriteByte(15);
		gi.WritePosition(self->s.origin);
		gi.WriteDir(vec3_origin);
		gi.WriteByte(226);
		gi.multicast(self->s.origin, MULTICAST_PVS, false);
		gi.sound(self, CHAN_WEAPON, sound_hit, 1, ATTN_NORM, 0);
	}
	G_FreeEdict(self);
}

void fire_enfbolt_strogg(edict_t* self, vec3_t start, vec3_t dir, int damage, int speed)
{
	edict_t* bolt;

	if (!self->enemy || self->enemy == self)
		return;

	//VectorCopy(SightEndtToDir(self, dir)[0], dir);
	dir = SightEndtToDir(self, dir);
	
	//VectorNormalize(dir);
	dir.normalize();

	bolt = G_Spawn();

	//VectorCopy(start, bolt->s.origin);
	bolt->s.origin = start;

	//VectorCopy(start, bolt->s.old_origin);
	bolt->s.old_origin = start;

	//vectoangles(dir, bolt->s.angles);
	bolt->s.angles = vectoangles(dir);

	//VectorScale(dir, speed, bolt->velocity);
	bolt->velocity[0] = speed * dir[0];
	bolt->velocity[1] = speed * dir[1];
	bolt->velocity[2] = speed * dir[2];

	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->clipmask = MASK_PROJECTILE;
	bolt->solid = SOLID_BBOX;
	bolt->s.effects |= EF_HYPERBLASTER;
	bolt->s.modelindex = gi.modelindex("models/monsters/laserstrogg/tris.md2");
	bolt->owner = self;
	bolt->touch = enfbolt_strogg_touch;
	bolt->nextthink = level.time + 2_sec;
	bolt->think = G_FreeEdict;
	bolt->dmg = damage;
	gi.linkentity(bolt);
	gi.sound(self, CHAN_WEAPON, sound_attack, 1, ATTN_NORM, 0);
}

void FireEnforcerStroggBolt(edict_t* self)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;
	vec3_t	vec;
	vec3_t	offset = { 30, 8.5, 16 };

	AngleVectors(self->s.angles, forward, right, NULL);
	//G_ProjectSource(self->s.origin, offset, forward, right, start);
	start = G_ProjectSource(self->s.origin, offset, forward, right);

	//VectorCopy(self->enemy->s.origin, vec);
	vec = self->enemy->s.origin;
	vec[2] += self->enemy->viewheight;

	//VectorSubtract(vec, start, dir);
	dir = vec - start;

	//VectorNormalize(dir);
	dir.normalize();

	fire_enfbolt_strogg(self, start, dir, 15, 600);
}

void StroggerCmdrFired(edict_t* self, int type)
{
	vec3_t start, aim, forward, right;
	float y_adjustment, z_adjustment;
	monster_muzzleflash_id_t flash_number;

	if (!self->enemy || !self->enemy->inuse) // PGM
		return;								 // PGM

	AngleVectors(self->s.angles, forward, right, nullptr);

	if (self->s.frame >= 35 && self->s.frame <= 38)
		flash_number = static_cast<monster_muzzleflash_id_t>(MZ2_GUNCMDR_CHAINGUN_2);
	else
		flash_number = static_cast<monster_muzzleflash_id_t>(MZ2_GUNCMDR_CHAINGUN_1);

	start = M_ProjectFlashSource(self, monster_flash_offset[flash_number], forward, right);

	switch (type)
	{
	case 1:
		y_adjustment = 6.25f;//cañon izquierdo
		z_adjustment = -12.0f;		
		break;
	case 2:
	default:
		y_adjustment = 1.25f;//cañon derecho
		z_adjustment = -12.0f;		
		break;
	}

	vec3_t right_adjustment = right * y_adjustment;
	vec3_t up_adjustment = vec3_t{ 0.0f, 0.0f, z_adjustment };
	vec3_t total_adjustment = right_adjustment + up_adjustment;
	start += total_adjustment;

	aim = self->enemy->s.origin - start;
	aim.normalize();

	PredictAim(self, self->enemy, start, 800, false, frandom() * 0.3f, &aim, nullptr);
	for (int i = 0; i < 3; i++)
		aim[i] += crandom_open() * 0.025f;

	monster_fire_flechette(self, start, aim, 4, 800, MZ2_WIDOW2_BEAM_SWEEP_1);
	gi.sound(self, CHAN_WEAPON, sound_attack, 1, ATTN_NORM, 0);
}

void StroggerCmdrFire1(edict_t* self)
{
	StroggerCmdrFired(self, 1);
}

void StroggerCmdrFire2(edict_t* self)
{
	StroggerCmdrFired(self, 2);
}

mframe_t enforcer_strogg_frames_attack2[] =
{
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	StroggerCmdrFire1},
	{ai_charge, 0,	StroggerCmdrFire2},
	{ai_charge, 0,	StroggerCmdrFire1}
};
MMOVE_T(enforcer_strogg_move_attack2) = { 35, 38, enforcer_strogg_frames_attack2, enforcer_strogg_run };

MONSTERINFO_ATTACK (enforcer_strogg_attack_again) (edict_t* self) ->void
{
	self->s.frame = 35;
	M_SetAnimation(self, &enforcer_strogg_move_attack2);
}

mframe_t enforcer_strogg_frames_attack1[] ={
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL},
	
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	StroggerCmdrFire2},
	{ai_charge, 0,	StroggerCmdrFire1},
	{ai_charge, 0,	StroggerCmdrFire2}
	
};
MMOVE_T(enforcer_strogg_move_attack1) = { 31, 38, enforcer_strogg_frames_attack1, enforcer_strogg_attack_again };

MONSTERINFO_ATTACK(enforcer_strogg_attack) (edict_t* self) -> void
{
	M_SetAnimation(self, &enforcer_strogg_move_attack1);
}

// Pain (1)
mframe_t enforcer_strogg_frames_pain1[] = {
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
	
};
MMOVE_T(enforcer_strogg_move_pain1) = { 66, 69, enforcer_strogg_frames_pain1, enforcer_strogg_run };

// Pain (2)
mframe_t enforcer_strogg_frames_pain2[] = {
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	
	{ai_move, 0,		NULL}
};
MMOVE_T(enforcer_strogg_move_pain2) = { 70, 74, enforcer_strogg_frames_pain2, enforcer_strogg_run };

// Pain (3)
mframe_t enforcer_strogg_frames_pain3[] = {
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
	
};
MMOVE_T(enforcer_strogg_move_pain3) = { 75, 82, enforcer_strogg_frames_pain3, enforcer_strogg_run };


// Pain (4)
mframe_t enforcer_strogg_frames_pain4[] = {
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 2,		NULL},
	
	{ai_move, 1,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 1,		NULL},
	{ai_move, 1,		NULL},
	
	{ai_move, 1,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 1,		NULL},
	
	{ai_move, 1,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
	
};
MMOVE_T(enforcer_strogg_move_pain4) = { 83, 101, enforcer_strogg_frames_pain4, enforcer_strogg_run };

// Pain
PAIN(enforcer_strogg_pain) (edict_t* self, edict_t* other, float kick, int damage, const mod_t& mod) -> void
{
	float r;

	if (skill->value == 3)
		return;
	if (self->pain_debounce_time > level.time)
		return;
	r = frandom();

	if (r < 0.5)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);		
	if (r < 0.2)
	{
		M_SetAnimation(self, &enforcer_strogg_move_pain1);
		self->pain_debounce_time = level.time + 6_sec;
	}
	else if (r < 0.4)
	{
		M_SetAnimation(self, &enforcer_strogg_move_pain2);
		self->pain_debounce_time = level.time + 1_sec;
	}
	else if (r < 0.7)
	{
		M_SetAnimation(self, &enforcer_strogg_move_pain3);
		self->pain_debounce_time = level.time + 1_sec;
	}
	else
	{
		M_SetAnimation(self, &enforcer_strogg_move_pain4);
		self->pain_debounce_time = level.time + 1_sec;
	}
}

MONSTERINFO_SETSKIN(enforcer_strogg_setskin) (edict_t* self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}

void enforcer_strogg_dead(edict_t* self)
{
	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, -8 };
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

// Death (1)
mframe_t enforcer_strogg_frames_death1[] ={
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 14,		NULL},
	
	{ai_move, 2,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	
	{ai_move, 3,		NULL},
	{ai_move, 5,		NULL},
	{ai_move, 5,		NULL},
	{ai_move, 5,		NULL},
	
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
};
MMOVE_T(enforcer_strogg_move_death1) = { 41, 54, enforcer_strogg_frames_death1, enforcer_strogg_dead };

// Death (2)
mframe_t enforcer_strogg_frames_death2[] = {
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,	    NULL},
	{ai_move, 0,		NULL},
	
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL},
	{ai_move, 0,		NULL}
};
MMOVE_T(enforcer_strogg_move_death2) = { 55, 65, enforcer_strogg_frames_death2, enforcer_strogg_dead };

// Death
DIE(enforcer_strogg_die) (edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t& mod) -> void
{

	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		ThrowGibs(self, damage, {
			{ "models/objects/gibs/bone/tris.md2" },
			{ "models/objects/gibs/sm_meat/tris.md2" },
			{ "models/monsters/enforcerstrogg/gibs/g_arm.md2" },
			{ "models/monsters/enforcerstrogg/gibs/g_arm.md2" },
			{ "models/monsters/enforcerstrogg/gibs/g_leg.md2" },
			{ "models/monsters/enforcerstrogg/gibs/g_leg.md2" },
			{ "models/monsters/enforcerstrogg/gibs/g_head.md2", GIB_HEAD }
			});
		self->deadflag = true;
		return;
	}
	if (self->deadflag == true)
		return;
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);

	self->deadflag = true;
	self->takedamage = true;

	if (frandom() < 0.5) {
		gitem_t* item2 = FindItemByClassname("ammo_mini_flechettes");
		if (item2 != NULL) {
			Drop_Item(self, item2);
		}
	}

	if (frandom() < 0.5)
		M_SetAnimation(self, &enforcer_strogg_move_death1);
	else
		M_SetAnimation(self, &enforcer_strogg_move_death2);
}

// Sight
MONSTERINFO_SIGHT(enforcer_strogg_sight) (edict_t* self, edict_t* other) -> void
{
	int r = irandom(4);

	switch (r)
	{
		case 0: gi.sound(self, CHAN_VOICE, sound_sight1, 1, ATTN_NORM, 0); break;
		case 1: gi.sound(self, CHAN_VOICE, sound_sight2, 1, ATTN_NORM, 0); break;
		case 2: gi.sound(self, CHAN_VOICE, sound_sight3, 1, ATTN_NORM, 0); break;
		case 3: gi.sound(self, CHAN_VOICE, sound_sight4, 1, ATTN_NORM, 0); break;
		default: gi.sound(self, CHAN_VOICE, sound_sight1, 1, ATTN_NORM, 0); break;
	}
}

// Search
MONSTERINFO_SEARCH(enforcer_strogg_search) (edict_t* self) -> void
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

void SP_monster_enforcer_strogg(edict_t* self)
{
	if (!M_AllowSpawn(self)) {
		G_FreeEdict(self);
		return;
	}

	self->s.modelindex = gi.modelindex("models/monsters/enforcerstrogg/tris.md2");
	gi.modelindex("models/monsters/enforcerstrogg/gibs/g_arm.md2");
	gi.modelindex("models/monsters/enforcerstrogg/gibs/g_leg.md2");
	gi.modelindex("models/monsters/enforcerstrogg/gibs/g_head.md2");

	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, 40 };
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	sound_death.assign("enforcer/death1_s.wav");
	sound_hit.assign("enforcer/enfstop.wav");
	sound_attack.assign("weapons/nail1b.wav");
	sound_search.assign("enforcer/idle1_s.wav");
	sound_pain1.assign("enforcer/pain1_s.wav");
	sound_pain2.assign("enforcer/pain2_s.wav");
	sound_sight1.assign("enforcer/sight1_s.wav");
	sound_sight2.assign("enforcer/sight2_s.wav");
	sound_sight3.assign("enforcer/sight3_s.wav");
	sound_sight4.assign("enforcer/sight4_s.wav");

	self->health = 100 * st.health_multiplier;;
	self->gib_health = -35;

	self->mass = 120;

	self->pain = enforcer_strogg_pain;
	self->die = enforcer_strogg_die;
	self->monsterinfo.stand = enforcer_strogg_stand;
	self->monsterinfo.walk = enforcer_strogg_walk;
	self->monsterinfo.run = enforcer_strogg_run;
	self->monsterinfo.attack = enforcer_strogg_attack;
	self->monsterinfo.sight = enforcer_strogg_sight;
	self->monsterinfo.search = enforcer_strogg_search;
	self->monsterinfo.setskin = enforcer_strogg_setskin;

	gi.linkentity(self);

	M_SetAnimation(self, &enforcer_strogg_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);
}