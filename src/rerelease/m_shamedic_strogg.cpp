// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

MEDIC

==============================================================================
*/

#include "g_local.h"
#include "m_shamedic_strogg.h"
#include "m_flash.h"

constexpr float MEDIC_MIN_DISTANCE = 32;
constexpr float MEDIC_MAX_HEAL_DISTANCE = 400;
constexpr gtime_t MEDIC_TRY_TIME = 10_sec;

// FIXME -
//
// owner moved to monsterinfo.healer instead
//
// For some reason, the healed monsters are rarely ending up in the floor
//
// 5/15/1998 I think I fixed these, keep an eye on them

void M_SetEffects(edict_t *ent);
bool FindTarget(edict_t *self);
void FoundTarget(edict_t *self);
void ED_CallSpawn(edict_t *ent);

static cached_soundindex sound_idle1;
static cached_soundindex sound_pain1;
static cached_soundindex sound_pain2;
static cached_soundindex sound_die;
static cached_soundindex sound_sight;
static cached_soundindex sound_search;
static cached_soundindex sound_hook_launch;
static cached_soundindex sound_hook_hit;
static cached_soundindex sound_hook_heal;
static cached_soundindex sound_hook_retract;

// PMM - commander sounds
static cached_soundindex commander_sound_idle1;
static cached_soundindex commander_sound_pain1;
static cached_soundindex commander_sound_pain2;
static cached_soundindex commander_sound_die;
static cached_soundindex commander_sound_sight;
static cached_soundindex commander_sound_search;
static cached_soundindex commander_sound_hook_launch;
static cached_soundindex commander_sound_hook_hit;
static cached_soundindex commander_sound_hook_heal;
static cached_soundindex commander_sound_hook_retract;
static cached_soundindex commander_sound_spawn;

constexpr const char *default_reinforcements = "monster_soldier_light 1;monster_soldier 2;monster_soldier_ss 2;monster_infantry 3;monster_gunner 4;monster_shamedic 5;monster_gladiator 6";
constexpr int32_t default_monster_slots_base = 3;

static const float inverse_log_slots = pow(2, MAX_REINFORCEMENTS);

constexpr std::array<vec3_t, MAX_REINFORCEMENTS> reinforcement_position = {
	vec3_t { 80, 0, 0 },
	vec3_t { 40, 60, 0 },
	vec3_t { 40, -60, 0 },
	vec3_t { 0, 80, 0 },
	vec3_t { 0, -80, 0 }
};

// filter out the reinforcement indices we can pick given the space we have left
static void M_PickValidReinforcements(edict_t *self, int32_t space, std::vector<uint8_t> &output)
{
	output.clear();

	for (uint8_t i = 0; i < self->monsterinfo.reinforcements.num_reinforcements; i++)
		if (self->monsterinfo.reinforcements.reinforcements[i].strength <= space)
			output.push_back(i);
}

// pick an array of reinforcements to use; note that this does not modify `self`
std::array<uint8_t, MAX_REINFORCEMENTS> M_PickReinforcements(edict_t* self, int32_t& num_chosen, int32_t max_slots = 0);
//std::array<uint8_t, MAX_REINFORCEMENTS> M_PickReinforcements(edict_t *self, int32_t &num_chosen, int32_t max_slots = 0)
//{
//	static std::vector<uint8_t> available;
//	std::array<uint8_t, MAX_REINFORCEMENTS> chosen;
//	chosen.fill(255);
//
//	// decide how many things we want to spawn;
//	// this is on a logarithmic scale
//	// so we don't spawn too much too often.
//	int32_t num_slots = max(1, (int32_t) log2(frandom(inverse_log_slots)));
//
//	// we only have this many slots left to use
//	int32_t remaining = self->monsterinfo.monster_slots - self->monsterinfo.monster_used;
//	
//	for (num_chosen = 0; num_chosen < num_slots; num_chosen++)
//	{
//		// ran out of slots!
//		if ((max_slots && num_chosen == max_slots) || !remaining)
//			break;
//
//		// get everything we could choose
//		M_PickValidReinforcements(self, remaining, available);
//
//		// can't pick any
//		if (!available.size())
//			break;
//
//		// select monster, TODO fairly
//		chosen[num_chosen] = random_element(available);
//
//		remaining -= self->monsterinfo.reinforcements.reinforcements[chosen[num_chosen]].strength;
//	}
//
//	return chosen;
//}

void M_SetupReinforcements(const char* reinforcements, reinforcement_list_t& list);
//void M_SetupReinforcements(const char *reinforcements, reinforcement_list_t &list)
//{
//	// count up the semicolons
//	list.num_reinforcements = 0;
//
//	if (!*reinforcements)
//		return;
//
//	list.num_reinforcements++;
//
//	for (size_t i = 0; i < strlen(reinforcements); i++)
//		if (reinforcements[i] == ';')
//			list.num_reinforcements++;
//
//	// allocate
//	list.reinforcements = (reinforcement_t *) gi.TagMalloc(sizeof(reinforcement_t) * list.num_reinforcements, TAG_LEVEL);
//
//	// parse
//	const char *p = reinforcements;
//	reinforcement_t *r = list.reinforcements;
//
//	st = {};
//
//	while (true)
//	{
//		const char *token = COM_ParseEx(&p, "; ");
//
//		if (!*token || r == list.reinforcements + list.num_reinforcements)
//			break;
//
//		r->classname = G_CopyString(token, TAG_LEVEL);
//
//		token = COM_ParseEx(&p, "; ");
//
//		r->strength = atoi(token);
//
//		edict_t *newEnt = G_Spawn();
//
//		newEnt->classname = r->classname;
//
//		newEnt->monsterinfo.aiflags |= AI_DO_NOT_COUNT;
//
//		ED_CallSpawn(newEnt);
//
//		r->mins = newEnt->mins;
//		r->maxs = newEnt->maxs;
//
//		G_FreeEdict(newEnt);
//
//		r++;
//	}
//}

void cleanupHeal(edict_t* self, bool change_frame);
//void cleanupHeal(edict_t *self, bool change_frame)
//{
//	// clean up target, if we have one and it's legit
//	if (self->enemy && self->enemy->inuse)
//		cleanupHealTarget(self->enemy);
//
//	if (self->oldenemy && self->oldenemy->inuse && self->oldenemy->health > 0)
//	{
//		self->enemy = self->oldenemy;
//		HuntTarget(self, false);
//	}
//	else
//	{
//		self->enemy = self->goalentity = nullptr;
//		self->oldenemy = nullptr;
//		if (!FindTarget(self))
//		{
//			// no valid enemy, so stop acting
//			self->monsterinfo.pausetime = HOLD_FOREVER;
//			self->monsterinfo.stand(self);
//			return;
//		}
//	}
//
//	if (change_frame)
//		self->monsterinfo.nextframe = FRAME_attack52;
//}

