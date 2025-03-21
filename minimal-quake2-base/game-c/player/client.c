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
 * Interface between client <-> game and client calculations.
 *
 * =======================================================================
 */

#include "../header/local.h"

void SP_info_player_start(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->think = NULL;
	self->nextthink = level.time + FRAMETIME;
}

void InitClientPersistant(gclient_t *client)
{
	memset(&client->pers, 0, sizeof(client->pers));

	client->pers.connected = true;
}

void InitClientResp(gclient_t *client)
{
	qboolean id_state = client->resp.id_state;

	memset(&client->resp, 0, sizeof(client->resp));

	client->resp.id_state = id_state;

	client->resp.enterframe = level.framenum;
}

/*
 * Returns the distance to the
 * nearest player from the given spot
 */
float PlayersRangeFromSpot(edict_t *spot)
{
	edict_t *player;
	float bestplayerdistance;
	vec3_t v;
	int n;
	float playerdistance;

	bestplayerdistance = 9999999;

	for (n = 1; n <= maxclients->value; n++)
	{
		player = &g_edicts[n];

		if (!player->inuse)
		{
			continue;
		}

		VectorSubtract(spot->s.origin, player->s.origin, v);
		playerdistance = VectorLength(v);

		if (playerdistance < bestplayerdistance)
		{
			bestplayerdistance = playerdistance;
		}
	}

	return bestplayerdistance;
}

/*
 * Chooses a player start, deathmatch start, etc
 */
