#include "header/local.h"

void Use_Areaportal(edict_t *ent, edict_t *other, edict_t *activator)
{
	ent->count ^= 1;
	gi.SetAreaPortalState(ent->style, ent->count);
}

void SP_func_areaportal(edict_t *ent)
{
	ent->use = Use_Areaportal;
	ent->count = 0;
}

#define START_OFF 1

static void
light_use(edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->spawnflags & START_OFF)
	{
		gi.configstring(CS_LIGHTS + self->style, "m");
		self->spawnflags &= ~START_OFF;
	}
	else
	{
		gi.configstring(CS_LIGHTS + self->style, "a");
		self->spawnflags |= START_OFF;
	}
}

void SP_light(edict_t *self)
{
	/* no targeted lights in deathmatch, because they cause global messages */
	if (!self->targetname || deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	if (self->style >= 32)
	{
		self->use = light_use;

		if (self->spawnflags & START_OFF)
		{
			gi.configstring(CS_LIGHTS + self->style, "a");
		}
		else
		{
			gi.configstring(CS_LIGHTS + self->style, "m");
		}
	}
}
