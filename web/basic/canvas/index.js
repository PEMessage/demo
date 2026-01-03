// if id="xxx" is valid javascript variable name
// we could direct use it, without document.getElemnetById(...)
// Only do this to make tsgo lsp work

// 【一个公式解密3D图形 | Tsoding】
// https://www.bilibili.com/video/BV1TzBoBqEY6/
/** @type {HTMLCanvasElement} */
const game = document.getElementById('game');

console.log(game)
game.width = 800
game.height = 800

const ctx = game.getContext("2d") // similiar to glfwMakeContextCurrent?
console.log(ctx)

// background
{
    ctx.fillStyle = "#101010"
    ctx.fillRect(0, 0, game.width, game.height)
}

// content
{
    // ctx.fillStyle = "green"
    ctx.fillStyle = "#50FF50"
    ctx.fillRect(0, 0, 100, 100)
}

