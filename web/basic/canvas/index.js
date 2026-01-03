// if id="xxx" is valid javascript variable name
// we could direct use it, without document.getElemnetById(...)
// Only do this to make tsgo lsp work

// 【一个公式解密3D图形 | Tsoding】
// https://www.bilibili.com/video/BV1TzBoBqEY6/
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

function transfer_z({x, y, z}, dz) {
    return {
        x,
        y,
        z: z + dz,
    }
}

dz = 1
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


time = 0
const FPS = 60
function frame() {

    const dt = 1 / FPS
    dz += dt

    clear()
    for (const v of vs) {
        point(screen(project(transfer_z(v, dz))))
    }


    setTimeout(frame, 1000 / FPS)
}
setTimeout(frame, 1000 / FPS)
