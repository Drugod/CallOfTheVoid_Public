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
// m_wizard.c

#include "g_local.h"
#include "m_wizard_prototype.h"
#include "m_flash.h"

static cached_soundindex sound_proj_hit;
static cached_soundindex sound_attack;
static cached_soundindex sound_death;
static cached_soundindex sound_idle1;
static cached_soundindex sound_idle2;
static cached_soundindex sound_pain;
static cached_soundindex sound_sight;

vec3_t SightEndtToDir(edict_t* self, vec3_t orig_dir);

mframe_t wizard_prototype_frames_stand [] = {
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL}
};
MMOVE_T(wizard_prototype_move_stand) = {0, 7, wizard_prototype_frames_stand, nullptr };

MONSTERINFO_STAND(wizard_prototype_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &wizard_prototype_move_stand);	
}

mframe_t wizard_prototype_frames_walk[] =
{
	{ai_walk, 8, NULL},
	{ai_walk, 8, NULL},
	{ai_walk, 8, NULL},
	{ai_walk, 8, NULL},
	{ai_walk, 8, NULL},
	{ai_walk, 8, NULL},
	{ai_walk, 8, NULL},
	{ai_walk, 8, NULL}
};
MMOVE_T(wizard_prototype_move_walk) = {0, 7, wizard_prototype_frames_walk, NULL };

MONSTERINFO_WALK(wizard_prototype_walk) (edict_t *self) -> void
{
	M_SetAnimation(self, &wizard_prototype_move_walk);
}

mframe_t wizard_prototype_frames_run [] ={
	{ai_run, 16, NULL},
	{ai_run, 16, NULL},
	{ai_run, 16, NULL},
	{ai_run, 16, NULL},
	{ai_run, 16, NULL},
	{ai_run, 16, NULL},
	{ai_run, 16, NULL},
	{ai_run, 16, NULL},
	{ai_run, 16, NULL},
	{ai_run, 16, NULL},
	{ai_run, 16, NULL},
	{ai_run, 16, NULL},
	{ai_run, 16, NULL},
	{ai_run, 16, NULL}
};
MMOVE_T(wizard_prototype_move_run) = {8, 21, wizard_prototype_frames_run, NULL};

MONSTERINFO_RUN(wizard_prototype_run) (edict_t *self) -> void
{
	M_SetAnimation(self, &wizard_prototype_move_run);	
}

void wizard_prototype_frame(edict_t *self)
{
	static int frame = 0;

	self->s.frame = (33 - frame);
	frame++;

	if (frame > 5)
		frame = 0;
}

TOUCH(spit_proto_touch) (edict_t* self, edict_t* other, const trace_t& tr, bool other_touching_self) -> void
{

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
		gi.WriteByte(209);
		gi.multicast(self->s.origin, MULTICAST_PVS, false);

		gi.sound(self, CHAN_WEAPON, sound_proj_hit, 1, ATTN_NORM, 0);
	}
	G_FreeEdict(self);
}

DIE(fire_spit_proto_die) (edict_t* self, edict_t* other, edict_t* inflictor, int damage, const vec3_t& point, const mod_t& mod) -> void
{
	if (mod.id == MOD_CRUSH)
		CTFResetGrapple(self);
}

void fire_spit_proto(edict_t* self, vec3_t start, vec3_t dir, int damage, int speed, int value)
{
	if (!self->enemy || self->enemy == self)
		return;

	edict_t* spit;
	spit = G_Spawn();
	spit->s.origin = start;
	spit->s.old_origin = start;
	spit->s.angles = vectoangles(dir);
	if (sv_maxvelocity->value > speed) {
		spit->velocity = dir * speed;
	}
	else {
		spit->velocity = dir * sv_maxvelocity->value;
	}
	spit->mins = { -1, -1, -1 };
	spit->maxs = { 1,  1,  1 };
	spit->movetype = MOVETYPE_FLYMISSILE;
	spit->flags |= FL_DODGE;
	spit->clipmask = MASK_SHOT;
	spit->solid = SOLID_BBOX;
	spit->s.effects |= (EF_BLASTER | EF_TRACKER);
	spit->s.modelindex = gi.modelindex("models/monsters/spitstrogg/tris.md2");
	spit->owner = self;
	spit->touch = spit_proto_touch;
	spit->nextthink = level.time + 10.0_sec;
	spit->think = G_FreeEdict;
	spit->dmg = damage;
	gi.linkentity(spit);
}

