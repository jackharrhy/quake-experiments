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
 * Monster movement support functions.
 *
 * =======================================================================
 */

#include "../header/local.h"

#define STEPSIZE 18

/*
 * Returns false if any part of the bottom of
 * the entity is off an edge that is not a staircase.
 */
int c_yes, c_no;

qboolean
M_CheckBottom(edict_t *ent)
{
	vec3_t mins, maxs, start, stop;
	trace_t trace;
	int x, y;
	float mid, bottom;

	VectorAdd(ent->s.origin, ent->mins, mins);
	VectorAdd(ent->s.origin, ent->maxs, maxs);

	/* if all of the points under the corners are
	   solid world, don't bother with the tougher
	   checks  the corners must be within 16 of the
	   midpoint */
	start[2] = mins[2] - 1;

	for (x = 0; x <= 1; x++)
	{
		for (y = 0; y <= 1; y++)
		{
			start[0] = x ? maxs[0] : mins[0];
			start[1] = y ? maxs[1] : mins[1];

			if (gi.pointcontents(start) != CONTENTS_SOLID)
			{
				goto realcheck;
			}
		}
	}

	c_yes++;
	return true; /* we got out easy */

realcheck:
	c_no++;

	/* check it for real... */
	start[2] = mins[2];

	/* the midpoint must be within 16 of the bottom */
	start[0] = stop[0] = (mins[0] + maxs[0]) * 0.5;
	start[1] = stop[1] = (mins[1] + maxs[1]) * 0.5;
	stop[2] = start[2] - 2 * STEPSIZE;
	trace = gi.trace(start, vec3_origin, vec3_origin,
					 stop, ent, MASK_MONSTERSOLID);

	if (trace.fraction == 1.0)
	{
		return false;
	}

	mid = bottom = trace.endpos[2];

	/* the corners must be within 16 of the midpoint */
	for (x = 0; x <= 1; x++)
	{
		for (y = 0; y <= 1; y++)
		{
			start[0] = stop[0] = x ? maxs[0] : mins[0];
			start[1] = stop[1] = y ? maxs[1] : mins[1];

			trace = gi.trace(start, vec3_origin, vec3_origin,
							 stop, ent, MASK_MONSTERSOLID);

			if ((trace.fraction != 1.0) && (trace.endpos[2] > bottom))
			{
				bottom = trace.endpos[2];
			}

			if ((trace.fraction == 1.0) || (mid - trace.endpos[2] > STEPSIZE))
			{
				return false;
			}
		}
	}

	c_yes++;
	return true;
}

/*
 * Called by monster program code.
 * The move will be adjusted for
 * slopes and stairs, but if the move
 * isn't possible, no move is done,
 * false is returned, and
 * pr_global_struct->trace_normal
 * is set to the normal of the
 * blocking wall
 */
