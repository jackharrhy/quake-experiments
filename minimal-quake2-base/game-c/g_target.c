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
 * Targets.
 *
 * =======================================================================
 */

#include "header/local.h"

/*
 * QUAKED target_temp_entity (1 0 0) (-8 -8 -8) (8 8 8)
 * Fire an origin based temp entity event to the clients.
 * "style"		type byte
 */
void Use_Target_Tent(edict_t *ent, edict_t *other, edict_t *activator)
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(ent->style);
	gi.WritePosition(ent->s.origin);
	gi.multicast(ent->s.origin, MULTICAST_PVS);
}

void SP_target_temp_entity(edict_t *ent)
{
	ent->use = Use_Target_Tent;
}

/* ========================================================== */

/*
 * QUAKED target_speaker (1 0 0) (-8 -8 -8) (8 8 8) looped-on looped-off reliable
 * "noise" wav file to play
 *
 * "attenuation"
 *  -1 = none, send to whole level
 *   1 = normal fighting sounds
 *   2 = idle sound level
 *   3 = ambient sound level
 *
 * "volume"	0.0 to 1.0
 *
 * Normal sounds play each time the target is used. The reliable flag
 * can be set for crucial voiceovers.
 *
 * Looped sounds are allways atten 3 / vol 1, and the use function toggles
 * it on/off. Multiple identical looping sounds will just increase volume
 * without any speed cost.
 */
void Use_Target_Speaker(edict_t *ent, edict_t *other, edict_t *activator)
{
	int chan;

	if (ent->spawnflags & 3)
	{
		/* looping sound toggles */
		if (ent->s.sound)
		{
			ent->s.sound = 0; /* turn it off */
		}
		else
		{
			ent->s.sound = ent->noise_index; /* start it */
		}
	}
	else
	{
		/* normal sound */
		if (ent->spawnflags & 4)
		{
			chan = CHAN_VOICE | CHAN_RELIABLE;
		}
		else
		{
			chan = CHAN_VOICE;
		}

		/* use a positioned_sound, because this entity won't
		   normally be sent to any clients because it is invisible */
		gi.positioned_sound(ent->s.origin, ent, chan, ent->noise_index,
							ent->volume, ent->attenuation, 0);
	}
}

void SP_target_speaker(edict_t *ent)
{
	char buffer[MAX_QPATH];

	if (!st.noise)
	{
		gi.dprintf("target_speaker with no noise set at %s\n",
				   vtos(ent->s.origin));
		return;
	}

	if (!strstr(st.noise, ".wav"))
	{
		Com_sprintf(buffer, sizeof(buffer), "%s.wav", st.noise);
	}
	else
	{
		Q_strlcpy(buffer, st.noise, sizeof(buffer));
	}

	ent->noise_index = gi.soundindex(buffer);

	if (!ent->volume)
	{
		ent->volume = 1.0;
	}

	if (!ent->attenuation)
	{
		ent->attenuation = 1.0;
	}
	else if (ent->attenuation == -1) /* use -1 so 0 defaults to 1 */
	{
		ent->attenuation = 0;
	}

	/* check for prestarted looping sound */
	if (ent->spawnflags & 1)
	{
		ent->s.sound = ent->noise_index;
	}

	ent->use = Use_Target_Speaker;

	/* must link the entity so we get areas and clusters so
	   the server can determine who to send updates to */
	gi.linkentity(ent);
}

/* ========================================================== */

/*
 * QUAKED target_explosion (1 0 0) (-8 -8 -8) (8 8 8)
 * Spawns an explosion temporary entity when used.
 *
 * "delay"		wait this long before going off
 * "dmg"		how much radius damage should be done, defaults to 0
 */
void target_explosion_explode(edict_t *self)
{
	float save;

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PHS);

	T_RadiusDamage(self, self->activator, self->dmg,
				   NULL, self->dmg + 40, MOD_EXPLOSIVE);

	save = self->delay;
	self->delay = 0;
	G_UseTargets(self, self->activator);
	self->delay = save;
}

void use_target_explosion(edict_t *self, edict_t *other, edict_t *activator)
{
	self->activator = activator;

	if (!self->delay)
	{
		target_explosion_explode(self);
		return;
	}

	self->think = target_explosion_explode;
	self->nextthink = level.time + self->delay;
}

void SP_target_explosion(edict_t *ent)
{
	ent->use = use_target_explosion;
	ent->svflags = SVF_NOCLIENT;
}

