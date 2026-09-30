GAME := comet-links
TITLE := COMET LINKS
include ../shared/game.mk
$(ROM): build/course.h
build/course.h: tools/course.py ../shared/worlds.py | build
	$(PYTHON) tools/course.py $@
