package game

import "base:runtime"
import "core:fmt"
import "core:mem"
import "core:os/os2"
import "core:strconv"
import "core:strings"
import "core:unicode"

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

	globals.RunFrame = RunFrame

	globals.ServerCommand = ServerCommand

	globals.edict_size = size_of(Edict)

	return &globals
}
