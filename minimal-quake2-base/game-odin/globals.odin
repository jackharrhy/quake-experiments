package game

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

cvar_sv_cheats: ^Cvar
cvar_sv_gravity: ^Cvar
cvar_sv_rollspeed: ^Cvar
cvar_sv_rollangle: ^Cvar
cvar_run_pitch: ^Cvar
cvar_run_roll: ^Cvar
cvar_bob_up: ^Cvar
cvar_bob_pitch: ^Cvar
cvar_bob_roll: ^Cvar