void SelectSpawnPoint(edict_t *ent, vec3_t origin, vec3_t angles)
{
	edict_t *spot = NULL;

	while ((spot = G_Find(spot, FOFS(classname), "info_player_start")) != NULL)
	{
		if (!game.spawnpoint[0] && !spot->targetname)
		{
			break;
		}

		if (!game.spawnpoint[0] || !spot->targetname)
		{
			continue;
		}

		if (Q_stricmp(game.spawnpoint, spot->targetname) == 0)
		{
			break;
		}
	}

	if (!spot)
	{
		if (!game.spawnpoint[0])
		{
			/* there wasn't a spawnpoint without a target, so use any */
			spot = G_Find(spot, FOFS(classname), "info_player_start");
		}

		if (!spot)
		{
			gi.error("Couldn't find spawn point %s\n", game.spawnpoint);
		}
	}

	VectorCopy(spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy(spot->s.angles, angles);
}

/* ====================================================================== */

void respawn(edict_t *self)
{
	if (deathmatch->value)
	{
		self->svflags &= ~SVF_NOCLIENT;
		PutClientInServer(self);

		/* add a teleportation effect */
		self->s.event = EV_PLAYER_TELEPORT;

		/* hold in place briefly */
		self->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
		self->client->ps.pmove.pm_time = 14;

		return;
	}

	/* restart the entire server */
	gi.AddCommandString("menu_loadgame\n");
}

/* ============================================================== */

/*
 * Called when a player connects
 * to a server or respawns in
 * a deathmatch.
 */
void PutClientInServer(edict_t *ent)
{
	vec3_t mins = {-16, -16, -24};
	vec3_t maxs = {16, 16, 32};
	int index;
	vec3_t spawn_origin, spawn_angles;
	gclient_t *client;
	int i;
	client_persistant_t saved;
	client_respawn_t resp;

	/* find a spawn point do it before setting health
	   back up, so farthest ranging doesn't count this
	   client */
	SelectSpawnPoint(ent, spawn_origin, spawn_angles);

	index = ent - g_edicts - 1;
	client = ent->client;

	/* deathmatch wipes most client data every spawn */
	if (deathmatch->value)
	{
		char userinfo[MAX_INFO_STRING];

		resp = client->resp;
		memcpy(userinfo, client->pers.userinfo, sizeof(userinfo));
		InitClientPersistant(client);
		ClientUserinfoChanged(ent, userinfo);
	}
	else
	{
		memset(&resp, 0, sizeof(resp));
	}

	/* clear everything but the persistant data */
	saved = client->pers;
	memset(client, 0, sizeof(*client));
	client->pers = saved;

	if (!client->pers.connected)
	{
		InitClientPersistant(client);
	}

	client->resp = resp;

	/* clear entity values */
	ent->groundentity = NULL;
	ent->client = &game.clients[index];
	ent->movetype = MOVETYPE_WALK;
	ent->viewheight = 22;
	ent->inuse = true;
	ent->classname = "player";
	ent->mass = 200;
	ent->solid = SOLID_BBOX;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->model = "players/male/tris.md2";
	ent->waterlevel = 0;
	ent->watertype = 0;

	VectorCopy(mins, ent->mins);
	VectorCopy(maxs, ent->maxs);
	VectorClear(ent->velocity);

	/* clear playerstate values */
	memset(&ent->client->ps, 0, sizeof(client->ps));

	client->ps.pmove.origin[0] = spawn_origin[0] * 8;
	client->ps.pmove.origin[1] = spawn_origin[1] * 8;
	client->ps.pmove.origin[2] = spawn_origin[2] * 8;
	client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;

	client->ps.fov = atoi(Info_ValueForKey(client->pers.userinfo, "fov"));

	if (client->ps.fov < 1)
	{
		client->ps.fov = 90;
	}
	else if (client->ps.fov > 160)
	{
		client->ps.fov = 160;
	}

	/* No weapon model in minimal version */
	client->ps.gunindex = 0;

	/* clear entity state values */
	ent->s.effects = 0;
	ent->s.skinnum = ent - g_edicts - 1;
	ent->s.modelindex = 255; /* will use the skin specified model */
	ent->s.modelindex2 = 0;	 /* no custom gun model */
	ent->s.skinnum = ent - g_edicts - 1;

	ent->s.frame = 0;
	VectorCopy(spawn_origin, ent->s.origin);
	ent->s.origin[2] += 1; /* make sure off ground */
	VectorCopy(ent->s.origin, ent->s.old_origin);

	/* set the delta angle */
	for (i = 0; i < 3; i++)
	{
		client->ps.pmove.delta_angles[i] = ANGLE2SHORT(
			spawn_angles[i] - client->resp.cmd_angles[i]);
	}

	ent->s.angles[PITCH] = 0;
	ent->s.angles[YAW] = spawn_angles[YAW];
	ent->s.angles[ROLL] = 0;
	VectorCopy(ent->s.angles, client->ps.viewangles);
	VectorCopy(ent->s.angles, client->v_angle);

	if (!KillBox(ent))
	{
		/* could't spawn in? */
	}

	gi.linkentity(ent);
}

/*
 * A client has just connected to
 * the server in deathmatch mode,
 * so clear everything out before
 * starting them.
 */
void ClientBeginDeathmatch(edict_t *ent)
{
	G_InitEdict(ent);

	InitClientResp(ent->client);

	/* locate ent at a spawn point */
	PutClientInServer(ent);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_LOGIN);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	gi.bprintf(PRINT_HIGH, "%s entered the game\n", ent->client->pers.netname);

	/* make sure all view stuff is valid */
	ClientEndServerFrame(ent);
}

/*
 * called when a client has finished connecting, and is ready
 * to be placed into the game.  This will happen every level load.
 */
void ClientBegin(edict_t *ent)
{
	int i;

	ent->client = game.clients + (ent - g_edicts - 1);

	if (deathmatch->value)
	{
		ClientBeginDeathmatch(ent);
		return;
	}

	/* if there is already a body waiting for
	   us (a loadgame), just take it, otherwise
	   spawn one from scratch */
	if (ent->inuse == true)
	{
		/* the client has cleared the client side viewangles upon
		   connecting to the server, which is different than the
		   state when the game is saved, so we need to compensate
		   with deltaangles */
		for (i = 0; i < 3; i++)
		{
			ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(
				ent->client->ps.viewangles[i]);
		}
	}
	else
	{
		/* a spawn point will completely reinitialize the entity
		   except for the persistant data that was initialized at
		   ClientConnect() time */
		G_InitEdict(ent);
		ent->classname = "player";
		InitClientResp(ent->client);
		PutClientInServer(ent);
	}

	/* send effect if in a multiplayer game */
	if (game.maxclients > 1)
	{
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_LOGIN);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

		gi.bprintf(PRINT_HIGH, "%s entered the game\n",
				   ent->client->pers.netname);
	}

	/* make sure all view stuff is valid */
	ClientEndServerFrame(ent);
}

