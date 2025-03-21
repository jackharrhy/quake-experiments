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
 * Main header file for the game module.
 *
 * =======================================================================
 */

#ifndef MQ2B_LOCAL_H
#define MQ2B_LOCAL_H

#include "shared.h"

/* define GAME_INCLUDE so that game.h does not define the
   short, server-visible gclient_t and edict_t structures,
   because we define the full size ones in this file */
#define GAME_INCLUDE
#include "game.h"
#include "menu.h"

/* the "gameversion" client command will print this plus compile date */
#define GAMEVERSION "mq2b"

/* protocol bytes that can be directly added to messages */
#define svc_muzzleflash 1
#define svc_muzzleflash2 2
#define svc_temp_entity 3
#define svc_layout 4
#define svc_inventory 5

/* ================================================================== */

/* view pitching times */
#define DAMAGE_TIME 0.5
#define FALL_TIME 0.3

/* edict->spawnflags */
#define SPAWNFLAG_NOT_DEATHMATCH 0x00000800

/* edict->flags */
#define FL_FLY 0x00000001
#define FL_SWIM 0x00000002 /* implied immunity to drowining */
#define FL_IMMUNE_LASER 0x00000004
#define FL_INWATER 0x00000008
#define FL_GODMODE 0x00000010
#define FL_NOTARGET 0x00000020
#define FL_IMMUNE_SLIME 0x00000040
#define FL_IMMUNE_LAVA 0x00000080
#define FL_PARTIALGROUND 0x00000100 /* not all corners are valid */
#define FL_WATERJUMP 0x00000200		/* player jumping out of water */
#define FL_NO_KNOCKBACK 0x00000800
#define FL_RESPAWN 0x80000000 /* used for item respawning */

#define FRAMETIME 0.1

/* memory tags to allow dynamic memory to be cleaned up */
#define TAG_GAME 765  /* clear when unloading the dll */
#define TAG_LEVEL 766 /* clear when loading a new level */

#define MELEE_DISTANCE 80

#define BODY_QUEUE_SIZE 8

/* range */
#define RANGE_MELEE 0
#define RANGE_NEAR 1
#define RANGE_MID 2
#define RANGE_FAR 3

/* gib types */
#define GIB_ORGANIC 0
#define GIB_METALLIC 1

/* monster ai flags */
#define AI_STAND_GROUND 0x00000001
#define AI_TEMP_STAND_GROUND 0x00000002
#define AI_SOUND_TARGET 0x00000004
#define AI_LOST_SIGHT 0x00000008
#define AI_PURSUIT_LAST_SEEN 0x00000010
#define AI_PURSUE_NEXT 0x00000020
#define AI_PURSUE_TEMP 0x00000040
#define AI_HOLD_FRAME 0x00000080
#define AI_GOOD_GUY 0x00000100
#define AI_BRUTAL 0x00000200
#define AI_NOSTEP 0x00000400
#define AI_DUCKED 0x00000800
#define AI_COMBAT_POINT 0x00001000
#define AI_MEDIC 0x00002000
#define AI_RESURRECTING 0x00004000

/* monster attack state */
#define AS_STRAIGHT 1
#define AS_SLIDING 2
#define AS_MELEE 3
#define AS_MISSILE 4

/* armor types */
#define ARMOR_NONE 0
#define ARMOR_JACKET 1
#define ARMOR_COMBAT 2
#define ARMOR_BODY 3
#define ARMOR_SHARD 4

/* handedness values */
#define RIGHT_HANDED 0
#define LEFT_HANDED 1
#define CENTER_HANDED 2

/* noise types for PlayerNoise */
#define PNOISE_SELF 0
#define PNOISE_IMPACT 2

/* edict->movetype values */
typedef enum
{
	MOVETYPE_NONE,	 /* never moves */
	MOVETYPE_NOCLIP, /* origin and angles change with no interaction */
	MOVETYPE_WALK,	 /* gravity */
	MOVETYPE_STEP,	 /* gravity, special edge handling */
	MOVETYPE_TOSS,	 /* gravity */
} movetype_t;

