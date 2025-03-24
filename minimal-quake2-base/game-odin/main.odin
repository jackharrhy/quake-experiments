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

	cvar_sv_gravity = gi.cvar("sv_gravity", "800", 0)
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

Spawn :: proc(worldspawn: bool = false) -> ^Edict {
	e: ^Edict
	if worldspawn {
		// worldspawn is always the first edict
		e = &g_edicts[0]
	} else {
		e = &g_edicts[g_maxclients + 1]

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
			error_log(fmt.tprintf("unknown key: %s, value: %s", key, value))
		}
	}

	info_log(fmt.tprintf("entity: classname=%s, origin=%v", entity.classname, entity.s.origin))
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

	e: ^Edict

	for i := 0; i < len(entities_str); i += 1 {
		if entities_str[i] == '{' {
			entity_start = i
		} else if entities_str[i] == '}' {
			entity_block := entities_str[entity_start:i + 1]
			if e == nil {
				e = Spawn(worldspawn = true)
			} else {
				e = Spawn()
			}
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
		error_log(
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

ClientBegin :: proc "c" (ent: ^Edict) {
	context = runtime.default_context()

	InitEdict(ent)

	ent.classname = "player"

	PutClientInServer(ent)

	gi.WriteByte(i32(Svc.MUZZLEFLASH))
	gi.WriteShort(i32(ent_index_from_edict(ent)))
	gi.WriteByte(i32(Muzzle_Flash.LOGIN))
	gi.multicast(raw_data(ent.s.origin[:]), .PVS)

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

		if !ent.inuse {
			continue
		}

		ent.s.old_origin = ent.s.origin

		// if the ground entity moved, make sure we are still on it
		if (ent.groundentity != nil) &&
		   (ent.groundentity.linkcount != ent.groundentity_linkcount) {
			ent.groundentity = nil
		}

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
