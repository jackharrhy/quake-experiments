package game

// NOTE these might be able to be enums?

GAME_API_VERSION: i32 = 3

TAG_GAME :: 765 // Clear when unloading the dll.
TAG_LEVEL :: 766 // Clear when loading a new level.

MAX_STATS :: 32

FRAMETIME :: 0.1 // Time between server frames in seconds.

MAX_QPATH :: 64 // Max length of a quake game pathname.
MAX_ENT_CLUSTERS :: 16

MAX_INFO_KEY :: 64
MAX_INFO_VALUE :: 64
MAX_INFO_STRING :: 512

MAX_TOUCH :: 32

FALL_TIME :: 0.3

PITCH :: 0 // up / down
YAW :: 1 // left / right
ROLL :: 2 // fall over
