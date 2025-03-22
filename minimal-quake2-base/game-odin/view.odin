package game

import "base:runtime"
import "core:fmt"
import "core:math"
import "core:math/linalg"

forward, right, up: [3]f32
xyspeed: f32

bobmove: f32
bobcycle: i32 // odd cycles are right foot going forward
bobfracsin: f32 // sin(bobfrac*M_PI)

AngleVectors :: proc(angles: [3]f32, forward, right, up: ^[3]f32) {
	// Convert degrees to radians
	yaw := angles.y * math.DEG_PER_RAD
	pitch := angles.x * math.DEG_PER_RAD
	roll := angles.z * math.DEG_PER_RAD

	// Calculate sine and cosine of angles
	sy, cy := math.sin(yaw), math.cos(yaw)
	sp, cp := math.sin(pitch), math.cos(pitch)
	sr, cr := math.sin(roll), math.cos(roll)

	if forward != nil {
		forward.x = cp * cy
		forward.y = cp * sy
		forward.z = -sp
	}

	if right != nil {
		right.x = (-sr * sp * cy + -cr * -sy)
		right.y = (-sr * sp * sy + -cr * cy)
		right.z = -sr * cp
	}

	if up != nil {
		up.x = (cr * sp * cy + -sr * -sy)
		up.y = (cr * sp * sy + -sr * cy)
		up.z = cr * cp
	}
}

SV_CalcRoll :: proc(angles: [3]f32, velocity: [3]f32) -> f32 {
	sign: f32
	side: f32
	value: f32

	side = linalg.dot(velocity, right)
	sign = side < 0 ? -1 : 1
	side = math.abs(side)

	value = cvar_sv_rollangle.value

	if side < cvar_sv_rollspeed.value {
		side = side * value / cvar_sv_rollspeed.value
	} else {
		side = value
	}

	return side * sign
}

SV_CalcViewOffset :: proc(ent: ^Edict) {
	angles: ^[3]f32
	bob: f32
	ratio: f32
	delta: f32
	v: [3]f32

	// base angles
	angles = &ent.client.ps.kick_angles

	// add angles based on weapon kick
	angles^ = ent.client.kick_angles

	// add pitch based on fall kick
	ratio = (ent.client.fall_time - g_level.time) / FALL_TIME

	if ratio < 0 {
		ratio = 0
	}

	angles[PITCH] += ratio * ent.client.fall_value

	// add angles based on velocity
	delta = linalg.dot(ent.velocity, forward)
	angles[PITCH] += delta * cvar_run_pitch.value

	delta = linalg.dot(ent.velocity, right)
	angles[ROLL] += delta * cvar_run_roll.value

	// add angles based on bob
	delta = bobfracsin * cvar_bob_pitch.value * xyspeed

	if (ent.client.ps.pmove.pm_flags & u8(PMF.DUCKED)) != 0 {
		delta *= 6 // crouching
	}

	angles[PITCH] += delta
	delta = bobfracsin * cvar_bob_roll.value * xyspeed

	if (ent.client.ps.pmove.pm_flags & u8(PMF.DUCKED)) != 0 {
		delta *= 6 // crouching
	}

	if (bobcycle & 1) != 0 {
		delta = -delta
	}

	angles[ROLL] += delta

	// base origin
	v = {0, 0, 0}

	// add view height
	v[2] += f32(ent.viewheight)

	// add fall height
	ratio = (ent.client.fall_time - g_level.time) / FALL_TIME

	if ratio < 0 {
		ratio = 0
	}

	v[2] -= ratio * ent.client.fall_value * 0.4

	// add bob height
	bob = bobfracsin * xyspeed * cvar_bob_up.value

	if bob > 6 {
		bob = 6
	}

	v[2] += bob

	// add kick offset
	v = v + ent.client.kick_origin

	// absolutely bound offsets so the view can never be outside the player box
	if v[0] < -14 {
		v[0] = -14
	} else if v[0] > 14 {
		v[0] = 14
	}

	if v[1] < -14 {
		v[1] = -14
	} else if v[1] > 14 {
		v[1] = 14
	}

	if v[2] < -22 {
		v[2] = -22
	} else if v[2] > 30 {
		v[2] = 30
	}

	ent.client.ps.viewoffset = v
}

SV_AddBlend :: proc(r, g, b, a: f32, v_blend: ^[4]f32) {
	a2, a3: f32

	if a <= 0 {
		return
	}

	a2 = v_blend[3] + (1 - v_blend[3]) * a // new total alpha
	a3 = v_blend[3] / a2 // fraction of color from old

	v_blend[0] = v_blend[0] * a3 + r * (1 - a3)
	v_blend[1] = v_blend[1] * a3 + g * (1 - a3)
	v_blend[2] = v_blend[2] * a3 + b * (1 - a3)
	v_blend[3] = a2
}

G_SetClientEvent :: proc(ent: ^Edict) {
	current_client := ent.client

	if ent.s.event != 0 {
		return
	}

	if ent.groundentity != nil && (xyspeed > 225) {
		if i32(current_client.bobtime + bobmove) != bobcycle {
			ent.s.event = i32(Entity_Event.FOOTSTEP)
		}
	}
}

