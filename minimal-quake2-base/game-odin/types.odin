package game
// Destination class for gi.multicast()

Multicast :: enum i32 {
	ALL,
	PHS,
	PVS,
	ALL_R,
	PHS_R,
	PVS_R,
}

// Protocol bytes that can be directly added to messages.
//
// NOTE: this was in local.h, so maybe we have to handle it here,
// rather than just assuming the engine code will handle it.
Svc :: enum i32 {
	MUZZLEFLASH  = 1,
	MUZZLEFLASH2 = 2,
	TEMP_ENTITY  = 3,
	LAYOUT       = 4,
	INVENTORY    = 5,
}

// Content flags for trace
Contents :: enum i32 {
	SOLID        = 1, // an eye is never valid in a solid
	WINDOW       = 2, // translucent, but not watery
	AUX          = 4,
	LAVA         = 8,
	SLIME        = 16,
	WATER        = 32,
	MIST         = 64,
	LAST_VISIBLE = 64,

	// remaining contents are non-visible, and don't eat brushes
	AREAPORTAL   = 0x8000,
	PLAYERCLIP   = 0x10000,
	MONSTERCLIP  = 0x20000,

	// currents can be added to any other contents, and may be mixed
	CURRENT_0    = 0x40000,
	CURRENT_90   = 0x80000,
	CURRENT_180  = 0x100000,
	CURRENT_270  = 0x200000,
	CURRENT_UP   = 0x400000,
	CURRENT_DOWN = 0x800000,
	ORIGIN       = 0x1000000, // removed before bsping an entity
	MONSTER      = 0x2000000, // should never be on a brush, only in game
	DEADMONSTER  = 0x4000000,
	DETAIL       = 0x8000000, // brushes to be added after vis leafs
	TRANSLUCENT  = 0x10000000, // auto set if any surface has trans
	LADDER       = 0x20000000,
}

// Content masks
MASK_ALL :: -1
MASK_SOLID :: int(Contents.SOLID) | int(Contents.WINDOW)
MASK_PLAYERSOLID ::
	int(Contents.SOLID) | int(Contents.PLAYERCLIP) | int(Contents.WINDOW) | int(Contents.MONSTER)
MASK_DEADSOLID :: int(Contents.SOLID) | int(Contents.PLAYERCLIP) | int(Contents.WINDOW)
MASK_MONSTERSOLID ::
	int(Contents.SOLID) | int(Contents.MONSTERCLIP) | int(Contents.WINDOW) | int(Contents.MONSTER)
MASK_WATER :: int(Contents.WATER) | int(Contents.LAVA) | int(Contents.SLIME)
MASK_OPAQUE :: int(Contents.SOLID) | int(Contents.SLIME) | int(Contents.LAVA)
MASK_SHOT ::
	int(Contents.SOLID) | int(Contents.MONSTER) | int(Contents.WINDOW) | int(Contents.DEADMONSTER)
MASK_CURRENT ::
	int(Contents.CURRENT_0) |
	int(Contents.CURRENT_90) |
	int(Contents.CURRENT_180) |
	int(Contents.CURRENT_270) |
	int(Contents.CURRENT_UP) |
	int(Contents.CURRENT_DOWN)

// Game print flags
Print :: enum i32 {
	LOW    = 0, // pickup messages
	MEDIUM = 1, // death messages
	HIGH   = 2, // critical messages
	CHAT   = 3, // chat messages
}


// Muzzle flashes / player effects.
Muzzle_Flash :: enum i32 {
	BLASTER          = 0,
	MACHINEGUN       = 1,
	SHOTGUN          = 2,
	CHAINGUN1        = 3,
	CHAINGUN2        = 4,
	CHAINGUN3        = 5,
	RAILGUN          = 6,
	ROCKET           = 7,
	GRENADE          = 8,
	LOGIN            = 9,
	LOGOUT           = 10,
	RESPAWN          = 11,
	BFG              = 12,
	SSHOTGUN         = 13,
	HYPERBLASTER     = 14,
	ITEMRESPAWN      = 15,
	IONRIPPER        = 16,
	BLUEHYPERBLASTER = 17,
	PHALANX          = 18,
	SILENCED         = 128, // bit flag ORed with one of the above numbers
	ETF_RIFLE        = 30,
	UNUSED           = 31,
	SHOTGUN2         = 32,
	HEATBEAM         = 33,
	BLASTER2         = 34,
	TRACKER          = 35,
	NUKE1            = 36,
	NUKE2            = 37,
	NUKE4            = 38,
	NUKE8            = 39,
}

PMF :: enum i32 {
	DUCKED         = 1,
	JUMP_HELD      = 2,
	ON_GROUND      = 4,
	TIME_WATERJUMP = 8, // pm_time is waterjump
	TIME_LAND      = 16, // pm_time is time before rejump
	TIME_TELEPORT  = 32, // pm_time is non-moving time
	NO_PREDICTION  = 64, // temporarily disables prediction (used for grappling hook)
}

