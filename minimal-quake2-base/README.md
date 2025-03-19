# quake experiments - minimal-quake2-base

goals:

- blank canvas for a prospective quake engineer
- copyright free base that boots Yamagi Quake II with a modified game dll, that throws little to no errors about missing files
- game dll written in odin
- player, in a box, running around
- dedicated server works
- menus work, even if they don't look pretty
- work well on linux/macos/windows
- target vulkan renderer for visual consistency
- possible to create something epic with this as its base
- just because its an old engine, it should not feel old / low quality
- easy to run build script that works multiplatform, shell would be fine if not for windows, python is a perfectly fine choice

non goals:

- work well on old computers
- graphics anything more complex than default dev textures
- telling you how to structure your game, just giving you a starting point on how to produce a game dll that _works_, with the basics you expect (physics, networking, etc.)
