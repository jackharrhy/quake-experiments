package game

import "base:runtime"
import "core:fmt"

ClientEndServerFrame :: proc(ent: ^Edict) {
	context = runtime.default_context()

	for i in 0 ..< 3 {
		ent.client.ps.pmove.origin[i] = i16(ent.s.origin[i] * 8.0)
		ent.client.ps.pmove.velocity[i] = i16(ent.velocity[i] * 8.0)
	}

	// TODO the players view
}
