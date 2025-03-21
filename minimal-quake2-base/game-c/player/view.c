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
 * The "camera" through which the player looks into the game.
 *
 * =======================================================================
 */

#include "../header/local.h"
#include "../monster/player.h"

static edict_t *current_player;
static gclient_t *current_client;

static vec3_t forward, right, up;
float xyspeed;

float bobmove;
int bobcycle;	  /* odd cycles are right foot going forward */
float bobfracsin; /* sin(bobfrac*M_PI) */

float SV_CalcRoll(vec3_t angles, vec3_t velocity)
{
	float sign;
	float side;
	float value;

	side = DotProduct(velocity, right);
	sign = side < 0 ? -1 : 1;
	side = fabs(side);

	value = sv_rollangle->value;

	if (side < sv_rollspeed->value)
	{
		side = side * value / sv_rollspeed->value;
	}
	else
	{
		side = value;
	}

	return side * sign;
}

/*
 * Auto pitching on slopes?
 *
 * fall from 128: 400 = 160000
 * fall from 256: 580 = 336400
 * fall from 384: 720 = 518400
 * fall from 512: 800 = 640000
 * fall from 640: 960 =
 *
 * damage = deltavelocity*deltavelocity  * 0.0001
 */
void SV_CalcViewOffset(edict_t *ent)
{
	float *angles;
	float bob;
	float ratio;
	float delta;
	vec3_t v;

	/* =================================== */

	/* base angles */
	angles = ent->client->ps.kick_angles;

	/* add angles based on weapon kick */
	VectorCopy(ent->client->kick_angles, angles);

	/* add angles based on damage kick */
	ratio = (ent->client->v_dmg_time - level.time) / DAMAGE_TIME;

	if (ratio < 0)
	{
		ratio = 0;
		ent->client->v_dmg_pitch = 0;
		ent->client->v_dmg_roll = 0;
	}

	angles[PITCH] += ratio * ent->client->v_dmg_pitch;
	angles[ROLL] += ratio * ent->client->v_dmg_roll;

	/* add pitch based on fall kick */
	ratio = (ent->client->fall_time - level.time) / FALL_TIME;

	if (ratio < 0)
	{
		ratio = 0;
	}

	angles[PITCH] += ratio * ent->client->fall_value;

	/* add angles based on velocity */
	delta = DotProduct(ent->velocity, forward);
	angles[PITCH] += delta * run_pitch->value;

	delta = DotProduct(ent->velocity, right);
	angles[ROLL] += delta * run_roll->value;

	/* add angles based on bob */
	delta = bobfracsin * bob_pitch->value * xyspeed;

	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		delta *= 6; /* crouching */
	}

	angles[PITCH] += delta;
	delta = bobfracsin * bob_roll->value * xyspeed;

	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		delta *= 6; /* crouching */
	}

	if (bobcycle & 1)
	{
		delta = -delta;
	}

	angles[ROLL] += delta;

	/* base origin */
	VectorClear(v);

	/* add view height */
	v[2] += ent->viewheight;

	/* add fall height */
	ratio = (ent->client->fall_time - level.time) / FALL_TIME;

	if (ratio < 0)
	{
		ratio = 0;
	}

	v[2] -= ratio * ent->client->fall_value * 0.4;

	/* add bob height */
	bob = bobfracsin * xyspeed * bob_up->value;

	if (bob > 6)
	{
		bob = 6;
	}

	v[2] += bob;

	/* add kick offset */
	VectorAdd(v, ent->client->kick_origin, v);

	/* absolutely bound offsets so the view
	   can never be outside the player box */
	if (v[0] < -14)
	{
		v[0] = -14;
	}
	else if (v[0] > 14)
	{
		v[0] = 14;
	}

	if (v[1] < -14)
	{
		v[1] = -14;
	}
	else if (v[1] > 14)
	{
		v[1] = 14;
	}

	if (v[2] < -22)
	{
		v[2] = -22;
	}
	else if (v[2] > 30)
	{
		v[2] = 30;
	}

	VectorCopy(v, ent->client->ps.viewoffset);
}

