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
#include "m_wizard.h"
#include "m_flash.h"

static cached_soundindex sound_proj_hit;
static cached_soundindex sound_attack;
static cached_soundindex sound_death;
static cached_soundindex sound_idle1;
static cached_soundindex sound_idle2;
static cached_soundindex sound_pain;
static cached_soundindex sound_sight;

vec3_t* SightEndtToDir(edict_t* self, vec3_t orig_dir);

// Stand
mframe_t wizard_frames_stand [] = {
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL}
};
MMOVE_T(wizard_move_stand) = {0, 7, wizard_frames_stand, NULL};

MONSTERINFO_STAND(wizard_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &wizard_move_stand);	
}

// Walk
mframe_t wizard_frames_walk[] =
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
MMOVE_T(wizard_move_walk) = {0, 7, wizard_frames_walk, NULL };

MONSTERINFO_WALK(wizard_walk) (edict_t *self) -> void
{
	M_SetAnimation(self, &wizard_move_walk);
}

// Run
mframe_t wizard_frames_run [] ={
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
MMOVE_T(wizard_move_run) = {8, 21, wizard_frames_run, NULL};

MONSTERINFO_RUN(wizard_run) (edict_t *self) -> void
{
	M_SetAnimation(self, &wizard_move_run);	
}

void wizard_frame(edict_t *self)
{
	static int frame = 0;

	self->s.frame = (33 - frame);
	frame++;

	if (frame > 5)
		frame = 0;
}

mframe_t wizard_frames_finish [] =
{
	{ai_charge, 0, wizard_frame},
	{ai_charge, 0, wizard_frame},
	{ai_charge, 0, wizard_frame},
	{ai_charge, 0, wizard_frame},
	{ai_charge, 0, wizard_frame}
};
MMOVE_T(wizard_move_finish) = {22, 26, wizard_frames_finish, wizard_run};

void wizard_finish_attack(edict_t *self)
{
	M_SetAnimation(self, &wizard_move_finish);
}

TOUCH(spit_touch) (edict_t* self, edict_t* other, const trace_t& tr, bool other_touching_self) -> void
{

	if (other == self->owner)
		return;
	if (tr.surface && (tr.surface->flags & SURF_SKY))
	{
		G_FreeEdict (self);
		return;
	}
	if (other->takedamage)
		T_Damage (other, self, self->owner, self->velocity, self->s.origin, tr.plane.normal, self->dmg, 1, DAMAGE_ENERGY, MOD_UNKNOWN);
	else
	{
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_WELDING_SPARKS);
		gi.WriteByte(15);
		gi.WritePosition(self->s.origin);
		gi.WriteDir(vec3_origin);
		gi.WriteByte(209);
		gi.multicast(self->s.origin, MULTICAST_PVS, false);

		gi.sound (self, CHAN_WEAPON, sound_proj_hit, 1, ATTN_NORM, 0);
	}
	G_FreeEdict(self);
}

DIE(fire_spit_die) (edict_t* self, edict_t* other, edict_t* inflictor, int damage, const vec3_t& point, const mod_t& mod) -> void
{
	if (mod.id == MOD_CRUSH)
		CTFResetGrapple(self);
}

void fire_spit(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed)
{
	edict_t	*spit;

	if (!self->enemy || self->enemy == self)
		return;

	//VectorCopy(SightEndtToDir(self, dir)[0], dir);
	dir = SightEndtToDir(self, dir)[0];
	//VectorNormalize (dir);
	dir.normalize();

	spit = G_Spawn();
	//VectorCopy(start, spit->s.origin);
	spit->s.origin = start;

	//VectorCopy(start, spit->s.old_origin);0
	spit->s.old_origin = start;	

	//vectoangles(dir, spit->s.angles);
	spit->s.angles = vectoangles(dir);

	//VectorScale(dir, speed, spit->velocity);
	spit->velocity[0] = speed * dir[0];
	spit->velocity[1] = speed * dir[1];
	spit->velocity[2] = speed * dir[2];

	spit->movetype = MOVETYPE_FLYMISSILE;
	spit->clipmask = MASK_PROJECTILE;
	spit->solid = SOLID_BBOX;
	spit->s.effects |= (EF_BLASTER|EF_TRACKER);

	spit->mins = {0, 0, 0};
	spit->maxs = {0, 0, 0};

	spit->s.modelindex = gi.modelindex ("models/monsters/spitstrogg/tris.md2");
	spit->owner = self;
	spit->touch = spit_touch;
	spit->dmg = damage;
	spit->die = fire_spit_die;
	spit->enemy = self->enemy;
	gi.linkentity(spit);
}	

