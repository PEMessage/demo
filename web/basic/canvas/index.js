// if id="xxx" is valid javascript variable name
// we could direct use it, without document.getElemnetById(...)
// Only do this to make tsgo lsp work

// 【一个公式解密3D图形 | Tsoding】
// https://www.bilibili.com/video/BV1TzBoBqEY6/
// https://github.com/tsoding/formula/blob/main/index.js
/** @type {HTMLCanvasElement} */
const game = document.getElementById('game');

const BACKGROUND = "#101010"
const FOREGROUND = "#50FF50"


console.log(game)
game.width = 800
game.height = 800

const ctx = game.getContext("2d") // similiar to glfwMakeContextCurrent?
console.log(ctx)

function clear(){
    ctx.fillStyle = BACKGROUND
    ctx.fillRect(0, 0, game.width, game.height)
}

function point({x, y}) {
    const s = 20
    ctx.fillStyle = FOREGROUND
    ctx.fillRect(x - s/2, y - s/2, s, s)
}

// convert to screen position
function screen(p) {
    return {
        // [-1, 1] -> [0, 2] -> [0, 1] -> [0, game.width]
        x: (p.x + 1) / 2 * game.width,
        // [-1, 1] -> [0, 2] -> [0, 1] -> [0, game.height]
        // need a extra filp to make it correct
        y: (-p.y + 1) / 2 * game.height
    }
}

function project({x, y, z}) {
    return {
        x: x/z,
        y: y/z,
    }
}

function line(p1, p2) {
    ctx.lineWidth = 3;
    ctx.strokeStyle = FOREGROUND
    ctx.beginPath();
    ctx.moveTo(p1.x, p1.y);
    ctx.lineTo(p2.x, p2.y);
    ctx.stroke();
}


function rotate_xz({x, y, z} , angle) {
    const c = Math.cos(angle)
    const s = Math.sin(angle)
    // [1 , 0] - rotate(angle) -> [cos , sin]
    // [0 , 1] - rotate(angle) -> [-sin, cos]
    return {
    x: x * c + z * -s,
    y: y,
    z: x * s + z * c
    }
}


function transfer_z({x, y, z}, dz) {
    return {
        x,
        y,
        z: z + dz,
    }
}

vs = [
    {x: 0.25, y: 0.25, z: 0.25},
    {x: 0.25, y: -0.25, z: 0.25},
    {x: -0.25, y: -0.25, z: 0.25},
    {x: -0.25, y: 0.25, z: 0.25},

    {x: 0.25, y: 0.25, z: -0.25},
    {x: 0.25, y: -0.25, z: -0.25},
    {x: -0.25, y: -0.25, z: -0.25},
    {x: -0.25, y: 0.25, z: -0.25},
]

loops = [
    [0, 1, 2, 3],
    [4, 5, 6, 7],
    [0, 4],
    [1, 5],
    [2, 6],
    [3, 7],
]


const FPS = 60

z = 1
angle = 0
const freq = 4
function frame() {

    const dt = 1 / FPS
    // z += dt
    angle += (2 * Math.PI / freq) * dt

    clear()
    // for (const v of vs) {
    //     point(screen(project(
    //         transfer_z(rotate_xz(v, angle), z)
    //     )))
    // }

    for (loop of loops) {
        for (let i = 0; i < loop.length; ++i) {
            let va_index = loop[i]
            let vb_index = loop[(i + 1) % loop.length]

            va = vs[va_index]
            vb = vs[vb_index]

            line(
               screen(project(
                   transfer_z(rotate_xz(va, angle), z)
               )),
               screen(project(
                   transfer_z(rotate_xz(vb, angle), z)
               ))
            )
        }
    }

    setTimeout(frame, 1000 / FPS)
}
setTimeout(frame, 1000 / FPS)
