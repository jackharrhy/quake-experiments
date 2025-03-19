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
 * Minimized player weapon code - only essential functions maintained
 *
 * =======================================================================
 */

#include "../header/local.h"
#include "../monster/player.h"

/* Project source function - kept for movement code */
void P_ProjectSource(edict_t *ent, vec3_t distance,
					 vec3_t forward, vec3_t right, vec3_t result)
{
	gclient_t *client = ent->client;
	float *point = ent->s.origin;
	vec3_t _distance;

	VectorCopy(distance, _distance);

	if (client->pers.hand == LEFT_HANDED)
	{
		_distance[1] *= -1;
	}
	else if (client->pers.hand == CENTER_HANDED)
	{
		_distance[1] = 0;
	}

	G_ProjectSource(point, _distance, forward, right, result);
}

/* Noise function to maintain API compatibility */
void PlayerNoise(edict_t *who, vec3_t where, int type)
{
	/* Minimal stub implementation */
}

/* Stub for item pickup - needed for map functionality */
qboolean
Pickup_Weapon(edict_t *ent, edict_t *other)
{
	/* Minimal stub implementation - don't handle weapons */
	return false;
}

/* Required by client code */
void ChangeWeapon(edict_t *ent)
{
	/* Minimal stub implementation */
	ent->client->ps.gunindex = 0; /* Hide weapon model */
}

/* Minimal stub - called by ClientThink */
void Think_Weapon(edict_t *ent)
{
	/* Minimal stub that does nothing */
}

/* Stub function to maintain API compatibility */
void Use_Weapon(edict_t *ent, gitem_t *item)
{
	/* Minimal stub that does nothing */
}

/* Stub function to maintain API compatibility */
void Drop_Weapon(edict_t *ent, gitem_t *item)
{
	/* Minimal stub that does nothing */
}

/* Stub for Weapon_Generic function */
void Weapon_Generic(edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST,
					int FRAME_IDLE_LAST, int FRAME_DEACTIVATE_LAST, int *pause_frames,
					int *fire_frames, void (*fire)(edict_t *ent))
{
	/* Minimal stub that does nothing */
}

/* All specific weapon functions as minimal stubs */
void weapon_grenade_fire(edict_t *ent, qboolean held)
{
	/* Empty stub */
}

void Weapon_Grenade(edict_t *ent)
{
	/* Empty stub */
}

void weapon_grenadelauncher_fire(edict_t *ent)
{
	/* Empty stub */
}

void Weapon_GrenadeLauncher(edict_t *ent)
{
	/* Empty stub */
}

void Weapon_RocketLauncher_Fire(edict_t *ent)
{
	/* Empty stub */
}

void Weapon_RocketLauncher(edict_t *ent)
{
	/* Empty stub */
}

void Weapon_Blaster_Fire(edict_t *ent)
{
	/* Empty stub */
}

void Weapon_Blaster(edict_t *ent)
{
	/* Empty stub */
}

void Weapon_HyperBlaster_Fire(edict_t *ent)
{
	/* Empty stub */
}

void Weapon_HyperBlaster(edict_t *ent)
{
	/* Empty stub */
}

void Machinegun_Fire(edict_t *ent)
{
	/* Empty stub */
}

void Weapon_Machinegun(edict_t *ent)
{
	/* Empty stub */
}

void Chaingun_Fire(edict_t *ent)
{
	/* Empty stub */
}

void Weapon_Chaingun(edict_t *ent)
{
	/* Empty stub */
}

void weapon_shotgun_fire(edict_t *ent)
{
	/* Empty stub */
}

void Weapon_Shotgun(edict_t *ent)
{
	/* Empty stub */
}

void weapon_supershotgun_fire(edict_t *ent)
{
	/* Empty stub */
}

void Weapon_SuperShotgun(edict_t *ent)
{
	/* Empty stub */
}

void weapon_railgun_fire(edict_t *ent)
{
	/* Empty stub */
}

void Weapon_Railgun(edict_t *ent)
{
	/* Empty stub */
}

void weapon_bfg_fire(edict_t *ent)
{
	/* Empty stub */
}

void Weapon_BFG(edict_t *ent)
{
	/* Empty stub */
}
