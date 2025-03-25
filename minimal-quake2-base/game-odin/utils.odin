package game

import "core:fmt"
import "core:math"
import "core:strings"

// Takes in an edict reference and returns the index of the edict in the g_edicts array.
//
// Quake II did this by some pointer arithmetic, ew!, so we're doing it the same way.
ent_index_from_edict :: proc "c" (ent: ^Edict) -> int {
	offset := uintptr(rawptr(ent)) - uintptr(rawptr(&g_edicts[0]))
	return int(offset / size_of(Edict))
}

// Finds an entity by its classname.
//
// Returns nil if no entity is found.
find_entity_by_classname :: proc(match: string) -> ^Edict {
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

send_muzzle_flash :: proc "c" (ent: ^Edict, muzzle_flash: Muzzle_Flash) {
	gi.WriteByte(i32(Svc.MUZZLEFLASH))
	gi.WriteShort(i32(ent_index_from_edict(ent)))
	gi.WriteByte(i32(muzzle_flash))
	gi.multicast(raw_data(ent.s.origin[:]), .PVS)
}

angle_to_short :: proc "c" (x: f32) -> i32 {
	return i32((x * 65536.0 / 360.0)) & 65535
}

short_to_angle :: proc "c" (x: i32) -> f32 {
	return f32(x) * (360.0 / 65536.0)
}

angle_vectors :: proc(angles: [3]f32, forward, right, up: ^[3]f32) {
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

error_log :: proc(text: string, args: ..any) {
	fmt.printfln("\x1b[31m%s\x1b[0m", fmt.tprintf(text, ..args))
}

info_log :: proc(text: string, args: ..any) {
	fmt.printfln("\x1b[32m%s\x1b[0m", fmt.tprintf(text, ..args))
}

debug_edicts :: proc() {
	debug_log("=== DEBUG EDICTS ===\n")

	for i in 0 ..< globals.num_edicts {
		e := &g_edicts[i]

		index := ent_index_from_edict(e)

		if !e.inuse {
			continue
		}

		classname := "NO CLASSNAME"
		if e.classname != "" {
			classname = string(e.classname)
		}

		debug_log("Edict %d: classname=%s pointer=%p index=%d", i, classname, e, index)
	}

	debug_log("\n=== END DEBUG EDICTS ===")
}
