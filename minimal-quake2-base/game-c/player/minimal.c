#include "../header/local.h"

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

void PlayerNoise(edict_t *who, vec3_t where, int type)
{
    edict_t *noise;

    if (deathmatch->value)
    {
        return;
    }

    if (who->flags & FL_NOTARGET)
    {
        return;
    }

    if (!who->mynoise)
    {
        noise = G_Spawn();
        noise->classname = "player_noise";
        VectorSet(noise->mins, -8, -8, -8);
        VectorSet(noise->maxs, 8, 8, 8);
        noise->owner = who;
        noise->svflags = SVF_NOCLIENT;
        who->mynoise = noise;

        noise = G_Spawn();
        noise->classname = "player_noise";
        VectorSet(noise->mins, -8, -8, -8);
        VectorSet(noise->maxs, 8, 8, 8);
        noise->owner = who;
        noise->svflags = SVF_NOCLIENT;
        who->mynoise2 = noise;
    }

    if (type == PNOISE_SELF)
    {
        noise = who->mynoise;
        level.sound_entity = noise;
        level.sound_entity_framenum = level.framenum;
    }
    else
    {
        noise = who->mynoise2;
        level.sound2_entity = noise;
        level.sound2_entity_framenum = level.framenum;
    }

    VectorCopy(where, noise->s.origin);
    VectorSubtract(where, noise->maxs, noise->absmin);
    VectorAdd(where, noise->maxs, noise->absmax);
    noise->last_sound_time = level.time;
    gi.linkentity(noise);
}