void abortHeal(edict_t* self, bool change_frame, bool gib, bool mark);
//void abortHeal(edict_t *self, bool change_frame, bool gib, bool mark)
//{
//	int				 hurt;
//	constexpr vec3_t pain_normal = { 0, 0, 1 };
//
//	if (self->enemy && self->enemy->inuse)
//	{
//		cleanupHealTarget(self->enemy);
//
//		// gib em!
//		if (mark)
//		{
//			// if the first badMedic slot is filled by a medic, skip it and use the second one
//			if ((self->enemy->monsterinfo.badMedic1) && (self->enemy->monsterinfo.badMedic1->inuse) && (!strncmp(self->enemy->monsterinfo.badMedic1->classname, "monster_shamedic", 13)))
//			{
//				self->enemy->monsterinfo.badMedic2 = self;
//			}
//			else
//			{
//				self->enemy->monsterinfo.badMedic1 = self;
//			}
//		}
//
//		if (gib)
//		{
//			if (self->enemy->gib_health)
//				hurt = -self->enemy->gib_health;
//			else
//				hurt = 500;
//
//			T_Damage(self->enemy, self, self, vec3_origin, self->enemy->s.origin,
//					 pain_normal, hurt, 0, DAMAGE_NONE, MOD_UNKNOWN);
//		}
//	}
//	// clean up self
//
//	// clean up target
//	cleanupHeal(self, change_frame);
//
//	self->monsterinfo.aiflags &= ~AI_MEDIC;
//	self->monsterinfo.medicTries = 0;
//}

//bool canReach(edict_t *self, edict_t *other)
//{
//	vec3_t	spot1;
//	vec3_t	spot2;
//	trace_t trace;
//
//	spot1 = self->s.origin;
//	spot1[2] += self->viewheight;
//	spot2 = other->s.origin;
//	spot2[2] += other->viewheight;
//	trace = gi.traceline(spot1, spot2, self, MASK_PROJECTILE | MASK_WATER);
//	return trace.fraction == 1.0f || trace.ent == other;
//}

edict_t *shamedic_FindDeadMonster(edict_t *self)
{
	float	 radius;
	edict_t *ent = nullptr;
	edict_t *best = nullptr;

	if (self->monsterinfo.react_to_damage_time > level.time)
		return nullptr;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		radius = MEDIC_MAX_HEAL_DISTANCE;
	else
		radius = 1024;

	while ((ent = findradius(ent, self->s.origin, radius)) != nullptr)
	{
		if (ent == self)
			continue;
		if (!(ent->svflags & SVF_MONSTER))
			continue;
		if (ent->monsterinfo.aiflags & AI_GOOD_GUY)
			continue;
		// check to make sure we haven't bailed on this guy already
		if ((ent->monsterinfo.badMedic1 == self) || (ent->monsterinfo.badMedic2 == self))
			continue;
		if (ent->monsterinfo.healer)
			// FIXME - this is correcting a bug that is somewhere else
			// if the healer is a monster, and it's in medic mode .. continue .. otherwise
			//   we will override the healer, if it passes all the other tests
			if ((ent->monsterinfo.healer->inuse) && (ent->monsterinfo.healer->health > 0) &&
				(ent->monsterinfo.healer->svflags & SVF_MONSTER) && (ent->monsterinfo.healer->monsterinfo.aiflags & AI_MEDIC))
				continue;
		if (ent->health > 0)
			continue;
		if ((ent->nextthink) && (ent->think != monster_dead_think))
			continue;
		if (!visible(self, ent))
			continue;
		if (!strncmp(ent->classname, "player", 6)) // stop it from trying to heal player_noise entities
			continue;
		// FIXME - there's got to be a better way ..
		// make sure we don't spawn people right on top of us
		if (realrange(self, ent) <= MEDIC_MIN_DISTANCE)
			continue;
		if (!best)
		{
			best = ent;
			continue;
		}
		if (ent->max_health <= best->max_health)
			continue;
		best = ent;
	}

	if (best)
		self->timestamp = level.time + MEDIC_TRY_TIME;

	return best;
}

MONSTERINFO_IDLE(shamedic_idle) (edict_t *self) -> void
{
	edict_t *ent;

	// PMM - commander sounds
	if (self->mass == 400)
		gi.sound(self, CHAN_VOICE, sound_idle1, 1, ATTN_IDLE, 0);
	else
		gi.sound(self, CHAN_VOICE, commander_sound_idle1, 1, ATTN_IDLE, 0);

	if (!self->oldenemy)
	{
		ent = shamedic_FindDeadMonster(self);
		if (ent)
		{
			self->oldenemy = self->enemy;
			self->enemy = ent;
			self->enemy->monsterinfo.healer = self;
			self->monsterinfo.aiflags |= AI_MEDIC;
			FoundTarget(self);
		}
	}
}

MONSTERINFO_SEARCH(shamedic_search) (edict_t *self) -> void
{
	edict_t *ent;

	// PMM - commander sounds
	if (self->mass == 400)
		gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_IDLE, 0);
	else
		gi.sound(self, CHAN_VOICE, commander_sound_search, 1, ATTN_IDLE, 0);

	if (!self->oldenemy)
	{
		ent = shamedic_FindDeadMonster(self);
		if (ent)
		{
			self->oldenemy = self->enemy;
			self->enemy = ent;
			self->enemy->monsterinfo.healer = self;
			self->monsterinfo.aiflags |= AI_MEDIC;
			FoundTarget(self);
		}
	}
}

MONSTERINFO_SIGHT(shamedic_sight) (edict_t *self, edict_t *other) -> void
{
	// PMM - commander sounds
	if (self->mass == 400)
		gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, commander_sound_sight, 1, ATTN_NORM, 0);
}

mframe_t shamedic_frames_stand[] = {
	{ ai_stand, 0, shamedic_idle },
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
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
};
MMOVE_T(shamedic_move_stand) = { FRAME_wait1, FRAME_wait90, shamedic_frames_stand, nullptr };

MONSTERINFO_STAND(shamedic_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &shamedic_move_stand);
}

mframe_t shamedic_frames_walk[] = {
	{ ai_walk, 6.2f },
	{ ai_walk, 18.1f, monster_footstep },
	{ ai_walk, 1 },
	{ ai_walk, 9 },
	{ ai_walk, 10 },
	{ ai_walk, 9 },
	{ ai_walk, 11 },
	{ ai_walk, 11.6f, monster_footstep },
	{ ai_walk, 2 },
	{ ai_walk, 9.9f },
	{ ai_walk, 14 },
	{ ai_walk, 9.3f }
};
MMOVE_T(shamedic_move_walk) = { FRAME_walk1, FRAME_walk12, shamedic_frames_walk, nullptr };

MONSTERINFO_WALK(shamedic_walk) (edict_t *self) -> void
{
	M_SetAnimation(self, &shamedic_move_walk);
}

