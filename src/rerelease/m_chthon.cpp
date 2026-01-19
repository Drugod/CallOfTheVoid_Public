#include "g_local.h"
#include "m_chthon.h"

static cached_soundindex sound_out1;
static cached_soundindex sound_sight;
static cached_soundindex sound_throw;
static cached_soundindex sound_pain;
static cached_soundindex sound_death;

extern const mmove_t chthon_move_attack;
extern const mmove_t chthon_move_idle;
extern const mmove_t chthon_move_rise;

bool FindTarget(edict_t* self);

/*
=================
fire_lavaball
=================
*/
TOUCH(lavaball_touch) (edict_t* ent, edict_t* other, const trace_t& tr, bool other_touching_self) -> void
{
    vec3_t origin;

    if (other == ent->owner)
        return;

    if (tr.surface && (tr.surface->flags & SURF_SKY))
    {
        G_FreeEdict(ent);
        return;
    }

    if (ent->owner->client)
        PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

    // calculate position for the explosion entity
    origin = ent->s.origin + tr.plane.normal;

    if (other->takedamage)
    {
        T_Damage(other, ent, ent->owner, ent->velocity, ent->s.origin, tr.plane.normal, ent->dmg, 0, DAMAGE_NONE, MOD_ROCKET);
    }
    else
    {
        // don't throw any debris in net games
        if (!deathmatch->integer && !coop->integer)
        {
            if (tr.surface && !(tr.surface->flags & (SURF_WARP | SURF_TRANS33 | SURF_TRANS66 | SURF_FLOWING)))
            {
                ThrowGibs(ent, 2, {
                    { (size_t)irandom(5), "models/objects/debris2/tris.md2", GIB_METALLIC | GIB_DEBRIS }
                    });
            }
        }
    }

    T_RadiusDamage(ent, ent->owner, (float)ent->radius_dmg, other, ent->dmg_radius, DAMAGE_NONE, MOD_R_SPLASH);

    gi.WriteByte(svc_temp_entity);
    if (ent->waterlevel)
        gi.WriteByte(TE_ROCKET_EXPLOSION_WATER);
    else
        gi.WriteByte(TE_ROCKET_EXPLOSION);
    gi.WritePosition(origin);
    gi.multicast(ent->s.origin, MULTICAST_PHS, false);

    G_FreeEdict(ent);
}

edict_t* fire_lavaball(edict_t* self, const vec3_t& start, const vec3_t& dir, int damage, int speed, float damage_radius, int radius_damage)
{
    edict_t* lavaball;

    lavaball = G_Spawn();
    lavaball->s.origin = start;
    lavaball->s.angles = vectoangles(dir);
    lavaball->velocity = dir * speed;
    lavaball->movetype = MOVETYPE_FLYMISSILE;
    lavaball->svflags |= SVF_PROJECTILE;
    lavaball->flags |= FL_DODGE;
    lavaball->clipmask = MASK_PROJECTILE;
    if (self->client && !G_ShouldPlayersCollide(true))
        lavaball->clipmask &= ~CONTENTS_PLAYER;
    lavaball->solid = SOLID_BBOX;
    lavaball->s.effects |= EF_ROCKET;
    lavaball->s.modelindex = gi.modelindex("models/objects/lavaball/tris.md2");
    lavaball->owner = self;
    lavaball->touch = lavaball_touch;
    lavaball->nextthink = level.time + gtime_t::from_sec(8000.f / speed);
    lavaball->think = G_FreeEdict;
    lavaball->dmg = damage;
    lavaball->radius_dmg = radius_damage;
    lavaball->dmg_radius = damage_radius;
    lavaball->s.sound = gi.soundindex("weapons/rockfly.wav");
    lavaball->classname = "lavaball";

    gi.linkentity(lavaball);

    return lavaball;
}

