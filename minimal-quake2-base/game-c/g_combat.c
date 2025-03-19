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
 * Minimal stub for combat system - only essential functions kept
 *
 * =======================================================================
 */

#include "header/local.h"

/*
 * Returns true if the inflictor can
 * directly damage the target. Used for
 * explosions and melee attacks.
 */
qboolean
CanDamage(edict_t *targ, edict_t *inflictor)
{
	vec3_t dest;
	trace_t trace;

	/* bmodels need special checking because their origin is 0,0,0 */
	if (targ->movetype == MOVETYPE_PUSH)
	{
		VectorAdd(targ->absmin, targ->absmax, dest);
		VectorScale(dest, 0.5, dest);
		trace = gi.trace(inflictor->s.origin, vec3_origin, vec3_origin,
						 dest, inflictor, MASK_SOLID);

		if (trace.fraction == 1.0)
		{
			return true;
		}

		if (trace.ent == targ)
		{
			return true;
		}

		return false;
	}

	trace = gi.trace(inflictor->s.origin, vec3_origin, vec3_origin,
					 targ->s.origin, inflictor, MASK_SOLID);

	if (trace.fraction == 1.0)
	{
		return true;
	}

	VectorCopy(targ->s.origin, dest);
	dest[0] += 15.0;
	dest[1] += 15.0;
	trace = gi.trace(inflictor->s.origin, vec3_origin, vec3_origin,
					 dest, inflictor, MASK_SOLID);

	if (trace.fraction == 1.0)
	{
		return true;
	}

	VectorCopy(targ->s.origin, dest);
	dest[0] += 15.0;
	dest[1] -= 15.0;
	trace = gi.trace(inflictor->s.origin, vec3_origin, vec3_origin,
					 dest, inflictor, MASK_SOLID);

	if (trace.fraction == 1.0)
	{
		return true;
	}

	VectorCopy(targ->s.origin, dest);
	dest[0] -= 15.0;
	dest[1] += 15.0;
	trace = gi.trace(inflictor->s.origin, vec3_origin, vec3_origin,
					 dest, inflictor, MASK_SOLID);

	if (trace.fraction == 1.0)
	{
		return true;
	}

	VectorCopy(targ->s.origin, dest);
	dest[0] -= 15.0;
	dest[1] -= 15.0;
	trace = gi.trace(inflictor->s.origin, vec3_origin, vec3_origin,
					 dest, inflictor, MASK_SOLID);

	if (trace.fraction == 1.0)
	{
		return true;
	}

	return false;
}

/* Stub functions to maintain API compatibility */
void Killed(edict_t *targ, edict_t *inflictor, edict_t *attacker,
			int damage, vec3_t point)
{
	/* Minimal implementation */
	if (targ->health < -999)
	{
		targ->health = -999;
	}

	if ((targ->movetype == MOVETYPE_PUSH) ||
		(targ->movetype == MOVETYPE_STOP) || (targ->movetype == MOVETYPE_NONE))
	{
		/* doors, triggers, etc */
		if (targ->die)
			targ->die(targ, inflictor, attacker, damage, point);
		return;
	}

	if (targ->die)
		targ->die(targ, inflictor, attacker, damage, point);
}

void T_Damage(edict_t *targ, edict_t *inflictor, edict_t *attacker,
			  vec3_t dir, vec3_t point, vec3_t normal, int damage,
			  int knockback, int dflags, int mod)
{
	/* Minimal stub implementation */
	if (!targ->takedamage)
		return;

	/* Simplified - just call pain and die if needed */
	if (targ->health <= 0)
	{
		if (targ->die)
			Killed(targ, inflictor, attacker, damage, point);
		return;
	}

	if (targ->pain)
		targ->pain(targ, attacker, knockback, damage);
}

void T_RadiusDamage(edict_t *inflictor, edict_t *attacker, float damage,
					edict_t *ignore, float radius, int mod)
{
	/* Empty stub - not needed for movement and map loading */
}