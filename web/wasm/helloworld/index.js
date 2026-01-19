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

function writeString(memory, offset, str) {
    const mem = new Uint8Array(memory.buffer);
    const bytes = new TextEncoder("utf-8").encode(str);
    mem.set(bytes, offset);
    mem[offset + bytes.length] = 0;
    return bytes.length + 1;
}


function setup_argc_argv(memory, jsruntimePtr, args) {
    const argc = args.length;
    const argvPtrs = [];
    let currentOffset = jsruntimePtr;

    // Write each argument string to memory
    for (const arg of args) {
        const bytesWritten = writeString(memory, currentOffset, arg);
        argvPtrs.push(currentOffset);
        currentOffset += bytesWritten;
    }

    // Create argv array
    // Align currentOffset to nearest multiple of 4
    const argvArrayOffset = (currentOffset + 3) & ~3;
    const argvArray = new Uint32Array(memory.buffer, argvArrayOffset, argc + 1);

    for (let i = 0; i < argc; i++) {
        argvArray[i] = argvPtrs[i];
    }
    argvArray[argc] = 0; // NULL terminator

    return { argc, argvArrayOffset };
}

/**
 * @param {string} path - The path to the WebAssembly (.wasm) file
 */
function runWasm(path) {
    let wasmInstance = null

    const libjs = {
        "console_log": (strPtr) => {
            const str = readString(wasmInstance.exports.memory, strPtr);
            console.log(str);
        },
    };

    WebAssembly.instantiateStreaming(fetch(path), {
        "env": make_environment(libjs)
    }).then(wasm => {
        wasmInstance = wasm.instance;
        const memory = wasmInstance.exports.memory;
        const jsruntimePtr = wasmInstance.exports.jsruntime;

        const args = [path, "arg1", "arg2", "arg3"];
        const { argc, argvArrayOffset } = setup_argc_argv(memory, jsruntimePtr, args);

        // Call main function
        const result = wasmInstance.exports.main(argc, argvArrayOffset);
    });
}
