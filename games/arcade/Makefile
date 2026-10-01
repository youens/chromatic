# Every game the cartridge links. Building their ROMs is enough: releasing
# them would run their tests, which rewrite tracked screenshots.
GAMES := neon-wake moonthread echo-vault bloom-circuit orbit-choir stormkite comet-links prism-well dot-swarm hello-dot dreambase-invaders dotwing
.PHONY: all test release
all:
	@for game in $(GAMES); do "$(MAKE)" -C ../$$game all || exit $$?; done
	python3 tools/build.py
test: all
	../../.tools/venv/bin/python tools/test.py
release: test
	mkdir -p dist art
	cp build/chromatic-arcade.gbc dist/chromatic-arcade.gbc
	cp build/launcher-intro.png build/launcher-shelf.png build/launcher-info.png build/launcher-records.png art/
	cd dist && shasum -a 256 chromatic-arcade.gbc > SHA256SUMS
