import QtQuick 2.6
import Sailfish.Silica 1.0
import "../components"

Page {
    id: page

    function echoName(i) {
        if (i === 0) return qsTr("Repaint the gate")
        if (i === 1) return qsTr("Widen the parking lot")
        if (i === 2) return qsTr("Frame the opening photo")
        if (i === 3) return qsTr("Celebrate the park's anniversary")
        if (i === 4) return qsTr("Sleep at the office")
        if (i === 5) return qsTr("Stronger coffee")
        if (i === 6) return qsTr("Skip the aviary in the morning")
        if (i === 7) return qsTr("Rehearse the park's story")
        if (i === 8) return qsTr("Put the old photos away")
        if (i === 9) return qsTr("Don't think about it")
        if (i === 10) return qsTr("Double the maintenance crew")
        if (i === 11) return qsTr("Light the paths at night")
        if (i === 12) return qsTr("A bigger office, farther from the gate")
        if (i === 13) return qsTr("Silence the gate's creak")
        if (i === 14) return qsTr("Earbuds for the commute")
        if (i === 15) return qsTr("Lunch at the desk")
        if (i === 16) return qsTr("Replace the bench by the pond")
        if (i === 17) return qsTr("Scan the archives, discard the originals")
        if (i === 18) return qsTr("Stop celebrating anniversaries")
        if (i === 19) return qsTr("Sell the house. The office will do.")
        return ""
    }

    function genName(id) {
        if (id === "gate") return qsTr("The gate")
        if (id === "kiosk") return qsTr("The kiosk")
        if (id === "paths") return qsTr("The paths")
        if (id === "aviary") return qsTr("The aviary")
        if (id === "carousel") return qsTr("The carousel")
        if (id === "pond") return qsTr("The pond")
        if (id === "wheel") return qsTr("The big wheel")
        if (id === "greenhouse") return qsTr("The greenhouse")
        if (id === "museum") return qsTr("The park museum")
        return id
    }

    function tierSituation(i) {
        if (i === 0) return qsTr("The banker wants to see the first summer's books.")
        if (i === 1) return qsTr("The insurer asks for the plot's history.")
        if (i === 2) return qsTr("The chamber of commerce wants the park's story for its brochure.")
        if (i === 3) return qsTr("The investors ask who signed the original permit.")
        if (i === 4) return qsTr("Close the round this week, or wait for the audit?")
        if (i === 5) return qsTr("The foreign fund wants to tour the park, no guide.")
        if (i === 6) return qsTr("A journalist is writing a profile of the founder.")
        if (i === 7) return qsTr("The inspector wants to see the permit again.")
        if (i === 8) return qsTr("The neighbours will sell if the park's story stays intact.")
        if (i === 9) return qsTr("Create a foundation in the park's name.")
        if (i === 10) return qsTr("The heritage listing requires a file on the origins.")
        return qsTr("A group offers to buy. They would keep everything: the name, the story.")
    }

    function tierFast(i) {
        if (i === 0) return qsTr("Show the prepared binder")
        if (i === 1) return qsTr("Send the usual file")
        if (i === 2) return qsTr("Proofread the text yourself")
        if (i === 3) return qsTr("Show the permit")
        if (i === 4) return qsTr("Close this week")
        if (i === 5) return qsTr("Mark out the route")
        if (i === 6) return qsTr("Rehearse the park's story")
        if (i === 7) return qsTr("Show the other one")
        if (i === 8) return qsTr("Promise")
        if (i === 9) return qsTr("In the park's name")
        if (i === 10) return qsTr("Have the file written")
        return qsTr("Refuse. It's mine.")
    }

    function tierSlow(i) {
        if (i === 0) return qsTr("Show all the binders")
        if (i === 1) return qsTr("Let their expert visit")
        if (i === 2) return qsTr("Let them interview the staff")
        if (i === 3) return qsTr("Answer: me.")
        if (i === 4) return qsTr("Wait for the audit")
        if (i === 5) return qsTr("Hand over the keys")
        if (i === 6) return qsTr("Talk about her, mostly")
        if (i === 7) return qsTr("Show the original")
        if (i === 8) return qsTr("Buy without promising, dearer")
        if (i === 9) return qsTr("In her name")
        if (i === 10) return qsTr("Write it yourself, all of it")
        return qsTr("Refuse. It's ours.")
    }

    function cooldownText(ms) {
        var h = Math.floor(ms / 3600000)
        if (h >= 1) return h + " h"
        return Math.max(1, Math.ceil(ms / 60000)) + " min"
    }

    function sourceName(id) {
        if (id === "tap") return qsTr("By hand")
        if (id === "opening") return qsTr("The grand opening")
        return genName(id)
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

    RemorsePopup { id: remorse }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        PullDownMenu {
            MenuItem {
                text: qsTr("Settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }
        }

        Column {
            id: column
            width: page.width

            PageHeader { title: qsTr("The park") }

            // The park itself: grows with every purchase.
            ParkView {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                height: Theme.itemSizeHuge * 1.6
                age: Game.founderAge
            }

            Item { width: 1; height: Theme.paddingMedium }

            // The ticker: the number the player chose to optimize.
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: Game.fmt(Game.recette)
                font.pixelSize: Theme.fontSizeHuge
                font.bold: true
                color: Theme.highlightColor
            }
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "+" + Game.fmt(Game.recettePerSec) + "/s"
                      + (Game.plateaued ? "  ·  " + qsTr("at capacity") : "")
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
            }
            // The quiet second ticker, easy to never look at.
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: Game.fmt(Game.soin) + " " + qsTr("care")
                      + (Game.bonusPercent > 0
                         ? "   ·   +" + Game.bonusPercent.toFixed(0) + " % " + qsTr("permanent")
                         : "")
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                opacity: 0.6
            }

            // Epoch recette over time: one honest chart on the front page.
            Canvas {
                id: chart
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                height: Theme.itemSizeLarge

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    var pts = Game.history()
                    if (pts.length < 2) return
                    var minV = pts[0].v, maxV = pts[0].v
                    var minT = pts[0].t, maxT = pts[pts.length - 1].t
                    for (var i = 0; i < pts.length; i++) {
                        if (pts[i].v < minV) minV = pts[i].v
                        if (pts[i].v > maxV) maxV = pts[i].v
                    }
                    if (maxV - minV < 1e-9 || maxT - minT <= 0) return
                    ctx.beginPath()
                    for (i = 0; i < pts.length; i++) {
                        var px = (pts[i].t - minT) / (maxT - minT) * width
                        var py = height - 2 - (pts[i].v - minV) / (maxV - minV) * (height - 4)
                        if (i === 0) ctx.moveTo(px, py)
                        else ctx.lineTo(px, py)
                    }
                    ctx.strokeStyle = Theme.highlightColor
                    ctx.lineWidth = 2
                    ctx.stroke()
                    ctx.lineTo(width, height)
                    ctx.lineTo(0, height)
                    ctx.closePath()
                    ctx.fillStyle = Theme.rgba(Theme.highlightColor, 0.12)
                    ctx.fill()
                }

                Connections {
                    target: Game
                    onStateChanged: chart.requestPaint()
                }
            }

            Item { width: 1; height: Theme.paddingLarge }

            // Tap zone.
            Rectangle {
                id: tapZone
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width - 2 * Theme.horizontalPageMargin
                height: Theme.itemSizeHuge
                radius: Theme.paddingMedium
                color: Theme.rgba(Theme.highlightBackgroundColor,
                                  tapArea.pressed ? 0.5 : 0.3)

                Label {
                    anchors.centerIn: parent
                    text: qsTr("Welcome visitors")
                    font.pixelSize: Theme.fontSizeLarge
                }

                Label {
                    id: floatLabel
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: 0
                    opacity: 0
                    font.pixelSize: Theme.fontSizeMedium
                    color: Theme.highlightColor
                }

                MouseArea {
                    id: tapArea
                    anchors.fill: parent
                    onClicked: {
                        Game.tap()
                        floatLabel.text = "+" + Game.fmt(Game.tapPower)
                        pulse.restart()
                        floatAnim.restart()
                    }
                }

                SequentialAnimation {
                    id: pulse
                    NumberAnimation { target: tapZone; property: "scale"; to: 0.97; duration: 40 }
                    NumberAnimation { target: tapZone; property: "scale"; to: 1.0; duration: 80 }
                }

                ParallelAnimation {
                    id: floatAnim
                    NumberAnimation {
                        target: floatLabel; property: "y"
                        from: tapZone.height / 2 - Theme.paddingLarge; to: -Theme.paddingLarge * 2
                        duration: 500
                    }
                    SequentialAnimation {
                        NumberAnimation { target: floatLabel; property: "opacity"; to: 1; duration: 80 }
                        NumberAnimation { target: floatLabel; property: "opacity"; to: 0; duration: 420 }
                    }
                }
            }

            Item { width: 1; height: Theme.paddingLarge }

            // One-shot inauguration node.
            BackgroundItem {
                id: openingItem
                visible: Game.openingVisible
                width: parent.width
                height: Theme.itemSizeMedium
                enabled: Game.recette >= Game.openingCost
                onClicked: Game.inaugurate()

                Rectangle {
                    anchors.fill: parent
                    color: Theme.rgba(Theme.highlightBackgroundColor, 0.15)
                }
                Column {
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    Label {
                        text: qsTr("The grand opening")
                        color: openingItem.enabled ? Theme.highlightColor : Theme.secondaryColor
                    }
                    Label {
                        text: qsTr("Open the gates for good. Once.")
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: Theme.secondaryColor
                    }
                }
                Label {
                    anchors {
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    text: Game.fmt(Game.openingCost)
                    color: openingItem.enabled ? Theme.highlightColor : Theme.secondaryColor
                }
            }

            // Funding: the next order of magnitude sits behind a question and real time.
            SectionHeader {
                visible: Game.raiseReady || Game.raisePending
                text: qsTr("Funding")
            }

            Column {
                visible: Game.raiseReady || Game.raisePending
                width: parent.width
                spacing: Theme.paddingMedium

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    wrapMode: Text.Wrap
                    text: tierSituation(Game.raiseTier)
                    color: Theme.highlightColor
                }

                Label {
                    visible: Game.raisePending
                    x: Theme.horizontalPageMargin
                    text: qsTr("Next round in %1").arg(cooldownText(Game.raiseCooldownLeft))
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.secondaryColor
                }

                Column {
                    visible: Game.raiseReady
                    width: parent.width
                    spacing: Theme.paddingSmall

                    Button {
                        anchors.horizontalCenter: parent.horizontalCenter
                        preferredWidth: Theme.buttonWidthLarge
                        text: tierFast(Game.raiseTier)
                        onClicked: Game.closeRaise(true)
                    }
                    Button {
                        anchors.horizontalCenter: parent.horizontalCenter
                        preferredWidth: Theme.buttonWidthLarge
                        text: tierSlow(Game.raiseTier)
                        onClicked: Game.closeRaise(false)
                    }
                }

                Item { width: 1; height: Theme.paddingSmall }
            }

            // Attractions header with the buy-amount toggle.
            Item {
                width: parent.width
                height: Theme.itemSizeSmall

                SectionHeader {
                    text: qsTr("Attractions")
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - buyToggle.width - 2 * Theme.horizontalPageMargin
                }

                Button {
                    id: buyToggle
                    anchors {
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    width: Theme.itemSizeExtraLarge
                    text: "×" + Game.buyAmount
                    onClicked: Game.buyAmount = Game.buyAmount === 1 ? 10
                             : Game.buyAmount === 10 ? 100 : 1
                }
            }

            Repeater {
                model: Game.generators

                Item {
                    width: column.width
                    height: Theme.itemSizeMedium
                    opacity: modelData.locked ? 0.4 : 1.0

    	            // Tap the row to run a manual cycle (managers make it continuous) —
                    // or to repair it when it's down.
                    MouseArea {
                        anchors.fill: parent
                        anchors.rightMargin: parent.width * 0.35
                        enabled: !modelData.locked && modelData.count > 0
                        onClicked: {
                            if (modelData.broken)
                                Game.repair(modelData.index)
                            else if (!modelData.manager && modelData.runningUntil <= 0)
                                Game.run(modelData.index)
                        }
                    }

                    Column {
                        x: Theme.horizontalPageMargin
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width * 0.5

                        Label {
                            text: genName(modelData.id)
                                  + (modelData.count > 0 ? "  ×" + modelData.count : "")
                            truncationMode: TruncationMode.Fade
                            width: parent.width
                        }
                        Label {
                            text: {
                                if (modelData.count <= 0) return ""
                                if (modelData.broken)
                                    return qsTr("out of order") + " · "
                                         + Game.fmt(modelData.repairCost)
                                var s
                                if (modelData.manager)
                                    s = "+" + Game.fmt(modelData.rate) + "/s · " + qsTr("managed")
                                else
                                    s = "▶ " + Game.fmt(modelData.payout)
                                      + " / " + (modelData.cycleMs / 1000) + " s"
                                if (modelData.nextAt > 0)
                                    s += " · ×2 @ " + modelData.nextAt
                                return s
                            }
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: modelData.broken ? "#C0603A" : Theme.secondaryColor
                        }

                        // Cycle progress.
                        Rectangle {
                            visible: !modelData.manager && modelData.runningUntil > 0
                            width: {
                                if (modelData.runningUntil <= 0) return 0
                                var p = 1 - (modelData.runningUntil - Game.nowMs) / modelData.cycleMs
                                return parent.width * Math.max(0, Math.min(1, p))
                            }
                            height: Math.max(2, Theme.paddingSmall / 2)
                            color: Theme.highlightColor
                        }
                    }

                    Button {
                        anchors {
                            right: managerBtn.visible ? managerBtn.left : parent.right
                            rightMargin: managerBtn.visible ? Theme.paddingSmall
                                                            : Theme.horizontalPageMargin
                            verticalCenter: parent.verticalCenter
                        }
                        preferredWidth: Theme.buttonWidthSmall
                        text: Game.fmt(modelData.cost)
                        enabled: Game.recette >= modelData.cost
                        onClicked: Game.buy(modelData.index)
                    }

                    IconButton {
                        id: managerBtn
                        visible: modelData.count > 0 && !modelData.manager
                        anchors {
                            right: parent.right
                            rightMargin: Theme.paddingSmall
                            verticalCenter: parent.verticalCenter
                        }
                        icon.source: "image://theme/icon-m-people"
                        enabled: Game.recette >= modelData.managerCost
                        onClicked: Game.hire(modelData.index)
                    }
                }
            }

            SectionHeader {
                visible: Game.echoes.length > 0
                text: qsTr("Improvements")
            }

            Repeater {
                model: Game.echoes

                BackgroundItem {
                    width: column.width
                    height: Theme.itemSizeSmall
                    enabled: !modelData.owned && Game.recette >= modelData.cost
                    opacity: modelData.owned ? 0.55 : 1.0
                    onClicked: Game.buyEcho(modelData.index)

                    Label {
                        x: Theme.horizontalPageMargin
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width * 0.6
                        text: echoName(modelData.index)
                        truncationMode: TruncationMode.Fade
                        color: modelData.owned ? Theme.secondaryColor : Theme.primaryColor
                    }
                    Label {
                        anchors {
                            right: parent.right
                            rightMargin: Theme.horizontalPageMargin
                            verticalCenter: parent.verticalCenter
                        }
                        text: modelData.owned
                              ? "+" + modelData.bonus.toFixed(0) + " %"
                              : Game.fmt(modelData.cost)
                        font.pixelSize: Theme.fontSizeSmall
                        color: modelData.owned ? Theme.secondaryColor : Theme.highlightColor
                    }
                }
            }

            // Income sources: the donut, one chart among the others.
            SectionHeader {
                visible: Game.donutVisible
                text: qsTr("Sources")
            }

            Item {
                id: donutCard
                visible: Game.donutVisible
                width: parent.width
                height: donut.height

                property var rows: Game.breakdown()

                Connections {
                    target: Game
                    onStateChanged: {
                        donutCard.rows = Game.breakdown()
                        donut.requestPaint()
                    }
                }

                Canvas {
                    id: donut
                    x: Theme.horizontalPageMargin
                    width: Theme.itemSizeHuge * 1.4
                    height: Theme.itemSizeHuge * 1.4

                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        var rows = donutCard.rows
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
                        model: donutCard.rows

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

            // Park life: the quiet side, in plain sight.
            SectionHeader {
                visible: Game.creatures.length > 0 || Game.soin >= 6
                text: qsTr("Park life")
            }

            Item {
                visible: Game.creatures.length > 0 || Game.soin >= 6
                width: parent.width
                height: lifeCol.height

                Column {
                    id: lifeCol
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

                    Canvas {
                        id: soinChart
                        width: parent.width
                        height: Theme.itemSizeSmall
                        visible: Game.soinHistory().length >= 2

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

                        Connections {
                            target: Game
                            onStateChanged: soinChart.requestPaint()
                        }
                    }
                }
            }

            // The founder.
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

                Canvas {
                    id: sleepChart
                    x: Theme.horizontalPageMargin
                    width: column.width - 2 * Theme.horizontalPageMargin
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

                Label {
                    visible: Game.decisionsVisible
                    x: Theme.horizontalPageMargin
                    text: qsTr("Decisions")
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                }

                Canvas {
                    id: decisionsChart
                    x: Theme.horizontalPageMargin
                    width: column.width - 2 * Theme.horizontalPageMargin
                    height: Theme.itemSizeLarge
                    visible: Game.decisionsVisible

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

                    Connections {
                        target: Game
                        onStateChanged: decisionsChart.requestPaint()
                    }
                }
            }

            Item { width: 1; height: Theme.paddingLarge }

            // Refound: the plain prestige loop.
            BackgroundItem {
                visible: Game.refoundVisible
                width: parent.width
                height: Theme.itemSizeMedium
                onClicked: remorse.execute(qsTr("Refounding"), function() { Game.refound() })

                Column {
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    Label { text: qsTr("Refound the park") }
                    Label {
                        text: qsTr("Start over with a permanent bonus")
                              + "  +" + Game.refoundGainPercent.toFixed(0) + " %"
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: Theme.secondaryColor
                    }
                }
            }

            Item { width: 1; height: Theme.paddingLarge }
        }

        VerticalScrollDecorator { }
    }
}