/* this structure is left intact through an entire game
   it should be initialized at dll load time, and read/written to
   the server.ssv file for savegames */
typedef struct
{
	gclient_t *clients; /* [maxclients] */

	char spawnpoint[512]; /* needed for coop respawns */

	/* store latched cvars here that we want to get at often */
	int maxclients;
	int maxentities;
} game_locals_t;

/* this structure is cleared as each map is entered
   it is read/written to the level.sav file for savegames */
typedef struct
{
	int framenum;
	float time;

	char level_name[MAX_QPATH]; /* the descriptive name (Outer Base, etc) */
	char mapname[MAX_QPATH];	/* the server name (base1, etc) */

	edict_t *sight_entity;
	int sight_entity_framenum;
	edict_t *sound_entity;
	int sound_entity_framenum;
	edict_t *sound2_entity;
	int sound2_entity_framenum;

	edict_t *current_entity; /* entity running from G_RunFrame */
} level_locals_t;

/* spawn_temp_t is only used to hold entity field values that
   can be set from the editor, but aren't actualy present
   in edict_t during gameplay */
typedef struct
{
	/* world vars */
	char *sky;
	float skyrotate;
	vec3_t skyaxis;
	char *nextmap;

	int lip;
	int distance;
	int height;
	char *noise;
	float pausetime;
	char *item;
	char *gravity;
	char *client_local_gravity;

	float minyaw;
	float maxyaw;
	float minpitch;
	float maxpitch;
} spawn_temp_t;

typedef struct
{
	/* fixed data */
	vec3_t start_origin;
	vec3_t start_angles;
	vec3_t end_origin;
	vec3_t end_angles;

	int sound_start;
	int sound_middle;
	int sound_end;

	float accel;
	float speed;
	float decel;
	float distance;

	float wait;

	/* state data */
	int state;
	vec3_t dir;
	float current_speed;
	float move_speed;
	float next_speed;
	float remaining_distance;
	float decel_distance;
	void (*endfunc)(edict_t *);
} moveinfo_t;

typedef struct
{
	void (*aifunc)(edict_t *self, float dist);
	float dist;
	void (*thinkfunc)(edict_t *self);
} mframe_t;

typedef struct
{
	int firstframe;
	int lastframe;
	mframe_t *frame;
	void (*endfunc)(edict_t *self);
} mmove_t;

extern game_locals_t game;
extern level_locals_t level;
extern game_import_t gi;
extern game_export_t globals;
extern spawn_temp_t st;

extern int sm_meat_index;
extern int snd_fry;

/* means of death */
#define MOD_UNKNOWN 0
#define MOD_BLASTER 1
#define MOD_SHOTGUN 2
#define MOD_SSHOTGUN 3
#define MOD_MACHINEGUN 4
#define MOD_CHAINGUN 5
#define MOD_GRENADE 6
#define MOD_G_SPLASH 7
#define MOD_ROCKET 8
#define MOD_R_SPLASH 9
#define MOD_HYPERBLASTER 10
#define MOD_RAILGUN 11
#define MOD_BFG_LASER 12
#define MOD_BFG_BLAST 13
#define MOD_BFG_EFFECT 14
#define MOD_HANDGRENADE 15
#define MOD_HG_SPLASH 16
#define MOD_WATER 17
#define MOD_SLIME 18
#define MOD_LAVA 19
#define MOD_CRUSH 20
#define MOD_TELEFRAG 21
#define MOD_FALLING 22
#define MOD_SUICIDE 23
#define MOD_HELD_GRENADE 24
#define MOD_EXPLOSIVE 25
#define MOD_BARREL 26
#define MOD_BOMB 27
#define MOD_EXIT 28
#define MOD_SPLASH 29
#define MOD_TARGET_LASER 30
#define MOD_TRIGGER_HURT 31
#define MOD_HIT 32
#define MOD_TARGET_BLASTER 33
#define MOD_FRIENDLY_FIRE 0x8000000