mframe_t shamedic_frames_run[] = {
	{ ai_run, 18 },
	{ ai_run, 22.5f, monster_footstep },
	{ ai_run, 25.4f, monster_done_dodge },
	{ ai_run, 23.4f, monster_footstep },
	{ ai_run, 24 },
	{ ai_run, 35.6f }
};
MMOVE_T(shamedic_move_run) = { FRAME_run1, FRAME_run6, shamedic_frames_run, nullptr };

MONSTERINFO_RUN(shamedic_run) (edict_t *self) -> void
{
	monster_done_dodge(self);

	if (!(self->monsterinfo.aiflags & AI_MEDIC))
	{
		edict_t *ent;

		ent = shamedic_FindDeadMonster(self);
		if (ent)
		{
			self->oldenemy = self->enemy;
			self->enemy = ent;
			self->enemy->monsterinfo.healer = self;
			self->monsterinfo.aiflags |= AI_MEDIC;
			FoundTarget(self);
			return;
		}
	}

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		M_SetAnimation(self, &shamedic_move_stand);
	else
		M_SetAnimation(self, &shamedic_move_run);
}

mframe_t shamedic_frames_pain1[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(shamedic_move_pain1) = { FRAME_paina2, FRAME_paina6, shamedic_frames_pain1, shamedic_run };

mframe_t shamedic_frames_pain2[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, monster_footstep },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, monster_footstep }
};
MMOVE_T(shamedic_move_pain2) = { FRAME_painb2, FRAME_painb13, shamedic_frames_pain2, shamedic_run };

PAIN(shamedic_pain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
{
	monster_done_dodge(self);

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3_sec;

	float r = frandom();

	if (self->mass > 400)
	{
		if (damage < 35)
		{
			gi.sound(self, CHAN_VOICE, commander_sound_pain1, 1, ATTN_NORM, 0);

			if (mod.id != MOD_CHAINFIST)
				return;
		}

		gi.sound(self, CHAN_VOICE, commander_sound_pain2, 1, ATTN_NORM, 0);
	}
	else if (r < 0.5f)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	
	if (!M_ShouldReactToPain(self, mod))
		return; // no pain anims in nightmare

	// if we're healing someone, we ignore pain
	if (mod.id != MOD_CHAINFIST && (self->monsterinfo.aiflags & AI_MEDIC))
		return;

	if (self->mass > 400)
	{
		self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;

		if (r < (min(((float) damage * 0.005f), 0.5f))) // no more than 50% chance of big pain
			M_SetAnimation(self, &shamedic_move_pain2);
		else
			M_SetAnimation(self, &shamedic_move_pain1);
	}
	else if (r < 0.5f)
		M_SetAnimation(self, &shamedic_move_pain1);
	else
		M_SetAnimation(self, &shamedic_move_pain2);

	// PMM - clear duck flag
	if (self->monsterinfo.aiflags & AI_DUCKED)
		monster_duck_up(self);

	abortHeal(self, false, false, false);
}

MONSTERINFO_SETSKIN(shamedic_setskin) (edict_t *self) -> void
{
	if ((self->health < (self->max_health / 2)))
		self->s.skinnum |= 1;
	else
		self->s.skinnum &= ~1;
}

constexpr spawnflags_t SPAWNFLAG_SHAMBLER_PRECISE = 1_spawnflag;

vec3_t FindShamedicOffset(edict_t* self)
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

//void ShamedicCastLightning(edict_t* self)
//{
//	if (!self->enemy)
//		return;
//
//	vec3_t start;
//	vec3_t dir;
//	vec3_t forward, right;
//	vec3_t offset = FindShamedicOffset(self);
//
//	AngleVectors(self->s.angles, forward, right, nullptr);
//	start = M_ProjectFlashSource(self, offset, forward, right);
//
//	//float y_adjustment = -6.5f;//center
//	//float y_adjustment = 20.5f;//left
//	float y_adjustment = -36.5f;//right
//	float z_adjustment = 32.0f;
//
//	vec3_t right_adjustment = right * y_adjustment;
//	vec3_t up_adjustment = vec3_t{ 0.0f, 0.0f, z_adjustment };
//	vec3_t total_adjustment = right_adjustment + up_adjustment;
//	start += total_adjustment;
//
//	dir = self->pos1 - start;
//	dir.normalize();
//
//	// calc direction to where we targted
//	PredictAim(self, self->enemy, start, 0, false, self->spawnflags.has(SPAWNFLAG_SHAMBLER_PRECISE) ? 0.f : 0.1f, &dir, nullptr);
//
//	vec3_t end = start + (dir * 8192);
//	trace_t tr = gi.traceline(start, end, self, MASK_PROJECTILE | CONTENTS_SLIME | CONTENTS_LAVA);
//
//	gi.WriteByte(svc_temp_entity);
//	gi.WriteByte(TE_LIGHTNING);
//	gi.WriteEntity(self);	// source entity
//	gi.WriteEntity(world); // destination entity
//	gi.WritePosition(start);
//	gi.WritePosition(tr.endpos);
//	gi.multicast(start, MULTICAST_PVS, false);
//
//	fire_bullet(self, start, dir, irandom(8, 12), 15, 0, 0, MOD_TESLA);
//}

// Nueva función que acepta ajustes en y y z y permite diferentes entidades de origen
void ShamedicCastLightningWithOffset(edict_t* self, float y_adjustment, float z_adjustment, edict_t* source_entity)
{
	if (!self->enemy)
		return;

	vec3_t start;
	vec3_t dir;
	vec3_t forward, right;
	vec3_t offset = FindShamedicOffset(self);

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, offset, forward, right);

	// Ajustamos el desplazamiento de la mano con los valores dados
	vec3_t right_adjustment = right * y_adjustment;
	vec3_t up_adjustment = vec3_t{ 0.0f, 0.0f, z_adjustment };
	vec3_t total_adjustment = right_adjustment + up_adjustment;
	start += total_adjustment;

	// Ajustamos la posición hacia adelante
	float adelante_ajuste = 64.0f; // Ajusta este valor según sea necesario
	vec3_t forward_adjustment = forward * adelante_ajuste;
	start += forward_adjustment;

	dir = self->pos1 - start;
	dir.normalize();

	// Predicción de la dirección del disparo
	PredictAim(self, self->enemy, start, 0, false, self->spawnflags.has(SPAWNFLAG_SHAMBLER_PRECISE) ? 0.f : 0.1f, &dir, nullptr);

	vec3_t end = start + (dir * 8192);
	trace_t tr = gi.traceline(start, end, source_entity, MASK_PROJECTILE | CONTENTS_SLIME | CONTENTS_LAVA);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_LIGHTNING);
	gi.WriteEntity(source_entity);   // Ahora usamos una entidad separada
	gi.WriteEntity(world);           // entidad destino
	gi.WritePosition(start);
	gi.WritePosition(tr.endpos);
	gi.multicast(start, MULTICAST_PVS, false);

	fire_bullet(self, start, dir, irandom(8, 12), 15, 0, 0, MOD_TESLA);
}