G_SetClientFrame :: proc(ent: ^Edict) {
	duck, run: bool

	if ent.s.modelindex != 255 {
		return // not in the player model
	}

	client := ent.client

	if (client.ps.pmove.pm_flags & u8(PMF.DUCKED)) != 0 {
		duck = true
	} else {
		duck = false
	}

	if xyspeed != 0 {
		run = true
	} else {
		run = false
	}

	// check for stand/duck and stop/go transitions
	needs_new_anim := false

	/*
	if (duck != client.anim_duck) && (client.anim_priority < ANIM_DEATH) {
		needs_new_anim = true
	}

	if (run != client.anim_run) && (client.anim_priority == ANIM_BASIC) {
		needs_new_anim = true
	}

	if ent.groundentity == nil && (client.anim_priority <= ANIM_WAVE) {
		needs_new_anim = true
	}

	if !needs_new_anim {
		if client.anim_priority == ANIM_REVERSE {
			if ent.s.frame > client.anim_end {
				ent.s.frame -= 1
				return
			}
		} else if ent.s.frame < client.anim_end {
			// continue an animation
			ent.s.frame += 1
			return
		}

		if client.anim_priority == ANIM_DEATH {
			return // stay there
		}

		if client.anim_priority == ANIM_JUMP {
			if ent.groundentity == nil {
				return // stay there
			}

			ent.client.anim_priority = ANIM_WAVE
			ent.s.frame = FRAME_jump3
			ent.client.anim_end = FRAME_jump6
			return
		}

		// If we get here, we need a new animation
		needs_new_anim = true
	}

	// Handle new animation
	if needs_new_anim {
		// return to either a running or standing frame
		client.anim_priority = ANIM_BASIC
		client.anim_duck = duck
		client.anim_run = run

		if ent.groundentity == nil {
			client.anim_priority = ANIM_JUMP

			if ent.s.frame != FRAME_jump2 {
				ent.s.frame = FRAME_jump1
			}

			client.anim_end = FRAME_jump2
		} else if run {
			// running
			if duck {
				ent.s.frame = FRAME_crwalk1
				client.anim_end = FRAME_crwalk6
			} else {
				ent.s.frame = FRAME_run1
				client.anim_end = FRAME_run6
			}
		} else {
			// standing
			if duck {
				ent.s.frame = FRAME_crstnd01
				client.anim_end = FRAME_crstnd19
			} else {
				ent.s.frame = FRAME_stand01
				client.anim_end = FRAME_stand40
			}
		}
	}
	*/
}

ClientEndServerFrame :: proc(ent: ^Edict) {
	context = runtime.default_context()
	bobtime: f32

	current_player := ent
	current_client := ent.client

	// If the origin or velocity have changed since ClientThink(),
	// update the pmove values. This will happen when the client
	// is pushed by a bmodel or kicked by an explosion.
	// If it wasn't updated here, the view position would lag a frame
	// behind the body position when pushed -- "sinking into plats"
	for i in 0 ..< 3 {
		ent.client.ps.pmove.origin[i] = i16(ent.s.origin[i] * 8.0)
		ent.client.ps.pmove.velocity[i] = i16(ent.velocity[i] * 8.0)
	}

	// Calculate view vectors
	AngleVectors(ent.client.v_angle, &forward, &right, &up)

	// set model angles from view angles so other things in
	// the world can tell which direction you are looking
	if ent.client.v_angle[PITCH] > 180 {
		ent.s.angles[PITCH] = (-360 + ent.client.v_angle[PITCH]) / 3
	} else {
		ent.s.angles[PITCH] = ent.client.v_angle[PITCH] / 3
	}

	ent.s.angles[YAW] = ent.client.v_angle[YAW]
	ent.s.angles[ROLL] = 0
	ent.s.angles[ROLL] = SV_CalcRoll(ent.s.angles, ent.velocity) * 4

	// calculate speed and cycle to be used for all cyclic walking effects
	xyspeed = math.sqrt(ent.velocity[0] * ent.velocity[0] + ent.velocity[1] * ent.velocity[1])

	if xyspeed < 5 {
		bobmove = 0
		current_client.bobtime = 0 // start at beginning of cycle again
	} else if ent.groundentity != nil {
		// so bobbing only cycles when on ground
		if xyspeed > 210 {
			bobmove = 0.25
		} else if xyspeed > 100 {
			bobmove = 0.125
		} else {
			bobmove = 0.0625
		}
	}

	current_client.bobtime += bobmove
	bobtime = current_client.bobtime

	if (current_client.ps.pmove.pm_flags & u8(PMF.DUCKED)) != 0 {
		bobtime *= 4
	}

	bobcycle = i32(bobtime)
	bobfracsin = math.abs(math.sin(bobtime * math.PI))

	// determine the view offsets
	SV_CalcViewOffset(ent)

	G_SetClientEvent(ent)

	G_SetClientFrame(ent)

	ent.client.oldvelocity = ent.velocity
	ent.client.oldviewangles = ent.client.ps.viewangles

	ent.client.kick_origin = {0, 0, 0}
	ent.client.kick_angles = {0, 0, 0}
}