extern int meansOfDeath;

extern edict_t *g_edicts;

#define FOFS(x) (size_t)&(((edict_t *)0)->x)
#define STOFS(x) (size_t)&(((spawn_temp_t *)0)->x)
#define LLOFS(x) (size_t)&(((level_locals_t *)0)->x)
#define CLOFS(x) (size_t)&(((gclient_t *)0)->x)

#define random() ((rand() & 0x7fff) / ((float)0x7fff))
#define crandom() (2.0 * (random() - 0.5))

extern cvar_t *maxentities;
extern cvar_t *deathmatch;
extern cvar_t *password;
extern cvar_t *g_select_empty;
extern cvar_t *dedicated;

extern cvar_t *sv_gravity;
extern cvar_t *sv_maxvelocity;

extern cvar_t *gun_x, *gun_y, *gun_z;
extern cvar_t *sv_rollspeed;
extern cvar_t *sv_rollangle;

extern cvar_t *run_pitch;
extern cvar_t *run_roll;
extern cvar_t *bob_up;
extern cvar_t *bob_pitch;
extern cvar_t *bob_roll;

extern cvar_t *sv_cheats;
extern cvar_t *maxclients;

extern cvar_t *flood_msgs;
extern cvar_t *flood_persecond;
extern cvar_t *flood_waitdelay;

extern cvar_t *sv_maplist;

extern cvar_t *aimfix;

#define world (&g_edicts[0])

/* item spawnflags */
#define ITEM_TRIGGER_SPAWN 0x00000001
#define ITEM_NO_TOUCH 0x00000002
/* 6 bits reserved for editor flags */
#define DROPPED_ITEM 0x00010000
#define DROPPED_PLAYER_ITEM 0x00020000
#define ITEM_TARGETS_USED 0x00040000

/* fields are needed for spawning from the entity string
   and saving / loading games */
#define FFL_SPAWNTEMP 1

typedef enum
{
	F_INT,
	F_FLOAT,
	F_LSTRING, /* string on disk, pointer in memory, TAG_LEVEL */
	F_GSTRING, /* string on disk, pointer in memory, TAG_GAME */
	F_VECTOR,
	F_ANGLEHACK,
	F_EDICT,  /* index on disk, pointer in memory */
	F_CLIENT, /* index on disk, pointer in memory */
	F_IGNORE
} fieldtype_t;

typedef struct
{
	char *name;
	int ofs;
	fieldtype_t type;
	int flags;
} field_t;

extern field_t fields[];

/* g_cmds.c */
qboolean CheckFlood(edict_t *ent);

/* g_utils.c */
qboolean KillBox(edict_t *ent);
void G_ProjectSource(vec3_t point, vec3_t distance, vec3_t forward,
					 vec3_t right, vec3_t result);
edict_t *G_Find(edict_t *from, int fieldofs, char *match);
edict_t *findradius(edict_t *from, vec3_t org, float rad);
edict_t *G_PickTarget(char *targetname);
void G_UseTargets(edict_t *ent, edict_t *activator);
void G_SetMovedir(vec3_t angles, vec3_t movedir);

void G_InitEdict(edict_t *e);
edict_t *G_Spawn(void);
void G_FreeEdict(edict_t *e);

void G_TouchTriggers(edict_t *ent);
void G_TouchSolids(edict_t *ent);

char *G_CopyString(char *in);

float *tv(float x, float y, float z);
char *vtos(vec3_t v);

float vectoyaw(vec3_t vec);
void vectoangles(vec3_t vec, vec3_t angles);

