package game

import "base:runtime"
import "core:fmt"
import "core:mem"
import "core:strconv"
import "core:strings"

setup_cvars :: proc() {
	cvar_maxentities = gi.cvar("maxentities", "1024", i32(Cvar_Flag.LATCH))
	cvar_maxclients = gi.cvar("maxclients", "4", i32(Cvar_Flag.SERVERINFO | Cvar_Flag.LATCH))

	cvar_sv_cheats = gi.cvar("sv_cheats", "0", 0)
	cvar_sv_gravity = gi.cvar("sv_gravity", "800", 0)
	cvar_sv_airaccelerate = gi.cvar("sv_airaccelerate", "1", 0)
	cvar_sv_rollspeed = gi.cvar("sv_rollspeed", "2", 0)
	cvar_sv_rollangle = gi.cvar("sv_rollangle", "2", 0)
	cvar_run_pitch = gi.cvar("run_pitch", "0.002", 0)
	cvar_run_roll = gi.cvar("run_roll", "0.005", 0)
	cvar_bob_up = gi.cvar("bob_up", "0.005", 0)
	cvar_bob_pitch = gi.cvar("bob_pitch", "0.002", 0)
	cvar_bob_roll = gi.cvar("bob_roll", "0.002", 0)

	cvar_sv_autobhop = gi.cvar("sv_autobhop", "0", 0)
}

setup_globals :: proc() {
	g_maxentities = auto_cast cvar_maxentities.value
	g_edicts = auto_cast gi.TagMalloc(g_maxentities * size_of(Edict), TAG_GAME)
	globals.edicts = g_edicts
	globals.max_edicts = g_maxentities

	g_maxclients = auto_cast cvar_maxclients.value
	g_clients = auto_cast gi.TagMalloc(g_maxclients * size_of(Client), TAG_GAME)

	globals.num_edicts = g_maxclients + 1
}

InitGame :: proc "c" () {
	context = runtime.default_context()

	log("Game is starting up.")

	setup_cvars()
	setup_globals()
}

ShutdownGame :: proc "c" () {
	context = runtime.default_context()

	gi.FreeTags(TAG_GAME)
	gi.FreeTags(TAG_LEVEL)
}

initialize_edict :: proc(e: ^Edict) {
	e.inuse = true
	e.classname = "noclass"
	e.gravity = 1.0
	e.s.number = i32(uintptr(rawptr(e)) - uintptr(rawptr(&g_edicts[0])) / size_of(Edict))
}

spawn_edict :: proc(worldspawn: bool = false) -> ^Edict {
	e: ^Edict
	if worldspawn {
		// worldspawn is always the first edict
		e = &g_edicts[0]
	} else {
		e = &g_edicts[g_maxclients + 1]

		for i := g_maxclients + 1; i < globals.num_edicts; i += 1 {
			e = &g_edicts[i]
			if !e.inuse {
				initialize_edict(e)
				return e
			}
		}

		if i := globals.num_edicts; i == globals.max_edicts {
			gi.error("Spawn: no free edicts")
		}
	}

	globals.num_edicts += 1
	e = &g_edicts[globals.num_edicts - 1]
	initialize_edict(e)
	return e
}

parse_entity :: proc(entity: ^Edict, entity_block: string) {
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
				e = spawn_edict(worldspawn = true)
			} else {
				e = spawn_edict()
			}
			parse_entity(e, entity_block)
			entity_start = 0
		}
	}
}

WriteGame :: proc "c" (filename: cstring, autosave: b32) {
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

ClientConnect :: proc "c" (ent: ^Edict, userinfo: cstring) -> b32 {
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
		// InitClientPersistant(ent.client)
	} else {
		error_log(
			"Putting client into an existing entity, I haven't handled this, not sure if I have to!",
		)
	}

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

	initialize_edict(ent)

	ent.classname = "player"

	put_client_in_server(ent)

	send_muzzle_flash(ent, .LOGIN)

	message := "A player has joined the game"
	gi.bprintf(i32(Print.HIGH), fmt.ctprintf("%s\n", message))

	client_end_server_frame(ent)
}

client_cmd_noclip :: proc(ent: ^Edict) {
	if cvar_sv_cheats.value == 0 {
		gi.cprintf(
			ent,
			i32(Print.HIGH),
			"You must run the server with '+set cheats 1' to enable this command.\n",
		)
		return
	}

	msg: cstring
	if ent.movetype == Movetype.NOCLIP {
		ent.movetype = Movetype.WALK
		msg = "noclip OFF\n"
	} else {
		ent.movetype = Movetype.NOCLIP
		msg = "noclip ON\n"
	}
	gi.cprintf(ent, i32(Print.HIGH), msg)
}

ClientCommand :: proc "c" (ent: ^Edict) {
	context = runtime.default_context()

	if ent.client == nil {
		return
	}

	cmd := gi.argv(0)

	switch cmd {
	case "noclip":
		client_cmd_noclip(ent)
	case:
	// TODO make it a chat message
	}
}

RunFrame :: proc "c" () {
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

	cmd := gi.argv(1)

	debug_log(fmt.tprintf("ServerCommand: %s", cmd))
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

		client_end_server_frame(ent)
	}
}
