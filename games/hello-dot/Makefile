GBDK_HOME ?= ../../.tools/gbdk
LCC := $(GBDK_HOME)/bin/lcc
PYTHON ?= python3
BUILD := build
ROM := $(BUILD)/hello-dot.gbc

.PHONY: all clean test preview play release deploy
all: $(ROM) web/hello-dot.gbc

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/assets.h: tools/assets.py | $(BUILD)
	$(PYTHON) tools/assets.py $@

$(ROM): src/main.c src/game.c src/game.h $(BUILD)/assets.h
	$(LCC) -Wm-yC -Wm-yt0 -Wm-yo2 -Wm-yn"HELLO DOT" -Wl-m -Wl-j -I$(BUILD) -o $@ src/main.c src/game.c

web/hello-dot.gbc: $(ROM)
	cp $(ROM) $@

test: all
	$(CC) -std=c99 -Wall -Wextra -Werror -Wno-error=sign-compare -Isrc src/game.c tests/game_test.c -o $(BUILD)/game-test
	$(BUILD)/game-test
	$(PYTHON) tools/check_rom.py $(ROM)

preview: all
	portless run --name hello-dot node tools/server.mjs

play:
	cp dist/hello-dot.gbc web/hello-dot.gbc
	portless run --name hello-dot node tools/server.mjs

release: test
	mkdir -p dist
	cp $(ROM) dist/hello-dot.gbc
	cd dist && shasum -a 256 hello-dot.gbc > SHA256SUMS

deploy:
	$(PYTHON) tools/check_rom.py dist/hello-dot.gbc
	cp dist/hello-dot.gbc web/hello-dot.gbc
	wrangler deploy

clean:
	rm -rf $(BUILD) web/hello-dot.gbc
