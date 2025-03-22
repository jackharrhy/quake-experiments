package game

import "base:runtime"
import "core:fmt"
import "core:mem"
import "core:os/os2"
import "core:strconv"
import "core:strings"
import "core:unicode"

GAME_API_VERSION: i32 = 3

Cvar_Flag :: enum i32 {
	ARCHIVE    = 1, // set to cause it to be saved to vars.rc
	USERINFO   = 2, // added to userinfo  when changed
	SERVERINFO = 4, // added to serverinfo when changed
	NOSET      = 8, // don't allow change from console at all, but can be set from the command line
	LATCH      = 16, // save changes until server restart
}


TAG_GAME :: 765 // Clear when unloading the dll.
TAG_LEVEL :: 766 // Clear when loading a new level.

MAX_STATS :: 32

FRAMETIME :: 0.1 // Time between server frames in seconds.

MAX_QPATH :: 64 // Max length of a quake game pathname.
MAX_ENT_CLUSTERS :: 16

MAX_INFO_KEY :: 64
MAX_INFO_VALUE :: 64
MAX_INFO_STRING :: 512

FALL_TIME :: 0.3

PITCH :: 0 // up / down
YAW :: 1 // left / right
ROLL :: 2 // fall over

gi: Game_Import
globals: Game_Export

g_edicts: [^]Edict
g_clients: [^]Client
g_level: Level_Locals

g_maxclients: i32
g_maxentities: i32
g_spawnpoint: string

cvar_maxentities: ^Cvar
cvar_maxclients: ^Cvar

cvar_sv_rollspeed: ^Cvar
cvar_sv_rollangle: ^Cvar
cvar_run_pitch: ^Cvar
cvar_run_roll: ^Cvar
cvar_bob_up: ^Cvar
cvar_bob_pitch: ^Cvar
cvar_bob_roll: ^Cvar

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
	framenum:       i32,
	time:           f32,
	level_name:     string,
	mapname:        string,
	current_entity: ^Edict,
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
	ps:            Player_State,
	ping:          i32,
	// --
	pers:          Client_Persistant,
	// --
	kick_angles:   [3]f32,
	kick_origin:   [3]f32,
	v_angle:       [3]f32,
	fall_time:     f32,
	fall_value:    f32,
	bobtime:       f32,
	oldvelocity:   [3]f32,
	oldviewangles: [3]f32,
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
	s:            Entity_State,
	client:       ^Client,
	inuse:        bool,
	linkcount:    i32,
	area:         Link,
	num_clusters: i32,
	clusternums:  [MAX_ENT_CLUSTERS]i32,
	headnode:     i32,
	areanum:      i32,
	areanum2:     i32,
	svflags:      i32,
	mins:         [3]f32,
	maxs:         [3]f32,
	absmin:       [3]f32,
	absmax:       [3]f32,
	size:         [3]f32,
	solid:        Solid,
	clipmask:     i32,
	owner:        ^Edict,
	// Don't touch anything above this!
	// The server expects the fields in this order!
	classname:    string,
	gravity:      f32,
	model:        string,
	velocity:     [3]f32,
	viewheight:   i32,
	groundentity: ^Edict,
}

Usercmd :: struct {}

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

Pmove :: struct {}
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
		start, mins, maxs, end: [3]f32,
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
	multicast:          proc "c" (origin: [3]f32, to: Multicast),
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

log :: proc(text: string) {
	c_text := fmt.ctprintf("%s\n", text)
	gi.dprintf(c_text)
}

prefixed_log :: proc(prefix: string, text: string) {
	c_text := fmt.ctprintf("%s: %s\n", prefix, text)
	gi.dprintf(c_text)
}

debug_log :: proc(text: string, args: ..any) {
	fmt.printfln("\x1b[33m%s\x1b[0m", fmt.tprintf(text, ..args))
}

@(export)
GetGameAPI :: proc "c" (game_import: ^Game_Import) -> ^Game_Export {
	context = runtime.default_context()

	gi = game_import^

	globals.apiversion = GAME_API_VERSION
	globals.Init = InitGame
	globals.Shutdown = ShutdownGame
	globals.SpawnEntities = SpawnEntities

	globals.WriteGame = WriteGame
	globals.ReadGame = ReadGame
	globals.WriteLevel = WriteLevel
	globals.ReadLevel = ReadLevel

	globals.ClientThink = ClientThink
	globals.ClientConnect = ClientConnect
	globals.ClientUserinfoChanged = ClientUserinfoChanged
	globals.ClientDisconnect = ClientDisconnect
	globals.ClientBegin = ClientBegin
	globals.ClientCommand = ClientCommand

	globals.RunFrame = G_RunFrame

	globals.ServerCommand = ServerCommand

	globals.edict_size = size_of(Edict)

	return &globals
}

