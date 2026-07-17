import QtQuick 2.6
import Sailfish.Silica 1.0

// Procedural pixel view of the park: every attraction bought adds something visible, so the
// place literally grows under the player's hands. Two-frame animation for a bit of life.
Canvas {
    id: view

    property int frame: 0
    property int age: 20

    // The place takes on a patina as the founder ages.
    function groundColor() {
        if (age < 35) return "#22301f"
        if (age < 55) return "#262f1e"
        return "#2a2c20"
    }
    function alleyColor() {
        if (age < 35) return "#8a795c"
        if (age < 55) return "#827257"
        return "#776a52"
    }

    // Seeded LCG so the layout is stable between paints (and playful between installs).
    property var _seed: 0
    function _srand(s) { _seed = s }
    function _rnd() {
        _seed = (_seed * 1103515245 + 12345) % 2147483648
        return _seed / 2147483648
    }

    // Log-ish visual count: 1 unit shows 1 sprite, growth shows a few more, capped.
    function shown(count, cap) {
        if (count <= 0) return 0
        var n = 1 + Math.floor(Math.log(count) / Math.LN2 / 1.2)
        return Math.min(cap, n)
    }

    function counts() {
        var c = {}
        var g = Game.generators
        for (var i = 0; i < g.length; i++)
            c[g[i].id] = g[i].count
        return c
    }

    Timer {
        interval: 700
        running: view.visible && Qt.application.active
        repeat: true
        onTriggered: {
            view.frame = (view.frame + 1) % 2
            view.requestPaint()
        }
    }

    Connections {
        target: Game
        onStateChanged: view.requestPaint()
    }

    onPaint: {
        var ctx = getContext("2d")
        var c = Math.max(4, Math.floor(width / 56))     // pixel cell
        var cols = Math.floor(width / c)
        var rows = Math.floor(height / c)
        var n = counts()

        function put(x, y, col) {
            if (x < 0 || y < 0 || x >= cols || y >= rows) return
            ctx.fillStyle = col
            ctx.fillRect(x * c, y * c, c, c)
        }

        // Ground: mottled grass.
        ctx.fillStyle = groundColor()
        ctx.fillRect(0, 0, width, height)
        _srand(7)
        for (var y = 0; y < rows; y++)
            for (var x = 0; x < cols; x++) {
                var r = _rnd()
                if (r < 0.05) put(x, y, "#283a25")
                else if (r < 0.08) put(x, y, "#1d2a1b")
            }

        // Main alley up from the gate.
        var cx = Math.floor(cols / 2)
        for (y = Math.floor(rows * 0.35); y < rows; y++) {
            put(cx, y, alleyColor())
            put(cx + 1, y, alleyColor())
        }

        // Paths: each visible unit adds a side alley.
        _srand(11)
        var paths = shown(n["paths"], 5)
        for (var i = 0; i < paths; i++) {
            var py = Math.floor(rows * (0.35 + 0.55 * _rnd()))
            var half = Math.floor(cols * (0.2 + 0.25 * _rnd()))
            for (x = cx - half; x <= cx + half; x++)
                put(x, py, "#7d6e54")
        }

        // The gate: brown arch at the bottom of the alley, wider with more gates.
        var gates = shown(n["gate"], 3)
        if (gates > 0) {
            var gw = 1 + gates
            for (i = -gw; i <= gw + 1; i++)
                put(cx + i, rows - 3, "#6d4c33")
            put(cx - gw, rows - 2, "#6d4c33")
            put(cx + gw + 1, rows - 2, "#6d4c33")
            put(cx - gw, rows - 1, "#5a3f2a")
            put(cx + gw + 1, rows - 1, "#5a3f2a")
        }

        // Kiosks: little orange stalls with a blinking pennant.
        _srand(23)
        var kiosks = shown(n["kiosk"], 6)
        for (i = 0; i < kiosks; i++) {
            var kx = Math.floor(cols * (0.1 + 0.8 * _rnd()))
            var ky = Math.floor(rows * (0.4 + 0.45 * _rnd()))
            put(kx, ky, "#b3552e")
            put(kx + 1, ky, "#b3552e")
            put(kx, ky - 1, "#d9d3c0")
            put(kx + 1, ky - 1, "#b3552e")
            put(kx + (frame === 0 ? 0 : 1), ky - 2, frame === 0 ? "#e0b23a" : "#b3552e")
        }

        // Aviary: teal domes, birds flit between two spots.
        _srand(37)
        var aviaries = shown(n["aviary"], 3)
        for (i = 0; i < aviaries; i++) {
            var ax = Math.floor(cols * (0.12 + 0.7 * _rnd()))
            var ay = Math.floor(rows * (0.12 + 0.2 * _rnd()))
            put(ax, ay, "#5d7d8a"); put(ax + 1, ay, "#5d7d8a"); put(ax + 2, ay, "#5d7d8a")
            put(ax, ay + 1, "#4d6a76"); put(ax + 1, ay + 1, "#4d6a76"); put(ax + 2, ay + 1, "#4d6a76")
            put(ax + (frame === 0 ? 0 : 2), ay - 1, "#d9d3c0")   // a bird
        }

        // Carousel: red-and-gold, alternating frames suggest the spin.
        _srand(41)
        var carousels = shown(n["carousel"], 2)
        for (i = 0; i < carousels; i++) {
            var rx = Math.floor(cols * (0.2 + 0.55 * _rnd()))
            var ry = Math.floor(rows * (0.45 + 0.3 * _rnd()))
            var c1 = frame === 0 ? "#a03a4a" : "#e0b23a"
            var c2 = frame === 0 ? "#e0b23a" : "#a03a4a"
            put(rx, ry, c1); put(rx + 2, ry, c2)
            put(rx + 1, ry, "#d9d3c0")
            put(rx + 1, ry - 1, "#a03a4a")
            put(rx, ry + 1, c2); put(rx + 2, ry + 1, c1)
        }

        // Pond: a blue blob with a drifting glint.
        _srand(53)
        var ponds = shown(n["pond"], 2)
        for (i = 0; i < ponds; i++) {
            var px = Math.floor(cols * (0.15 + 0.6 * _rnd()))
            var pyy = Math.floor(rows * (0.15 + 0.55 * _rnd()))
            for (y = 0; y < 3; y++)
                for (x = 0; x < 4; x++)
                    if (!((x === 0 || x === 3) && (y === 0 || y === 2)))
                        put(px + x, pyy + y, "#3a6a8a")
            put(px + 1 + frame, pyy + 1, "#5d8db0")
        }

        // Big wheel: a ring that turns, frame by frame.
        _srand(71)
        var wheels = shown(n["wheel"], 1)
        for (i = 0; i < wheels; i++) {
            var whx = Math.floor(cols * (0.65 + 0.2 * _rnd()))
            var why = Math.floor(rows * (0.15 + 0.2 * _rnd()))
            var s1 = frame === 0 ? "#7a6fa0" : "#9a8fc0"
            var s2 = frame === 0 ? "#9a8fc0" : "#7a6fa0"
            put(whx, why - 2, s1); put(whx, why + 2, s1)
            put(whx - 2, why, s2); put(whx + 2, why, s2)
            put(whx - 1, why - 1, s2); put(whx + 1, why - 1, s1)
            put(whx - 1, why + 1, s1); put(whx + 1, why + 1, s2)
            put(whx, why, "#d9d3c0")
            put(whx, why + 3, "#5a5470")
        }

        // Greenhouse: pale glass, a glint sliding across.
        _srand(79)
        var greenhouses = shown(n["greenhouse"], 2)
        for (i = 0; i < greenhouses; i++) {
            var ghx = Math.floor(cols * (0.1 + 0.7 * _rnd()))
            var ghy = Math.floor(rows * (0.55 + 0.3 * _rnd()))
            for (x = 0; x < 3; x++) {
                put(ghx + x, ghy, "#9fc9a8")
                put(ghx + x, ghy + 1, "#7fae8c")
            }
            put(ghx + (frame === 0 ? 0 : 2), ghy, "#d8ecd8")
        }

        // The museum: stone, columns, a pediment. It doesn't move.
        _srand(83)
        var museums = shown(n["museum"], 1)
        for (i = 0; i < museums; i++) {
            var mx = Math.floor(cols * (0.25 + 0.4 * _rnd()))
            var my = Math.floor(rows * (0.12 + 0.15 * _rnd()))
            for (x = 0; x < 4; x++)
                put(mx + x, my, "#a8a398")
            put(mx, my + 1, "#8f8a80"); put(mx + 1, my + 1, "#a8a398")
            put(mx + 2, my + 1, "#8f8a80"); put(mx + 3, my + 1, "#a8a398")
            for (x = 0; x < 4; x++)
                put(mx + x, my + 2, "#7d786e")
        }

        // The kiosk cat: it sits somewhere with a view, and it watches. Every frame.
        if ((n["aviary"] || 0) > 0) {
            _srand(89)
            var ktx = Math.floor(cols * (0.3 + 0.4 * _rnd()))
            var kty = Math.floor(rows * (0.5 + 0.3 * _rnd()))
            put(ktx, kty, "#3a3a42")
            put(ktx, kty - 1, "#3a3a42")
            put(ktx + (frame === 0 ? 1 : -1), kty - 1, "#3a3a42")   // an ear, or the tail
            put(ktx, kty + 0, "#3a3a42")
            put(ktx, kty, "#4a4a52")
        }

        // Creatures: the quiet side wandering in.
        _srand(67)
        var crs = Game.creatures
        for (i = 0; i < crs.length && i < 14; i++) {
            var wx = Math.floor(cols * (0.08 + 0.84 * _rnd()))
            var wy = Math.floor(rows * (0.3 + 0.6 * _rnd()))
            put(wx + (frame === 0 ? 0 : (i % 2 === 0 ? 1 : -1)), wy, app.creatureColor(crs[i]))
        }
    }
}
