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
        x: (p.x + 1) / 2 * game.width,
        y: (p.y + 1) / 2 * game.height
    }
}

clear()
point(screen({x: 0, y: 0}))

