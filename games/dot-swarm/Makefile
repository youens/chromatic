GBDK_HOME ?= ../../.tools/gbdk
PYTHON ?= python3
ROM := build/dot-swarm.gbc
.PHONY: all release test
all: $(ROM)
build/assets.h: tools/assets.py ../shared/assets.py
	mkdir -p build
	$(PYTHON) tools/assets.py $@
$(ROM): src/main.c src/runtime.c ../shared/runtime.h build/assets.h
	$(GBDK_HOME)/bin/lcc -Wm-yC -Wm-yt0 -Wm-yo2 -Wm-yn"DOT SWARM" -Wl-m -Wl-j -I../shared -Ibuild -o $@ src/runtime.c src/main.c
release: all
	mkdir -p dist
	cp $(ROM) dist/dot-swarm.gbc
	cd dist && shasum -a 256 dot-swarm.gbc > SHA256SUMS
test: all
	../../.tools/venv/bin/python tools/test.py
