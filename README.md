# Chromatic Arcade

**[Play the collection](https://chromatic.youens.com)** · Original games for Game Boy Color and ModRetro Chromatic. One cartridge, many games.

| Game | What you do |
| --- | --- |
| [Neon Wake](games/neon-wake/) | Outrun traffic on a bending neon highway. |
| [Moonthread](games/moonthread/) | Reverse gravity through six lunar chambers. |
| [Echo Vault](games/echo-vault/) | Reveal a maze with sonar while evading listening guards. |
| [Bloom Circuit](games/bloom-circuit/) | Rotate roots to power five living circuit gardens. |
| [Orbit Choir](games/orbit-choir/) | Strike musical notes around an eight-lane orbit. |
| [Hello Dot](games/hello-dot/) | Chain sparks, dash through bugs, and make a big hello. |
| [Stormkite](games/stormkite/) | Fly a paper kite through a parallax storm and break the Thunderhead. |
| [Comet Links](games/comet-links/) | Putt a comet across nine holes where every planet bends the shot. |
| [Prism Well](games/prism-well/) | Line up falling crystals and cascade through ancient stone. |
| [Dreambase Invaders](games/dreambase-invaders/) | Eat the partner logos a lunar base fires up at you, and transform. |

Each browser player runs the exact same GBC ROM offered for download. The
collection includes a ZIP of every cartridge. The
[Chromatic Arcade cartridge](games/arcade/) puts every game behind one
launcher, with best scores saved to the cartridge. The original
[Hello Dot site](https://hellodot.youens.com) also remains available.

## Develop

```sh
./games/hello-dot/tools/setup.sh
make games
make site
make preview
```

Open the Portless URL, normally **https://chromatic.localhost**. Production uses
Cloudflare Workers Static Assets; `make deploy` publishes the checked-in ROMs
and current site. See [collection/README.md](collection/README.md).

`make test` plays every shared-runtime cartridge in PyBoy (including full
campaigns of Stormkite, Comet Links and Prism Well), runs the existing Hello
Dot checks, and plays all eight levels of Dreambase Invaders.
`make -C games/arcade release` builds and tests the anthology cartridge. Install the pinned test dependencies using the setup documented in
[Hello Dot's README](games/hello-dot/README.md).

## Hardware research

- [Capabilities](docs/chromatic-capabilities.md): display, CPU, graphics, sound,
  connectivity, storage, firmware, and limits.
- [Development and loading](docs/development.md): toolchains, Mac workflow,
  live demos and cartridge options.
- [Original project ideas](docs/project-ideas.md).
- [Research sources](docs/research-sources.md).
- [Collection design notes](docs/collection-design.md).
- [Emulator validation results](docs/collection-validation.json).

The ROMs are tested in emulators. Physical Chromatic testing and the exact
DevDay cartridge bundle remain unconfirmed.
