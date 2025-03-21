#include "header/local.h"

void InitTrigger(edict_t *self)
{
	if (!VectorCompare(self->s.angles, vec3_origin))
	{
		G_SetMovedir(self->s.angles, self->movedir);
	}

	self->solid = SOLID_TRIGGER;
	self->movetype = MOVETYPE_NONE;
	gi.setmodel(self, self->model);
	self->svflags = SVF_NOCLIENT;
}

void trigger_gravity_touch(edict_t *self, edict_t *other, cplane_t *plane,
						   csurface_t *surf)
{
	other->gravity = self->gravity;

	if (self->client_local_gravity != 0)
	{
		other->client_local_gravity = self->client_local_gravity;
	}
}

void SP_trigger_gravity(edict_t *self)
{
	if (st.gravity == 0)
	{
		gi.dprintf("trigger_gravity without gravity set at %s\n", vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}

	if (!st.client_local_gravity == 0)
	{
		self->client_local_gravity = atoi(st.client_local_gravity);
	}

	InitTrigger(self);
	self->gravity = atoi(st.gravity);
	self->touch = trigger_gravity_touch;
}
