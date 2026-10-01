GBDK_HOME ?= ../../.tools/gbdk
PYTHON ?= python3
LCC := $(GBDK_HOME)/bin/lcc
ROM := build/dotwing.gbc
SOURCES := src/main.c src/screens.c src/play.c src/art.c src/fixed.c src/bridge.c
.PHONY: all release test clean
all: $(ROM)
build/art.stamp: tools/assets.py ../hello-dot/tools/assets.py
	mkdir -p build
	$(PYTHON) tools/assets.py
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