/* ========================================================== */

/*
 * QUAKED target_changelevel (1 0 0) (-8 -8 -8) (8 8 8)
 * Changes level to "map" when fired
 */
void use_target_changelevel(edict_t *self, edict_t *other, edict_t *activator)
{
	if (!deathmatch->value)
	{
		if (g_edicts[1].health <= 0)
		{
			return;
		}
	}

	/* if noexit, do a ton of damage to other */
	if (deathmatch->value && !((int)dmflags->value & DF_ALLOW_EXIT) &&
		(other != world))
	{
		T_Damage(other, self, self, vec3_origin, other->s.origin,
				 vec3_origin, 10 * other->max_health, 1000, 0,
				 MOD_EXIT);
		return;
	}

	/* if multiplayer, let everyone know who hit the exit */
	if (deathmatch->value)
	{
		if (activator && activator->client)
		{
			gi.bprintf(PRINT_HIGH, "%s exited the level.\n",
					   activator->client->pers.netname);
		}
	}
}

void SP_target_changelevel(edict_t *ent)
{
	if (!ent->map)
	{
		gi.dprintf("target_changelevel with no map at %s\n", vtos(ent->s.origin));
		G_FreeEdict(ent);
		return;
	}

	/* ugly hack because *SOMEBODY* screwed up their map */
	if ((Q_stricmp(level.mapname, "fact1") == 0) && (Q_stricmp(ent->map, "fact3") == 0))
	{
		ent->map = "fact3$secret1";
	}

	ent->use = use_target_changelevel;
	ent->svflags = SVF_NOCLIENT;
}

/* ========================================================== */

/*
 * QUAKED target_splash (1 0 0) (-8 -8 -8) (8 8 8)
 * Creates a particle splash effect when used.
 *
 * Set "sounds" to one of the following:
 * 1) sparks
 * 2) blue water
 * 3) brown water
 * 4) slime
 * 5) lava
 * 6) blood
 *
 * "count"	how many pixels in the splash
 * "dmg"	if set, does a radius damage at this location when it splashes
 *          useful for lava/sparks
 */
void use_target_splash(edict_t *self, edict_t *other, edict_t *activator)
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_SPLASH);
	gi.WriteByte(self->count);
	gi.WritePosition(self->s.origin);
	gi.WriteDir(self->movedir);
	gi.WriteByte(self->sounds);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	if (self->dmg)
	{
		T_RadiusDamage(self, activator, self->dmg, NULL,
					   self->dmg + 40, MOD_SPLASH);
	}
}

void SP_target_splash(edict_t *self)
{
	self->use = use_target_splash;
	G_SetMovedir(self->s.angles, self->movedir);

	if (!self->count)
	{
		self->count = 32;
	}

	self->svflags = SVF_NOCLIENT;
}

/* ========================================================== */

/*
 * QUAKED target_spawner (1 0 0) (-8 -8 -8) (8 8 8)
 * Set target to the type of entity you want spawned.
 * Useful for spawning monsters and gibs in the factory levels.
 *
 * For monsters:
 *  Set direction to the facing you want it to have.
 *
 * For gibs:
 *  Set direction if you want it moving and
 *  speed how fast it should be moving otherwise it
 *  will just be dropped
 */
void ED_CallSpawn(edict_t *ent);

void use_target_spawner(edict_t *self, edict_t *other, edict_t *activator)
{
	edict_t *ent;

	ent = G_Spawn();
	ent->classname = self->target;
	VectorCopy(self->s.origin, ent->s.origin);
	VectorCopy(self->s.angles, ent->s.angles);
	ED_CallSpawn(ent);
	gi.unlinkentity(ent);
	KillBox(ent);
	gi.linkentity(ent);

	if (self->speed)
	{
		VectorCopy(self->movedir, ent->velocity);
	}
}

void SP_target_spawner(edict_t *self)
{
	self->use = use_target_spawner;
	self->svflags = SVF_NOCLIENT;

	if (self->speed)
	{
		G_SetMovedir(self->s.angles, self->movedir);
		VectorScale(self->movedir, self->speed, self->movedir);
	}
}

/* ========================================================== */

/*
 * QUAKED target_laser (0 .5 .8) (-8 -8 -8) (8 8 8) START_ON RED GREEN BLUE YELLOW ORANGE FAT
 * When triggered, fires a laser.  You can either set a target
 * or a direction.
 */

