# Building

To build `../bin/sassc.wasm`:

* Clone https://github.com/yurydelendik/sassc into parent (libsass) folder
* Run `make`

# Running

Prerequisites:
* node v12 (e.g. via `nvm use 12`).
* install "node-wasi" dependency via `npm i` in the "wasm" directory.

Examples:

```
node --experimental-wasm-bigint cli.js -v
node --experimental-wasm-bigint cli.js ./test.sass
node --experimental-wasm-bigint cli.js ./test.sass ./test.css
```

Note: Use "./" path for the file names

# wasi-sdk prerequisite

Clone, build and install the [wask-sdk](https://github.com/CraneStation/wasi-sdk), e.g. `make PREFIX=~/bin/wasi-sdk/ LLVM_PROJ_DIR=~/Work/llvm-project/`

Note: I used the commit `bdaa39ea6ca4` of the llvm-project, since the older versions (e.g. LLVM 8 included by default in wasi-sdk) when used causing the "std::__2::random_device::random_device" error.