/* damage flags */
#define DAMAGE_RADIUS 0x00000001		/* damage was indirect */
#define DAMAGE_NO_ARMOR 0x00000002		/* armour does not protect from this damage */
#define DAMAGE_ENERGY 0x00000004		/* damage is from an energy based weapon */
#define DAMAGE_NO_KNOCKBACK 0x00000008	/* do not affect velocity, just view angles */
#define DAMAGE_BULLET 0x00000010		/* damage is from a bullet (used for ricochets) */
#define DAMAGE_NO_PROTECTION 0x00000020 /* armor, shields, invulnerability, and godmode have no effect */

#define DEFAULT_BULLET_HSPREAD 300
#define DEFAULT_BULLET_VSPREAD 500
#define DEFAULT_SHOTGUN_HSPREAD 1000
#define DEFAULT_SHOTGUN_VSPREAD 500
#define DEFAULT_DEATHMATCH_SHOTGUN_COUNT 12
#define DEFAULT_SHOTGUN_COUNT 12
#define DEFAULT_SSHOTGUN_COUNT 20

/* g_misc.c */
void ThrowHead(edict_t *self, char *gibname, int damage, int type);
void ThrowClientHead(edict_t *self, int damage);
void ThrowGib(edict_t *self, char *gibname, int damage, int type);
void BecomeExplosion1(edict_t *self);

/* g_client.c */
void respawn(edict_t *ent);
void PutClientInServer(edict_t *ent);
void InitClientPersistant(gclient_t *client);
void InitClientResp(gclient_t *client);
void ClientBeginServerFrame(edict_t *ent);
void ClientUserinfoChanged(edict_t *ent, char *userinfo);

/* g_player.c */
void player_pain(edict_t *self, edict_t *other, float kick, int damage);
void player_die(edict_t *self, edict_t *inflictor, edict_t *attacker,
				int damage, vec3_t point);

/* g_svcmds.c */
void ServerCommand(void);

/* p_view.c */
void ClientEndServerFrame(edict_t *ent);

/* p_hud.c */
void MoveClientToIntermission(edict_t *client);
void G_SetStats(edict_t *ent);
void ValidateSelectedItem(edict_t *ent);

/* p_minimal.c */
void PlayerNoise(edict_t *who, vec3_t where, int type);
void P_ProjectSource(edict_t *ent, vec3_t distance,
					 vec3_t forward, vec3_t right, vec3_t result);

/* m_move.c */
qboolean M_CheckBottom(edict_t *ent);
qboolean M_walkmove(edict_t *ent, float yaw, float dist);
void M_ChangeYaw(edict_t *ent);

/* client_t->anim_priority */
#define ANIM_BASIC 0 /* stand / run */
#define ANIM_WAVE 1
#define ANIM_JUMP 2
#define ANIM_PAIN 3
#define ANIM_ATTACK 4
#define ANIM_DEATH 5
#define ANIM_REVERSE 6

/* client data that stays across multiple level loads */
typedef struct
{
	char userinfo[MAX_INFO_STRING];
	char netname[16];
	int hand;

	qboolean connected; /* a loadgame will leave valid entities that
						   just don't have a connection yet */
} client_persistant_t;

/* client data that stays across deathmatch respawns */
typedef struct
{
	int enterframe; /* level.framenum the client entered the game */
	qboolean id_state;
	float lastidtime;
	qboolean ready;
	struct ghost_s *ghost; /* for ghost codes */
	vec3_t cmd_angles;	   /* angles sent over in the last command */
} client_respawn_t;

/* this structure is cleared on each PutClientInServer(),
   except for 'client->pers' */
struct gclient_s
{
	/* known to server */
	player_state_t ps; /* communicated by server to clients */
	int ping;

	/* private to game */
	client_persistant_t pers;
	client_respawn_t resp;
	pmove_state_t old_pmove; /* for detecting out-of-pmove changes */

	int buttons;
	int oldbuttons;
	int latched_buttons;

