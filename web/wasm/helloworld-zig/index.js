import { WASI, File, OpenFile, ConsoleStdout, PreopenDirectory } from "https://unpkg.com/@bjorn3/browser_wasi_shim@0.4.2/dist/index.js";

let args = ["bin", "arg1", "arg2"];
let env = ["FOO=bar"];
let wasi = new WASI(args, env, [
    new OpenFile(new File([])), // stdin
    ConsoleStdout.lineBuffered(msg => console.log(`[WASI stdout] ${msg}`)),
    ConsoleStdout.lineBuffered(msg => console.warn(`[WASI stderr] ${msg}`)),
]);

let wasm = await WebAssembly.compileStreaming(fetch("build/main.wasm"));
let inst = await WebAssembly.instantiate(wasm, {
    "wasi_snapshot_preview1": wasi.wasiImport,
});
wasi.start(inst);