static inline bool IsAllowedLaserHit(edict_t* inflictor, edict_t* attacker, const mod_t& mod)
{
    const bool byMod = (mod.id == MOD_TARGET_LASER);

    const bool byClass =
        (inflictor && inflictor->classname && !strcmp(inflictor->classname, "event_lighting")) ||
        (attacker && attacker->classname && !strcmp(attacker->classname, "event_lighting"));

    /*char buffer[256];
    snprintf(buffer, sizeof(buffer), "DEBUG: Encontrado edict con inf=\"%s\" y con att=\"%s\"\n", inflictor->classname, attacker->classname);
    gi.Com_PrintFmt("{}", buffer);  // OJO: el primer parámetro debe ser literal*/

    return (byMod || byClass);
}

static void chthon_start_rise(edict_t* self)
{
    self->enemy = NULL; // para evitar reenfoques accidentales
    self->monsterinfo.aiflags |= AI_LOST_SIGHT; // o el flag equivalente en tu fork
    self->flags |= FL_IMMUNE_LAVA;              // sigue inmune durante el ascenso si viene de lava
    self->takedamage = false;                   // opcional: no dañable hasta emerger
    M_SetAnimation(self, &chthon_move_rise);
}

USE(chthon_use_trigger) (edict_t* self, edict_t* other, edict_t* activator) -> void
{
    if (!self->inuse) return;
    self->use = nullptr;

    self->svflags &= ~SVF_NOCLIENT;
    self->solid = SOLID_BBOX;
    gi.linkentity(self);

    chthon_start_rise(self);
}

static void chthon_face(edict_t* self)
{
    if (!self->enemy || !self->enemy->inuse) {
            return;
    }
    vec3_t to_enemy = self->enemy->s.origin - self->s.origin;
    float yaw = vectoyaw(to_enemy);
    self->ideal_yaw = yaw;
    M_ChangeYaw(self);
}

static void chthon_fire_missile_offset(edict_t* self, const vec3_t& offset)
{
    if (!self->enemy || !self->enemy->inuse)
        return;

    vec3_t forward, right, up;

    AngleVectors(self->s.angles, forward, right, up);

    vec3_t start = self->s.origin + forward * offset.x + right * offset.y + up * offset.z;
    vec3_t target = self->enemy->s.origin;

    target.z += self->enemy->viewheight;

    vec3_t dir = target - start;
    dir.normalize();

    fire_lavaball(self, start, dir, 60, 600, 120.0f, 60);
    gi.sound(self, CHAN_WEAPON, sound_throw, 0.5, ATTN_NONE, 0);
}

static void chthon_attack_fire1(edict_t* self)
{
    chthon_fire_missile_offset(self, { 100.0f, 100.0f, 200.0f });
}

static void chthon_attack_fire2(edict_t* self)
{
    chthon_fire_missile_offset(self, { 100.0f, -100.0f, 200.0f });
}

static void chthon_attack_loop(edict_t* self)
{
    M_SetAnimation(self, &chthon_move_attack);
}

mframe_t chthon_frames_attack[] = {
    { ai_charge, 0, chthon_face }, // FRAME_attack1
    { ai_charge, 0, chthon_face }, // FRAME_attack2
    { ai_charge, 0, chthon_face }, // FRAME_attack3
    { ai_charge, 0, chthon_face }, // FRAME_attack4
    { ai_charge, 0, chthon_face }, // FRAME_attack5
    { ai_charge, 0, chthon_face }, // FRAME_attack6
    { ai_charge, 0, chthon_face }, // FRAME_attack7
    { ai_charge, 0, chthon_face }, // FRAME_attack8
    { ai_charge, 0, chthon_attack_fire1 }, // FRAME_attack9 – first missile
    { ai_charge, 0, chthon_face }, // FRAME_attack10
    { ai_charge, 0, chthon_face }, // FRAME_attack11
    { ai_charge, 0, chthon_face }, // FRAME_attack12
    { ai_charge, 0, chthon_face }, // FRAME_attack13
    { ai_charge, 0, chthon_face }, // FRAME_attack14
    { ai_charge, 0, chthon_face }, // FRAME_attack15
    { ai_charge, 0, chthon_face }, // FRAME_attack16
    { ai_charge, 0, chthon_face }, // FRAME_attack17
    { ai_charge, 0, chthon_face }, // FRAME_attack18
    { ai_charge, 0, chthon_face }, // FRAME_attack19
    { ai_charge, 0, chthon_attack_fire2 }, // 20 – second missile
    { ai_charge, 0, chthon_face }, // FRAME_attack21
    { ai_charge, 0, chthon_face }, // FRAME_attack22
    { ai_charge, 0, chthon_face }  // FRAME_attack23
};
MMOVE_T(chthon_move_attack) = { 58, 80, chthon_frames_attack, chthon_attack_loop };