// Función de ataque doble usando entidades separadas para cada rayo
void ShamedicDoubleLightningAttack(edict_t* self)
{
	// Crear entidades dummy para el rayo izquierdo y derecho
	edict_t* left_entity = G_Spawn();
	edict_t* right_entity = G_Spawn();

	// Copiar algunas propiedades del Shamedic
	left_entity->classname = self->classname;
	right_entity->classname = self->classname;

	// Disparo desde la mano izquierda
	ShamedicCastLightningWithOffset(self, 22.5f, 26.0f, left_entity);

	// Disparo desde la mano derecha
	ShamedicCastLightningWithOffset(self, -26.5f, 22.0f, right_entity);

	// Eliminar las entidades dummy después de que terminen los efectos
	G_FreeEdict(left_entity);
	G_FreeEdict(right_entity);
}

void ShamedicCastLeftLightning(edict_t* self)
{
	// Crear entidades dummy para el rayo izquierdo y derecho
	edict_t* left_entity = G_Spawn();

	// Copiar algunas propiedades del Shamedic
	left_entity->classname = self->classname;
	ShamedicCastLightningWithOffset(self, 9.5f, 36.0f, left_entity); // Disparo desde la mano izquierda
}

void shamedic_dead(edict_t *self)
{
	self->mins = { -32, -32, -0 };
	self->maxs = { 32, 32, 24 };
	monster_dead(self);
}

static void shamedic_shrink(edict_t *self)
{
	//self->maxs[2] = -2;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

mframe_t shamedic_frames_death[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, -18.f, monster_footstep },
	{ ai_move, -10.f, shamedic_shrink },
	{ ai_move, -6.f },
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
	{ ai_move }
};
MMOVE_T(shamedic_move_death) = { FRAME_death2, FRAME_death30, shamedic_frames_death, shamedic_dead };

DIE(shamedic_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	// if we had a pending patient, he was already freed up in Killed

	// check for gib
	if (M_CheckGib(self, mod))
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		self->s.skinnum /= 2;

		ThrowGibs(self, damage, {
			{ 2, "models/objects/gibs/bone/tris.md2" },
			{ "models/objects/gibs/sm_meat/tris.md2" },
			{ "models/objects/gibs/sm_metal/tris.md2", GIB_METALLIC },
			{ "models/monsters/medic/gibs/chest.md2", GIB_SKINNED },
			{ 2, "models/monsters/shamblerthunder/gibs/g_leg.md2", GIB_SKINNED | GIB_UPRIGHT },
			{ "models/monsters/medic/gibs/hook.md2", GIB_SKINNED | GIB_UPRIGHT },
			{ "models/monsters/shamblerthunder/gibs/g_arm.md2", GIB_SKINNED | GIB_UPRIGHT },
			{ "models/monsters/shamblerthunder/gibs/g_head.md2", GIB_SKINNED | GIB_HEAD }
		});

		self->deadflag = true;
		return;
	}

	if (self->deadflag)
		return;

	// regular death
	//	PMM
	if (self->mass == 400)
		gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, commander_sound_die, 1, ATTN_NORM, 0);
	//
	self->deadflag = true;
	self->takedamage = true;

	M_SetAnimation(self, &shamedic_move_death);
}

mframe_t shamedic_frames_duck[] = {
	{ ai_move, -1 },
	{ ai_move, -1, monster_duck_down },
	{ ai_move, -1, monster_duck_hold },
	{ ai_move, -1 },
	{ ai_move, -1 },
	{ ai_move, -1 }, // PMM - duck up used to be here
	{ ai_move, -1 },
	{ ai_move, -1 },
	{ ai_move, -1 },
	{ ai_move, -1 },
	{ ai_move, -1 },
	{ ai_move, -1 },
	{ ai_move, -1, monster_duck_up }
};
MMOVE_T(shamedic_move_duck) = { FRAME_duck2, FRAME_duck14, shamedic_frames_duck, shamedic_run };

// PMM -- moved dodge code to after attack code so I can reference attack frames

mframe_t shamedic_frames_attackHyperLighting[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, ShamedicDoubleLightningAttack },
	{ ai_charge, 0, ShamedicDoubleLightningAttack },
	{ ai_charge, 0, ShamedicDoubleLightningAttack },
	{ ai_charge, 0, ShamedicDoubleLightningAttack },
	{ ai_charge, 0, ShamedicDoubleLightningAttack },
	{ ai_charge, 0, ShamedicDoubleLightningAttack },
	{ ai_charge, 0, ShamedicDoubleLightningAttack },
	{ ai_charge, 0, ShamedicDoubleLightningAttack },
	{ ai_charge, 0, ShamedicDoubleLightningAttack },
	{ ai_charge, 0, ShamedicDoubleLightningAttack },
	{ ai_charge, 0, ShamedicDoubleLightningAttack },
	{ ai_charge, 0, ShamedicDoubleLightningAttack },
	{ ai_charge },
	{ ai_charge },
	// [Paril-KEX] end on 36 as intended
	{ ai_charge, 2.f }, // 33
	{ ai_charge, 3.f, monster_footstep },
};
MMOVE_T(shamedic_move_attackHyperLighting) = { FRAME_attack15, FRAME_attack34, shamedic_frames_attackHyperLighting, shamedic_run };

static void shamedic_quick_attack(edict_t *self)
{
	if (frandom() < 0.5f)
	{
		M_SetAnimation(self, &shamedic_move_attackHyperLighting, false);
		self->monsterinfo.nextframe = FRAME_attack16;
	}
}

void shamedic_continue(edict_t *self)
{
	if (visible(self, self->enemy))
		if (frandom() <= 0.95f)
			M_SetAnimation(self, &shamedic_move_attackHyperLighting, false);
}

mframe_t shamedic_frames_attackLighting[] = {
	{ ai_charge, 5 },
	{ ai_charge, 3 },
	{ ai_charge, 2 },
	{ ai_charge, 0, shamedic_quick_attack },
	{ ai_charge, 0, monster_footstep },
	{ ai_charge },
	{ ai_charge, 0, ShamedicCastLeftLightning },
	{ ai_charge, 0, ShamedicCastLeftLightning },
	{ ai_charge, 0, ShamedicCastLeftLightning },
	{ ai_charge, 0, ShamedicCastLeftLightning },
	{ ai_charge },
	{ ai_charge, 0, shamedic_continue } // Change to shamedic_continue... Else, go to frame 32
};
MMOVE_T(shamedic_move_attackLighting) = { FRAME_attack3, FRAME_attack14, shamedic_frames_attackLighting, shamedic_run };

void shamedic_hook_launch(edict_t *self)
{
	// PMM - commander sounds
	if (self->mass == 400)
		gi.sound(self, CHAN_WEAPON, sound_hook_launch, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_WEAPON, commander_sound_hook_launch, 1, ATTN_NORM, 0);
}

