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
 * Minimized HUD implementation - removed weapons, ammo, health display
 *
 * =======================================================================
 */

#include "../header/local.h"

/*
 * ======================================================================
 *
 * INTERMISSION
 *
 * ======================================================================
 */

void MoveClientToIntermission(edict_t *ent)
{
	if (deathmatch->value || coop->value)
	{
		ent->client->showscores = true;
	}

	VectorCopy(level.intermission_origin, ent->s.origin);
	ent->client->ps.pmove.origin[0] = level.intermission_origin[0] * 8;
	ent->client->ps.pmove.origin[1] = level.intermission_origin[1] * 8;
	ent->client->ps.pmove.origin[2] = level.intermission_origin[2] * 8;
	VectorCopy(level.intermission_angle, ent->client->ps.viewangles);
	ent->client->ps.pmove.pm_type = PM_FREEZE;
	ent->client->ps.gunindex = 0;
	ent->client->ps.blend[3] = 0;
	ent->client->ps.rdflags &= ~RDF_UNDERWATER;

	/* clean up powerup info */
	ent->client->quad_framenum = 0;
	ent->client->invincible_framenum = 0;
	ent->client->breather_framenum = 0;
	ent->client->enviro_framenum = 0;

	ent->viewheight = 0;
	ent->s.modelindex = 0;
	ent->s.modelindex2 = 0;
	ent->s.modelindex3 = 0;
	ent->s.modelindex = 0;
	ent->s.effects = 0;
	ent->s.sound = 0;
	ent->solid = SOLID_NOT;
}

void BeginIntermission(edict_t *targ)
{
	int i, n;
	edict_t *ent, *client;

	if (level.intermissiontime)
	{
		return; /* already activated */
	}

	game.autosaved = false;

	/* respawn any dead clients */
	for (i = 0; i < maxclients->value; i++)
	{
		client = g_edicts + 1 + i;

		if (!client->inuse)
		{
			continue;
		}

		if (client->health <= 0)
		{
			respawn(client);
		}
	}

	level.intermissiontime = level.time;
	level.changemap = targ->map;

	if (strstr(level.changemap, "*"))
	{
		if (coop->value)
		{
			for (i = 0; i < maxclients->value; i++)
			{
				client = g_edicts + 1 + i;

				if (!client->inuse)
				{
					continue;
				}

				/* strip players of all keys between units */
				for (n = 0; n < MAX_ITEMS; n++)
				{
					if (itemlist[n].flags & IT_KEY)
					{
						client->client->pers.inventory[n] = 0;
					}
				}
			}
		}
	}
	else
	{
		if (!deathmatch->value)
		{
			level.exitintermission = 1; /* go immediately to the next level */
			return;
		}
	}

	level.exitintermission = 0;

	/* find an intermission spot */
	ent = G_Find(NULL, FOFS(classname), "info_player_intermission");

	if (!ent)
	{
		/* the map creator forgot to put in an intermission point... */
		ent = G_Find(NULL, FOFS(classname), "info_player_start");

		if (!ent)
		{
			ent = G_Find(NULL, FOFS(classname), "info_player_deathmatch");
		}
	}
	else
	{
		/* chose one of four spots */
		i = rand() & 3;

		while (i--)
		{
			ent = G_Find(ent, FOFS(classname), "info_player_intermission");

			if (!ent) /* wrap around the list */
			{
				ent = G_Find(ent, FOFS(classname), "info_player_intermission");
			}
		}
	}

	VectorCopy(ent->s.origin, level.intermission_origin);
	VectorCopy(ent->s.angles, level.intermission_angle);

	/* move all clients to the intermission point */
	for (i = 0; i < maxclients->value; i++)
	{
		client = g_edicts + 1 + i;

		if (!client->inuse)
		{
			continue;
		}

		MoveClientToIntermission(client);
	}
}

/* Empty stub for scoreboard - not needed for movement */
void DeathmatchScoreboardMessage(edict_t *ent, edict_t *killer)
{
	/* Minimal stub */
}

/* Empty stub for scoreboard - not needed for movement */
void DeathmatchScoreboard(edict_t *ent)
{
	/* Minimal stub */
}

/* Empty stub for scoreboard - not needed for movement */
void Cmd_Score_f(edict_t *ent)
{
	/* Minimal stub */
}

/* Empty stub for help display - not needed for movement */
void HelpComputer(edict_t *ent)
{
	/* Minimal stub */
}

/* Empty stub for help display - not needed for movement */
void Cmd_Help_f(edict_t *ent)
{
	/* Minimal stub */
}

/* Simplified stats setting - remove weapon, ammo displays */
void G_SetStats(edict_t *ent)
{
	/* Minimal implementation - clear stats */
	memset(ent->client->ps.stats, 0, sizeof(ent->client->ps.stats));

	/* Set minimal required stats */
	ent->client->ps.stats[STAT_LAYOUTS] = 0;

	if (ent->client->chase_target)
		ent->client->ps.stats[STAT_CHASE] = CS_PLAYERSKINS +
											(ent->client->chase_target - g_edicts) - 1;
}
