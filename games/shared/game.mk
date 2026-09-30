GBDK_HOME ?= ../../.tools/gbdk
PYTHON ?= python3
LCC := $(GBDK_HOME)/bin/lcc
ROM := build/$(GAME).gbc
.PHONY: all release clean
all: $(ROM)
build:
	mkdir -p build
build/assets.h: ../shared/assets.py ../shared/worlds.py ../hello-dot/tools/assets.py | build
	$(PYTHON) ../shared/assets.py $(GAME) $@
$(ROM): src/main.c $(EXTRA) ../shared/runtime.c ../shared/runtime.h ../shared/digits.h build/assets.h
	$(LCC) -Wm-yC -Wm-yt0 -Wm-yo2 -Wm-yn"$(TITLE)" -Wl-m -Wl-j -I../shared -Ibuild -o $@ ../shared/runtime.c $(EXTRA) src/main.c
release: all
	mkdir -p dist
	cp $(ROM) dist/$(GAME).gbc
	cd dist && shasum -a 256 $(GAME).gbc > SHA256SUMS
clean:
	rm -rf build