void SV_AddBlend(float r, float g, float b, float a, float *v_blend)
{
	float a2, a3;

	if (a <= 0)
	{
		return;
	}

	a2 = v_blend[3] + (1 - v_blend[3]) * a; /* new total alpha */
	a3 = v_blend[3] / a2;					/* fraction of color from old */

	v_blend[0] = v_blend[0] * a3 + r * (1 - a3);
	v_blend[1] = v_blend[1] * a3 + g * (1 - a3);
	v_blend[2] = v_blend[2] * a3 + b * (1 - a3);
	v_blend[3] = a2;
}

void SV_CalcBlend(edict_t *ent)
{
	int contents;
	vec3_t vieworg;

	ent->client->ps.blend[0] = ent->client->ps.blend[1] =
		ent->client->ps.blend[2] =
			ent->client->ps.blend[3] = 0;

	/* add for contents */
	VectorAdd(ent->s.origin, ent->client->ps.viewoffset, vieworg);
	contents = gi.pointcontents(vieworg);

	if (contents & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER))
	{
		ent->client->ps.rdflags |= RDF_UNDERWATER;
	}
	else
	{
		ent->client->ps.rdflags &= ~RDF_UNDERWATER;
	}

	if (contents & (CONTENTS_SOLID | CONTENTS_LAVA))
	{
		SV_AddBlend(1.0, 0.3, 0.0, 0.6, ent->client->ps.blend);
	}
	else if (contents & CONTENTS_SLIME)
	{
		SV_AddBlend(0.0, 0.1, 0.05, 0.6, ent->client->ps.blend);
	}
	else if (contents & CONTENTS_WATER)
	{
		SV_AddBlend(0.5, 0.3, 0.2, 0.4, ent->client->ps.blend);
	}

	/* add for damage */
	if (ent->client->damage_alpha > 0)
	{
		SV_AddBlend(ent->client->damage_blend[0],
					ent->client->damage_blend[1],
					ent->client->damage_blend[2],
					ent->client->damage_alpha,
					ent->client->ps.blend);
	}

	if (ent->client->bonus_alpha > 0)
	{
		SV_AddBlend(0.85, 0.7, 0.3, ent->client->bonus_alpha,
					ent->client->ps.blend);
	}

	/* drop the damage value */
	ent->client->damage_alpha -= 0.06;

	if (ent->client->damage_alpha < 0)
	{
		ent->client->damage_alpha = 0;
	}

	/* drop the bonus value */
	ent->client->bonus_alpha -= 0.1;

	if (ent->client->bonus_alpha < 0)
	{
		ent->client->bonus_alpha = 0;
	}
}

void G_SetClientEvent(edict_t *ent)
{
	if (ent->s.event)
	{
		return;
	}

	if (ent->groundentity && (xyspeed > 225))
	{
		if ((int)(current_client->bobtime + bobmove) != bobcycle)
		{
			ent->s.event = EV_FOOTSTEP;
		}
	}
}

void G_SetClientSound(edict_t *ent)
{
	if (ent->waterlevel && (ent->watertype & (CONTENTS_LAVA | CONTENTS_SLIME)))
	{
		ent->s.sound = snd_fry;
	}
	else
	{
		ent->s.sound = 0;
	}
}

