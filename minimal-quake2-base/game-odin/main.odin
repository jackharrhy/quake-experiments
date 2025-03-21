package game

import "base:runtime"
import "core:fmt"
import "core:mem"

GAME_API_VERSION: i32 = 3

CVAR_ARCHIVE: i32 = 1 // set to cause it to be saved to vars.rc
CVAR_USERINFO: i32 = 2 // added to userinfo when changed
CVAR_SERVERINFO: i32 = 4 // added to serverinfo when changed
CVAR_NOSET: i32 = 8 // don't allow change from console at all, but can be set from the command line
CVAR_LATCH: i32 = 16 // save changes until server restart

TAG_GAME :: 765 // clear when unloading the dll
TAG_LEVEL :: 766 // clear when loading a new level

MAX_STATS :: 32

FRAMETIME :: 0.1

MAX_QPATH :: 64 // max length of a quake game pathname
MAX_ENT_CLUSTERS :: 16

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

Client :: struct {
	ps:      Player_State,
	ping:    i32,
	v_angle: [3]f32,
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
	movetype:     i32,
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
Multicast :: enum i32 {}
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
	fmt.eprintfln(text, ..args)
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

	debug_log("GetGameAPI")

	return &globals
}

InitGame :: proc "c" () {
	context = runtime.default_context()

	log("Game is starting up.")

	cvar_maxentities = gi.cvar("maxentities", "1024", CVAR_LATCH)
	cvar_maxclients = gi.cvar("maxclients", "4", CVAR_SERVERINFO | CVAR_LATCH)

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

	debug_log("ShutdownGame")

	gi.FreeTags(TAG_GAME)
	gi.FreeTags(TAG_LEVEL)
}

SpawnEntities :: proc "c" (mapname: cstring, entities: cstring, spawnpoint: cstring) {
	context = runtime.default_context()

	debug_log(
		"SpawnEntities: mapname: %s, entities: %s, spawnpoint: %s",
		mapname,
		entities,
		spawnpoint,
	)

	gi.FreeTags(TAG_LEVEL)

	mem.zero_slice(g_edicts[:globals.max_edicts])

	g_level = {
		mapname = string(mapname),
	}

	g_spawnpoint = string(spawnpoint)

	for i in 0 ..< g_maxclients {
		g_edicts[i + 1].client = &g_clients[i]
	}

	debug_log("SpawnEntities")
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

ClientConnect :: proc "c" (ent: ^Edict, userinfo: cstring) -> bool {
	context = runtime.default_context()

	// TODO parse user info into something useful
	// like, getting their ip, seeing if the password
	// they have provided is correct, etc.

	debug_log(fmt.tprintf("ClientConnect: ent: %p, userinfo: %s", ent, userinfo))

	if ent == nil || userinfo == nil {
		return false
	}

	ent_index := uintptr(rawptr(ent)) - uintptr(rawptr(&g_edicts[0]))
	client_index := ent_index / size_of(Edict) - 1

	debug_log(
		fmt.tprintf("ClientConnect: ent_index: %d, client_index: %d", ent_index, client_index),
	)

	ent.client = &g_clients[client_index]

	return true
}

ClientUserinfoChanged :: proc "c" (ent: ^Edict, userinfo: cstring) {
	context = runtime.default_context()

	debug_log("ClientUserinfoChanged")
}

ClientDisconnect :: proc "c" (ent: ^Edict) {
	context = runtime.default_context()

	debug_log("ClientDisconnect")
}

ClientBegin :: proc "c" (ent: ^Edict) {
	context = runtime.default_context()

	ent.client.ps.pmove.origin = [3]i16{0, 10, 0}
	ent.client.ps.fov = 90

	debug_log("ClientBegin")
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

		if ent.client == nil {
			continue
		}

		if ent.client.v_angle[Angle_Index.PITCH] > 180 {
			ent.s.angles[Angle_Index.PITCH] = (-360 + ent.client.v_angle[Angle_Index.PITCH]) / 3
		} else {
			ent.s.angles[Angle_Index.PITCH] = ent.client.v_angle[Angle_Index.PITCH] / 3
		}
	}
}
