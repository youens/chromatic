# Chromatic Fun by Youens website

**https://chromatic.youens.com**

A static game collection. Each play page uses binjgb to run the same ROM
that its download button serves. There are no JavaScript rewrites of the games.
`/games/chromatic-arcade/` plays the whole anthology cartridge, launcher
included; its saves last until the page reloads.
The separate Hello Dot site remains at https://hellodot.youens.com.

## Build, preview, deploy

From the repository root:

```sh
make site
make preview
make deploy
```

`make site` copies the checked-in release ROMs, artwork, and emulator into the
ignored `collection/dist` directory, then renders the collection and game pages.
No frontend dependencies or remote fonts are required. `make games` rebuilds the
standalone cartridges before a release, and
`make -C games/arcade release` rebuilds the anthology. See each game's README for GBDK setup.

Preview uses Portless at **https://chromatic.localhost**. Production uses Workers
Static Assets in the existing Youens Cloudflare account. Wrangler manages the
`chromatic.youens.com` custom domain and HTTPS. No GitHub source files or local
toolchain files are included in the static deployment.

## Controls and behavior

Keyboard: arrows or WASD to move, X or Space for A, Z or Shift for B, Enter or P
for Start, C for Select, M for sound. The anthology page adds a Start + Select
button to return to its launcher. Touch controls are shown on every play page. Standard
mapped controllers are supported after clicking Play. Fullscreen and cartridge
restart controls are beside the sound button. A hidden browser tab suspends
emulation without skipping game time.

The page supplies server-rendered Open Graph and X large-image metadata. The
first six covers are illustrative key art; Stormkite, Comet Links and Prism
Well use pixel posters built from their own tiles. Actual screenshots appear on
the play pages. `chromatic-arcade-roms.zip` contains every standalone
cartridge, the anthology cartridge and the GBDK runtime license.

The homepage share image, `web/chromatic-share-v1.jpg`, still reads "Six little
worlds". Its text is part of the generated illustration, so it needs a new
version rather than an edit.

binjgb comes from the original Hello Dot vendor directory with its MIT license.
Cover art generation prompts are documented in each game's `art/README.md`.

Promotional artwork uses the evergreen line "One cartridge, many games".
Illustrated covers exist for the first ten games. Dreambase Invaders uses a
pixel poster from its own sprites until its illustrated key art is added (see
its `art/README.md`). Native screenshots are separately shown on each play page. The collection page plays the exact full cartridge.
