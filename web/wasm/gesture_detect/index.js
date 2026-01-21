import { WASI, File, OpenFile, ConsoleStdout, PreopenDirectory } from "@bjorn3/browser_wasi_shim";


// 1. init crt
// =================================

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

// 2. init canvas
// =================================

const BACKGROUND = "#101010"
const FOREGROUND = "#50FF50"
const POINT_SIZE = 15

const screen = document.getElementById('screen');
screen.width = 800
screen.height = 600

const ctx = screen.getContext("2d") // similiar to glfwMakeContextCurrent?
ctx.fillStyle = BACKGROUND
ctx.fillRect(0, 0, screen.width, screen.height); // Fill the background

function getCanvasCoordinates(x, y) {
    const rect = screen.getBoundingClientRect();
    return {
        x: x - rect.left,
        y: y - rect.top
    };
}

function isInCanvas(x, y) {
    return x >= 0 && x <= screen.width &&
           y >= 0 && y <= screen.height;
}

function redrawCanvas({x, y, pressed}) {
    // Clear the entire canvas
    ctx.fillStyle = BACKGROUND;
    ctx.fillRect(0, 0, screen.width, screen.height);

    // Only draw the point if it exists
    if (pressed) {
        ctx.fillStyle = FOREGROUND;
        ctx.fillRect(
            x,
            y,
            POINT_SIZE,
            POINT_SIZE
        );
    }
}

let pointer = {
    x: 0,
    y: 0,
    pressed: false
}

function setupMouseEvent(callback) {

    // Add event listeners for mouse events
    screen.addEventListener('mousedown', (e) => {
        const coords = getCanvasCoordinates(e.clientX, e.clientY);
        if (isInCanvas(coords.x , coords.y)) {
            pointer.pressed = true;
            pointer.x = coords.x;
            pointer.y = coords.y;
        }

        callback(pointer);
    });

    screen.addEventListener('mouseup', () => {
        pointer.pressed = false
        callback(pointer);
    });

    screen.addEventListener('mousemove', (e) => {
        const coords = getCanvasCoordinates(e.clientX, e.clientY);
        if (isInCanvas(coords.x , coords.y)) {
            pointer.pressed = e.buttons === 1;
            pointer.x = coords.x;
            pointer.y = coords.y;
        }
        callback(pointer);
    });
}


setupMouseEvent(redrawCanvas)
