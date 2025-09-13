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
#include "m_wizard_barracuda_strogg.h"
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
mframe_t wizarcuda_strogg_frames_stand [] = {
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL}
};
MMOVE_T(wizarcuda_strogg_move_stand) = {0, 7, wizarcuda_strogg_frames_stand, NULL};

MONSTERINFO_STAND(wizarcuda_strogg_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &wizarcuda_strogg_move_stand);	
}

// Walk
mframe_t wizarcuda_strogg_frames_walk[] =
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
MMOVE_T(wizarcuda_strogg_move_walk) = {0, 7, wizarcuda_strogg_frames_walk, NULL };

MONSTERINFO_WALK(wizarcuda_strogg_walk) (edict_t *self) -> void
{
	M_SetAnimation(self, &wizarcuda_strogg_move_walk);
}

// Run
mframe_t wizarcuda_strogg_frames_run [] ={
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
MMOVE_T(wizarcuda_strogg_move_run) = {8, 21, wizarcuda_strogg_frames_run, NULL};

MONSTERINFO_RUN(wizarcuda_strogg_run) (edict_t *self) -> void
{
	M_SetAnimation(self, &wizarcuda_strogg_move_run);	
}

void wizarcuda_strogg_frame(edict_t *self)
{
	static int frame = 0;

	self->s.frame = (33 - frame);
	frame++;

	if (frame > 5)
		frame = 0;
}

mframe_t wizarcuda_strogg_frames_finish [] =
{
	{ai_charge, 0, wizarcuda_strogg_frame},
	{ai_charge, 0, wizarcuda_strogg_frame},
	{ai_charge, 0, wizarcuda_strogg_frame},
	{ai_charge, 0, wizarcuda_strogg_frame},
	{ai_charge, 0, wizarcuda_strogg_frame}
};
MMOVE_T(wizarcuda_strogg_move_finish) = {22, 26, wizarcuda_strogg_frames_finish, wizarcuda_strogg_run};

void wizarcuda_strogg_finish_attack(edict_t *self)
{
	M_SetAnimation(self, &wizarcuda_strogg_move_finish);
}

void scraggacudaSpit_checker(edict_t* self)
{
	self->heatbeam_time = level.time + 2.0_sec;

	self->locked_angles = self->s.angles;

	vec3_t start, dir, forward, right;
	AngleVectors(self->s.angles, forward, right, nullptr);
	monster_muzzleflash_id_t flash_number =
	static_cast<monster_muzzleflash_id_t>(MZ2_INFANTRY_MACHINEGUN_14 + (self->s.frame - MZ2_INFANTRY_MACHINEGUN_14));
	start = M_ProjectFlashSource(self, monster_flash_offset[flash_number], forward, right);

	float x_adjustment = 20.0f;
	float y_adjustment = -18.0f;
	float z_adjustment = -15.0f;

	vec3_t forward_adjustment = forward * x_adjustment;
	vec3_t right_adjustment = right * y_adjustment;
	vec3_t up_adjustment = vec3_t{ 0.0f, 0.0f, z_adjustment };
	vec3_t total_adjustment = forward_adjustment + right_adjustment + up_adjustment;
	start += total_adjustment;

	dir = self->enemy->s.origin - start;
	dir.normalize();

	self->locked_start = start;
	self->locked_dir = dir;
}

void scraggacudaSpit(edict_t* self)
{
	// Forzar que el ángulo se mantenga fijo
	self->s.angles = self->locked_angles;

	vec3_t start = self->locked_start;
	vec3_t dir = self->locked_dir;

	monster_fire_heatbeam(self, start, dir, start, 1, 0, MZ2_WIDOW2_BEAM_SWEEP_1);

	if (self->heatbeam_time > level.time)
		self->s.frame = 25;
}


void wizarcuda_strogg_prespit(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_attack, 1, ATTN_NORM, 0);
}

void wicarus_fire_blaster(edict_t* self);
mframe_t wizarcuda_strogg_frames_attack [] =
{
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, scraggacudaSpit_checker},
	{ai_charge, 0, scraggacudaSpit},
	{ai_charge, 0, NULL}
};
MMOVE_T(wizarcuda_strogg_move_attack) = {22, 27, wizarcuda_strogg_frames_attack, wizarcuda_strogg_run };

