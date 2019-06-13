WASI_SDK ?= ~/bin/wasi-sdk
MAKE_OPTS = \
	CC=$(WASI_SDK)/bin/clang \
	CXX=$(WASI_SDK)/bin/clang++ \
	AR=$(WASI_SDK)/bin/llvm-ar \
	WASM=1

build:
	$(MAKE) -C ../.. $(MAKE_OPTS) sassc

clean:
	$(MAKE) -C ../.. $(MAKE_OPTS) clean-all
	rm -rf pack/

pack: build
	mkdir -p pkg/
	cp package.json.in pkg/package.json
	cp README.md.in pkg/README.md
	mkdir -p pkg/wasm/
	cp cli.js pkg/wasm/
	mkdir -p pkg/bin/
	cp ../bin/sassc.wasm pkg/bin/

.PHONY: build clean pack