void WizardSpit(edict_t *self)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;
	vec3_t	vec;
	vec3_t	offset = {0, 0, 30};

	AngleVectors (self->s.angles, forward, right, NULL);
	//G_ProjectSource(self->s.origin, offset, forward, right, start);
	start = G_ProjectSource(self->s.origin, offset, forward, right);

	//VectorCopy(self->enemy->s.origin, vec);
	vec = self->enemy->s.origin;
	vec[2] += self->enemy->viewheight;

	//VectorSubtract(vec, start, dir);
	dir = vec - start;

	//VectorNormalize(dir);
	dir.normalize();

	fire_spit(self, start, dir, 9, 600);
}

void wizard_prespit(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_attack, 1, ATTN_NORM, 0);
}

// Attack
mframe_t wizard_frames_attack [] =
{
	{ai_charge, 0, wizard_prespit},
	{ai_charge, 0, WizardSpit},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, WizardSpit},
	{ai_charge, 0, NULL}
};
//MMOVE_T(wizard_move_attack) = {22, 27, wizard_frames_attack, wizard_finish_attack};
MMOVE_T(wizard_move_attack) = {22, 27, wizard_frames_attack, wizard_run };

MONSTERINFO_ATTACK (wizard_attack) (edict_t *self) -> void
{
	M_SetAnimation(self, &wizard_move_attack);	
}

// Pain
mframe_t wizard_frames_pain [] ={
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
MMOVE_T(wizard_move_pain) = {42, 45, wizard_frames_pain, wizard_run};

PAIN (wizard_pain) (edict_t* self, edict_t* other, float kick, int damage, const mod_t& mod) -> void
{
	if (skill->value == 3)
		return;
	if (level.time < self->pain_debounce_time)
		return;
	gi.sound (self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

	self->pain_debounce_time = level.time + 3_sec;
	M_SetAnimation(self, &wizard_move_pain);	
}

void wizard_fling(edict_t* self)
{
	self->velocity[0] = -200 + 400 * crandom();
	self->velocity[1] = -200 + 400 * crandom();
	self->velocity[2] = 100 + 100 * crandom();

	self->mins = {-16, -16, -24};
	self->maxs = {16, 16, 8};
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;

}

void wizard_dead(edict_t *self)
{
	self->nextthink = 0_ms;
	gi.linkentity(self);
}

// Death
mframe_t wizard_frames_death [] ={
	{ai_move, 0, wizard_fling},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
MMOVE_T(wizard_move_death) = {46, 53, wizard_frames_death, wizard_dead};

DIE (wizard_die) (edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t& mod) -> void
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
	M_SetAnimation(self, &wizard_move_death);		
}

MONSTERINFO_SIGHT (wizard_sight) (edict_t* self, edict_t* other) -> void
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

MONSTERINFO_SEARCH (wizard_search) (edict_t *self) -> void
{
	float r;
	r = frandom() * 5;

	if (r > 4.5)
		gi.sound(self, CHAN_VOICE, sound_idle1, 1, ATTN_NORM, 0);
	if (r < 1.5)
		gi.sound(self, CHAN_VOICE, sound_idle2, 1, ATTN_NORM, 0);
}

MONSTERINFO_SETSKIN(wizard_setskin) (edict_t* self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}


void SP_monster_wizard(edict_t *self)
{
	if (!M_AllowSpawn(self)) {
		G_FreeEdict(self);
		return;
	}

	sound_proj_hit.assign("wizard/hit.wav");
	sound_attack.assign("wizard/wattack.wav");
	sound_death.assign("wizard/wdeath.wav");
	sound_idle1.assign("wizard/widle1.wav");
	sound_idle2.assign("wizard/widle2.wav");
	sound_pain.assign("wizard/wpain.wav");
	sound_sight.assign("wizard/wsight.wav");

	self->mins = {-16, -16, -24};	
	self->maxs = {16, 16, 40};	
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->s.modelindex = gi.modelindex("models/monsters/wizard/tris.md2");

	self->health = 80;
	self->gib_health = -40;

	self->mass = 80;

	self->pain = wizard_pain;
	self->die = wizard_die;
	self->monsterinfo.stand = wizard_stand;
	self->monsterinfo.walk = wizard_walk;
	self->monsterinfo.run = wizard_run;
	self->monsterinfo.attack = wizard_attack;
	self->monsterinfo.sight = wizard_sight;
	self->monsterinfo.search = wizard_search;
	self->monsterinfo.setskin = wizard_setskin;

	gi.linkentity(self);
	M_SetAnimation(self, &wizard_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	flymonster_start(self);
}