MONSTERINFO_ATTACK(chthon_attack) (edict_t* self) -> void
{
    M_SetAnimation(self, &chthon_move_attack);
}

// Shock A
mframe_t chthon_frames_painA[] = {
    { ai_move, 0, chthon_face }, // FRAME_shocka1
    { ai_move, 0, chthon_face }, // FRAME_shocka2
    { ai_move, 0, chthon_face }, // FRAME_shocka3
    { ai_move, 0, chthon_face }, // FRAME_shocka4
    { ai_move, 0, chthon_face }, // FRAME_shocka5
    { ai_move, 0, chthon_face }, // FRAME_shocka6
    { ai_move, 0, chthon_face }, // FRAME_shocka7
    { ai_move, 0, chthon_face }, // FRAME_shocka8
    { ai_move, 0, chthon_face }, // FRAME_shocka9
    { ai_move, 0, chthon_face }  // FRAME_shocka10
};
MMOVE_T(chthon_move_painA) = { FRAME_shocka1, FRAME_shocka10, chthon_frames_painA, chthon_attack };

// Shock B
mframe_t chthon_frames_painB[] = {
    { ai_move, 0, chthon_face }, // FRAME_shockb1
    { ai_move, 0, chthon_face }, // FRAME_shockb2
    { ai_move, 0, chthon_face }, // FRAME_shockb3
    { ai_move, 0, chthon_face }, // FRAME_shockb4
    { ai_move, 0, chthon_face }, // FRAME_shockb5
    { ai_move, 0, chthon_face }, // FRAME_shockb6
    { ai_move, 0, chthon_face }, // FRAME_shockb7
    { ai_move, 0, chthon_face }, // FRAME_shockb8
    { ai_move, 0, chthon_face }, // FRAME_shockb9
    { ai_move, 0, chthon_face }  // FRAME_shockb10
};
MMOVE_T(chthon_move_painB) = { FRAME_shockb1, FRAME_shockb10, chthon_frames_painB, chthon_attack };

// Shock C
mframe_t chthon_frames_painC[] = {
    { ai_move, 0, chthon_face }, // FRAME_shockc1
    { ai_move, 0, chthon_face }, // FRAME_shockc2
    { ai_move, 0, chthon_face }, // FRAME_shockc3
    { ai_move, 0, chthon_face }, // FRAME_shockc4
    { ai_move, 0, chthon_face }, // FRAME_shockc5
    { ai_move, 0, chthon_face }, // FRAME_shockc6
    { ai_move, 0, chthon_face }, // FRAME_shockc7
    { ai_move, 0, chthon_face }, // FRAME_shockc8
    { ai_move, 0, chthon_face }, // FRAME_shockc9
    { ai_move, 0, chthon_face }  // FRAME_shockc10
};
MMOVE_T(chthon_move_painC) = { FRAME_shockc1, FRAME_shockc10, chthon_frames_painC, chthon_attack };

