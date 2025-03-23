package game

import "base:runtime"
import "core:mem"

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
		pm.state.velocity[i] = i16(ent.velocity[i] * 8)
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
		// ent.groundentity_linkcount = pm.groundentity.linkcount
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
