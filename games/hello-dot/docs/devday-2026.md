# DevDay 2026: the inspiration for Hello Dot

Researched September 29, 2026, using OpenAI's launch pages and the live character chooser. These are announcement-day notes, not a promise of availability on every account.

## What was announced today

The [official DevDay recap](https://openai.com/index/devday-2026-recap/) covers more than 20 announcements. Highlights include Dots, GPT-6.1 Sol, the Ultrafast speed tier, Private Intelligence, cloud Codex and a refreshed CLI, code review and security tooling, the Decisions API, computer use in the Agents API, plugin extensions and event-driven automations, and collaborative spaces, pages, and slides in ChatGPT. The recap links each announcement and describes rollout limits.

## Which Dot we used

[Introducing dots](https://openai.com/index/introducing-dots/) presents Dots as ongoing agents with distinct, colorful characters. We inspected the actual launch artwork in the browser, including its “Choose your dot character” control. The page's imagery identifies pink Iggy, blue Felipe, green Todd, yellow Alfred, and purple Jojo.

Our reference is **Alfred**: a soft yellow triangular character with round dark glasses and a bow tie. We drew a new 16 × 16 pixel interpretation for the Game Boy Color. The game uses a warm yellow player, mint sparks, pink bugs, and lavender accents on a dark field. These are our game palette choices, not a claim that they are official brand color specifications.

The [Dot getting-started guide](https://help.openai.com/en/articles/20001530-getting-started-with-your-dot) supplies product context. [OpenAI's brand page](https://openai.com/brand/) is the general brand reference. Hello Dot is an independent fan game, not an official OpenAI game or integration. It makes no AI requests and works entirely offline once the ROM is loaded.

## Translating the character into a game

The friendly character and “hello” theme suggested a welcoming arcade loop: gather sparks, make connections quickly, and turn a full charge into a celebratory burst. Glitches are simple pink bugs, and dashing turns them from hazards into bonus points. Sixty-second rounds make it easy to pass around a handheld and compare scores.

No launch images, proprietary fonts, or audio were downloaded into the game. Pixel graphics are generated from our editable patterns in `tools/assets.py`; the melody and effects are programmed in `src/main.c`.
