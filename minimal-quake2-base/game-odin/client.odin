package game

import "base:runtime"
import "core:fmt"
import "core:mem"
import "core:strings"

pm_passent: ^Edict

ClientThink :: proc "c" (ent: ^Edict, cmd: ^Usercmd) {
	context = runtime.default_context()
	if ent == nil {
		return
	}

	// The server needs to know what the current entity is when calling
	// the trace function, so it has to be a global here.
	pm_passent = ent

	client := ent.client

	pm: Pmove = {}

	if ent.movetype == .NOCLIP {
		client.ps.pmove.pm_type = .SPECTATOR
	} else if ent.s.modelindex != 255 {
		client.ps.pmove.pm_type = .GIB
	} else {
		client.ps.pmove.pm_type = .NORMAL
	}

	client.ps.pmove.gravity = i16(cvar_sv_gravity.value)

	pm.state = client.ps.pmove

	for i in 0 ..< 3 {
		pm.state.origin[i] = i16(ent.s.origin[i] * 8)
		tmp_vel := i32(ent.velocity[i] * 8)
		pm.state.velocity[i] = i16(tmp_vel)
	}

	if mem.compare_ptrs(&client.old_pmove, &pm.state, size_of(pm.state)) != 0 {
		debug_log("pm.snapinitial = true")
		pm.snapinitial = true
	}

	pm.cmd = cmd^
	pm.trace = proc "c" (start, mins, maxs, end: [^]f32) -> Trace {
		return gi.trace(start, mins, maxs, end, pm_passent, i32(MASK_PLAYERSOLID))
	}
	pm.pointcontents = gi.pointcontents

	gi.Pmove(&pm)

	client.ps.pmove = pm.state
	client.old_pmove = pm.state

	for i in 0 ..< 3 {
		ent.s.origin[i] = f32(pm.state.origin[i]) * 0.125
		ent.velocity[i] = f32(pm.state.velocity[i]) * 0.125
	}

	ent.mins = pm.mins
	ent.maxs = pm.maxs

	// TODO jump sound

	ent.viewheight = i32(pm.viewheight)
	ent.waterlevel = pm.waterlevel
	ent.watertype = pm.watertype
	ent.groundentity = pm.groundentity

	if pm.groundentity != nil {
		debug_edicts()
		debug_log("pm.groundentity: %p", pm.groundentity)
		ent.groundentity_linkcount = pm.groundentity.linkcount
	}

	client.v_angle = pm.viewangles
	client.ps.viewangles = pm.viewangles

	gi.linkentity(ent)

	if ent.movetype != .NOCLIP {
		// TODO G_TouchTriggers(ent)
	}

	for i in 0 ..< pm.numtouch {
		// TODO touch	
	}

	client.oldbuttons = client.buttons
	client.buttons = i32(cmd.buttons)
	client.latched_buttons |= client.buttons & ~client.oldbuttons

	ent.light_level = i32(cmd.lightlevel)
}

PutClientInServer :: proc(ent: ^Edict) {
	spawn_point := FindEntityByClassName("info_player_start")

	if spawn_point == nil {
		gi.error("PutClientInServer: no spawn point found")
	}

	client := ent.client

	old_pers := client.pers
	mem.zero_item(client)
	client.pers = old_pers

	client.pers.connected = true

	ent.groundentity = nil
	ent.client = client
	ent.movetype = .WALK
	ent.viewheight = 22
	ent.inuse = true
	ent.classname = "player"
	ent.mass = 200
	ent.solid = .BBOX
	ent.clipmask = i32(MASK_PLAYERSOLID)
	ent.model = "players/male/tris.md2"
	ent.waterlevel = 0
	ent.watertype = 0
	ent.mins = [3]f32{-16, -16, -24}
	ent.maxs = [3]f32{16, 16, 32}

	client.ps = {}

	client.ps.pmove.origin[0] = i16(spawn_point.s.origin[0] * 8)
	client.ps.pmove.origin[1] = i16(spawn_point.s.origin[1] * 8)
	client.ps.pmove.origin[2] = i16(spawn_point.s.origin[2] * 8)
	client.ps.pmove.pm_flags &= ~u8(PMF.NO_PREDICTION)

	client.ps.fov = 90 // TODO get from userinfo

	client.ps.gunindex = 0

	ent.s.effects = 0
	ent.s.skinnum = auto_cast ent_index_from_edict(ent)
	ent.s.modelindex = 255
	ent.s.modelindex2 = 0

	ent.s.frame = 0
	ent.s.origin = spawn_point.s.origin
	ent.s.origin[2] += 1 // make sure off ground
	ent.s.angles = spawn_point.s.angles
	ent.s.old_origin = ent.s.origin

	ent.s.angles[PITCH] = 0
	ent.s.angles[YAW] = spawn_point.s.angles[YAW]
	ent.s.angles[ROLL] = 0
	ent.s.angles = client.v_angle

	gi.linkentity(ent)
}
