.PHONY: all test release
all:
	"$(MAKE)" -C ../.. games
	"$(MAKE)" -C ../hello-dot all
	"$(MAKE)" -C ../dot-swarm all
	"$(MAKE)" -C ../dreambase-invaders all
	python3 tools/build.py
test: all
	../../.tools/venv/bin/python tools/test.py
release: test
	mkdir -p dist art
	cp build/chromatic-arcade.gbc dist/chromatic-arcade.gbc
	cp build/launcher-intro.png build/launcher-shelf.png build/launcher-info.png build/launcher-records.png art/
	cd dist && shasum -a 256 chromatic-arcade.gbc > SHA256SUMS
