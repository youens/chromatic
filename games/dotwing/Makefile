GBDK_HOME ?= ../../.tools/gbdk
PYTHON ?= $(if $(wildcard ../../.tools/venv/bin/python),../../.tools/venv/bin/python,python3)
LCC := $(GBDK_HOME)/bin/lcc
ROM := build/dotwing.gbc
SOURCES := src/main.c src/screens.c src/play.c src/rivals.c src/art.c src/fixed.c src/world.c src/terraina.c src/terrainb.c src/vbl.c
.PHONY: all release test clean
all: $(ROM)
build/art.stamp: $(filter-out tools/test.py,$(wildcard tools/*.py))
	mkdir -p build
	$(PYTHON) tools/assets.py
	$(PYTHON) tools/music.py
	touch $@
$(ROM): $(SOURCES) src/dotwing.h src/art_ids.h build/art.stamp
	$(LCC) -Wf--opt-code-size -Wm-yC -Wm-yt0x1B -Wm-yo8 -Wm-ya1 -Wm-ynDOTWING -Wl-m -Wl-j -Isrc -Ibuild -o $@ $(SOURCES)
release: all
	mkdir -p dist
	cp $(ROM) dist/dotwing.gbc
	cd dist && shasum -a 256 dotwing.gbc > SHA256SUMS
test: all
	../../.tools/venv/bin/python tools/test.py
clean:
	rm -rf build
