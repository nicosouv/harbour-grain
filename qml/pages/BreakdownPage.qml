import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    property var rows: Game.breakdown()

    function sourceName(id) {
        if (id === "tap") return qsTr("By hand")
        if (id === "gate") return qsTr("The gate")
        if (id === "kiosk") return qsTr("The kiosk")
        if (id === "paths") return qsTr("The paths")
        if (id === "aviary") return qsTr("The aviary")
        if (id === "carousel") return qsTr("The carousel")
        if (id === "pond") return qsTr("The pond")
        if (id === "opening") return qsTr("The grand opening")
        return id
    }

    Connections {
        target: Game
        onStateChanged: page.rows = Game.breakdown()
    }

    SilicaListView {
        anchors.fill: parent
        model: page.rows

        header: PageHeader { title: qsTr("Breakdown") }

        footer: Column {
            width: page.width
            visible: Game.founderVisible
            height: Game.founderVisible ? implicitHeight : 0

            SectionHeader { text: qsTr("The founder") }

            DetailItem { label: qsTr("Age"); value: "" + Game.founderAge }
            DetailItem { label: qsTr("Sleep"); value: Game.sleepPercent.toFixed(0) + " %" }
            DetailItem { label: qsTr("Focus"); value: Game.focusPercent.toFixed(0) + " %" }

            Canvas {
                id: sleepChart
                x: Theme.horizontalPageMargin
                width: page.width - 2 * Theme.horizontalPageMargin
                height: Theme.itemSizeLarge
                visible: Game.sleepHistory().length >= 2

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

                Connections {
                    target: Game
                    onStateChanged: sleepChart.requestPaint()
                }
            }

            Item { width: 1; height: Theme.paddingLarge }
        }

        delegate: Item {
            width: page.width
            height: Theme.itemSizeMedium

            Column {
                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - 2 * Theme.horizontalPageMargin

                Row {
                    width: parent.width
                    Label {
                        width: parent.width * 0.6
                        text: sourceName(modelData.id)
                        truncationMode: TruncationMode.Fade
                    }
                    Label {
                        width: parent.width * 0.4
                        horizontalAlignment: Text.AlignRight
                        text: (modelData.share * 100).toFixed(1) + " %"
                        color: Theme.highlightColor
                    }
                }
                Label {
                    text: Game.fmt(modelData.total)
                          + (modelData.perSec > 0
                             ? "  ·  +" + Game.fmt(modelData.perSec) + "/s" : "")
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                }
            }
        }

        VerticalScrollDecorator { }
    }
}
