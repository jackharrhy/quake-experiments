/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * Item spawning.
 *
 * =======================================================================
 */

#include "header/local.h"

typedef struct
{
	char *name;
	void (*spawn)(edict_t *ent);
} spawn_t;

void SP_info_player_start(edict_t *ent);

void SP_worldspawn(edict_t *ent);

void SP_light(edict_t *self);

spawn_t spawns[] = {
	{"info_player_start", SP_info_player_start},
	{"worldspawn", SP_worldspawn},
	{"light", SP_light},

	{NULL, NULL}};

/*
 * Finds the spawn function for the entity and calls it
 */
void ED_CallSpawn(edict_t *ent)
{
	spawn_t *s;

	if (!ent->classname)
	{
		gi.dprintf("ED_CallSpawn: NULL classname\n");
		return;
	}

	/* check normal spawn functions */
	for (s = spawns; s->name; s++)
	{
		if (!strcmp(s->name, ent->classname))
		{
			/* found it */
			s->spawn(ent);
			return;
		}
	}

	gi.dprintf("%s doesn't have a spawn function\n", ent->classname);
}

char *
ED_NewString(char *string)
{
	char *newb, *new_p;
	int i, l;

	l = strlen(string) + 1;

	newb = gi.TagMalloc(l, TAG_LEVEL);

	new_p = newb;

	for (i = 0; i < l; i++)
	{
		if ((string[i] == '\\') && (i < l - 1))
		{
			i++;

			if (string[i] == 'n')
			{
				*new_p++ = '\n';
			}
			else
			{
				*new_p++ = '\\';
			}
		}
		else
		{
			*new_p++ = string[i];
		}
	}

	return newb;
}

/*
 * Takes a key/value pair and sets
 * the binary values in an edict
 */
