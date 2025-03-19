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

---

`game-c` is based off of [yquake2/ctf](https://github.com/yquake2/ctf/tree/c7a4b27bf67c9b09fe906bfb2263ff9f66fb57b6)

the plan is, to tear game-c down to its _core_, and then rewrite that into `game-odin`

`game-odin`, currently _boots_ and does not crash, but it does very little... for now

why ctf over the base game? honestly i don't know, in my brain ctf was maybe something that came out afterwards maybe with some patches,
and felt like a better choice than the expansions for something that would be played multiplayer

but none of the above is based on facts, only vibes