void target_laser_think(edict_t *self)
{
	edict_t *ignore;
	vec3_t start;
	vec3_t end;
	trace_t tr;
	vec3_t point;
	vec3_t last_movedir;

	if (self->enemy)
	{
		VectorCopy(self->movedir, last_movedir);
		VectorMA(self->enemy->absmin, 0.5, self->enemy->size, point);
		VectorSubtract(point, self->s.origin, self->movedir);
		VectorNormalize(self->movedir);

		if (!VectorCompare(self->movedir, last_movedir))
		{
			self->spawnflags |= 0x80000000;
		}
	}

	ignore = self;
	VectorCopy(self->s.origin, start);
	VectorMA(start, 2048, self->movedir, end);

	while (1)
	{
		tr = gi.trace(start, NULL, NULL, end, ignore,
					  CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_DEADMONSTER);

		if (!tr.ent)
		{
			break;
		}

		/* hurt it if we can */
		if ((tr.ent->takedamage) && !(tr.ent->flags & FL_IMMUNE_LASER))
		{
			T_Damage(tr.ent, self, self->activator, self->movedir, tr.endpos,
					 vec3_origin, self->dmg, 1, DAMAGE_ENERGY, MOD_TARGET_LASER);
		}

		ignore = tr.ent;
		VectorCopy(tr.endpos, start);
	}

	VectorCopy(tr.endpos, self->s.old_origin);

	self->nextthink = level.time + FRAMETIME;
}

void target_laser_on(edict_t *self)
{
	if (!self->activator)
	{
		self->activator = self;
	}

	self->spawnflags |= 0x80000001;
	self->svflags &= ~SVF_NOCLIENT;
	target_laser_think(self);
}

void target_laser_off(edict_t *self)
{
	self->spawnflags &= ~1;
	self->svflags |= SVF_NOCLIENT;
	self->nextthink = 0;
}

void target_laser_use(edict_t *self, edict_t *other, edict_t *activator)
{
	self->activator = activator;

	if (self->spawnflags & 1)
	{
		target_laser_off(self);
	}
	else
	{
		target_laser_on(self);
	}
}

void target_laser_start(edict_t *self)
{
	edict_t *ent;

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	self->s.renderfx |= RF_BEAM | RF_TRANSLUCENT;
	self->s.modelindex = 1; /* must be non-zero */

	/* set the beam diameter */
	if (self->spawnflags & 64)
	{
		self->s.frame = 16;
	}
	else
	{
		self->s.frame = 4;
	}

	/* set the color */
	if (self->spawnflags & 2)
	{
		self->s.skinnum = 0xf2f2f0f0;
	}
	else if (self->spawnflags & 4)
	{
		self->s.skinnum = 0xd0d1d2d3;
	}
	else if (self->spawnflags & 8)
	{
		self->s.skinnum = 0xf3f3f1f1;
	}
	else if (self->spawnflags & 16)
	{
		self->s.skinnum = 0xdcdddedf;
	}
	else if (self->spawnflags & 32)
	{
		self->s.skinnum = 0xe0e1e2e3;
	}

	if (!self->enemy)
	{
		if (self->target)
		{
			ent = G_Find(NULL, FOFS(targetname), self->target);

			if (!ent)
			{
				gi.dprintf("%s at %s: %s is a bad target\n", self->classname,
						   vtos(self->s.origin), self->target);
			}

			self->enemy = ent;
		}
		else
		{
			G_SetMovedir(self->s.angles, self->movedir);
		}
	}

	self->use = target_laser_use;
	self->think = target_laser_think;

	if (!self->dmg)
	{
		self->dmg = 1;
	}

	VectorSet(self->mins, -8, -8, -8);
	VectorSet(self->maxs, 8, 8, 8);
	gi.linkentity(self);

	if (self->spawnflags & 1)
	{
		target_laser_on(self);
	}
	else
	{
		target_laser_off(self);
	}
}

void SP_target_laser(edict_t *self)
{
	/* let everything else get spawned before we start firing */
	self->think = target_laser_start;
	self->nextthink = level.time + 1;
}

/* ========================================================== */

/*
 * QUAKED target_lightramp (0 .5 .8) (-8 -8 -8) (8 8 8) TOGGLE
 * speed		How many seconds the ramping will take
 * message		two letters; starting lightlevel and ending lightlevel
 */