void ED_ParseField(char *key, char *value, edict_t *ent)
{
	field_t *f;
	byte *b;
	float v;
	vec3_t vec;

	for (f = fields; f->name; f++)
	{
		if (!Q_stricmp(f->name, key))
		{
			/* found it */
			if (f->flags & FFL_SPAWNTEMP)
			{
				b = (byte *)&st;
			}
			else
			{
				b = (byte *)ent;
			}

			switch (f->type)
			{
			case F_LSTRING:
				*(char **)(b + f->ofs) = ED_NewString(value);
				break;
			case F_VECTOR:
				sscanf(value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
				((float *)(b + f->ofs))[0] = vec[0];
				((float *)(b + f->ofs))[1] = vec[1];
				((float *)(b + f->ofs))[2] = vec[2];
				break;
			case F_INT:
				*(int *)(b + f->ofs) = atoi(value);
				break;
			case F_FLOAT:
				*(float *)(b + f->ofs) = atof(value);
				break;
			case F_ANGLEHACK:
				v = atof(value);
				((float *)(b + f->ofs))[0] = 0;
				((float *)(b + f->ofs))[1] = v;
				((float *)(b + f->ofs))[2] = 0;
				break;
			case F_IGNORE:
				break;
			default:
				break;
			}

			return;
		}
	}

	gi.dprintf("%s is not a field\n", key);
}

/*
 * Parses an edict out of the given string,
 * returning the new position ed should be
 * a properly initialized empty edict.
 */
char *
ED_ParseEdict(char *data, edict_t *ent)
{
	qboolean init;
	char keyname[256];
	char *com_token;

	init = false;
	memset(&st, 0, sizeof(st));

	/* go through all the dictionary pairs */
	while (1)
	{
		/* parse key */
		com_token = COM_Parse(&data);

		if (com_token[0] == '}')
		{
			break;
		}

		if (!data)
		{
			gi.error("ED_ParseEntity: EOF without closing brace");
		}

		strncpy(keyname, com_token, sizeof(keyname) - 1);

		/* parse value */
		com_token = COM_Parse(&data);

		if (!data)
		{
			gi.error("ED_ParseEntity: EOF without closing brace");
		}

		if (com_token[0] == '}')
		{
			gi.error("ED_ParseEntity: closing brace without data");
		}

		init = true;

		/* keynames with a leading underscore are used
		   for utility comments, and are immediately
		   discarded by quake */
		if (keyname[0] == '_')
		{
			continue;
		}

		ED_ParseField(keyname, com_token, ent);
	}

	if (!init)
	{
		memset(ent, 0, sizeof(*ent));
	}

	return data;
}

/*
 * Creates a server's entity / program execution context by
 * parsing textual entity definitions out of an ent file.
 */
void SpawnEntities(char *mapname, char *entities, char *spawnpoint)
{
	edict_t *ent;
	int inhibit;
	char *com_token;
	int i;

	gi.FreeTags(TAG_LEVEL);

	memset(&level, 0, sizeof(level));
	memset(g_edicts, 0, game.maxentities * sizeof(g_edicts[0]));

	strncpy(level.mapname, mapname, sizeof(level.mapname) - 1);
	strncpy(game.spawnpoint, spawnpoint, sizeof(game.spawnpoint) - 1);

	/* set client fields on player ents */
	for (i = 0; i < game.maxclients; i++)
	{
		g_edicts[i + 1].client = game.clients + i;
	}

	ent = NULL;
	inhibit = 0;

	/* parse ents */
	while (1)
	{
		/* parse the opening brace */
		com_token = COM_Parse(&entities);

		if (!entities)
		{
			break;
		}

		if (com_token[0] != '{')
		{
			gi.error("ED_LoadFromFile: found %s when expecting {", com_token);
		}

		if (!ent)
		{
			ent = g_edicts;
		}
		else
		{
			ent = G_Spawn();
		}

		entities = ED_ParseEdict(entities, ent);

		if (ent != g_edicts)
		{
			if (deathmatch->value)
			{
				if (ent->spawnflags & SPAWNFLAG_NOT_DEATHMATCH)
				{
					G_FreeEdict(ent);
					inhibit++;
					continue;
				}
			}

			ent->spawnflags &= ~SPAWNFLAG_NOT_DEATHMATCH;
		}

		ED_CallSpawn(ent);
	}

	gi.dprintf("%i entities inhibited.\n", inhibit);
}

/*QUAKED worldspawn (0 0 0) ?
 *
 * Only used for the world.
 * "sky"	environment map name
 * "skyaxis"	vector axis for rotating sky
 * "skyrotate"	speed of rotation in degrees/second
 * "sounds"	music cd track number
 * "gravity"	800 is default gravity
 * "message"	text to print at user logon
 */
void SP_worldspawn(edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BSP;
	ent->inuse = true;	   /* since the world doesn't use G_Spawn() */
	ent->s.modelindex = 1; /* world model is always index 1 */

	/* make some data visible to the server */
	if (ent->message && ent->message[0])
	{
		gi.configstring(CS_NAME, ent->message);
		Q_strlcpy(level.level_name, ent->message, sizeof(level.level_name));
	}
	else
	{
		Q_strlcpy(level.level_name, level.mapname, sizeof(level.level_name));
	}

	if (st.sky && st.sky[0])
	{
		gi.configstring(CS_SKY, st.sky);
	}
	else
	{
		gi.configstring(CS_SKY, "unit1_");
	}

	gi.configstring(CS_SKYROTATE, va("%f", st.skyrotate));

	gi.configstring(CS_SKYAXIS, va("%f %f %f", st.skyaxis[0], st.skyaxis[1], st.skyaxis[2]));

	gi.configstring(CS_CDTRACK, va("%i", ent->sounds));

	gi.configstring(CS_MAXCLIENTS, va("%i", (int)(maxclients->value)));

	if (!st.gravity)
	{
		gi.cvar_set("sv_gravity", "800");
	}
	else
	{
		gi.cvar_set("sv_gravity", st.gravity);
	}

	snd_fry = gi.soundindex("player/fry.wav"); /* standing in lava / slime */

	gi.soundindex("player/lava1.wav");
	gi.soundindex("player/lava2.wav");

	gi.soundindex("misc/pc_up.wav");
	gi.soundindex("misc/talk1.wav");

	gi.soundindex("misc/udeath.wav");

	/* sexed sounds */
	gi.soundindex("*death1.wav");
	gi.soundindex("*death2.wav");
	gi.soundindex("*death3.wav");
	gi.soundindex("*death4.wav");
	gi.soundindex("*fall1.wav");
	gi.soundindex("*fall2.wav");
	gi.soundindex("*gurp1.wav"); /* drowning damage */
	gi.soundindex("*gurp2.wav");
	gi.soundindex("*jump1.wav"); /* player jump */
	gi.soundindex("*pain25_1.wav");
	gi.soundindex("*pain25_2.wav");
	gi.soundindex("*pain50_1.wav");
	gi.soundindex("*pain50_2.wav");
	gi.soundindex("*pain75_1.wav");
	gi.soundindex("*pain75_2.wav");
	gi.soundindex("*pain100_1.wav");
	gi.soundindex("*pain100_2.wav");

	/* sexed models */
	gi.modelindex("#w_blaster.md2");
	gi.modelindex("#w_shotgun.md2");
	gi.modelindex("#w_sshotgun.md2");
	gi.modelindex("#w_machinegun.md2");
	gi.modelindex("#w_chaingun.md2");
	gi.modelindex("#a_grenades.md2");
	gi.modelindex("#w_glauncher.md2");
	gi.modelindex("#w_rlauncher.md2");
	gi.modelindex("#w_hyperblaster.md2");
	gi.modelindex("#w_railgun.md2");
	gi.modelindex("#w_bfg.md2");

	/* ------------------- */

	gi.soundindex("player/gasp1.wav"); /* gasping for air */
	gi.soundindex("player/gasp2.wav"); /* head breaking surface, not gasping */

	gi.soundindex("player/watr_in.wav");  /* feet hitting water */
	gi.soundindex("player/watr_out.wav"); /* feet leaving water */

	gi.soundindex("player/watr_un.wav"); /* head going underwater */

	gi.soundindex("player/u_breath1.wav");
	gi.soundindex("player/u_breath2.wav");

	gi.soundindex("world/land.wav");   /* landing thud */
	gi.soundindex("misc/h2ohit1.wav"); /* landing splash */

	gi.soundindex("infantry/inflies1.wav");

	sm_meat_index = gi.modelindex("models/objects/gibs/sm_meat/tris.md2");
	gi.modelindex("models/objects/gibs/arm/tris.md2");
	gi.modelindex("models/objects/gibs/bone/tris.md2");
	gi.modelindex("models/objects/gibs/bone2/tris.md2");
	gi.modelindex("models/objects/gibs/chest/tris.md2");
	gi.modelindex("models/objects/gibs/skull/tris.md2");
	gi.modelindex("models/objects/gibs/head2/tris.md2");

	/* Setup light animation tables. 'a' is total darkness, 'z' is doublebright. */

	/* 0 normal */
	gi.configstring(CS_LIGHTS + 0, "m");

	/* 1 FLICKER (first variety) */
	gi.configstring(CS_LIGHTS + 1, "mmnmmommommnonmmonqnmmo");

	/* 2 SLOW STRONG PULSE */
	gi.configstring(CS_LIGHTS + 2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");

	/* 3 CANDLE (first variety) */
	gi.configstring(CS_LIGHTS + 3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");

	/* 4 FAST STROBE */
	gi.configstring(CS_LIGHTS + 4, "mamamamamama");

	/* 5 GENTLE PULSE 1 */
	gi.configstring(CS_LIGHTS + 5, "jklmnopqrstuvwxyzyxwvutsrqponmlkj");

	/* 6 FLICKER (second variety) */
	gi.configstring(CS_LIGHTS + 6, "nmonqnmomnmomomno");

	/* 7 CANDLE (second variety) */
	gi.configstring(CS_LIGHTS + 7, "mmmaaaabcdefgmmmmaaaammmaamm");

	/* 8 CANDLE (third variety) */
	gi.configstring(CS_LIGHTS + 8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");

	/* 9 SLOW STROBE (fourth variety) */
	gi.configstring(CS_LIGHTS + 9, "aaaaaaaazzzzzzzz");

	/* 10 FLUORESCENT FLICKER */
	gi.configstring(CS_LIGHTS + 10, "mmamammmmammamamaaamammma");

	/* 11 SLOW PULSE NOT FADE TO BLACK */
	gi.configstring(CS_LIGHTS + 11, "abcdefghijklmnopqrrqponmlkjihgfedcba");

	/* styles 32-62 are assigned by the light program for switchable lights */

	/* 63 testing */
	gi.configstring(CS_LIGHTS + 63, "a");
}
