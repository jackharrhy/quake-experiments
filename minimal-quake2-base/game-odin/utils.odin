package game

import "core:fmt"

// Takes in an edict reference and returns the index of the edict in the g_edicts array.
//
// Quake II did this by some pointer arithmetic, ew!, so we're doing it the same way.
ent_index_from_edict :: proc "c" (ent: ^Edict) -> int {
	offset := uintptr(rawptr(ent)) - uintptr(rawptr(&g_edicts[0]))
	return int(offset / size_of(Edict))
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