constexpr vec3_t shamedic_cable_offsets[] = {
	{ 45.0f, -9.2f, 15.5f },
	{ 48.4f, -9.7f, 15.2f },
	{ 47.8f, -9.8f, 15.8f },
	{ 47.3f, -9.3f, 14.3f },
	{ 45.4f, -10.1f, 13.1f },
	{ 41.9f, -12.7f, 12.0f },
	{ 37.8f, -15.8f, 11.2f },
	{ 34.3f, -18.4f, 10.7f },
	{ 32.7f, -19.7f, 10.4f },
	{ 32.7f, -19.7f, 10.4f }
};

void shamedic_cable_attack(edict_t *self)
{
	vec3_t	offset, start, end, f, r;
	trace_t tr;
	vec3_t	dir;
	float	distance;

	if ((!self->enemy) || (!self->enemy->inuse) || (self->enemy->s.effects & EF_GIB))
	{
		abortHeal(self, false, false, false);
		return;
	}

	// we switched back to a player; let the animation finish
	if (self->enemy->client)
		return;

	// see if our enemy has changed to a client, or our target has more than 0 health,
	// abort it .. we got switched to someone else due to damage
	if (self->enemy->health > 0)
	{
		abortHeal(self, false, false, false);
		return;
	}

	AngleVectors(self->s.angles, f, r, nullptr);
	offset = shamedic_cable_offsets[self->s.frame - FRAME_attack42];
	start = M_ProjectFlashSource(self, offset, f, r);

	// check for max distance
	// not needed, done in checkattack
	// check for min distance
	dir = start - self->enemy->s.origin;
	distance = dir.length();
	if (distance < MEDIC_MIN_DISTANCE)
	{
		abortHeal(self, true, true, false);
		return;
	}

	tr = gi.traceline(start, self->enemy->s.origin, self, MASK_SOLID);
	if (tr.fraction != 1.0f && tr.ent != self->enemy)
	{
		if (tr.ent == world)
		{
			// give up on second try
			if (self->monsterinfo.medicTries > 1)
			{
				abortHeal(self, true, false, true);
				return;
			}
			self->monsterinfo.medicTries++;
			cleanupHeal(self, 1);
			return;
		}
		abortHeal(self, true, false, false);
		return;
	}

	if (self->s.frame == FRAME_attack43)
	{
		// PMM - commander sounds
		if (self->mass == 400)
			gi.sound(self->enemy, CHAN_AUTO, sound_hook_hit, 1, ATTN_NORM, 0);
		else
			gi.sound(self->enemy, CHAN_AUTO, commander_sound_hook_hit, 1, ATTN_NORM, 0);

		self->enemy->monsterinfo.aiflags |= AI_RESURRECTING;
		self->enemy->takedamage = false;
		M_SetEffects(self->enemy);
	}
	else if (self->s.frame == FRAME_attack50)
	{
		vec3_t maxs;
		self->enemy->spawnflags = SPAWNFLAG_NONE;
		self->enemy->monsterinfo.aiflags &= AI_STINKY | AI_SPAWNED_MASK;
		self->enemy->target = nullptr;
		self->enemy->targetname = nullptr;
		self->enemy->combattarget = nullptr;
		self->enemy->deathtarget = nullptr;
		self->enemy->healthtarget = nullptr;
		self->enemy->itemtarget = nullptr;
		self->enemy->monsterinfo.healer = self;

		maxs = self->enemy->maxs;
		maxs[2] += 48; // compensate for change when they die

		tr = gi.trace(self->enemy->s.origin, self->enemy->mins, maxs, self->enemy->s.origin, self->enemy, MASK_MONSTERSOLID);

		if (tr.startsolid || tr.allsolid)
		{
			abortHeal(self, true, true, false);
			return;
		}
		else if (tr.ent != world)
		{
			abortHeal(self, true, true, false);
			return;
		}
		else
		{
			self->enemy->monsterinfo.aiflags |= AI_DO_NOT_COUNT;

			// backup & restore health stuff, because of multipliers
			int32_t old_max_health = self->enemy->max_health;
			item_id_t old_power_armor_type = self->enemy->monsterinfo.initial_power_armor_type;
			int32_t old_power_armor_power = self->enemy->monsterinfo.max_power_armor_power;
			int32_t old_base_health = self->enemy->monsterinfo.base_health;
			int32_t old_health_scaling = self->enemy->monsterinfo.health_scaling;
			auto reinforcements = self->enemy->monsterinfo.reinforcements;
			int32_t monster_slots = self->enemy->monsterinfo.monster_slots;
			int32_t monster_used = self->enemy->monsterinfo.monster_used;
			int32_t old_gib_health = self->enemy->gib_health;

			st = {};
			st.keys_specified.emplace("reinforcements");
			st.reinforcements = "";

			ED_CallSpawn(self->enemy);

			self->enemy->monsterinfo.reinforcements = reinforcements;
			self->enemy->monsterinfo.monster_slots = monster_slots;
			self->enemy->monsterinfo.monster_used = monster_used;

			self->enemy->gib_health = old_gib_health / 2;
			self->enemy->health = self->enemy->max_health = old_max_health;
			self->enemy->monsterinfo.power_armor_power = self->enemy->monsterinfo.max_power_armor_power = old_power_armor_power;
			self->enemy->monsterinfo.power_armor_type = self->enemy->monsterinfo.initial_power_armor_type = old_power_armor_type;
			self->enemy->monsterinfo.base_health = old_base_health;
			self->enemy->monsterinfo.health_scaling = old_health_scaling;

			if (self->enemy->monsterinfo.setskin)
				self->enemy->monsterinfo.setskin(self->enemy);

			if (self->enemy->think)
			{
				self->enemy->nextthink = level.time;
				self->enemy->think(self->enemy);
			}
			self->enemy->monsterinfo.aiflags &= ~AI_RESURRECTING;
			self->enemy->monsterinfo.aiflags |= AI_IGNORE_SHOTS | AI_DO_NOT_COUNT;
			// turn off flies
			self->enemy->s.effects &= ~EF_FLIES;
			self->enemy->monsterinfo.healer = nullptr;

			if ((self->oldenemy) && (self->oldenemy->inuse) && (self->oldenemy->health > 0))
			{
				self->enemy->enemy = self->oldenemy;
				FoundTarget(self->enemy);
			}
			else
			{
				self->enemy->enemy = nullptr;
				if (!FindTarget(self->enemy))
				{
					// no valid enemy, so stop acting
					self->enemy->monsterinfo.pausetime = HOLD_FOREVER;
					self->enemy->monsterinfo.stand(self->enemy);
				}
				self->enemy = nullptr;
				self->oldenemy = nullptr;
				if (!FindTarget(self))
				{
					// no valid enemy, so stop acting
					self->monsterinfo.pausetime = HOLD_FOREVER;
					self->monsterinfo.stand(self);
					return;
				}
			}

			cleanupHeal(self, false);
			return;
		}
	}
	else
	{
		if (self->s.frame == FRAME_attack44)
		{
			// PMM - medic commander sounds
			if (self->mass == 400)
				gi.sound(self, CHAN_WEAPON, sound_hook_heal, 1, ATTN_NORM, 0);
			else
				gi.sound(self, CHAN_WEAPON, commander_sound_hook_heal, 1, ATTN_NORM, 0);
		}
	}

	// adjust start for beam origin being in middle of a segment
	start += (f * 8);

	// adjust end z for end spot since the monster is currently dead
	end = self->enemy->s.origin;
	end[2] = (self->enemy->absmin[2] + self->enemy->absmax[2]) / 2;

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_MEDIC_CABLE_ATTACK);
	gi.WriteEntity(self);
	gi.WritePosition(start);
	gi.WritePosition(end);
	gi.multicast(self->s.origin, MULTICAST_PVS, false);
}

