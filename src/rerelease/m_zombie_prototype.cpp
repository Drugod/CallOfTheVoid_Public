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
// m_zombie.c

#include "g_local.h"
#include "m_zombie_prototype.h"

static cached_soundindex sound_sight;
static cached_soundindex sound_search;
static cached_soundindex sound_fling;
static cached_soundindex sound_pain1;
static cached_soundindex sound_pain2;
static cached_soundindex sound_fall;
static cached_soundindex sound_miss;
static cached_soundindex sound_hit;

void zombie_prototype_down(edict_t *self);
void zombie_prototype_get_up_attempt(edict_t *self);
vec3_t* SightEndtToDir(edict_t* self, vec3_t orig_dir);

// Stand
mframe_t zombie_prototype_frames_stand [] =
{
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
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
MMOVE_T(zombie_prototype_move_stand) = {0, 14, zombie_prototype_frames_stand, NULL};

MONSTERINFO_STAND(zombie_prototype_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &zombie_prototype_move_stand);	
}

void zombie_prototype_reset_state(edict_t *self)
{
	self->zombie_state = 0;
}

// Run
mframe_t zombie_prototype_frames_run [] =
{
	{ai_run, 1, zombie_prototype_reset_state},
	{ai_run, 1, NULL},
	{ai_run, 0, NULL},
	{ai_run, 1, NULL},
	{ai_run, 2, NULL},
	{ai_run, 3, NULL},
	{ai_run, 4, NULL},
	{ai_run, 4, NULL},
	{ai_run, 2, NULL},
	{ai_run, 0, NULL},
	{ai_run, 0, NULL},
	{ai_run, 0, NULL},
	{ai_run, 2, NULL},
	{ai_run, 4, NULL},
	{ai_run, 6, NULL},
	{ai_run, 7, NULL},
	{ai_run, 3, NULL},
	{ai_run, 8, NULL}
};
MMOVE_T(zombie_prototype_move_run) = {34, 51, zombie_prototype_frames_run, NULL};

MONSTERINFO_RUN(zombie_prototype_run) (edict_t *self) -> void
{
	M_SetAnimation(self, &zombie_prototype_move_run);	
}

// walk
mframe_t zombie_prototype_frames_walk [] =
{
	{ai_walk, 0, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 3, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 1, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 2, NULL},
	{ai_walk, 1, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL},
	{ai_walk, 0, NULL}
};
MMOVE_T(zombie_prototype_move_walk) = {15, 33, zombie_prototype_frames_walk, NULL};

MONSTERINFO_WALK(zombie_prototype_walk) (edict_t *self) -> void
{
	M_SetAnimation(self, &zombie_prototype_move_walk);	
}

// Sight
MONSTERINFO_SIGHT(zombie_prototype_sight) (edict_t* self, edict_t* other) -> void
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

TOUCH(zombie_prototype_gib_touch) (edict_t* self, edict_t* other, const trace_t& tr, bool other_touching_self) -> void
{
	vec3_t point;
	vec3_t normal;
	int	   damage;
	if (other == self->owner)
		return;
	if (tr.surface && (tr.surface->flags & SURF_SKY))
	{
		G_FreeEdict(self);
		return;
	}
	if (other->takedamage)
	{
		gi.sound(self, CHAN_WEAPON, sound_hit, 1, ATTN_NORM, 0);
		normal = self->velocity;
		normal.normalize();
		point = self->s.origin + (normal * self->maxs[0]);
		damage = self->dmg;

		//T_Damage(other, self, self->owner, self->s.origin, self->s.origin, vec3_origin, self->dmg, self->dmg, 0, 0);
		T_Damage(other, self, self, self->velocity, point, normal, damage, damage, DAMAGE_NONE, MOD_UNKNOWN);
		G_FreeEdict(self);
		return;
	}
	gi.sound(self, CHAN_WEAPON, sound_miss, 1, ATTN_NORM, 0);
	self->avelocity = { 0, 0, 0 };
	self->velocity = { 0, 0, 0 };
	self->touch = NULL;
}