// Entity events for effects that take place relative to an existing entity's origin
Entity_Event :: enum i32 {
	NONE            = 0,
	ITEM_RESPAWN    = 1,
	FOOTSTEP        = 2,
	FALLSHORT       = 3,
	FALL            = 4,
	FALLFAR         = 5,
	PLAYER_TELEPORT = 6,
	OTHER_TELEPORT  = 7,
}


Level_Locals :: struct {
	framenum:   i32,
	time:       f32,
	level_name: string,
	mapname:    string,
}

Pmtype :: enum i32 {
	NORMAL    = 0,
	SPECTATOR = 1,
	DEAD      = 2,
	GIB       = 3,
	FREEZE    = 4,
}

Pmove_State :: struct {
	pm_type:      Pmtype,
	origin:       [3]i16,
	velocity:     [3]i16,
	pm_flags:     u8,
	pm_time:      u8,
	gravity:      i16,
	delta_angles: [3]i16,
}

Player_State :: struct {
	pmove:       Pmove_State,
	viewangles:  [3]f32,
	viewoffset:  [3]f32,
	kick_angles: [3]f32,
	gunangles:   [3]f32,
	gunoffset:   [3]f32,
	gunindex:    i32,
	gunframe:    i32,
	blend:       [4]f32,
	fov:         f32,
	rdflags:     i32,
	stats:       [MAX_STATS]i16,
}

Client_Persistant :: struct {
	userinfo:  [MAX_INFO_STRING]u8,
	netname:   [16]u8,
	connected: bool,
}

Client :: struct {
	ps:              Player_State,
	ping:            i32,
	// --
	pers:            Client_Persistant,
	buttons:         i32,
	oldbuttons:      i32,
	latched_buttons: i32,
	kick_angles:     [3]f32,
	kick_origin:     [3]f32,
	v_angle:         [3]f32,
	fall_time:       f32,
	fall_value:      f32,
	bobtime:         f32,
	oldvelocity:     [3]f32,
	oldviewangles:   [3]f32,
	old_pmove:       Pmove_State,
}

Entity_State :: struct {
	number:      i32,
	origin:      [3]f32,
	angles:      [3]f32,
	old_origin:  [3]f32,
	modelindex:  i32,
	modelindex2: i32,
	modelindex3: i32,
	modelindex4: i32,
	frame:       i32,
	skinnum:     i32,
	effects:     u32,
	renderfx:    i32,
	solid:       i32,
	sound:       i32,
	event:       i32,
}

Link :: struct {
	prev: ^Link,
	next: ^Link,
}

Solid :: enum i32 {
	NOT     = 0,
	TRIGGER = 1,
	BBOX    = 2,
	BSP     = 3,
}

Movetype :: enum i32 {
	NONE   = 0,
	NOCLIP = 1,
	WALK   = 2,
	STEP   = 3,
	TOSS   = 4,
}

Angle_Index :: enum {
	PITCH = 0, /* up / down */
	YAW   = 1, /* left / right */
	ROLL  = 2, /* fall over */
}

Edict :: struct {
	s:                      Entity_State,
	client:                 ^Client,
	inuse:                  bool,
	linkcount:              i32,
	area:                   Link,
	num_clusters:           i32,
	clusternums:            [MAX_ENT_CLUSTERS]i32,
	headnode:               i32,
	areanum:                i32,
	areanum2:               i32,
	svflags:                i32,
	mins:                   [3]f32,
	maxs:                   [3]f32,
	absmin:                 [3]f32,
	absmax:                 [3]f32,
	size:                   [3]f32,
	solid:                  Solid,
	clipmask:               i32,
	owner:                  ^Edict,
	// Don't touch anything above this!
	// The server expects the fields in this order!
	classname:              string,
	movetype:               Movetype,
	gravity:                f32,
	model:                  string,
	velocity:               [3]f32,
	viewheight:             i32,
	groundentity:           ^Edict,
	groundentity_linkcount: i32,
	watertype:              i32,
	waterlevel:             i32,
	light_level:            i32,
}

Usercmd :: struct {
	msec:        u8,
	buttons:     u8,
	angles:      [3]i16,
	forwardmove: i16,
	sidemove:    i16,
	upmove:      i16,
	impulse:     u8,
	lightlevel:  u8, // light level the player is standing on
}

Trace :: struct {
	allsolid:   bool,
	startsolid: bool,
	fraction:   f32,
	endpos:     [3]f32,
	plane:      Cplane,
	surface:    ^Csurface,
	contents:   i32,
	ent:        ^Edict,
}

Cplane :: struct {
	normal:   [3]f32,
	dist:     f32,
	type:     u8,
	signbits: u8,
	pad:      [2]u8,
}

Csurface :: struct {
	name:  [16]u8,
	flags: i32,
	value: i32,
}

