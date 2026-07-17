import QtQuick 2.6
import Sailfish.Silica 1.0
import "pages"
import "cover"

ApplicationWindow {
    id: app

    // Species palette shared by the menagerie and the cover (pixel-art pass comes later).
    function creatureColor(species) {
        var map = {
            "sparrow":  "#C8A15A",
            "rabbit":   "#E8E0D0",
            "hedgehog": "#8A6E4B",
            "duck":     "#4E8C5A",
            "squirrel": "#B0562F",
            "turtle":   "#5E7F52",
            "frog":     "#7DA33F",
            "koi":      "#D06040"
        }
        return map[species] || "#999999"
    }

    function narrationText(key) {
        if (key === "open") return qsTr("We did it. We actually did it.")
        if (key === "refound") return qsTr("This time, we do it right.")
        if (key === "echo1") return qsTr("A coat of paint, and everything starts fresh.")
        if (key === "echo2") return qsTr("Saturdays are full now.")
        if (key === "echo3") return qsTr("That photo. Everyone's smiling in it.")
        if (key === "echo4") return qsTr("Ten years already. She gave a speech.")
        if (key === "echo5") return qsTr("I sleep better here than at home anyway.")
        if (key === "echo6") return qsTr("Third coffee. The numbers won't sit still.")
        if (key === "echo7") return qsTr("The birds stare at me in the morning.")
        if (key === "echo8") return qsTr("I know the story by heart. That's the problem.")
        if (key === "echo9") return qsTr("Old photos are better in a drawer.")
        if (key === "echo10") return qsTr("It passes. It always ends up passing.")
        return ""
    }

    function maybeNarrate() {
        if (narrator.visible || !Game.arrived)
            return
        var key = Game.pendingNarration
        if (key !== "")
            narrator.show(narrationText(key))
    }

    initialPage: Component { ParkPage { } }
    cover: Component { CoverPage { } }
    allowedOrientations: defaultAllowedOrientations

    Component.onCompleted: maybeNarrate()

    // Persist pending taps and elapsed production whenever the app goes to the background.
    Connections {
        target: Qt.application
        onActiveChanged: {
            if (!Qt.application.active)
                Game.flushNow()
            else
                app.maybeNarrate()
        }
    }

    Connections {
        target: Game
        onStateChanged: app.maybeNarrate()
    }

    // Interference: the page underneath glitches while a stray thought passes.
    Item {
        id: narrator
        anchors.fill: parent
        visible: false
        z: 9000

        property real t: 0
        property string line: ""

        function show(text) {
            line = text
            t = 0
            visible = true
            glitchAnim.restart()
            hideTimer.restart()
        }

        function dismiss() {
            if (!visible)
                return
            visible = false
            glitchAnim.stop()
            hideTimer.stop()
            Game.ackNarration()
        }

        ShaderEffectSource {
            id: pageGrab
            sourceItem: pageStack
            hideSource: false
            live: true
        }

        ShaderEffect {
            anchors.fill: parent
            property variant source: pageGrab
            property real time: narrator.t

            fragmentShader: "
                uniform sampler2D source;
                uniform lowp float qt_Opacity;
                uniform highp float time;
                varying highp vec2 qt_TexCoord0;
                highp float hash(highp float n) { return fract(sin(n) * 43758.5453); }
                void main() {
                    highp vec2 uv = qt_TexCoord0;
                    highp float band = floor(uv.y * 64.0);
                    highp float jump = step(0.82, hash(band + floor(time * 9.0)));
                    uv.x += (hash(band + floor(time * 7.0)) - 0.5) * 0.05 * jump;
                    lowp vec4 c = texture2D(source, uv);
                    lowp float g = dot(c.rgb, vec3(0.299, 0.587, 0.114));
                    lowp vec3 m = mix(c.rgb, vec3(g), 0.65) * 0.5;
                    lowp float n = (hash(uv.x * 157.0 + uv.y * 311.0 + floor(time * 24.0)) - 0.5) * 0.12;
                    gl_FragColor = vec4(m + vec3(n), 1.0) * qt_Opacity;
                }"
        }

        NumberAnimation {
            id: glitchAnim
            target: narrator
            property: "t"
            from: 0; to: 20
            duration: 20000
        }

        Timer {
            id: hideTimer
            interval: 5200
            onTriggered: narrator.dismiss()
        }

        // The thought, with a slight chromatic tear.
        Item {
            anchors.centerIn: parent
            width: parent.width - 4 * Theme.horizontalPageMargin

            Label {
                x: 2 + Math.sin(narrator.t * 31.0) * 2
                y: -1
                width: parent.width
                text: narrator.line
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: Theme.fontSizeLarge
                color: "#D04040"
                opacity: 0.45
            }
            Label {
                x: -2 - Math.sin(narrator.t * 27.0) * 2
                y: 1
                width: parent.width
                text: narrator.line
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: Theme.fontSizeLarge
                color: "#40C0D0"
                opacity: 0.45
            }
            Label {
                width: parent.width
                text: narrator.line
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: Theme.fontSizeLarge
                color: Theme.primaryColor
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: narrator.dismiss()
        }
    }

    // First launch (and any fresh life): where this starts.
    Rectangle {
        id: intro
        anchors.fill: parent
        z: 9500
        color: "#161A20"
        visible: !Game.arrived

        MouseArea { anchors.fill: parent }   // swallow input under the overlay

        Column {
            anchors.centerIn: parent
            width: parent.width - 4 * Theme.horizontalPageMargin
            spacing: Theme.paddingLarge

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Grain"
                font.pixelSize: Theme.fontSizeHuge
                color: Theme.highlightColor
            }

            Label {
                width: parent.width
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("You're twenty. A plot at the edge of town, a friend who believes in it as much as you do, and enough money to last one summer.")
                font.pixelSize: Theme.fontSizeMedium
                color: Theme.primaryColor
            }

            Label {
                width: parent.width
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Make it a place people remember.")
                font.pixelSize: Theme.fontSizeMedium
                color: Theme.secondaryHighlightColor
            }

            Item { width: 1; height: Theme.paddingLarge }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Open the park")
                onClicked: Game.arrive()
            }
        }
    }
}