void shamedic_hook_retract(edict_t *self)
{
	if (self->mass == 400)
		gi.sound(self, CHAN_WEAPON, sound_hook_retract, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_WEAPON, sound_hook_retract, 1, ATTN_NORM, 0);

	self->monsterinfo.aiflags &= ~AI_MEDIC;

	if (self->oldenemy && self->oldenemy->inuse && self->oldenemy->health > 0)
	{
		self->enemy = self->oldenemy;
		HuntTarget(self, false);
	}
	else
	{
		self->enemy = self->goalentity = nullptr;
		self->oldenemy = nullptr;
		if (!FindTarget(self))
		{
			// no valid enemy, so stop acting
			self->monsterinfo.pausetime = HOLD_FOREVER;
			self->monsterinfo.stand(self);
			return;
		}
	}
}

mframe_t shamedic_frames_attackCable[] = {
	// ROGUE - negated 36-40 so he scoots back from his target a little
	// ROGUE - switched 33-36 to ai_charge
	// ROGUE - changed frame 52 to 60 to compensate for changes in 36-40
	// [Paril-KEX] started on 36 as they intended
	{ ai_charge, -4.7f }, // 37
	{ ai_charge, -5.f },
	{ ai_charge, -6.f },
	{ ai_charge, -4.f }, // 40
	{ ai_charge, 0, monster_footstep },
	{ ai_move, 0, shamedic_hook_launch },	// 42
	{ ai_move, 0, shamedic_cable_attack }, // 43
	{ ai_move, 0, shamedic_cable_attack },
	{ ai_move, 0, shamedic_cable_attack },
	{ ai_move, 0, shamedic_cable_attack },
	{ ai_move, 0, shamedic_cable_attack },
	{ ai_move, 0, shamedic_cable_attack },
	{ ai_move, 0, shamedic_cable_attack },
	{ ai_move, 0, shamedic_cable_attack },
	{ ai_move, 0, shamedic_cable_attack }, // 51
	{ ai_move, 0, shamedic_hook_retract }, // 52
	{ ai_move, -1.5f },
	{ ai_move, -1.2f, monster_footstep },
	{ ai_move, -3.f }
};
MMOVE_T(shamedic_move_attackCable) = { FRAME_attack37, FRAME_attack55, shamedic_frames_attackCable, shamedic_run };

void shamedic_start_spawn(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, commander_sound_spawn, 1, ATTN_NORM, 0);
	self->monsterinfo.nextframe = FRAME_attack48;
}

void shamedic_determine_spawn(edict_t *self)
{
	vec3_t f, r, offset, startpoint, spawnpoint;
	int	   count;
	int	   num_success = 0;

	AngleVectors(self->s.angles, f, r, nullptr);

	int num_summoned;
	self->monsterinfo.chosen_reinforcements = M_PickReinforcements(self, num_summoned);

	for (count = 0; count < num_summoned; count++)
	{
		offset = reinforcement_position[count];

		if (self->s.scale)
			offset *= self->s.scale;

		startpoint = M_ProjectFlashSource(self, offset, f, r);
		// a little off the ground
		startpoint[2] += 10 * (self->s.scale ? self->s.scale : 1.0f);

		auto &reinforcement = self->monsterinfo.reinforcements.reinforcements[self->monsterinfo.chosen_reinforcements[count]];

		if (FindSpawnPoint(startpoint, reinforcement.mins, reinforcement.maxs, spawnpoint, 32))
		{
			if (CheckGroundSpawnPoint(spawnpoint, reinforcement.mins, reinforcement.maxs, 256, -1))
			{
				num_success++;
				// we found a spot, we're done here
				count = num_summoned;
			}
		}
	}

	// see if we have any success by spinning around
	if (num_success == 0)
	{
		for (count = 0; count < num_summoned; count++)
		{
			offset = reinforcement_position[count];

			if (self->s.scale)
				offset *= self->s.scale;

			// check behind
			offset[0] *= -1.0f;
			offset[1] *= -1.0f;
			startpoint = M_ProjectFlashSource(self, offset, f, r);
			// a little off the ground
			startpoint[2] += 10;

			auto &reinforcement = self->monsterinfo.reinforcements.reinforcements[self->monsterinfo.chosen_reinforcements[count]];

			if (FindSpawnPoint(startpoint, reinforcement.mins, reinforcement.maxs, spawnpoint, 32))
			{
				if (CheckGroundSpawnPoint(spawnpoint, reinforcement.mins, reinforcement.maxs, 256, -1))
				{
					num_success++;
					// we found a spot, we're done here
					count = num_summoned;
				}
			}
		}

		if (num_success)
		{
			self->monsterinfo.aiflags |= AI_MANUAL_STEERING;
			self->ideal_yaw = anglemod(self->s.angles[YAW]) + 180;
			if (self->ideal_yaw > 360.0f)
				self->ideal_yaw -= 360.0f;
		}
	}

	if (num_success == 0)
		self->monsterinfo.nextframe = FRAME_attack53;
}

void shamedic_spawngrows(edict_t *self)
{
	vec3_t f, r, offset, startpoint, spawnpoint;
	int	   count;
	int	   num_summoned; // should be 1, 3, or 5
	int	   num_success = 0;
	float  current_yaw;

	// if we've been directed to turn around
	if (self->monsterinfo.aiflags & AI_MANUAL_STEERING)
	{
		current_yaw = anglemod(self->s.angles[YAW]);
		if (fabsf(current_yaw - self->ideal_yaw) > 0.1f)
		{
			self->monsterinfo.aiflags |= AI_HOLD_FRAME;
			return;
		}

		// done turning around
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
		self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
	}

	AngleVectors(self->s.angles, f, r, nullptr);

	num_summoned = 0;

	for (int32_t i = 0; i < MAX_REINFORCEMENTS; i++, num_summoned++)
		if (self->monsterinfo.chosen_reinforcements[i] == 255)
			break;

	for (count = 0; count < num_summoned; count++)
	{
		offset = reinforcement_position[count];

		startpoint = M_ProjectFlashSource(self, offset, f, r);

		// a little off the ground
		startpoint[2] += 10 * (self->s.scale ? self->s.scale : 1.0f);

		auto &reinforcement = self->monsterinfo.reinforcements.reinforcements[self->monsterinfo.chosen_reinforcements[count]];

		if (FindSpawnPoint(startpoint, reinforcement.mins, reinforcement.maxs, spawnpoint, 32))
		{
			if (CheckGroundSpawnPoint(spawnpoint, reinforcement.mins, reinforcement.maxs, 256, -1))
			{
				num_success++;
				float radius = (reinforcement.maxs - reinforcement.mins).length() * 0.5f;
				SpawnGrow_Spawn(spawnpoint + (reinforcement.mins + reinforcement.maxs), radius, radius * 2.f);
			}
		}
	}

	if (num_success == 0)
		self->monsterinfo.nextframe = FRAME_attack53;
}