mframe_t chthon_frames_idle[] = {
    { ai_run, 0, chthon_face }, // FRAME_walk1
    { ai_run, 0, chthon_face }, // FRAME_walk2
    { ai_run, 0, chthon_face }, // FRAME_walk3
    { ai_run, 0, chthon_face }, // FRAME_walk4
    { ai_run, 0, chthon_face }, // FRAME_walk5
    { ai_run, 0, chthon_face }, // FRAME_walk6
    { ai_run, 0, chthon_face }, // FRAME_walk7
    { ai_run, 0, chthon_face }, // FRAME_walk8
    { ai_run, 0, chthon_face }, // FRAME_walk9
    { ai_run, 0, chthon_face }, // FRAME_walk10
    { ai_run, 0, chthon_face }, // FRAME_walk11
    { ai_run, 0, chthon_face }, // FRAME_walk12
    { ai_run, 0, chthon_face }, // FRAME_walk13
    { ai_run, 0, chthon_face }, // FRAME_walk14
    { ai_run, 0, chthon_face }, // FRAME_walk15
    { ai_run, 0, chthon_face }, // FRAME_walk16
    { ai_run, 0, chthon_face }, // FRAME_walk17
    { ai_run, 0, chthon_face }, // FRAME_walk18
    { ai_run, 0, chthon_face }, // FRAME_walk19
    { ai_run, 0, chthon_face }, // FRAME_walk20
    { ai_run, 0, chthon_face }, // FRAME_walk21
    { ai_run, 0, chthon_face }, // FRAME_walk22
    { ai_run, 0, chthon_face }, // FRAME_walk23
    { ai_run, 0, chthon_face }, // FRAME_walk24
    { ai_run, 0, chthon_face }, // FRAME_walk25
    { ai_run, 0, chthon_face }, // FRAME_walk26
    { ai_run, 0, chthon_face }, // FRAME_walk27
    { ai_run, 0, chthon_face }, // FRAME_walk28
    { ai_run, 0, chthon_face }, // FRAME_walk29
    { ai_run, 0, chthon_face }, // FRAME_walk30
    { ai_run, 0, chthon_face }  // FRAME_walk31
};
MMOVE_T(chthon_move_idle) = { FRAME_walk1, FRAME_walk31, chthon_frames_idle, NULL };
MONSTERINFO_IDLE(chthon_idle) (edict_t* self) -> void
{
    M_SetAnimation(self, &chthon_move_idle);
}

static void chthon_rise2_think(edict_t* self)
{ 
    gi.sound(self, CHAN_VOICE, sound_out1, 0.5, ATTN_NONE, 0);
}

static void ai_anim(edict_t* self, float dist) {
    // No mover, no girar, no buscar enemigo. Solo avanza el frame.
    (void)self; (void)dist;
}

mframe_t chthon_frames_rise[] = {
    { ai_anim, 0 }, // FRAME_rise1
    { ai_anim, 0 }, // FRAME_rise2
    { ai_anim, 0, chthon_rise2_think }, // FRAME_rise3
    { ai_anim, 0 }, // FRAME_rise4
    { ai_anim, 0 }, // FRAME_rise5
    { ai_anim, 0 }, // FRAME_rise6
    { ai_anim, 0 }, // FRAME_rise7
    { ai_anim, 0 }, // FRAME_rise8
    { ai_anim, 0 }, // FRAME_rise9
    { ai_anim, 0 }, // FRAME_rise10
    { ai_anim, 0 }, // FRAME_rise11
    { ai_anim, 0 }, // FRAME_rise12
    { ai_anim, 0 }, // FRAME_rise13
    { ai_anim, 0 }, // FRAME_rise14
    { ai_anim, 0 }, // FRAME_rise15
    { ai_anim, 0 }, // FRAME_rise16
    { ai_anim, 0 }  // FRAME_rise17
};
MMOVE_T(chthon_move_rise) = { FRAME_rise1, FRAME_rise17, chthon_frames_rise, chthon_attack_loop };

static void chthon_death1_think(edict_t *self) 
{ 
    gi.sound(self, CHAN_VOICE, sound_death, 0.5, ATTN_NONE, 0);
}