qboolean
SV_movestep(edict_t *ent, vec3_t move, qboolean relink)
{
	float dz;
	vec3_t oldorg, neworg, end;
	trace_t trace;
	int i;
	float stepsize;
	vec3_t test;
	int contents;

	/* try the move */
	VectorCopy(ent->s.origin, oldorg);
	VectorAdd(ent->s.origin, move, neworg);

	/* flying monsters don't step up */
	if (ent->flags & (FL_SWIM | FL_FLY))
	{
		/* try one move with vertical motion, then one without */
		for (i = 0; i < 2; i++)
		{
			VectorAdd(ent->s.origin, move, neworg);

			if ((i == 0) && ent->enemy)
			{
				if (!ent->goalentity)
				{
					ent->goalentity = ent->enemy;
				}

				dz = ent->s.origin[2] - ent->goalentity->s.origin[2];

				if (ent->goalentity->client)
				{
					if (dz > 40)
					{
						neworg[2] -= 8;
					}

					if (!((ent->flags & FL_SWIM) && (ent->waterlevel < 2)))
					{
						if (dz < 30)
						{
							neworg[2] += 8;
						}
					}
				}
				else
				{
					if (dz > 8)
					{
						neworg[2] -= 8;
					}
					else if (dz > 0)
					{
						neworg[2] -= dz;
					}
					else if (dz < -8)
					{
						neworg[2] += 8;
					}
					else
					{
						neworg[2] += dz;
					}
				}
			}

			trace = gi.trace(ent->s.origin, ent->mins, ent->maxs,
							 neworg, ent, MASK_MONSTERSOLID);

			/* fly monsters don't enter water voluntarily */
			if (ent->flags & FL_FLY)
			{
				if (!ent->waterlevel)
				{
					test[0] = trace.endpos[0];
					test[1] = trace.endpos[1];
					test[2] = trace.endpos[2] + ent->mins[2] + 1;
					contents = gi.pointcontents(test);

					if (contents & MASK_WATER)
					{
						return false;
					}
				}
			}

			/* swim monsters don't exit water voluntarily */
			if (ent->flags & FL_SWIM)
			{
				if (ent->waterlevel < 2)
				{
					test[0] = trace.endpos[0];
					test[1] = trace.endpos[1];
					test[2] = trace.endpos[2] + ent->mins[2] + 1;
					contents = gi.pointcontents(test);

					if (!(contents & MASK_WATER))
					{
						return false;
					}
				}
			}

			if (trace.fraction == 1)
			{
				VectorCopy(trace.endpos, ent->s.origin);

				if (relink)
				{
					gi.linkentity(ent);
					G_TouchTriggers(ent);
				}

				return true;
			}

			if (!ent->enemy)
			{
				break;
			}
		}

		return false;
	}

	stepsize = 1;

	neworg[2] += stepsize;
	VectorCopy(neworg, end);
	end[2] -= stepsize * 2;

	trace = gi.trace(neworg, ent->mins, ent->maxs, end, ent, MASK_MONSTERSOLID);

	if (trace.allsolid)
	{
		return false;
	}

	if (trace.startsolid)
	{
		neworg[2] -= stepsize;
		trace = gi.trace(neworg, ent->mins, ent->maxs, end,
						 ent, MASK_MONSTERSOLID);

		if (trace.allsolid || trace.startsolid)
		{
			return false;
		}
	}

	/* don't go in to water */
	if (ent->waterlevel == 0)
	{
		test[0] = trace.endpos[0];
		test[1] = trace.endpos[1];
		test[2] = trace.endpos[2] + ent->mins[2] + 1;
		contents = gi.pointcontents(test);

		if (contents & MASK_WATER)
		{
			return false;
		}
	}

	if (trace.fraction == 1)
	{
		/* if monster had the ground pulled out, go ahead and fall */
		if (ent->flags & FL_PARTIALGROUND)
		{
			VectorAdd(ent->s.origin, move, ent->s.origin);

			if (relink)
			{
				gi.linkentity(ent);
				G_TouchTriggers(ent);
			}

			ent->groundentity = NULL;
			return true;
		}

		return false; /* walked off an edge */
	}

	/* check point traces down for dangling corners */
	VectorCopy(trace.endpos, ent->s.origin);

	if (!M_CheckBottom(ent))
	{
		if (ent->flags & FL_PARTIALGROUND)
		{
			/* entity had floor mostly pulled out
			   from underneath it and is trying to
			   correct */
			if (relink)
			{
				gi.linkentity(ent);
				G_TouchTriggers(ent);
			}

			return true;
		}

		VectorCopy(oldorg, ent->s.origin);
		return false;
	}

	if (ent->flags & FL_PARTIALGROUND)
	{
		ent->flags &= ~FL_PARTIALGROUND;
	}

	ent->groundentity = trace.ent;
	ent->groundentity_linkcount = trace.ent->linkcount;

	/* the move is ok */
	if (relink)
	{
		gi.linkentity(ent);
		G_TouchTriggers(ent);
	}

	return true;
}

/* ============================================================================ */

void M_ChangeYaw(edict_t *ent)
{
	float ideal;
	float current;
	float move;
	float speed;

	current = anglemod(ent->s.angles[YAW]);
	ideal = ent->ideal_yaw;

	if (current == ideal)
	{
		return;
	}

	move = ideal - current;
	speed = ent->yaw_speed;

	if (ideal > current)
	{
		if (move >= 180)
		{
			move = move - 360;
		}
	}
	else
	{
		if (move <= -180)
		{
			move = move + 360;
		}
	}

	if (move > 0)
	{
		if (move > speed)
		{
			move = speed;
		}
	}
	else
	{
		if (move < -speed)
		{
			move = -speed;
		}
	}

	ent->s.angles[YAW] = anglemod(current + move);
}

/*
 * Turns to the movement direction, and
 * walks the current distance if facing it.
 */
qboolean
SV_StepDirection(edict_t *ent, float yaw, float dist)
{
	vec3_t move, oldorigin;
	float delta;

	ent->ideal_yaw = yaw;
	M_ChangeYaw(ent);

	yaw = yaw * M_PI * 2 / 360;
	move[0] = cos(yaw) * dist;
	move[1] = sin(yaw) * dist;
	move[2] = 0;

	VectorCopy(ent->s.origin, oldorigin);

	if (SV_movestep(ent, move, false))
	{
		delta = ent->s.angles[YAW] - ent->ideal_yaw;

		if ((delta > 45) && (delta < 315))
		{
			/* not turned far enough, so don't take the step */
			VectorCopy(oldorigin, ent->s.origin);
		}

		gi.linkentity(ent);
		G_TouchTriggers(ent);
		return true;
	}

	gi.linkentity(ent);
	G_TouchTriggers(ent);
	return false;
}

void SV_FixCheckBottom(edict_t *ent)
{
	ent->flags |= FL_PARTIALGROUND;
}

qboolean
SV_CloseEnough(edict_t *ent, edict_t *goal, float dist)
{
	int i;

	for (i = 0; i < 3; i++)
	{
		if (goal->absmin[i] > ent->absmax[i] + dist)
		{
			return false;
		}

		if (goal->absmax[i] < ent->absmin[i] - dist)
		{
			return false;
		}
	}

	return true;
}

qboolean
M_walkmove(edict_t *ent, float yaw, float dist)
{
	vec3_t move;

	if (!ent->groundentity && !(ent->flags & (FL_FLY | FL_SWIM)))
	{
		return false;
	}

	yaw = yaw * M_PI * 2 / 360;

	move[0] = cos(yaw) * dist;
	move[1] = sin(yaw) * dist;
	move[2] = 0;

	return SV_movestep(ent, move, true);
}