Pmove :: struct {
	// state (in / out)
	state:         Pmove_State,

	// command (in)
	cmd:           Usercmd,
	snapinitial:   bool, // if state has been changed outside pmove

	// results (out)
	numtouch:      i32,
	touchents:     [MAX_TOUCH]^Edict,
	viewangles:    [3]f32, // clamped
	viewheight:    f32,
	mins, maxs:    [3]f32, // bounding box size
	groundentity:  ^Edict,
	watertype:     i32,
	waterlevel:    i32,

	// callbacks to test the world
	trace:         proc "c" (start, mins, maxs, end: [^]f32) -> Trace,
	pointcontents: proc "c" (point: [3]f32) -> i32,
}
Cvar :: struct {
	name:           cstring,
	string:         cstring,
	latched_string: cstring,
	flags:          i32,
	modified:       bool,
	value:          f32,
	next:           ^Cvar,
	default_string: cstring,
}

Game_Import :: struct {
	bprintf:            proc "c" (printlevel: i32, fmt: cstring, rest: ..cstring),
	dprintf:            proc "c" (fmt: cstring, rest: ..cstring),
	cprintf:            proc "c" (ent: ^Edict, printlevel: i32, fmt: cstring, rest: ..cstring),
	centerprintf:       proc "c" (ent: ^Edict, fmt: cstring, rest: ..cstring),
	sound:              proc "c" (
		ent: ^Edict,
		channel, soundindex: i32,
		volume, attenuation, timeofs: f32,
	),
	positioned_sound:   proc "c" (
		origin: [3]f32,
		ent: ^Edict,
		channel, soundindex: i32,
		volume, attenuation, timeofs: f32,
	),
	configstring:       proc "c" (num: i32, string: cstring),
	error:              proc "c" (fmt: cstring, rest: ..cstring) -> !,
	modelindex:         proc "c" (name: cstring) -> i32,
	soundindex:         proc "c" (name: cstring) -> i32,
	imageindex:         proc "c" (name: cstring) -> i32,
	setmodel:           proc "c" (ent: ^Edict, name: cstring),
	trace:              proc "c" (
		start, mins, maxs, end: [^]f32,
		passent: ^Edict,
		contentmask: i32,
	) -> Trace,
	pointcontents:      proc "c" (point: [3]f32) -> i32,
	inPVS:              proc "c" (p1, p2: [3]f32) -> bool,
	inPHS:              proc "c" (p1, p2: [3]f32) -> bool,
	SetAreaPortalState: proc "c" (portalnum: i32, open: bool),
	AreasConnected:     proc "c" (area1, area2: i32) -> bool,
	linkentity:         proc "c" (ent: ^Edict),
	unlinkentity:       proc "c" (ent: ^Edict),
	BoxEdicts:          proc "c" (
		mins, maxs: [3]f32,
		list: ^^Edict,
		maxcount, areatype: i32,
	) -> i32,
	Pmove:              proc "c" (pmove: ^Pmove),
	multicast:          proc "c" (origin: [^]f32, to: Multicast),
	unicast:            proc "c" (ent: ^Edict, reliable: bool),
	WriteChar:          proc "c" (c: i32),
	WriteByte:          proc "c" (c: i32),
	WriteShort:         proc "c" (c: i32),
	WriteLong:          proc "c" (c: i32),
	WriteFloat:         proc "c" (f: f32),
	WriteString:        proc "c" (s: cstring),
	WritePosition:      proc "c" (pos: [3]f32),
	WriteDir:           proc "c" (pos: [3]f32),
	WriteAngle:         proc "c" (f: f32),
	TagMalloc:          proc "c" (size, tag: i32) -> rawptr,
	TagFree:            proc "c" (block: rawptr),
	FreeTags:           proc "c" (tag: i32),
	cvar:               proc "c" (var_name, value: cstring, flags: i32) -> ^Cvar,
	cvar_set:           proc "c" (var_name, value: cstring) -> ^Cvar,
	cvar_forceset:      proc "c" (var_name, value: cstring) -> ^Cvar,
	argc:               proc "c" () -> i32,
	argv:               proc "c" (n: i32) -> cstring,
	args:               proc "c" () -> cstring,
	AddCommandString:   proc "c" (text: cstring),
	DebugGraph:         proc "c" (value: f32, color: i32),
}


Game_Export :: struct {
	apiversion:            i32,
	Init:                  proc "c" (),
	Shutdown:              proc "c" (),
	SpawnEntities:         proc "c" (mapname, entstring, spawnpoint: cstring),
	WriteGame:             proc "c" (filename: cstring, autosave: bool),
	ReadGame:              proc "c" (filename: cstring),
	WriteLevel:            proc "c" (filename: cstring),
	ReadLevel:             proc "c" (filename: cstring),
	ClientConnect:         proc "c" (ent: ^Edict, userinfo: cstring) -> bool,
	ClientBegin:           proc "c" (ent: ^Edict),
	ClientUserinfoChanged: proc "c" (ent: ^Edict, userinfo: cstring),
	ClientDisconnect:      proc "c" (ent: ^Edict),
	ClientCommand:         proc "c" (ent: ^Edict),
	ClientThink:           proc "c" (ent: ^Edict, cmd: ^Usercmd),
	RunFrame:              proc "c" (),
	ServerCommand:         proc "c" (),
	edicts:                ^Edict,
	edict_size:            i32,
	num_edicts:            i32,
	max_edicts:            i32,
}
