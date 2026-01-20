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

function read_string(memory, offset) {
    const mem = new Uint8Array(memory.buffer);
    let end = offset;
    while (mem[end] !== 0) end++;
    const bytes = mem.subarray(offset, end);
    return new TextDecoder("utf-8").decode(bytes);
}

function write_string(memory, offset, str) {
    const mem = new Uint8Array(memory.buffer);
    const bytes = new TextEncoder("utf-8").encode(str);
    mem.set(bytes, offset);
    mem[offset + bytes.length] = 0;
    return bytes.length + 1;
}

function write_u32(memory, offset, value) {
    const mem = new Uint32Array(memory.buffer, offset, 1);
    mem[0] = value;
    return 4;
}


function setup_crt(memory, offset, args) {
    const argc = args.length;
    let current = offset;

    // 1. write argv[x] * argc
    const argv_ptrs = [];
    for (const arg of args) {
        argv_ptrs.push(current);

        const bytes_written = write_string(memory, current, arg);
        current += bytes_written;
    }

    // 2. write argv itself
    // Align currentOffset to nearest multiple of 4
    current = (current + 3) & ~3;
    const argv = current // save start pointer
    for (const arg of argv_ptrs) {
        const bytes_written = write_u32(memory, current, arg);
        current += bytes_written;
    }

    return {argc, argv};
}

/**
 * @param {string} path - The path to the WebAssembly (.wasm) file
 */
async function runWasm(path) {
    let inst = null

    const libjs = {
        "console_log": (strPtr) => {
            const str = read_string(inst.exports.memory, strPtr);
            console.log(str);
        },
    };

    inst = (await WebAssembly.instantiateStreaming(fetch(path), {
        "env": make_environment(libjs)
    })).instance

    const memory = inst.exports.memory;
    const jsruntime_ptr = inst.exports.jsruntime;

    const args = [path, "arg1", "arg2", "arg3"];
    const {argc, argv} = setup_crt(memory, jsruntime_ptr, args);

    const result = inst.exports.main(argc, argv);
    return result;
}


export { runWasm }
