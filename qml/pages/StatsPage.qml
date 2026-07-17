import QtQuick 2.6
import Sailfish.Silica 1.0

// The dashboard: every number the optimizer could want, one chart among the others.
Page {
    id: page

    property var rows: Game.breakdown()

    Connections {
        target: Game
        onStateChanged: {
            page.rows = Game.breakdown()
            donut.requestPaint()
            soinChart.requestPaint()
            sleepChart.requestPaint()
            decisionsChart.requestPaint()
        }
    }

    function sourceName(id) {
        if (id === "tap") return qsTr("By hand")
        if (id === "gate") return qsTr("The gate")
        if (id === "kiosk") return qsTr("The kiosk")
        if (id === "paths") return qsTr("The paths")
        if (id === "aviary") return qsTr("The aviary")
        if (id === "carousel") return qsTr("The carousel")
        if (id === "pond") return qsTr("The pond")
        if (id === "wheel") return qsTr("The big wheel")
        if (id === "greenhouse") return qsTr("The greenhouse")
        if (id === "museum") return qsTr("The park museum")
        if (id === "opening") return qsTr("The grand opening")
        return id
    }

    function sourceColor(id) {
        if (id === "tap") return "#9A9A9A"
        if (id === "gate") return "#6D4C33"
        if (id === "kiosk") return "#B3552E"
        if (id === "paths") return "#8A795C"
        if (id === "aviary") return "#5D7D8A"
        if (id === "carousel") return "#A03A4A"
        if (id === "pond") return "#3A6A8A"
        if (id === "wheel") return "#7A6FA0"
        if (id === "greenhouse") return "#6FA07A"
        if (id === "museum") return "#8F8A80"
        if (id === "opening") return "#E0B23A"
        return "#777777"
    }

    function durText(ms) {
        if (ms <= 0) return ""
        var d = Math.floor(ms / 86400000)
        if (d >= 1) return d + " j"
        var h = Math.floor(ms / 3600000)
        if (h >= 1) return h + " h"
        return Math.max(1, Math.floor(ms / 60000)) + " min"
    }

    function spanOf(pts) {
        if (!pts || pts.length < 2) return 0
        return pts[pts.length - 1].t - pts[0].t
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: page.width

            PageHeader { title: qsTr("Dashboard") }

            SectionHeader { text: qsTr("Sources") }

            Item {
                width: parent.width
                height: donut.height

                Canvas {
                    id: donut
                    x: Theme.horizontalPageMargin
                    width: Theme.itemSizeHuge * 1.4
                    height: Theme.itemSizeHuge * 1.4

                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        var rows = page.rows
                        if (!rows || rows.length === 0) return
                        var cx = width / 2, cy = height / 2
                        var radius = width / 2 - Theme.paddingSmall
                        var ring = radius * 0.42
                        var a = -Math.PI / 2
                        for (var i = 0; i < rows.length; i++) {
                            var span = rows[i].share * 2 * Math.PI
                            if (span <= 0) continue
                            ctx.beginPath()
                            ctx.arc(cx, cy, radius - ring / 2, a, a + span, false)
                            ctx.lineWidth = ring
                            ctx.strokeStyle = sourceColor(rows[i].id)
                            ctx.stroke()
                            a += span
                        }
                    }
                }

                Column {
                    anchors {
                        left: donut.right
                        leftMargin: Theme.paddingLarge
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin
                        verticalCenter: donut.verticalCenter
                    }

                    Repeater {
                        model: page.rows

                        Row {
                            spacing: Theme.paddingSmall

                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                width: Theme.paddingMedium
                                height: Theme.paddingMedium
                                radius: 2
                                color: sourceColor(modelData.id)
                            }
                            Label {
                                text: sourceName(modelData.id) + "  "
                                      + (modelData.share * 100).toFixed(1) + " %"
                                font.pixelSize: Theme.fontSizeExtraSmall
                                color: Theme.secondaryColor
                            }
                        }
                    }
                }
            }

            SectionHeader {
                visible: Game.creatures.length > 0 || Game.soin >= 6
                text: qsTr("Park life")
            }

            Column {
                visible: Game.creatures.length > 0 || Game.soin >= 6
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin

                Flow {
                    width: parent.width
                    spacing: Theme.paddingSmall

                    Repeater {
                        model: Game.creatures

                        Rectangle {
                            width: Theme.paddingLarge
                            height: Theme.paddingLarge
                            radius: width / 2
                            color: app.creatureColor(modelData)
                            opacity: 0.9
                        }
                    }
                }

                Label {
                    text: Game.creatures.length + " " + qsTr("animals")
                          + "  ·  +" + Game.creatureBonusPercent.toFixed(1) + " %"
                          + "  ·  " + Game.fmt(Game.soin) + " " + qsTr("care")
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                }

                Item {
                    width: parent.width
                    height: soinChart.height
                    visible: Game.soinHistory().length >= 2

                    Canvas {
                        id: soinChart
                        width: parent.width
                        height: Theme.itemSizeSmall

                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            var pts = Game.soinHistory()
                            if (pts.length < 2) return
                            var minT = pts[0].t, maxT = pts[pts.length - 1].t
                            var maxV = pts[pts.length - 1].v
                            if (maxT - minT <= 0 || maxV <= 0) return
                            ctx.beginPath()
                            for (var i = 0; i < pts.length; i++) {
                                var px = (pts[i].t - minT) / (maxT - minT) * width
                                var py = height - 2 - (pts[i].v / maxV) * (height - 4)
                                if (i === 0) ctx.moveTo(px, py)
                                else ctx.lineTo(px, py)
                            }
                            ctx.strokeStyle = "#7DA33F"
                            ctx.lineWidth = 2
                            ctx.stroke()
                        }
                    }

                    Label {
                        anchors { right: parent.right; bottom: parent.bottom }
                        text: durText(spanOf(Game.soinHistory()))
                        font.pixelSize: Theme.fontSizeTiny
                        color: Theme.secondaryColor
                        opacity: 0.7
                    }
                }
            }

            SectionHeader {
                visible: Game.founderVisible
                text: qsTr("The founder")
            }

            Column {
                visible: Game.founderVisible
                width: parent.width

                DetailItem { label: qsTr("Age"); value: "" + Game.founderAge }
                DetailItem { label: qsTr("Sleep"); value: Game.sleepPercent.toFixed(0) + " %" }
                DetailItem { label: qsTr("Focus"); value: Game.focusPercent.toFixed(0) + " %" }

                Item {
                    x: Theme.horizontalPageMargin
                    width: column.width - 2 * Theme.horizontalPageMargin
                    height: sleepChart.height
                    visible: Game.sleepHistory().length >= 2

                    Canvas {
                        id: sleepChart
                        width: parent.width
                        height: Theme.itemSizeLarge

                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            var pts = Game.sleepHistory()
                            if (pts.length < 2) return
                            var minT = pts[0].t, maxT = pts[pts.length - 1].t
                            if (maxT - minT <= 0) return
                            ctx.beginPath()
                            for (var i = 0; i < pts.length; i++) {
                                var px = (pts[i].t - minT) / (maxT - minT) * width
                                var py = height - 2 - (pts[i].v - 0.3) / 0.7 * (height - 4)
                                if (i === 0) ctx.moveTo(px, py)
                                else ctx.lineTo(px, py)
                            }
                            ctx.strokeStyle = Theme.secondaryHighlightColor
                            ctx.lineWidth = 2
                            ctx.stroke()
                        }
                    }

                    Label {
                        anchors { right: parent.right; bottom: parent.bottom }
                        text: qsTr("Sleep") + "  ·  " + durText(spanOf(Game.sleepHistory()))
                        font.pixelSize: Theme.fontSizeTiny
                        color: Theme.secondaryColor
                        opacity: 0.7
                    }
                }

                Item {
                    x: Theme.horizontalPageMargin
                    width: column.width - 2 * Theme.horizontalPageMargin
                    height: decisionsChart.height
                    visible: Game.decisionsVisible

                    Canvas {
                        id: decisionsChart
                        width: parent.width
                        height: Theme.itemSizeLarge

                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            var pts = Game.decisionsHistory()
                            if (pts.length < 2) return
                            var minT = pts[0].t, maxT = pts[pts.length - 1].t
                            var maxV = Math.max(pts[pts.length - 1].b, pts[pts.length - 1].s, 1)
                            if (maxT - minT <= 0) return
                            function drawLine(fieldB, color) {
                                ctx.beginPath()
                                for (var i = 0; i < pts.length; i++) {
                                    var px = (pts[i].t - minT) / (maxT - minT) * width
                                    var v = fieldB ? pts[i].b : pts[i].s
                                    var py = height - 2 - (v / maxV) * (height - 4)
                                    if (i === 0) ctx.moveTo(px, py)
                                    else ctx.lineTo(px, py)
                                }
                                ctx.strokeStyle = color
                                ctx.lineWidth = 2
                                ctx.stroke()
                            }
                            drawLine(true, "#C0603A")
                            drawLine(false, Theme.secondaryHighlightColor)
                        }
                    }

                    Label {
                        anchors { right: parent.right; bottom: parent.bottom }
                        text: qsTr("Decisions") + "  ·  " + durText(spanOf(Game.decisionsHistory()))
                        font.pixelSize: Theme.fontSizeTiny
                        color: Theme.secondaryColor
                        opacity: 0.7
                    }
                }
            }

            Item { width: 1; height: Theme.paddingLarge }
        }

        VerticalScrollDecorator { }
    }
}