void shamedic_finish_spawn(edict_t *self)
{
	edict_t *ent;
	vec3_t	 f, r, offset, startpoint, spawnpoint;
	int		 count;
	int		 num_summoned; // should be 1, 3, or 5
	edict_t *designated_enemy;

	AngleVectors(self->s.angles, f, r, nullptr);

	num_summoned = 0;

	for (int32_t i = 0; i < MAX_REINFORCEMENTS; i++, num_summoned++)
		if (self->monsterinfo.chosen_reinforcements[i] == 255)
			break;

	for (count = 0; count < num_summoned; count++)
	{
		auto &reinforcement = self->monsterinfo.reinforcements.reinforcements[self->monsterinfo.chosen_reinforcements[count]];
		offset = reinforcement_position[count];

		startpoint = M_ProjectFlashSource(self, offset, f, r);

		// a little off the ground
		startpoint[2] += 10 * (self->s.scale ? self->s.scale : 1.0f);

		ent = nullptr;
		if (FindSpawnPoint(startpoint, reinforcement.mins, reinforcement.maxs, spawnpoint, 32))
		{
			if (CheckSpawnPoint(spawnpoint, reinforcement.mins, reinforcement.maxs))
				ent = CreateGroundMonster(spawnpoint, self->s.angles, reinforcement.mins, reinforcement.maxs, reinforcement.classname, 256);
		}

		if (!ent)
			continue;

		if (ent->think)
		{
			ent->nextthink = level.time;
			ent->think(ent);
		}

		ent->monsterinfo.aiflags |= AI_IGNORE_SHOTS | AI_DO_NOT_COUNT | AI_SPAWNED_MEDIC_C;
		ent->monsterinfo.commander = self;
		ent->monsterinfo.monster_slots = reinforcement.strength;
		self->monsterinfo.monster_used += reinforcement.strength;

		if (self->monsterinfo.aiflags & AI_MEDIC)
			designated_enemy = self->oldenemy;
		else
			designated_enemy = self->enemy;

		if (coop->integer)
		{
			designated_enemy = PickCoopTarget(ent);
			if (designated_enemy)
			{
				// try to avoid using my enemy
				if (designated_enemy == self->enemy)
				{
					designated_enemy = PickCoopTarget(ent);
					if (!designated_enemy)
						designated_enemy = self->enemy;
				}
			}
			else
				designated_enemy = self->enemy;
		}

		if ((designated_enemy) && (designated_enemy->inuse) && (designated_enemy->health > 0))
		{
			ent->enemy = designated_enemy;
			FoundTarget(ent);
		}
		else
		{
			ent->enemy = nullptr;
			ent->monsterinfo.stand(ent);
		}
	}
}

mframe_t shamedic_frames_callReinforcements[] = {
	// ROGUE - 33-36 now ai_charge
	{ ai_charge, 2 }, // 33
	{ ai_charge, 3 },
	{ ai_charge, 5 },
	{ ai_charge, 4.4f }, // 36
	{ ai_charge, 4.7f },
	{ ai_charge, 5 },
	{ ai_charge, 6 },
	{ ai_charge, 4 }, // 40
	{ ai_charge, 0, monster_footstep },
	{ ai_move, 0, shamedic_start_spawn }, // 42
	{ ai_move },					   // 43 -- 43 through 47 are skipped
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, shamedic_determine_spawn }, // 48
	{ ai_charge, 0, shamedic_spawngrows },	   // 49
	{ ai_move },						   // 50
	{ ai_move },						   // 51
	{ ai_move, -15, shamedic_finish_spawn },  // 52
	{ ai_move, -1.5f },
	{ ai_move, -1.2f },
	{ ai_move, -3, monster_footstep }
};
MMOVE_T(shamedic_move_callReinforcements) = { FRAME_attack33, FRAME_attack55, shamedic_frames_callReinforcements, shamedic_run };

MONSTERINFO_ATTACK(shamedic_attack) (edict_t *self) -> void
{
	monster_done_dodge(self);

	float enemy_range = range_to(self, self->enemy);

	// signal from checkattack to spawn
	if (self->monsterinfo.aiflags & AI_BLOCKED)
	{
		M_SetAnimation(self, &shamedic_move_callReinforcements);
		self->monsterinfo.aiflags &= ~AI_BLOCKED;
	}

	float r = frandom();
	if (self->monsterinfo.aiflags & AI_MEDIC)
	{
		if ((self->mass > 400) && (r > 0.8f) && M_SlotsLeft(self))
			M_SetAnimation(self, &shamedic_move_callReinforcements);
		else
			M_SetAnimation(self, &shamedic_move_attackCable);
	}
	else
	{
		if (self->monsterinfo.attack_state == AS_BLIND)
		{
			M_SetAnimation(self, &shamedic_move_callReinforcements);
			return;
		}
		if ((self->mass > 400) && (r > 0.2f) && (enemy_range > RANGE_MELEE) && M_SlotsLeft(self))
			M_SetAnimation(self, &shamedic_move_callReinforcements);
		else
			M_SetAnimation(self, &shamedic_move_attackLighting);
	}
}

MONSTERINFO_CHECKATTACK(shamedic_checkattack) (edict_t *self) -> bool
{
	if (self->monsterinfo.aiflags & AI_MEDIC)
	{
		// if our target went away
		if ((!self->enemy) || (!self->enemy->inuse))
		{
			abortHeal(self, true, false, false);
			return false;
		}

		// if we ran out of time, give up
		if (self->timestamp < level.time)
		{
			abortHeal(self, true, false, true);
			self->timestamp = 0_ms;
			return false;
		}

		if (realrange(self, self->enemy) < MEDIC_MAX_HEAL_DISTANCE + 10)
		{
			shamedic_attack(self);
			return true;
		}
		else
		{
			self->monsterinfo.attack_state = AS_STRAIGHT;
			return false;
		}
	}

	if (self->enemy->client && !visible(self, self->enemy) && M_SlotsLeft(self))
	{
		self->monsterinfo.attack_state = AS_BLIND;
		return true;
	}

	// give a LARGE bias to spawning things when we have room
	// use AI_BLOCKED as a signal to attack to spawn
	if (self->monsterinfo.monster_slots && (frandom() < 0.8f) && (M_SlotsLeft(self) > self->monsterinfo.monster_slots * 0.8f) && (realrange(self, self->enemy) > 150))
	{
		self->monsterinfo.aiflags |= AI_BLOCKED;
		self->monsterinfo.attack_state = AS_MISSILE;
		return true;
	}

	// ROGUE
	// since his idle animation looks kinda bad in combat, always attack
	// when he's on a combat point
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		self->monsterinfo.attack_state = AS_MISSILE;
		return true;
	}

	return M_CheckAttack(self);
}