void fire_zombie_prototype_gib(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed)
{
	edict_t	*gib;
	vec3_t	end;
	vec3_t	dir;

	if (!self->enemy || self->enemy == self)
		return;

	end = self->enemy->s.origin;
	end[2] += self->enemy->viewheight;
	dir = end - self->s.origin;
	dir.normalize();

	//VectorCopy(SightEndtToDir(self, aimdir)[0], aimdir);
	aimdir = SightEndtToDir(self, aimdir)[0];
	//VectorNormalize(aimdir);
	aimdir.normalize();

	gib = G_Spawn();
	//VectorCopy(start, gib->s.origin);
	gib->s.origin = start;

	//VectorCopy(start, gib->s.old_origin);
	gib->s.old_origin = start;

	//vectoangles(aimdir, gib->s.angles);
	gib->s.angles = vectoangles(aimdir);

	//VectorScale(aimdir, speed, gib->velocity);
	/*gib->velocity[0] = speed * aimdir[0];
	gib->velocity[1] = speed * aimdir[1];
	gib->velocity[2] = speed * aimdir[2];*/
	gib->velocity[0] = speed * dir[0];
	gib->velocity[1] = speed * dir[1];
	gib->velocity[2] = speed * dir[2];

	//VectorMA(gib->velocity, 200 + crandom() * 10.0, up, gib->velocity);
	/*gib->velocity[0] = gib->velocity[0] + (200 + crandom() * 10.0 * up[0]);
	gib->velocity[1] = gib->velocity[1] + (200 + crandom() * 10.0 * up[1]);
	gib->velocity[2] = gib->velocity[2] + (200 + crandom() * 10.0 * up[2]);*/

	//VectorMA(gib->velocity, crandom() * 10.0, right, gib->velocity);
	/*gib->velocity[0] = gib->velocity[0] + (crandom() * 10.0 * right[0]);
	gib->velocity[1] = gib->velocity[1] + (crandom() * 10.0 * right[1]);
	gib->velocity[2] = gib->velocity[2] + (crandom() * 10.0 * right[2]);*/

	//VectorSet(gib->avelocity, 300, 300, 300);
	gib->avelocity[0] = 300;
	gib->avelocity[1] = 300;
	gib->avelocity[2] = 300;

	gib->movetype = MOVETYPE_BOUNCE;
	gib->clipmask = MASK_PROJECTILE;
	gib->solid = SOLID_BBOX;
	gib->s.effects |= EF_GIB;
	//VectorClear(gib->mins);
	gib->mins[0] = 0;
	gib->mins[1] = 0;
	gib->mins[2] = 0;
	//VectorClear(gib->maxs);
	gib->maxs[0] = 0;
	gib->maxs[1] = 0;
	gib->maxs[2] = 0;
	gib->s.modelindex = gi.modelindex ("models/objects/gibs/sm_meat/tris.md2");
	gib->owner = self;
	gib->touch = zombie_prototype_gib_touch;
	//gib->nextthink = level.time + 2_ms;
	//gib->think = G_FreeEdict;
	gib->dmg = damage;
	gib->enemy = self->enemy;
	gi.linkentity(gib);
	gi.sound(self, CHAN_WEAPON, sound_fling, 1, ATTN_NORM, 0);
}

void FireZombiePrototypeGib(edict_t *self)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;
	vec3_t	vec;
	vec3_t offset = { 16, 0, 8 };

	AngleVectors(self->s.angles, forward, right, NULL);
	//G_ProjectSource(self->s.origin, offset, forward, right, start);
	start = G_ProjectSource(self->s.origin, offset, forward, right);
	
	//VectorCopy(self->enemy->s.origin, vec);
	vec = self->enemy->s.origin;
	dir = vec - start;

	//VectorNormalize(dir);
	dir.normalize();
	fire_zombie_prototype_gib(self, start, vec, 10, 1200);
}

// Attack (1)
mframe_t zombie_prototype_frames_attack1 [] =
{
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, FireZombiePrototypeGib}
};
MMOVE_T (zombie_prototype_move_attack1) = {52, 64, zombie_prototype_frames_attack1, zombie_prototype_run};

