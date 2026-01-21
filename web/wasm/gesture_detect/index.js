import { WASI, File, OpenFile, ConsoleStdout, PreopenDirectory } from "@bjorn3/browser_wasi_shim";

let args = ["bin", "arg1", "arg2"];
let env = ["FOO=bar"];
let wasi = new WASI(args, env, [
    new OpenFile(new File([])), // stdin
    ConsoleStdout.lineBuffered(msg => console.log(`[WASI stdout] ${msg}`)),
    ConsoleStdout.lineBuffered(msg => console.warn(`[WASI stderr] ${msg}`)),
]);

let inst = (await WebAssembly.instantiateStreaming(fetch("build/main.wasm"), {
    "wasi_snapshot_preview1": wasi.wasiImport,
})).instance;
wasi.start(inst);

