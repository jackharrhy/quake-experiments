# quake experiments - minimal-quake2-base

state:

- still pulling in files from pak0 as needed, not yet overwritten with alternatives
- no sounds / models, just _some_ custom images
- odin code has a working player controller, still some things missing that are _needed_, like disconnect / server commands / user info / save / loads

todo

- sounds, player jump, player fall
- player model
- player animations

goals:

- blank canvas for a prospective quake engineer
- copyright free base that boots Yamagi Quake II with a modified game dll, that throws little to no errors about missing files
- game dll written in odin
- player, in a box, running around
- dedicated server works
- work well on linux/macos/windows
- target opengl 3.2 renderer for visual consistency, and support (thanks macos)
- possible to create something epic with this as its base
- just because its an old engine, it should not feel old / low quality
- easy to run build script that works multiplatform, shell would be fine if not for windows, python is a perfectly fine choice

non goals:

- work well on old computers
- graphics anything more complex than default dev textures
- telling you how to structure your game, just giving you a starting point on how to produce a game dll that _works_, with the basics you expect (physics, networking, etc.)

---

`game-c` is based off of [yquake2/ctf](https://github.com/yquake2/ctf/tree/c7a4b27bf67c9b09fe906bfb2263ff9f66fb57b6)

it was used as a test to see _how much_ you can rip away from the game until it stops compiling / working.

its (mostly) surved its purpose, might get moved to a parent directory at some point since it has no real path
forward here, but it was _very_ useful for getting started!

---

While this minimal base isn't supposed to contain much graphics / vibes, I still want some level of art direction for _some_ vibes.

Color scheme: https://lospec.com/palette-list/endesga-32

- backdrop: #181425
- menu backdrop: #3a4466

Font: https://fonts.google.com/specimen/DynaPuff