InitGame :: proc "c" () {
	context = runtime.default_context()

	log("Game is starting up.")

	cvar_maxentities = gi.cvar("maxentities", "1024", i32(Cvar_Flag.LATCH))
	cvar_maxclients = gi.cvar("maxclients", "4", i32(Cvar_Flag.SERVERINFO | Cvar_Flag.LATCH))

	cvar_sv_rollspeed = gi.cvar("sv_rollspeed", "2", 0)
	cvar_sv_rollangle = gi.cvar("sv_rollangle", "2", 0)
	cvar_run_pitch = gi.cvar("run_pitch", "0.002", 0)
	cvar_run_roll = gi.cvar("run_roll", "0.005", 0)
	cvar_bob_up = gi.cvar("bob_up", "0.005", 0)
	cvar_bob_pitch = gi.cvar("bob_pitch", "0.002", 0)
	cvar_bob_roll = gi.cvar("bob_roll", "0.002", 0)

	g_maxentities = auto_cast cvar_maxentities.value
	g_edicts = auto_cast gi.TagMalloc(g_maxentities * size_of(Edict), TAG_GAME)
	globals.edicts = g_edicts
	globals.max_edicts = g_maxentities

	g_maxclients = auto_cast cvar_maxclients.value
	g_clients = auto_cast gi.TagMalloc(g_maxclients * size_of(Client), TAG_GAME)

	globals.num_edicts = g_maxclients + 1
}

ShutdownGame :: proc "c" () {
	context = runtime.default_context()

	gi.FreeTags(TAG_GAME)
	gi.FreeTags(TAG_LEVEL)
}

InitEdict :: proc(e: ^Edict) {
	e.inuse = true
	e.classname = "noclass"
	e.gravity = 1.0
	e.s.number = i32(uintptr(rawptr(e)) - uintptr(rawptr(&g_edicts[0])) / size_of(Edict))
}

Spawn :: proc() -> ^Edict {
	e := &g_edicts[g_maxclients + 1]

	for i := g_maxclients + 1; i < globals.num_edicts; i += 1 {
		e = &g_edicts[i]
		if !e.inuse {
			InitEdict(e)
			return e
		}
	}

	if i := globals.num_edicts; i == globals.max_edicts {
		gi.error("Spawn: no free edicts")
	}

	globals.num_edicts += 1
	e = &g_edicts[globals.num_edicts - 1]
	InitEdict(e)
	return e
}

Parse_Entity :: proc(entity: ^Edict, entity_block: string) {
	if len(entity_block) < 2 {
		return
	}

	content := strings.trim_space(entity_block[1:len(entity_block) - 1])

	lines := strings.split(content, "\n")
	defer delete(lines)

	for line in lines {
		trimmed := strings.trim_space(line)
		if trimmed == "" {
			continue
		}

		state :: enum {
			OUTSIDE,
			IN_KEY,
			AFTER_KEY,
			IN_VALUE,
		}

		current_state := state.OUTSIDE
		key := ""
		value := ""

		for c, i in trimmed {
			switch current_state {
			case .OUTSIDE:
				if c == '"' {
					current_state = .IN_KEY
					key = ""
				}

			case .IN_KEY:
				if c == '"' {
					current_state = .AFTER_KEY
				} else {
					key = fmt.tprintf("%s%c", key, c)
				}

			case .AFTER_KEY:
				if c == '"' {
					current_state = .IN_VALUE
					value = ""
				}

			case .IN_VALUE:
				if c == '"' {
					current_state = .OUTSIDE
				} else {
					value = fmt.tprintf("%s%c", value, c)
				}
			}
		}

		switch key {
		case "classname":
			// TODO this has to be a clone so it lives longer than this scope
			// but, it should get cleaned up when we clean up the edict, so for
			// now, lets clone it here and worry about freeing it later
			entity.classname = strings.clone(value)
		case "model":
			entity.model = strings.clone(value)
		case "origin":
			parts := strings.split(value, " ")
			if len(parts) >= 3 {
				entity.s.origin[0] = strconv.parse_f32(parts[0]) or_else 0
				entity.s.origin[1] = strconv.parse_f32(parts[1]) or_else 0
				entity.s.origin[2] = strconv.parse_f32(parts[2]) or_else 0
			} else {
				gi.error(fmt.ctprintf("origin: invalid number of arguments: %s", value))
			}
			defer delete(parts)
		case:
			debug_log(fmt.tprintf("unknown key: %s, value: %s", key, value))
		}
	}
}

// Finds an entity by its classname.
//
// Returns nil if no entity is found.
FindEntityByClassName :: proc(match: string) -> ^Edict {
	start_index: i32 = 0

	for i: i32 = 0; i < globals.num_edicts; i += 1 {
		ent := &g_edicts[i]

		if !ent.inuse {
			continue
		}

		if ent.classname == "" {
			continue
		}

		if strings.equal_fold(ent.classname, match) {
			return ent
		}
	}

	return nil
}

