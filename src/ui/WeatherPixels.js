.pragma library

const cell = 2
const greens = [
    [["#2f5638", 0.23], ["#4c7e4c", 0.45], ["#64985c", 0.65]],
    [["#487547", 0.37], ["#6b924f", 0.52], ["#84a964", 0.67]],
    [["#314e3a", 0.24], ["#497b4c", 0.46], ["#5b915b", 0.65]]
]
const drops = [
    [["#76b2ce", 121 / 255], ["#b0d8e8", 121 / 255]],
    [["#a3ccde", 179 / 255], ["#d1e9ef", 179 / 255]]
]

// Same tapered rain silhouette and colour bands as the overlay. The preview
// supplies its existing demo positions, blade deflections and flash timing.
const dropOutline = (() => {
    const outline = new Array(12)
    for (let i = 0; i <= 6; ++i) {
        const t = i / 6, u = 1 - t
        const x = -(3 * u * u * t * 0.35 + 3 * u * t * t)
        const y = -3.5 * u * u * u - 6 * u * u * t + 0.65 * t * t * t
        outline[i] = [x, y]
        if (i > 0 && i < 6)
            outline[outline.length - i] = [-x, y]
    }
    return outline
})()

function drawRows(ctx, outline, paintAt, opacity = 1) {
    let top = Infinity, bottom = -Infinity
    for (const point of outline) {
        top = Math.min(top, point[1])
        bottom = Math.max(bottom, point[1])
    }
    if (top >= bottom)
        return
    let runX = 0, runY = 0, runWidth = 0, runBottom = 0, runPaint
    function flush() {
        if (runWidth <= 0)
            return
        ctx.fillStyle = runPaint[0]
        ctx.globalAlpha = Math.round(runPaint[1] * opacity * 16) / 16
        ctx.fillRect(runX, runY, runWidth, runBottom - runY)
    }
    const end = Math.ceil(bottom / cell) * cell
    for (let y = Math.floor(top / cell) * cell; y < end; y += cell) {
        const sampleY = (Math.max(y, top) + Math.min(y + cell, bottom)) * 0.5
        let left = Infinity, right = -Infinity
        for (let i = 0; i < outline.length; ++i) {
            const a = outline[i], b = outline[(i + 1) % outline.length]
            if ((a[1] > sampleY) === (b[1] > sampleY))
                continue
            const x = a[0] + (b[0] - a[0]) * (sampleY - a[1]) / (b[1] - a[1])
            left = Math.min(left, x)
            right = Math.max(right, x)
        }
        if (left >= right)
            continue
        let x = Math.ceil((left - cell * 0.5) / cell) * cell
        const edge = Math.ceil((right - cell * 0.5) / cell) * cell
        const width = Math.max(cell, edge - x)
        if (edge <= x)
            x = Math.floor((left + right) * 0.5 / cell) * cell
        const paint = paintAt((sampleY - top) / (bottom - top))
        if (runX !== x || runWidth !== width || runPaint !== paint || runBottom !== y) {
            flush()
            runX = x
            runY = y
            runWidth = width
            runPaint = paint
        }
        runBottom = y + cell
    }
    flush()
    ctx.globalAlpha = 1
}

function drop(ctx, x, y, size, slope, highlight) {
    const c = size / Math.sqrt(1 + slope * slope), s = slope * c
    const outline = dropOutline.map(p => [x + p[0] * c + p[1] * s, y - p[0] * s + p[1] * c])
    const palette = drops[highlight ? 1 : 0]
    drawRows(ctx, outline, progress => palette[progress < 0.38 ? 0 : 1])
}

function splash(ctx, x, y) {
    const left = Math.round(x - cell), top = Math.round(y + cell) - 2 * cell
    ctx.globalAlpha = Math.round(154 / 255 * 16) / 16
    ctx.fillStyle = "#84cbe0"
    ctx.fillRect(left + cell, top, cell, cell)
    ctx.fillRect(left, top + cell, cell, cell)
    ctx.fillStyle = "#d5f1ed"
    ctx.fillRect(left + cell, top + cell, cell, cell)
    ctx.globalAlpha = 1
}