/*
 * Called whenever the player updates a userinfo variable.
 *
 * The game can override any of the settings in place
 * (forcing skins or names, etc) before copying it off.
 */
void ClientUserinfoChanged(edict_t *ent, char *userinfo)
{
	char *s;
	int playernum;

	/* check for malformed or illegal info strings */
	if (!Info_Validate(userinfo))
	{
		strcpy(userinfo, "\\name\\badinfo\\skin\\male/grunt");
	}

	/* set name */
	s = Info_ValueForKey(userinfo, "name");
	strncpy(ent->client->pers.netname, s, sizeof(ent->client->pers.netname) - 1);

	/* set skin */
	s = Info_ValueForKey(userinfo, "skin");

	playernum = ent - g_edicts - 1;

	/* combine name and skin into a configstring */
	gi.configstring(CS_PLAYERSKINS + playernum,
					va("%s\\%s", ent->client->pers.netname, s));

	/* set player name field (used in id_state view) */
	gi.configstring(CS_GENERAL + playernum, ent->client->pers.netname);

	ent->client->ps.fov = atoi(Info_ValueForKey(userinfo, "fov"));

	if (ent->client->ps.fov < 1)
	{
		ent->client->ps.fov = 90;
	}
	else if (ent->client->ps.fov > 160)
	{
		ent->client->ps.fov = 160;
	}

	/* handedness */
	s = Info_ValueForKey(userinfo, "hand");

	if (strlen(s))
	{
		ent->client->pers.hand = atoi(s);
	}

	/* save off the userinfo in case we want to check something later */
	strncpy(ent->client->pers.userinfo, userinfo,
			sizeof(ent->client->pers.userinfo) - 1);
}

/*
 * Called when a player begins connecting to the server.
 * The game can refuse entrance to a client by returning false.
 * If the client is allowed, the connection process will continue
 * and eventually get to ClientBegin()
 * Changing levels will NOT cause this to be called again, but
 * loadgames will.
 */
qboolean
ClientConnect(edict_t *ent, char *userinfo)
{
	char *value;

	/* check to see if they are on the banned IP list */
	value = Info_ValueForKey(userinfo, "ip");

	/* check for a password */
	value = Info_ValueForKey(userinfo, "password");

	if (*password->string && strcmp(password->string, "none") &&
		strcmp(password->string, value))
	{
		Info_SetValueForKey(userinfo, "rejmsg",
							"Password required or incorrect.");
		return false;
	}

	/* they can connect */
	ent->client = game.clients + (ent - g_edicts - 1);

	/* if there is already a body waiting for us (a loadgame),
	   just take it, otherwise spawn one from scratch */
	if (ent->inuse == false)
	{
		/* clear the respawning variables */
		ent->client->resp.id_state = true;

		InitClientResp(ent->client);

		InitClientPersistant(ent->client);
	}

	ClientUserinfoChanged(ent, userinfo);

	if (game.maxclients > 1)
	{
		gi.dprintf("%s connected\n", ent->client->pers.netname);
	}

	ent->client->pers.connected = true;
	return true;
}

/*
 * Called when a player drops from the server.
 * Will not be called between levels.
 */
void ClientDisconnect(edict_t *ent)
{
	int playernum;

	if (!ent->client)
	{
		return;
	}

	gi.bprintf(PRINT_HIGH, "%s disconnected\n", ent->client->pers.netname);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_LOGOUT);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	gi.unlinkentity(ent);
	ent->s.modelindex = 0;
	ent->solid = SOLID_NOT;
	ent->inuse = false;
	ent->classname = "disconnected";
	ent->client->pers.connected = false;

	playernum = ent - g_edicts - 1;
	gi.configstring(CS_PLAYERSKINS + playernum, "");
}

/* ============================================================== */

edict_t *pm_passent;

/*
 * pmove doesn't need to know about
 * passent and contentmask
 */
trace_t
PM_trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	return gi.trace(start, mins, maxs, end, pm_passent, MASK_PLAYERSOLID);
}

unsigned
CheckBlock(void *b, int c)
{
	int v, i;

	v = 0;

	for (i = 0; i < c; i++)
	{
		v += ((byte *)b)[i];
	}

	return v;
}