SpawnEntities :: proc "c" (mapname: cstring, entities: cstring, spawnpoint: cstring) {
	context = runtime.default_context()

	gi.FreeTags(TAG_LEVEL)

	mem.zero_slice(g_edicts[:globals.max_edicts])

	g_level = {
		mapname = string(mapname),
	}

	g_spawnpoint = string(spawnpoint)

	for i in 0 ..< g_maxclients {
		g_edicts[i + 1].client = &g_clients[i]
	}

	entities_str := string(entities)

	entity_start := 0

	for i := 0; i < len(entities_str); i += 1 {
		if entities_str[i] == '{' {
			entity_start = i
		} else if entities_str[i] == '}' && entity_start > 0 {
			entity_block := entities_str[entity_start:i + 1]
			e := Spawn()
			Parse_Entity(e, entity_block)
			entity_start = 0
		}
	}
}

WriteGame :: proc "c" (filename: cstring, autosave: bool) {
	context = runtime.default_context()

	debug_log("WriteGame")
}

ReadGame :: proc "c" (filename: cstring) {
	context = runtime.default_context()

	debug_log("ReadGame")
}

WriteLevel :: proc "c" (filename: cstring) {
	context = runtime.default_context()

	debug_log("WriteLevel")
}

ReadLevel :: proc "c" (filename: cstring) {
	context = runtime.default_context()

	debug_log("ReadLevel")
}

ClientThink :: proc "c" (ent: ^Edict, cmd: ^Usercmd) {
	context = runtime.default_context()
}

// Takes in an edict reference and returns the index of the edict in the g_edicts array.
//
// Quake II did this by some pointer arithmetic, ew!, so we're doing it the same way.
ent_index_from_edict :: proc "c" (ent: ^Edict) -> int {
	offset := uintptr(rawptr(ent)) - uintptr(rawptr(&g_edicts[0]))
	return int(offset / size_of(Edict))
}

ClientConnect :: proc "c" (ent: ^Edict, userinfo: cstring) -> bool {
	context = runtime.default_context()

	// TODO parse user info into something useful
	// like, getting their ip, seeing if the password
	// they have provided is correct, etc.

	if ent == nil || userinfo == nil {
		return false
	}

	client_index := ent_index_from_edict(ent) - 1
	ent.client = &g_clients[client_index]

	if (ent.inuse == false) {
		InitClientPersistant(ent.client)
	} else {
		debug_log(
			"Putting client into an existing entity, I haven't handled this, not sure if I have to!",
		)
	}

	return true
}

InitClientPersistant :: proc(client: ^Client) {
	debug_log("InitClientPersistant")
}

ClientUserinfoChanged :: proc "c" (ent: ^Edict, userinfo: cstring) {
	context = runtime.default_context()

	debug_log("ClientUserinfoChanged")
}

ClientDisconnect :: proc "c" (ent: ^Edict) {
	context = runtime.default_context()

	debug_log("ClientDisconnect")
}

PutClientInServer :: proc(ent: ^Edict) {
	spawn_point := FindEntityByClassName("info_player_start")

	if spawn_point == nil {
		gi.error("PutClientInServer: no spawn point found")
	}

	ent.s.origin = spawn_point.s.origin
	ent.s.angles = spawn_point.s.angles
}

ClientBegin :: proc "c" (ent: ^Edict) {
	context = runtime.default_context()

	InitEdict(ent)

	ent.classname = "player"
	ent.client.ps.fov = 90 // TODO get from userinfo

	PutClientInServer(ent)

	/*
	gi.WriteByte(i32(Svc.MUZZLEFLASH))
	gi.WriteShort(i32(ent_index_from_edict(ent)))
	gi.WriteByte(i32(Muzzle_Flash.LOGIN))
	gi.multicast(ent.s.origin, .PVS)
	*/

	message := "A player has joined the game"
	gi.bprintf(i32(Print.HIGH), fmt.ctprintf("%s\n", message))

	ClientEndServerFrame(ent)
}

ClientCommand :: proc "c" (ent: ^Edict) {
	context = runtime.default_context()

	debug_log("ClientCommand")
}

G_RunFrame :: proc "c" () {
	context = runtime.default_context()

	g_level.framenum += 1
	g_level.time = f32(g_level.framenum) * FRAMETIME

	for i: i32 = 0; i < globals.num_edicts; i += 1 {
		ent := &g_edicts[i]

		g_level.current_entity = ent

		ent.s.old_origin = ent.s.origin

		if i > 0 && i <= i32(cvar_maxclients.value) {
			ClientBeginServerFrame(ent)
			continue
		}
	}

	ClientEndServerFrames()

	free_all(context.temp_allocator)
}

ServerCommand :: proc "c" () {
	context = runtime.default_context()

	debug_log("ServerCommand")
}

ClientBeginServerFrame :: proc "c" (ent: ^Edict) {
	context = runtime.default_context()
}

ClientEndServerFrames :: proc "c" () {
	context = runtime.default_context()

	for i: i32 = 0; i < globals.num_edicts; i += 1 {
		ent := &g_edicts[i]

		if ent.client == nil || ent.inuse == false {
			continue
		}

		ClientEndServerFrame(ent)
	}
}