	vec3_t kick_angles; /* weapon kicks */
	vec3_t kick_origin;
	float v_dmg_roll, v_dmg_pitch, v_dmg_time; /* damage kicks */
	float fall_time, fall_value;			   /* for view drop on fall */
	float damage_alpha;
	float bonus_alpha;
	vec3_t damage_blend;
	vec3_t v_angle; /* aiming direction */
	float bobtime;	/* so off-ground doesn't change it */
	vec3_t oldviewangles;
	vec3_t oldvelocity;

	float next_drown_time;
	int old_waterlevel;
	int breather_sound;

	/* animation vars */
	int anim_end;
	int anim_priority;
	qboolean anim_duck;
	qboolean anim_run;

	float pickup_msg_time;

	float flood_locktill; /* locked from talking */
	float flood_when[10]; /* when messages were said */
	int flood_whenhead;	  /* head pointer for when said */
};

struct edict_s
{
	entity_state_t s;
	struct gclient_s *client; /* NULL if not a player
								 the server expects the first part
								 of gclient_s to be a player_state_t
								 but the rest of it is opaque */

	qboolean inuse;
	int linkcount;

	link_t area; /* linked to a division node or leaf */

	int num_clusters; /* if -1, use headnode instead */
	int clusternums[MAX_ENT_CLUSTERS];
	int headnode; /* unused if num_clusters != -1 */
	int areanum, areanum2;

	/* ================================ */

	int svflags;
	vec3_t mins, maxs;
	vec3_t absmin, absmax, size;
	solid_t solid;
	int clipmask;
	edict_t *owner;

	/* DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER */
	/* EXPECTS THE FIELDS IN THAT ORDER! */

	/* ================================ */
	int movetype;

	char *model;
	float freetime; /* sv.time when the object was freed */

	/* only used locally in game, not by server */
	char *message;
	char *classname;
	int spawnflags;

	float timestamp;

	float angle; /* set in qe3, -1 = up, -2 = down */
	char *target;
	char *targetname;
	char *killtarget;
	char *team;
	char *pathtarget;
	char *deathtarget;
	char *combattarget;
	edict_t *target_ent;

	float speed, accel, decel;
	vec3_t movedir;
	vec3_t pos1, pos2;

	vec3_t velocity;
	vec3_t avelocity;
	int mass;
	float gravity;
	float client_local_gravity;

	edict_t *goalentity;
	edict_t *movetarget;
	float yaw_speed;
	float ideal_yaw;

	float nextthink;
	void (*prethink)(edict_t *ent);
	void (*think)(edict_t *self);
	void (*blocked)(edict_t *self, edict_t *other); /* move to moveinfo? */
	void (*touch)(edict_t *self, edict_t *other, cplane_t *plane,
				  csurface_t *surf);
	void (*use)(edict_t *self, edict_t *other, edict_t *activator);

	float touch_debounce_time;	   /* are all these legit?  do we need more/less of them? */
	float fly_sound_debounce_time; /* move to clientinfo */
	float last_move_time;

	char *map; /* target_changelevel */

	int viewheight; /* height above origin where eyesight is determined */
	int sounds;		/* make this a spawntemp var? */
	int count;

	edict_t *chain;
	edict_t *enemy;
	edict_t *oldenemy;
	edict_t *activator;
	edict_t *groundentity;
	int groundentity_linkcount;

	edict_t *mynoise; /* can go in client only */
	edict_t *mynoise2;

	int noise_index;
	int noise_index2;
	float volume;
	float attenuation;

	/* timing variables */
	float wait;
	float delay; /* before firing targets */
	float random;

	float last_sound_time;

	int watertype;
	int waterlevel;

	vec3_t move_origin;
	vec3_t move_angles;

	/* move this to clientinfo? */
	int light_level;

	int style; /* also used as areaportal number */

	/* common data blocks */
	moveinfo_t moveinfo;
};

#endif /* MQ2B_LOCAL_H */
