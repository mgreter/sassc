#!/usr/bin/env node --experimental-wasm-bigint

'use strict';

const fs = require('fs');
const path = require('path');
const WASI = require('wasi');

const wasmPath = path.join(__dirname, "../bin/sassc.wasm");
const bin = fs.readFileSync(wasmPath);

const mod = new WebAssembly.Module(bin);

const wasi = new WASI({
  preopenDirectories: { ".": "." },
  args: [wasmPath, ...process.argv.slice(2)],
});
const instance = new WebAssembly.Instance(mod, {
  wasi_unstable: wasi.exports,
});

wasi.setMemory(instance.exports.memory);

instance.exports._start();