MONSTERINFO_ATTACK (wizarcuda_strogg_attack) (edict_t *self) -> void
{
	M_SetAnimation(self, &wizarcuda_strogg_move_attack);	
}

// Pain
mframe_t wizarcuda_strogg_frames_pain [] ={
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
MMOVE_T(wizarcuda_strogg_move_pain) = {42, 45, wizarcuda_strogg_frames_pain, wizarcuda_strogg_run};

PAIN (wizarcuda_strogg_pain) (edict_t* self, edict_t* other, float kick, int damage, const mod_t& mod) -> void
{
	self->heatbeam_time = 0.0_sec;
	if (skill->value == 3)
		return;
	if (level.time < self->pain_debounce_time)
		return;
	gi.sound (self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

	self->pain_debounce_time = level.time + 3_sec;
	M_SetAnimation(self, &wizarcuda_strogg_move_pain);	
}

void wizarcuda_strogg_fling(edict_t* self)
{
	self->velocity[0] = -200 + 400 * crandom();
	self->velocity[1] = -200 + 400 * crandom();
	self->velocity[2] = 100 + 100 * crandom();

	self->mins = {-16, -16, -24};
	self->maxs = {16, 16, 8};
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;

}

void wizarcuda_strogg_dead(edict_t *self)
{
	self->nextthink = 0_ms;
	gi.linkentity(self);
}

// Death
mframe_t wizarcuda_strogg_frames_death [] ={
	{ai_move, 0, wizarcuda_strogg_fling},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
MMOVE_T(wizarcuda_strogg_move_death) = {46, 53, wizarcuda_strogg_frames_death, wizarcuda_strogg_dead};

DIE (wizarcuda_strogg_die) (edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t& mod) -> void
{

	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		ThrowGibs(self, damage, {
			{ "models/monsters/scraggacuda/gibs/g_arm.md2", GIB_ACID },
			{ "models/monsters/scraggacuda/gibs/g_arm.md2", GIB_ACID },
			{ "models/monsters/scraggacuda/gibs/g_leg.md2", GIB_ACID },
			{ "models/monsters/scraggacuda/gibs/g_head.md2", GIB_ACID | GIB_HEAD }
			});
		self->deadflag = true;
		return;
	}
	if (self->deadflag == true)
		return;
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);

	self->deadflag = true;
	self->takedamage = true;
	M_SetAnimation(self, &wizarcuda_strogg_move_death);		
}

MONSTERINFO_SIGHT (wizarcuda_strogg_sight) (edict_t* self, edict_t* other) -> void
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

MONSTERINFO_SEARCH (wizarcuda_strogg_search) (edict_t *self) -> void
{
	float r;
	r = frandom() * 5;

	if (r > 4.5)
		gi.sound(self, CHAN_VOICE, sound_idle1, 1, ATTN_NORM, 0);
	if (r < 1.5)
		gi.sound(self, CHAN_VOICE, sound_idle2, 1, ATTN_NORM, 0);
}

MONSTERINFO_SETSKIN(wizarcuda_strogg_setskin) (edict_t* self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}


void SP_monster_wizarcuda_strogg(edict_t *self)
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

	self->s.modelindex = gi.modelindex("models/monsters/scraggacuda/tris.md2");
	gi.modelindex("models/monsters/scraggacuda/gibs/g_arm.md2");
	gi.modelindex("models/monsters/scraggacuda/gibs/g_leg.md2");
	gi.modelindex("models/monsters/scraggacuda/gibs/g_head.md2");

	self->health = 120 * st.health_multiplier;
	self->gib_health = -40;

	self->mass = 80;

	self->pain = wizarcuda_strogg_pain;
	self->die = wizarcuda_strogg_die;
	self->monsterinfo.stand = wizarcuda_strogg_stand;
	self->monsterinfo.walk = wizarcuda_strogg_walk;
	self->monsterinfo.run = wizarcuda_strogg_run;
	self->monsterinfo.attack = wizarcuda_strogg_attack;
	self->monsterinfo.sight = wizarcuda_strogg_sight;
	self->monsterinfo.search = wizarcuda_strogg_search;
	self->monsterinfo.setskin = wizarcuda_strogg_setskin;

	gi.linkentity(self);
	M_SetAnimation(self, &wizarcuda_strogg_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	flymonster_start(self);
}