void ShamMedicCommanderCache()
{
	gi.modelindex("models/items/spawngro3/tris.md2");
}

MONSTERINFO_DUCK(shamedic_duck) (edict_t *self, gtime_t eta) -> bool
{
	//	don't dodge if you're healing
	if (self->monsterinfo.aiflags & AI_MEDIC)
		return false;

	if ((self->monsterinfo.active_move == &shamedic_move_attackHyperLighting) ||
		(self->monsterinfo.active_move == &shamedic_move_attackCable) ||
		(self->monsterinfo.active_move == &shamedic_move_attackLighting) ||
		(self->monsterinfo.active_move == &shamedic_move_callReinforcements))
	{
		// he ignores skill
		self->monsterinfo.unduck(self);
		return false;
	}

	M_SetAnimation(self, &shamedic_move_duck);

	return true;
}

MONSTERINFO_SIDESTEP(shamedic_sidestep) (edict_t *self) -> bool
{
	if ((self->monsterinfo.active_move == &shamedic_move_attackHyperLighting) ||
		(self->monsterinfo.active_move == &shamedic_move_attackCable) ||
		(self->monsterinfo.active_move == &shamedic_move_attackLighting) ||
		(self->monsterinfo.active_move == &shamedic_move_callReinforcements))
	{
		// if we're shooting, don't dodge
		return false;
	}

	if (self->monsterinfo.active_move != &shamedic_move_run)
		M_SetAnimation(self, &shamedic_move_run);

	return true;
}

//===========
// PGM
MONSTERINFO_BLOCKED(shamedic_blocked) (edict_t *self, float dist) -> bool
{
	if (blocked_checkplat(self, dist))
		return true;

	return false;
}
// PGM
//===========

/*QUAKED monster_shamedic_commander (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
 */
/*QUAKED monster_shamedic (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
model="models/monsters/shamblerthunder/tris.md2"
*/
void SP_monster_shamedic(edict_t *self)
{
	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict( self );
		return;
	}

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/shamblerthunder/tris.md2");

	gi.modelindex("models/monsters/medic/gibs/chest.md2");
	gi.modelindex("models/monsters/shamblerthunder/gibs/g_arm.md2");
	gi.modelindex("models/monsters/shamblerthunder/gibs/g_head.md2");
	gi.modelindex("models/monsters/medic/gibs/hook.md2");
	gi.modelindex("models/monsters/shamblerthunder/gibs/g_leg.md2");
	
	self->mins = { -32, -32, 0 };
	self->maxs = { 32, 32, 96 };

	// PMM
	if (strcmp(self->classname, "monster_shamedic_commander") == 0)
	{
		self->health = 550 * st.health_multiplier;
		self->gib_health = -130;
		self->mass = 600;
		self->yaw_speed = 40; // default is 20
		ShamMedicCommanderCache();
	}
	else
	{
		// PMM
		self->health = 550 * st.health_multiplier;
		self->gib_health = -130;
		self->mass = 400;
	}

	self->pain = shamedic_pain;
	self->die = shamedic_die;

	self->monsterinfo.stand = shamedic_stand;
	self->monsterinfo.walk = shamedic_walk;
	self->monsterinfo.run = shamedic_run;
	// pmm
	self->monsterinfo.dodge = M_MonsterDodge;
	self->monsterinfo.duck = shamedic_duck;
	self->monsterinfo.unduck = monster_duck_up;
	self->monsterinfo.sidestep = shamedic_sidestep;
	self->monsterinfo.blocked = shamedic_blocked;
	// pmm
	self->monsterinfo.attack = shamedic_attack;
	self->monsterinfo.melee = nullptr;
	self->monsterinfo.sight = shamedic_sight;
	self->monsterinfo.idle = shamedic_idle;
	self->monsterinfo.search = shamedic_search;
	self->monsterinfo.checkattack = shamedic_checkattack;
	self->monsterinfo.setskin = shamedic_setskin;

	gi.linkentity(self);

	M_SetAnimation(self, &shamedic_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);

	// PMM
	self->monsterinfo.aiflags |= AI_IGNORE_SHOTS;

	if (self->mass > 400)
	{
		self->s.skinnum = 2;

		// commander sounds
		commander_sound_idle1.assign("medic_commander/medidle.wav");
		commander_sound_pain1.assign("medic_commander/medpain1.wav");
		commander_sound_pain2.assign("medic_commander/medpain2.wav");
		commander_sound_die.assign("medic_commander/meddeth.wav");
		commander_sound_sight.assign("medic_commander/medsght.wav");
		commander_sound_search.assign("medic_commander/medsrch.wav");
		commander_sound_hook_launch.assign("medic_commander/medatck2c.wav");
		commander_sound_hook_hit.assign("medic_commander/medatck3a.wav");
		commander_sound_hook_heal.assign("medic_commander/medatck4a.wav");
		commander_sound_hook_retract.assign("medic_commander/medatck5a.wav");
		commander_sound_spawn.assign("medic_commander/monsterspawn1.wav");
		gi.soundindex("tank/tnkatck3.wav");

		const char *reinforcements = default_reinforcements;

		if (!st.was_key_specified("monster_slots"))
			self->monsterinfo.monster_slots = default_monster_slots_base;
		if (st.was_key_specified("reinforcements"))
			reinforcements = st.reinforcements;

		if (self->monsterinfo.monster_slots && reinforcements && *reinforcements)
		{
			if (skill->integer)
				self->monsterinfo.monster_slots += floor(self->monsterinfo.monster_slots * (skill->value / 2.f));

			M_SetupReinforcements(reinforcements, self->monsterinfo.reinforcements);
		}
	}
	else
	{
		sound_idle1.assign("medic/idle.wav");
		sound_pain1.assign("medic/medpain1.wav");
		sound_pain2.assign("medic/medpain2.wav");
		sound_die.assign("medic/meddeth1.wav");
		sound_sight.assign("medic/medsght1.wav");
		sound_search.assign("medic/medsrch1.wav");
		sound_hook_launch.assign("medic/medatck2.wav");
		sound_hook_hit.assign("medic/medatck3.wav");
		sound_hook_heal.assign("medic/medatck4.wav");
		sound_hook_retract.assign("medic/medatck5.wav");
		gi.soundindex("medic/medatck1.wav");

		self->s.skinnum = 0;
	}
	// pmm
}