static void WizardSpitleft(edict_t* self)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;
	vec3_t	vec;

	AngleVectors(self->s.angles, forward, right, NULL);
	start = self->s.origin;
	start[0] += forward[0] * 14.0f;
	start[1] += forward[1] * 14.0f;
	start[2] = self->s.origin[2] + 45;
	start[0] += right[0] * 14.0f;
	start[1] += right[1] * 14.0f;

	/*char buffer[256];
	snprintf(buffer, sizeof(buffer), "WizardSpitleft start = (\"%.2f\",\"%.2f\",\"%.2f\"\n", start[0], start[1], start[2]);
	gi.Com_PrintFmt("{}", buffer);*/

	vec = self->enemy->s.origin;
	vec[2] += self->enemy->viewheight;

	dir = vec - start;
	dir.normalize();

	fire_spit_proto(self, start, dir, 9, 600, 0);
}

static void WizardSpitRight(edict_t* self)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;
	vec3_t	vec;

	AngleVectors(self->s.angles, forward, right, NULL);
	right[2] = 0;
	right.normalize();

	start = self->s.origin;
	start[0] += forward[0] * 14.0f;
	start[1] += forward[1] * 14.0f;
	start[2] = self->s.origin[2] + 25;
	start[0] += right[0] * -14.0f;
	start[1] += right[1] * -14.0f;

	/*char buffer[256];
	snprintf(buffer, sizeof(buffer), "WizardSpitright start = (\"%.2f\",\"%.2f\",\"%.2f\"\n", start[0], start[1], start[2]);
	gi.Com_PrintFmt("{}", buffer);*/

	vec = self->enemy->s.origin;
	vec[2] += self->enemy->viewheight;

	dir = vec - start;
	dir.normalize();

	fire_spit_proto(self, start, dir, 9, 600, 1);
}

THINK(WizardPrototypeSpitManager)(edict_t* self) -> void
{
	if (!self->enemy || self->enemy->health <= 0)
		return;

	edict_t* wiz = self->owner;

	if (!wiz || !wiz->inuse || wiz->deadflag || wiz->health <= 0) {
		G_FreeEdict(self);
		return;
	}

	if (self->count == 0) {
		WizardSpitleft(wiz);
	}else{
		WizardSpitRight(wiz);
	}

	self->think = G_FreeEdict;
	self->nextthink = level.time + 1.0_sec;
}

static void wizard_prototype_prespit(edict_t* self)
{
	if (!self->enemy || self->enemy->health <= 0)
		return;

	gi.sound(self, CHAN_WEAPON, sound_attack, 1, ATTN_NORM, 0);
	edict_t* spitleft;
	spitleft = G_Spawn();
	spitleft->s.origin = self->s.origin;
	spitleft->s.angles = self->s.angles;
	spitleft->enemy = self->enemy;
	spitleft->owner = self;
	spitleft->think = WizardPrototypeSpitManager;
	spitleft->count = 0;
	spitleft->nextthink = level.time + 0.3_sec;
	gi.linkentity(spitleft);

	edict_t* spitright;
	spitright = G_Spawn();
	spitright->s.origin = self->s.origin;
	spitright->s.angles = self->s.angles;
	spitright->enemy = self->enemy;
	spitright->owner = self;
	spitright->think = WizardPrototypeSpitManager;
	spitright->count = 1;
	spitright->nextthink = level.time + 0.8_sec;
	gi.linkentity(spitright);
}

mframe_t wizard_prototype_frames_attack[] =
{
	{ai_charge, 0, wizard_prototype_prespit},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL}
};
MMOVE_T(wizard_prototype_move_attack) = { 22, 27, wizard_prototype_frames_attack, wizard_prototype_run };

