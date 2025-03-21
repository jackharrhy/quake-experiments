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
 * Game command processing.
 *
 * =======================================================================
 */

#include "header/local.h"
#include "monster/player.h"

void Cmd_Noclip_f(edict_t *ent)
{
	char *msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf(ent, PRINT_HIGH,
				   "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	if (ent->movetype == MOVETYPE_NOCLIP)
	{
		ent->movetype = MOVETYPE_WALK;
		msg = "noclip OFF\n";
	}
	else
	{
		ent->movetype = MOVETYPE_NOCLIP;
		msg = "noclip ON\n";
	}

	gi.cprintf(ent, PRINT_HIGH, msg);
}

int PlayerSort(void const *a, void const *b)
{
	int anum, bnum;

	anum = *(int *)a;
	bnum = *(int *)b;

	anum = game.clients[anum].ps.stats[STAT_FRAGS];
	bnum = game.clients[bnum].ps.stats[STAT_FRAGS];

	if (anum < bnum)
	{
		return -1;
	}

	if (anum > bnum)
	{
		return 1;
	}

	return 0;
}

void Cmd_Players_f(edict_t *ent)
{
	int i;
	int count;
	char small[64];
	char large[1280];
	int index[256];

	count = 0;

	for (i = 0; i < maxclients->value; i++)
	{
		if (game.clients[i].pers.connected)
		{
			index[count] = i;
			count++;
		}
	}

	/* sort by frags */
	qsort(index, count, sizeof(index[0]), PlayerSort);

	/* print information */
	large[0] = 0;

	for (i = 0; i < count; i++)
	{
		Com_sprintf(small, sizeof(small), "%3i %s\n",
					game.clients[index[i]].ps.stats[STAT_FRAGS],
					game.clients[index[i]].pers.netname);

		if (strlen(small) + strlen(large) > sizeof(large) - 100)
		{
			/* can't print all of them in one packet */
			strcat(large, "...\n");
			break;
		}

		strcat(large, small);
	}

	gi.cprintf(ent, PRINT_HIGH, "%s\n%i players\n", large, count);
}

void Cmd_Wave_f(edict_t *ent)
{
	int i;

	i = atoi(gi.argv(1));

	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		return;
	}

	if (ent->client->anim_priority > ANIM_WAVE)
	{
		return;
	}

	ent->client->anim_priority = ANIM_WAVE;

	switch (i)
	{
	case 0:
		gi.cprintf(ent, PRINT_HIGH, "flipoff\n");
		ent->s.frame = FRAME_flip01 - 1;
		ent->client->anim_end = FRAME_flip12;
		break;
	case 1:
		gi.cprintf(ent, PRINT_HIGH, "salute\n");
		ent->s.frame = FRAME_salute01 - 1;
		ent->client->anim_end = FRAME_salute11;
		break;
	case 2:
		gi.cprintf(ent, PRINT_HIGH, "taunt\n");
		ent->s.frame = FRAME_taunt01 - 1;
		ent->client->anim_end = FRAME_taunt17;
		break;
	case 3:
		gi.cprintf(ent, PRINT_HIGH, "wave\n");
		ent->s.frame = FRAME_wave01 - 1;
		ent->client->anim_end = FRAME_wave11;
		break;
	case 4:
	default:
		gi.cprintf(ent, PRINT_HIGH, "point\n");
		ent->s.frame = FRAME_point01 - 1;
		ent->client->anim_end = FRAME_point12;
		break;
	}
}

qboolean
CheckFlood(edict_t *ent)
{
	int i;
	gclient_t *cl;

	if (flood_msgs->value)
	{
		cl = ent->client;

		if (level.time < cl->flood_locktill)
		{
			gi.cprintf(ent, PRINT_HIGH, "You can't talk for %d more seconds\n",
					   (int)(cl->flood_locktill - level.time));
			return true;
		}

		i = cl->flood_whenhead - flood_msgs->value + 1;

		if (i < 0)
		{
			i = (sizeof(cl->flood_when) / sizeof(cl->flood_when[0])) + i;
		}

		if (cl->flood_when[i] &&
			(level.time - cl->flood_when[i] < flood_persecond->value))
		{
			cl->flood_locktill = level.time + flood_waitdelay->value;
			gi.cprintf(ent, PRINT_CHAT,
					   "Flood protection:  You can't talk for %d seconds.\n",
					   (int)flood_waitdelay->value);
			return true;
		}

		cl->flood_whenhead = (cl->flood_whenhead + 1) %
							 (sizeof(cl->flood_when) / sizeof(cl->flood_when[0]));
		cl->flood_when[cl->flood_whenhead] = level.time;
	}

	return false;
}

void Cmd_Say_f(edict_t *ent, qboolean team, qboolean arg0)
{
	int j;
	edict_t *other;
	char *p;
	char text[2048];

	if ((gi.argc() < 2) && !arg0)
	{
		return;
	}

	Com_sprintf(text, sizeof(text), "%s: ", ent->client->pers.netname);

	if (arg0)
	{
		strcat(text, gi.argv(0));
		strcat(text, " ");
		strcat(text, gi.args());
	}
	else
	{
		p = gi.args();

		if (*p == '"')
		{
			p++;
			p[strlen(p) - 1] = 0;
		}

		strcat(text, p);
	}

	/* don't let text be too long for malicious reasons */
	if (strlen(text) > 150)
	{
		text[150] = 0;
	}

	strcat(text, "\n");

	if (CheckFlood(ent))
	{
		return;
	}

	if (dedicated->value)
	{
		gi.cprintf(NULL, PRINT_CHAT, "%s", text);
	}

	for (j = 1; j <= game.maxclients; j++)
	{
		other = &g_edicts[j];

		if (!other->inuse)
		{
			continue;
		}

		if (!other->client)
		{
			continue;
		}

		gi.cprintf(other, PRINT_CHAT, "%s", text);
	}
}

void ClientCommand(edict_t *ent)
{
	char *cmd;

	if (!ent->client)
	{
		return; /* not fully in game yet */
	}

	cmd = gi.argv(0);

	if (Q_stricmp(cmd, "players") == 0)
	{
		Cmd_Players_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "say") == 0)
	{
		Cmd_Say_f(ent, false, false);
		return;
	}

	if (Q_stricmp(cmd, "noclip") == 0)
	{
		Cmd_Noclip_f(ent);
	}
	else if (Q_stricmp(cmd, "wave") == 0)
	{
		Cmd_Wave_f(ent);
	}
	else /* anything that doesn't match a command will be a chat */
	{
		Cmd_Say_f(ent, false, true);
	}
}
