function make_environment(...envs) {
    return new Proxy(envs, {
        get(target, prop, receiver) {
            for (let env of envs) {
                if (env.hasOwnProperty(prop)) {
                    return env[prop];
                }
            }
            return (...args) => {console.error("NOT IMPLEMENTED: "+prop, args)}
        }
    });
}


function readString(memory, offset) {
    const mem = new Uint8Array(memory.buffer);
    let end = offset;
    while (mem[end] !== 0) end++;
    const bytes = mem.subarray(offset, end);
    return new TextDecoder("utf-8").decode(bytes);
}

/**
 * @param {string} path - The path to the WebAssembly (.wasm) file
 */
function runWasm(path) {
    let w = null

    const libjs = {
        "console_log": (strPtr) => {
            // strPtr is the pointer to the string in WebAssembly memory
            const str = readString(w.instance.exports.memory, strPtr);
            console.log(str);
        },
    }

    WebAssembly.instantiateStreaming(fetch(path), {
        "env": make_environment(libjs)
    }).then(wasm => {
        w = wasm // attach w actual wasm handle, allow console_log to reference
        wasm.instance.exports.main(0, 0)
    })
}
