.PHONY: build preview deploy
build:
	python3 ../tools/build_collection.py
preview: build
	portless run --name chromatic node server.mjs
deploy: build
	wrangler deploy