void G_SetClientFrame(edict_t *ent)
{
	gclient_t *client;
	qboolean duck, run;

	if (ent->s.modelindex != 255)
	{
		return; /* not in the player model */
	}

	client = ent->client;

	if (client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		duck = true;
	}
	else
	{
		duck = false;
	}

	if (xyspeed)
	{
		run = true;
	}
	else
	{
		run = false;
	}

	/* check for stand/duck and stop/go transitions */
	if ((duck != client->anim_duck) && (client->anim_priority < ANIM_DEATH))
	{
		goto newanim;
	}

	if ((run != client->anim_run) && (client->anim_priority == ANIM_BASIC))
	{
		goto newanim;
	}

	if (!ent->groundentity && (client->anim_priority <= ANIM_WAVE))
	{
		goto newanim;
	}

	if (client->anim_priority == ANIM_REVERSE)
	{
		if (ent->s.frame > client->anim_end)
		{
			ent->s.frame--;
			return;
		}
	}
	else if (ent->s.frame < client->anim_end)
	{
		/* continue an animation */
		ent->s.frame++;
		return;
	}

	if (client->anim_priority == ANIM_DEATH)
	{
		return; /* stay there */
	}

	if (client->anim_priority == ANIM_JUMP)
	{
		if (!ent->groundentity)
		{
			return; /* stay there */
		}

		ent->client->anim_priority = ANIM_WAVE;
		ent->s.frame = FRAME_jump3;
		ent->client->anim_end = FRAME_jump6;
		return;
	}

newanim:

	/* return to either a running or standing frame */
	client->anim_priority = ANIM_BASIC;
	client->anim_duck = duck;
	client->anim_run = run;

	if (!ent->groundentity)
	{
		client->anim_priority = ANIM_JUMP;

		if (ent->s.frame != FRAME_jump2)
		{
			ent->s.frame = FRAME_jump1;
		}

		client->anim_end = FRAME_jump2;
	}
	else if (run)
	{
		/* running */
		if (duck)
		{
			ent->s.frame = FRAME_crwalk1;
			client->anim_end = FRAME_crwalk6;
		}
		else
		{
			ent->s.frame = FRAME_run1;
			client->anim_end = FRAME_run6;
		}
	}
	else
	{
		/* standing */
		if (duck)
		{
			ent->s.frame = FRAME_crstnd01;
			client->anim_end = FRAME_crstnd19;
		}
		else
		{
			ent->s.frame = FRAME_stand01;
			client->anim_end = FRAME_stand40;
		}
	}
}

/*
 * Called for each player at the end of the server frame
 * and right after spawning
 */
void ClientEndServerFrame(edict_t *ent)
{
	float bobtime;
	int i;

	current_player = ent;
	current_client = ent->client;

	/* If the origin or velocity have changed since ClientThink(),
	   update the pmove values.  This will happen when the client
	   is pushed by a bmodel or kicked by an explosion.

	   If it wasn't updated here, the view position would lag a frame
	   behind the body position when pushed -- "sinking into plats" */
	for (i = 0; i < 3; i++)
	{
		current_client->ps.pmove.origin[i] = ent->s.origin[i] * 8.0;
		current_client->ps.pmove.velocity[i] = ent->velocity[i] * 8.0;
	}

	AngleVectors(ent->client->v_angle, forward, right, up);

	/* set model angles from view angles so other things in
	   the world can tell which direction you are looking */
	if (ent->client->v_angle[PITCH] > 180)
	{
		ent->s.angles[PITCH] = (-360 + ent->client->v_angle[PITCH]) / 3;
	}
	else
	{
		ent->s.angles[PITCH] = ent->client->v_angle[PITCH] / 3;
	}

	ent->s.angles[YAW] = ent->client->v_angle[YAW];
	ent->s.angles[ROLL] = 0;
	ent->s.angles[ROLL] = SV_CalcRoll(ent->s.angles, ent->velocity) * 4;

	/* calculate speed and cycle to be used for
	   all cyclic walking effects */
	xyspeed = sqrt(ent->velocity[0] * ent->velocity[0] + ent->velocity[1] *
															 ent->velocity[1]);

	if (xyspeed < 5)
	{
		bobmove = 0;
		current_client->bobtime = 0; /* start at beginning of cycle again */
	}
	else if (ent->groundentity)
	{
		/* so bobbing only cycles when on ground */
		if (xyspeed > 210)
		{
			bobmove = 0.25;
		}
		else if (xyspeed > 100)
		{
			bobmove = 0.125;
		}
		else
		{
			bobmove = 0.0625;
		}
	}

	bobtime = (current_client->bobtime += bobmove);

	if (current_client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		bobtime *= 4;
	}

	bobcycle = (int)bobtime;
	bobfracsin = fabs(sin(bobtime * M_PI));

	/* determine the view offsets */
	SV_CalcViewOffset(ent);

	/* determine the full screen color blend
	   must be after viewoffset, so eye contents can be
	   accurately determined */
	SV_CalcBlend(ent);

	G_SetClientEvent(ent);

	G_SetClientSound(ent);

	G_SetClientFrame(ent);

	VectorCopy(ent->velocity, ent->client->oldvelocity);
	VectorCopy(ent->client->ps.viewangles, ent->client->oldviewangles);

	/* clear weapon kicks */
	VectorClear(ent->client->kick_origin);
	VectorClear(ent->client->kick_angles);
}
