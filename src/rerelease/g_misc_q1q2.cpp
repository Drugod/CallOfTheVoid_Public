// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_misc.c

#include "g_local.h"

/*QUAKED misc_chtondead (1 .5 0) (-176 -120 -24) (176 120 72)
This is a large stationary viper as seen in Paul's intro
*/
void SP_misc_chtondead(edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->mins = { -176, -120, -24 };
	ent->maxs = { 176, 120, 72 };
	ent->s.modelindex = gi.modelindex("models/props/chtondead/tris.md2");
	gi.linkentity(ent);
}