MONSTERINFO_ATTACK (wizard_prototype_attack) (edict_t *self) -> void
{
	M_SetAnimation(self, &wizard_prototype_move_attack);	
}

mframe_t wizard_prototype_frames_pain [] ={
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
MMOVE_T(wizard_prototype_move_pain) = {42, 45, wizard_prototype_frames_pain, wizard_prototype_run};

PAIN (wizard_prototype_pain) (edict_t* self, edict_t* other, float kick, int damage, const mod_t& mod) -> void
{
	if (skill->value == 3)
		return;

	if (level.time < self->pain_debounce_time)
		return;

	gi.sound (self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

	self->pain_debounce_time = level.time + 3_sec;
	M_SetAnimation(self, &wizard_prototype_move_pain);	
}

void wizard_prototype_fling(edict_t* self)
{
	self->velocity[0] = -200 + 400 * crandom();
	self->velocity[1] = -200 + 400 * crandom();
	self->velocity[2] = 100 + 100 * crandom();

	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
}

void wizard_prototype_dead(edict_t *self)
{
	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, 8 };
	monster_dead(self);
}

mframe_t wizard_prototype_frames_death [] ={
	{ai_move, 0, wizard_prototype_fling},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
MMOVE_T(wizard_prototype_move_death) = {46, 53, wizard_prototype_frames_death, wizard_prototype_dead};

DIE (wizard_prototype_die) (edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t& mod) -> void
{

	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		ThrowGibs(self, damage, {
			{ "models/objects/gibs/bone/tris.md2" },
			{ "models/objects/gibs/sm_meat/tris.md2" },
			{ "models/objects/gibs/head2/tris.md2", GIB_HEAD }
			});
		self->deadflag = true;
		return;
	}

	if (self->deadflag == true)
		return;

	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);

	self->deadflag = true;
	self->takedamage = true;
	M_SetAnimation(self, &wizard_prototype_move_death);		
}

MONSTERINFO_SIGHT (wizard_prototype_sight) (edict_t* self, edict_t* other) -> void
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

MONSTERINFO_SEARCH (wizard_prototype_search) (edict_t *self) -> void
{
	float r = frandom() * 5;

	if (r > 4.5)
		gi.sound(self, CHAN_VOICE, sound_idle1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_idle2, 1, ATTN_NORM, 0);
}

MONSTERINFO_SETSKIN(wizard_prototype_setskin) (edict_t* self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}

void SP_monster_wizard_prototype(edict_t *self)
{
	if (!M_AllowSpawn(self)) {
		G_FreeEdict(self);
		return;
	}

	sound_proj_hit.assign("wizard/hit.wav");
	sound_attack.assign("wizard/wattack_s.wav");
	sound_death.assign("wizard/wdeath_s.wav");
	sound_idle1.assign("wizard/widle1_s.wav");
	sound_idle2.assign("wizard/widle2_s.wav");
	sound_pain.assign("wizard/wpain_s.wav");
	sound_sight.assign("wizard/wsight_s.wav");

	self->mins = {-16, -16, -24};	
	self->maxs = {16, 16, 40};	
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->s.modelindex = gi.modelindex("models/monsters/wizard_prototype/tris.md2");

	self->health = 80 * st.health_multiplier;
	self->gib_health = -40;

	self->mass = 80;

	self->pain = wizard_prototype_pain;
	self->die = wizard_prototype_die;
	self->monsterinfo.stand = wizard_prototype_stand;
	self->monsterinfo.walk = wizard_prototype_walk;
	self->monsterinfo.run = wizard_prototype_run;
	self->monsterinfo.attack = wizard_prototype_attack;
	self->monsterinfo.sight = wizard_prototype_sight;
	self->monsterinfo.search = wizard_prototype_search;
	self->monsterinfo.setskin = wizard_prototype_setskin;

	gi.linkentity(self);
	M_SetAnimation(self, &wizard_prototype_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	flymonster_start(self);
}