void target_lightramp_think(edict_t *self)
{
	char style[2];

	style[0] = 'a' + self->movedir[0] + (level.time - self->timestamp) / FRAMETIME * self->movedir[2];
	style[1] = 0;
	gi.configstring(CS_LIGHTS + self->enemy->style, style);

	if ((level.time - self->timestamp) < self->speed)
	{
		self->nextthink = level.time + FRAMETIME;
	}
	else if (self->spawnflags & 1)
	{
		char temp;

		temp = self->movedir[0];
		self->movedir[0] = self->movedir[1];
		self->movedir[1] = temp;
		self->movedir[2] *= -1;
	}
}

void target_lightramp_use(edict_t *self, edict_t *other, edict_t *activator)
{
	if (!self->enemy)
	{
		edict_t *e;

		/* check all the targets */
		e = NULL;

		while (1)
		{
			e = G_Find(e, FOFS(targetname), self->target);

			if (!e)
			{
				break;
			}

			if (strcmp(e->classname, "light") != 0)
			{
				gi.dprintf("%s at %s ", self->classname, vtos(self->s.origin));
				gi.dprintf("target %s (%s at %s) is not a light\n",
						   self->target, e->classname, vtos(e->s.origin));
			}
			else
			{
				self->enemy = e;
			}
		}

		if (!self->enemy)
		{
			gi.dprintf("%s target %s not found at %s\n",
					   self->classname, self->target, vtos(self->s.origin));
			G_FreeEdict(self);
			return;
		}
	}

	self->timestamp = level.time;
	target_lightramp_think(self);
}

void SP_target_lightramp(edict_t *self)
{
	if (!self->message || (strlen(self->message) != 2) ||
		(self->message[0] < 'a') || (self->message[0] > 'z') ||
		(self->message[1] < 'a') || (self->message[1] > 'z') ||
		(self->message[0] == self->message[1]))
	{
		gi.dprintf("target_lightramp has bad ramp (%s) at %s\n",
				   self->message, vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}

	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	if (!self->target)
	{
		gi.dprintf("%s with no target at %s\n", self->classname,
				   vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}

	self->svflags |= SVF_NOCLIENT;
	self->use = target_lightramp_use;
	self->think = target_lightramp_think;

	self->movedir[0] = self->message[0] - 'a';
	self->movedir[1] = self->message[1] - 'a';
	self->movedir[2] = (self->movedir[1] - self->movedir[0]) / (self->speed / FRAMETIME);
}

/* ========================================================== */

/* QUAKED target_earthquake (1 0 0) (-8 -8 -8) (8 8 8)
 * When triggered, this initiates a level-wide earthquake.
 * All players and monsters are affected.
 * "speed"		severity of the quake (default:200)
 * "count"		duration of the quake (default:5)
 */
void target_earthquake_think(edict_t *self)
{
	int i;
	edict_t *e;

	if (self->last_move_time < level.time)
	{
		gi.positioned_sound(self->s.origin, self, CHAN_AUTO,
							self->noise_index, 1.0, ATTN_NONE, 0);
		self->last_move_time = level.time + 0.5;
	}

	for (i = 1, e = g_edicts + i; i < globals.num_edicts; i++, e++)
	{
		if (!e->inuse)
		{
			continue;
		}

		if (!e->client)
		{
			continue;
		}

		if (!e->groundentity)
		{
			continue;
		}

		e->groundentity = NULL;
		e->velocity[0] += crandom() * 150;
		e->velocity[1] += crandom() * 150;
		e->velocity[2] = self->speed * (100.0 / e->mass);
	}

	if (level.time < self->timestamp)
	{
		self->nextthink = level.time + FRAMETIME;
	}
}

void target_earthquake_use(edict_t *self, edict_t *other, edict_t *activator)
{
	self->timestamp = level.time + self->count;
	self->nextthink = level.time + FRAMETIME;
	self->activator = activator;
	self->last_move_time = 0;
}

void SP_target_earthquake(edict_t *self)
{
	if (!self->targetname)
	{
		gi.dprintf("untargeted %s at %s\n", self->classname,
				   vtos(self->s.origin));
	}

	if (!self->count)
	{
		self->count = 5;
	}

	if (!self->speed)
	{
		self->speed = 200;
	}

	self->svflags |= SVF_NOCLIENT;
	self->think = target_earthquake_think;
	self->use = target_earthquake_use;

	self->noise_index = gi.soundindex("world/quake.wav");
}