static void chthon_death_end(edict_t *self)
{
    //level.killed_monsters++;
    G_UseTargets(self, self->enemy);
    G_FreeEdict(self);
}

mframe_t chthon_frames_death[] = {
    { ai_move, 0, chthon_death1_think }, // FRAME_death1
    { ai_move }, // FRAME_death2
    { ai_move }, // FRAME_death3
    { ai_move }, // FRAME_death4
    { ai_move }, // FRAME_death5
    { ai_move }, // FRAME_death6
    { ai_move }, // FRAME_death7
    { ai_move }, // FRAME_death8
    { ai_move }, // FRAME_death9
    { ai_move, 0, chthon_death_end }  // FRAME_death10
};
MMOVE_T(chthon_move_death) = { 48, 57, chthon_frames_death, nullptr };

MONSTERINFO_STAND(chthon_stand) (edict_t* self) -> void
{
    M_SetAnimation(self, &chthon_move_rise);
}

MONSTERINFO_WALK(chthon_walk) (edict_t* self) -> void
{
    M_SetAnimation(self, &chthon_move_rise);
}

MONSTERINFO_RUN(chthon_run) (edict_t* self) -> void //entra aqui cuando ve al jugador
{
    M_SetAnimation(self, &chthon_move_rise);
}

MONSTERINFO_SIGHT(chthon_sight) (edict_t* self, edict_t* other) -> void
{
    gi.sound(self, CHAN_VOICE, sound_sight, 0.5, ATTN_NONE, 0);
}

MONSTERINFO_SEARCH(chthon_search) (edict_t* self) -> void {}

DIE(chthon_die) (edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t& mod) -> void
{
    if (self->deadflag)
      return;

    gi.sound(self, CHAN_VOICE, sound_death, 0.5, ATTN_NONE, 0);
    self->deadflag = true;
    self->count = 0;
    M_SetAnimation(self, &chthon_move_death);
}

PAIN(chthon_pain) (edict_t* self, edict_t* other, float kick, int damage, const mod_t& mod) -> void
{
        if (level.time < self->pain_debounce_time)
            return;

        self->pain_debounce_time = level.time + 1.0_sec;

        gi.sound(self, CHAN_VOICE, sound_pain, 0.5, ATTN_NONE, 0);

        if (self->health > 0 && self->health <= 16) {
            M_SetAnimation(self, &chthon_move_painB);
        }
        else if (self->health <= 0) {
            M_SetAnimation(self, &chthon_move_painC);
        }
        else {
            M_SetAnimation(self, &chthon_move_painA);
        }
}

void SP_monster_chthon(edict_t *self)
{
    if (!M_AllowSpawn(self))
    {
        G_FreeEdict(self);
        return;
    }
    
    sound_out1.assign("chthon/out1.wav");
    sound_sight.assign("chthon/sight1.wav");
    sound_throw.assign("chthon/throw.wav");
    sound_pain.assign("chthon/pain.wav");
    sound_death.assign("chthon/death.wav");

    self->flags |= FL_IMMUNE_LAVA;
    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;
    self->s.modelindex = gi.modelindex("models/monsters/chthon/tris.md2");

    self->mins = { -128, -128, -24 };
    self->maxs = { 128, 128, 256 };

    if (skill->integer == 0)
        self->health = 16;
    else
        self->health = 50;

    self->gib_health = -1;
    self->mass = 1000;

    self->pain = chthon_pain;
    self->die = chthon_die;
    self->monsterinfo.stand = chthon_stand;
    self->monsterinfo.walk = chthon_walk;
    self->monsterinfo.run    = chthon_run;
    self->monsterinfo.attack = chthon_attack;
    self->monsterinfo.sight  = chthon_sight;
    self->monsterinfo.search = chthon_search;
    gi.linkentity(self);

    self->monsterinfo.scale = MODEL_SCALE;

    walkmonster_start(self);

    self->takedamage = false;
}
