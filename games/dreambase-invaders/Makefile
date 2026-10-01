GBDK_HOME ?= ../../.tools/gbdk
PYTHON ?= python3
LCC := $(GBDK_HOME)/bin/lcc
ROM := build/dreambase-invaders.gbc
SOURCES := src/main.c src/fixed.c src/screens.c src/play.c src/art.c
.PHONY: all release test clean
all: $(ROM)
build/assets.h: tools/assets.py tools/art_data.py
	mkdir -p build
	$(PYTHON) tools/assets.py $@
$(ROM): $(SOURCES) src/dreambase.h build/assets.h
	$(LCC) -Wf--opt-code-size -Wm-yC -Wm-yt0 -Wm-yo2 -Wm-yn"DREAMBASE INV" -Wl-m -Wl-j -Isrc -Ibuild -o $@ $(SOURCES)
release: test
	mkdir -p dist
	cp $(ROM) dist/dreambase-invaders.gbc
	cd dist && shasum -a 256 dreambase-invaders.gbc > SHA256SUMS
test: all
	../../.tools/venv/bin/python tools/test.py
clean:
	rm -rf build