void PrintPmove(pmove_t *pm)
{
	unsigned c1, c2;

	c1 = CheckBlock(&pm->s, sizeof(pm->s));
	c2 = CheckBlock(&pm->cmd, sizeof(pm->cmd));
	Com_Printf("sv %3i:%i %i\n", pm->cmd.impulse, c1, c2);
}

/*
 * This will be called once for each client frame, which will
 * usually be a couple times for each server frame.
 */
void ClientThink(edict_t *ent, usercmd_t *ucmd)
{
	gclient_t *client;
	edict_t *other;
	int i, j;
	pmove_t pm;

	level.current_entity = ent;
	client = ent->client;

	pm_passent = ent;

	/* set up for pmove */
	memset(&pm, 0, sizeof(pm));

	if (ent->movetype == MOVETYPE_NOCLIP)
	{
		client->ps.pmove.pm_type = PM_SPECTATOR;
	}
	else if (ent->s.modelindex != 255)
	{
		client->ps.pmove.pm_type = PM_GIB;
	}
	else
	{
		client->ps.pmove.pm_type = PM_NORMAL;
	}

	if (ent->client_local_gravity != 0)
	{
		client->ps.pmove.gravity = ent->client_local_gravity;
	}
	else
	{
		client->ps.pmove.gravity = sv_gravity->value;
	}

	pm.s = client->ps.pmove;

	for (i = 0; i < 3; i++)
	{
		pm.s.origin[i] = ent->s.origin[i] * 8;
		/* save to an int first, in case the short overflows
		 * so we get defined behavior (at least with -fwrapv) */
		int tmpVel = ent->velocity[i] * 8;
		pm.s.velocity[i] = tmpVel;
	}

	if (memcmp(&client->old_pmove, &pm.s, sizeof(pm.s)))
	{
		pm.snapinitial = true;
	}

	pm.cmd = *ucmd;

	pm.trace = PM_trace; /* adds default parms */
	pm.pointcontents = gi.pointcontents;

	/* perform a pmove */
	gi.Pmove(&pm);

	/* save results of pmove */
	client->ps.pmove = pm.s;
	client->old_pmove = pm.s;

	for (i = 0; i < 3; i++)
	{
		ent->s.origin[i] = pm.s.origin[i] * 0.125;
		ent->velocity[i] = pm.s.velocity[i] * 0.125;
	}

	VectorCopy(pm.mins, ent->mins);
	VectorCopy(pm.maxs, ent->maxs);

	client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
	client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
	client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);

	if (ent->groundentity && !pm.groundentity && (pm.cmd.upmove >= 10) &&
		(pm.waterlevel == 0))
	{
		gi.sound(ent, CHAN_VOICE, gi.soundindex("*jump1.wav"), 1, ATTN_NORM, 0);
		PlayerNoise(ent, ent->s.origin, PNOISE_SELF);
	}

	ent->viewheight = pm.viewheight;
	ent->waterlevel = pm.waterlevel;
	ent->watertype = pm.watertype;
	ent->groundentity = pm.groundentity;

	if (pm.groundentity)
	{
		ent->groundentity_linkcount = pm.groundentity->linkcount;
	}

	VectorCopy(pm.viewangles, client->v_angle);
	VectorCopy(pm.viewangles, client->ps.viewangles);

	gi.linkentity(ent);

	if (ent->movetype != MOVETYPE_NOCLIP)
	{
		G_TouchTriggers(ent);
	}

	/* touch other objects */
	for (i = 0; i < pm.numtouch; i++)
	{
		other = pm.touchents[i];

		for (j = 0; j < i; j++)
		{
			if (pm.touchents[j] == other)
			{
				break;
			}
		}

		if (j != i)
		{
			continue; /* duplicated */
		}

		if (!other->touch)
		{
			continue;
		}

		other->touch(other, ent, NULL, NULL);
	}

	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;

	ent->light_level = ucmd->lightlevel;
}

/*
 * This will be called once for each server frame,
 * before running any other entities in the world.
 */
void ClientBeginServerFrame(edict_t *ent)
{
	gclient_t *client;

	client = ent->client;

	client->latched_buttons = 0;
}