// Attack (2)
mframe_t zombie_prototype_frames_attack2 [] =
{
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, FireZombiePrototypeGib}
};
MMOVE_T (zombie_prototype_move_attack2) = {65, 78, zombie_prototype_frames_attack2, zombie_prototype_run};

// Attack (3)
mframe_t zombie_prototype_frames_attack3 [] =
{
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, FireZombiePrototypeGib}
};
MMOVE_T (zombie_prototype_move_attack3) = {79, 90, zombie_prototype_frames_attack3, zombie_prototype_run};

// Attack
MONSTERINFO_ATTACK (zombie_prototype_attack)(edict_t *self) ->void
{
	float r = frandom();
	if (r < 0.3)
		M_SetAnimation(self, &zombie_prototype_move_attack1);	
	else if (r < 0.6)
		M_SetAnimation(self, &zombie_prototype_move_attack2);			
	else
		M_SetAnimation(self, &zombie_prototype_move_attack3);	
}

mframe_t zombie_prototype_frames_get_up [] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
MMOVE_T (zombie_prototype_move_get_up) = {173, 191, zombie_prototype_frames_get_up, zombie_prototype_run};

void zombie_prototype_pain1(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
}

void zombie_prototype_pain2(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
}

void zombie_prototype_hit_floor(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_fall, 1, ATTN_NORM, 0);
}

void zombie_prototype_get_up(edict_t *self)
{
	self->maxs = { 16,16,40 };
	self->takedamage = true;
	self->health = 60;
	zombie_prototype_sight(self, self->enemy);

	if (!M_walkmove(self, 0, 0))
	{
		zombie_prototype_get_up_attempt(self);
		return;
	}
	M_SetAnimation(self, &zombie_prototype_move_get_up);
}

void zombie_prototype_start_fall(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
}

// Down
mframe_t zombie_prototype_frames_get_up_attempt [] =
{
	{ai_move, 0,		zombie_prototype_get_up_attempt}
};
MMOVE_T (zombie_prototype_move_get_up_attempt) = {173, 173, zombie_prototype_frames_get_up_attempt, NULL};

void zombie_prototype_get_up_attempt(edict_t *self)
{
	static int down = 0;
	zombie_prototype_down(self);

	// Try getting up in 5 seconds
	if (down >= 500){
		down = 0;
		zombie_prototype_get_up(self);
		return;
	}
	self->s.frame = 172;
	M_SetAnimation(self, &zombie_prototype_move_get_up_attempt);	
	down++;
}

void zombie_prototype_down(edict_t *self)
{
	self->takedamage = false;
	self->health = 60;
	self->maxs = { 16, 16, 0 };
}