function grass(ctx, x, bottom, height, lean, shade) {
    const segments = 8
    const outline = new Array(segments * 2 + 1)
    function point(t, right) {
        const u = 1 - t
        const start = right ? 2.7 : -2.7
        const control = right ? lean * 0.32 + 2 : lean * 0.2 - 2
        const rise = 2 * u * t * (right ? 0.45 : 0.65) + t * t
        return [x + u * u * start + 2 * u * t * control + t * t * lean, bottom - rise * height]
    }
    for (let i = 0; i < segments; ++i) {
        outline[i] = point(i / segments, false)
        outline[outline.length - 1 - i] = point(i / segments, true)
    }
    outline[segments] = [x + lean, bottom - height]
    const palette = greens[shade]
    drawRows(ctx, outline, progress => palette[progress < 0.35 ? 2 : progress < 0.69 ? 1 : 0],
             Math.min(1, height / cell))
}

function lightning(ctx, width, height, strength) {
    // The overlay uses the same eight-point grid and stepped shade in its shader.
    const step = 8, center = width * 0.71, radius = Math.max(width * 0.65, height)
    ctx.fillStyle = "#cadeff"
    for (let y = 0; y < height; y += step) {
        const dy = (y + step * 0.5) / radius
        let runX = 0, runOpacity = 0
        for (let x = 0; x < width; x += step) {
            const dx = (x + step * 0.5 - center) / radius
            const falloff = Math.max(0, 1 - dx * dx - dy * dy)
            const band = Math.round(falloff * falloff * falloff * 8) / 8
            const opacity = Math.round(strength * band * 24) * 0.12 / 24
            if (opacity === runOpacity)
                continue
            if (runOpacity > 0) {
                ctx.globalAlpha = runOpacity
                ctx.fillRect(runX, y, x - runX, step)
            }
            runX = x
            runOpacity = opacity
        }
        if (runOpacity > 0) {
            ctx.globalAlpha = runOpacity
            ctx.fillRect(runX, y, width - runX, step)
        }
    }
    ctx.globalAlpha = 1
}

function crystal(ctx, x, y, angle) {
    const filled = new Set()
    const cells = []
    function walk(fromX, fromY, toX, toY) {
        const dx = Math.abs(toX - fromX), dy = -Math.abs(toY - fromY)
        const stepX = fromX < toX ? 1 : -1, stepY = fromY < toY ? 1 : -1
        let error = dx + dy
        while (true) {
            const key = fromX + "," + fromY
            if (!filled.has(key)) {
                filled.add(key)
                cells.push([fromX, fromY])
            }
            if (fromX === toX && fromY === toY)
                break
            const twice = 2 * error
            if (twice >= dy) {
                error += dy
                fromX += stepX
            }
            if (twice <= dx) {
                error += dx
                fromY += stepY
            }
        }
    }
    for (let arm = 0; arm < 3; ++arm) {
        const direction = angle + arm * Math.PI / 3
        const dx = Math.cos(direction) * 4, dy = Math.sin(direction) * 4
        walk(Math.floor((x - dx) / cell), Math.floor((y - dy) / cell),
             Math.floor((x + dx) / cell), Math.floor((y + dy) / cell))
    }
    ctx.fillStyle = "#ecebda"
    ctx.globalAlpha = 0.85
    for (const point of cells)
        ctx.fillRect(point[0] * cell, point[1] * cell, cell, cell)
    ctx.globalAlpha = 1
}

function snow(ctx, x, y, radius, bright) {
    ctx.fillStyle = "#f4f4df"
    ctx.globalAlpha = bright ? 0.9 : 0.55
    const radiusSquared = radius * radius
    let drawn = false
    for (let row = Math.floor((y - radius) / cell) * cell; row < y + radius; row += cell) {
        const dy = row + cell * 0.5 - y
        for (let column = Math.floor((x - radius) / cell) * cell; column < x + radius; column += cell) {
            const dx = column + cell * 0.5 - x
            if (dx * dx + dy * dy <= radiusSquared) {
                ctx.fillRect(column, row, cell, cell)
                drawn = true
            }
        }
    }
    if (!drawn)
        ctx.fillRect(Math.floor(x / cell) * cell, Math.floor(y / cell) * cell, cell, cell)
    ctx.globalAlpha = 1
}
