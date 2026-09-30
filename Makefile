GAMES := dot-swarm neon-wake moonthread echo-vault bloom-circuit orbit-choir stormkite comet-links prism-well
.PHONY: all games test site preview deploy
all: games site
games:
	@for game in $(GAMES); do "$(MAKE)" -C games/$$game release || exit $$?; done
site:
	"$(MAKE)" -C collection build
test:
	.tools/venv/bin/python tools/test_collection.py
	"$(MAKE)" -C games/hello-dot test
preview:
	"$(MAKE)" -C collection preview
deploy:
	"$(MAKE)" -C collection deploy