// Pain (1)
mframe_t zombie_prototype_frames_pain1 [] =
{
	{ai_move, 0, zombie_prototype_pain1},
	{ai_move, 3, NULL},
	{ai_move, 1, NULL},
	{ai_move, 1, NULL},
	{ai_move, 3, NULL},
	{ai_move, 1, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
MMOVE_T (zombie_prototype_move_pain1) = {91, 102, zombie_prototype_frames_pain1, zombie_prototype_run};

// Pain (2)
mframe_t zombie_prototype_frames_pain2 [] =
{
	{ai_move, 0, zombie_prototype_pain2},
	{ai_move, 2, NULL},
	{ai_move, 8, NULL},
	{ai_move, 6, NULL},
	{ai_move, 2, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, zombie_prototype_hit_floor},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 1, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
MMOVE_T (zombie_prototype_move_pain2) = {103, 130, zombie_prototype_frames_pain2, zombie_prototype_run};

// Pain (3)
mframe_t zombie_prototype_frames_pain3 [] =
{
	{ai_move, 0, zombie_prototype_pain2},
	{ai_move, 0, NULL},
	{ai_move, 3, NULL},
	{ai_move, 1, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 1, NULL},
	{ai_move, 1, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
MMOVE_T (zombie_prototype_move_pain3) = {131, 148, zombie_prototype_frames_pain3, zombie_prototype_run};

// Pain (4)
mframe_t zombie_prototype_frames_pain4 [] =
{
	{ai_move, 0, zombie_prototype_pain1},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 1, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
MMOVE_T (zombie_prototype_move_pain4) = {149, 161, zombie_prototype_frames_pain4, zombie_prototype_run};

// Pain (5)
mframe_t zombie_prototype_frames_fall_start [] =
{
	{ai_move, 0,	zombie_prototype_start_fall},
	{ai_move, -8,	NULL},
	{ai_move, -5,	NULL},
	{ai_move, -3,	NULL},
	{ai_move, -1,	NULL},
	{ai_move, -2,	NULL},
	{ai_move, -1,	NULL},
	{ai_move, -1,	NULL},
	{ai_move, -2,	NULL},
	{ai_move, 0,	zombie_prototype_hit_floor},
	{ai_move, 0,	zombie_prototype_down}
};
MMOVE_T (zombie_prototype_move_fall_start) = {162, 172, zombie_prototype_frames_fall_start, zombie_prototype_get_up_attempt};

// Pain
PAIN (zombie_prototype_pain) (edict_t* self, edict_t* other, float kick, int damage, const mod_t& mod) -> void
{
	float r;
	self->health = 60;

	if (damage < 9)
		return;
	if (self->zombie_state == 2)
		return;
	if (damage >= 25)
	{
		self->zombie_state = 2;
		M_SetAnimation(self, &zombie_prototype_move_fall_start);
		return;
	}
	if (self->pain_debounce_time > level.time)
	{
		self->zombie_state = 2;
		M_SetAnimation(self, &zombie_prototype_move_fall_start);		
		return;
	}
	if (self->zombie_state)
	{
		self->pain_debounce_time = level.time + 3_sec;
		return;
	}
	self->zombie_state = 1;

	// decino: No pain animations in Nightmare mode
	if (skill->value >= 3)
		return;
	r = frandom();

	if (r < 0.25)
		M_SetAnimation(self, &zombie_prototype_move_pain1);	
	else if (r <  0.5)
		M_SetAnimation(self, &zombie_prototype_move_pain2);	
	else if (r <  0.75)
		M_SetAnimation(self, &zombie_prototype_move_pain3);	
	else
		M_SetAnimation(self, &zombie_prototype_move_pain4);	
}

// Death
DIE (zombie_prototype_die) (edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t& mod) -> void
{
	gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

	ThrowGibs(self, damage, {
		{ "models/objects/gibs/bone/tris.md2" },
		{ "models/objects/gibs/sm_meat/tris.md2" },
		{ "models/objects/gibs/head2/tris.md2", GIB_HEAD }
		});
	self->deadflag = true;
}

// Search
MONSTERINFO_SEARCH (zombie_prototype_search) (edict_t *self) -> void
{
	if (frandom() < 0.2)
		gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

void SP_monster_zombie_prototype(edict_t *self)
{
	self->s.modelindex = gi.modelindex("models/monsters/zombie_prototype/tris.md2");
	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, 40 };
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;	
	
	self->health = 60 * st.health_multiplier;

	sound_sight.assign("zombie/z_idle_s.wav");
	sound_search.assign("zombie/idle_w2_s.wav");
	sound_fling.assign("zombie/z_shot1.wav");
	sound_pain1.assign("zombie/z_pain_s.wav");
	sound_pain2.assign("zombie/z_pain1_s.wav");
	sound_fall.assign("zombie/z_fall.wav");
	sound_miss.assign("zombie/z_miss.wav");
	sound_hit.assign("zombie/z_hit.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->gib_health = -5;
	self->mass = 60;

	self->monsterinfo.stand = zombie_prototype_stand;
	self->monsterinfo.walk = zombie_prototype_walk;
	self->monsterinfo.run = zombie_prototype_run;
	self->monsterinfo.attack = zombie_prototype_attack;
	self->monsterinfo.sight = zombie_prototype_sight;
	self->monsterinfo.search = zombie_prototype_search;

	self->pain = zombie_prototype_pain;
	self->die = zombie_prototype_die;

	gi.linkentity(self);

	M_SetAnimation(self, &zombie_prototype_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);
}