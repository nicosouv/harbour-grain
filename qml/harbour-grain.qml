import QtQuick 2.6
import Sailfish.Silica 1.0
import "pages"
import "cover"

ApplicationWindow {
    id: app

    // Species palette shared by the park view and the cover (pixel-art pass comes later).
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
        if (key === "openAgain") return qsTr("Same party. Same words. Still works.")
        if (key === "refound") return qsTr("This time, we do it right.")
        if (key === "refound2") return qsTr("Once more. Cleanly.")
        if (key === "refound3") return qsTr("Starting over, I know how. It's staying I never learned.")
        if (key === "bury1") return qsTr("There. Gone.")
        if (key === "sit1") return qsTr("I stayed a while. The park can wait.")
        if (key === "bury5") return qsTr("It comes back more often, doesn't it?")
        if (key === "sit5") return qsTr("She said I seemed calm tonight.")
        if (key === "wealth10k") return qsTr("Ten thousand. We toasted at the kiosk.")
        if (key === "wealth100k") return qsTr("A hundred thousand. Dad never believed it.")
        if (key === "wealth1m") return qsTr("A million. I signed without reading.")
        if (key === "wealth10m") return qsTr("Ten million. The notary calls me sir.")
        if (key === "wealth100m") return qsTr("A hundred million. It doesn't feel like anything.")
        if (key === "wealth1b") return qsTr("A billion. The first bill is framed in the office.")
        if (key === "milestone50") return qsTr("Numbers double. Everything must double.")
        if (key === "milestone100") return qsTr("A hundred. I remember when one was enough.")
        if (key === "milestone200") return qsTr("Two hundred. She stopped counting with me.")
        if (key === "milestone400") return qsTr("Four hundred. Who is this for, again?")
        if (key === "manager_gate") return qsTr("Someone else will open the gate.")
        if (key === "manager_kiosk") return qsTr("I don't know the summer staff anymore.")
        if (key === "manager_paths") return qsTr("People tell me the paths are lovely.")
        if (key === "manager_aviary") return qsTr("She spends more time at the aviary than I do.")
        if (key === "manager_carousel") return qsTr("I don't hear the carousel music anymore.")
        if (key === "manager_pond") return qsTr("The park runs without me now.")
        if (key === "hands_off") return qsTr("I don't need to be here anymore.")
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
        if (key === "first_gate") return qsTr("A gate. It's a start.")
        if (key === "first_kiosk") return qsTr("Waffle smell, already.")
        if (key === "first_paths") return qsTr("Clean paths. People follow paths.")
        if (key === "first_aviary") return qsTr("She wanted birds. Birds it is.")
        if (key === "first_carousel") return qsTr("A carousel. Like in her village.")
        if (key === "first_pond") return qsTr("A pond. People throw coins in it.")
        if (key === "first_milestone") return qsTr("Twenty-five. We count in dozens now.")
        if (key === "donut") return qsTr("Where the money comes from. Good question.")
        if (key === "creature1") return qsTr("Something has settled in the aviary.")
        if (key === "creature5") return qsTr("They stay. Even in winter.")
        if (key === "creature12") return qsTr("She named them all. I forget names.")
        if (key === "birthday30") return qsTr("Thirty. The park is as old as our start.")
        if (key === "birthday40") return qsTr("Forty. She says I work too much.")
        if (key === "birthday50") return qsTr("Fifty. Knees, back, books.")
        if (key === "birthday60") return qsTr("Sixty. The regulars have grandkids now.")
        if (key === "birthday70") return qsTr("Seventy. I know every creak of this place.")
        if (key.indexOf("anniv") === 0) {
            var y = parseInt(key.substring(5), 10)
            if (y < 10) {
                var v1 = y % 3
                if (v1 === 0) return qsTr("Year %1. Cake at the kiosk.").arg(y)
                if (v1 === 1) return qsTr("Year %1. She gave a speech, again.").arg(y)
                return qsTr("Year %1. Fireworks, small ones.").arg(y)
            }
            if (y < 20) {
                var v2 = y % 3
                if (v2 === 0) return qsTr("Year %1. Regulars since day one.").arg(y)
                if (v2 === 1) return qsTr("Year %1. The photos are yellowing in the office.").arg(y)
                return qsTr("Year %1. The oak by the gate is taller than the gate.").arg(y)
            }
            if (y < 30) {
                var v3 = y % 3
                if (v3 === 0) return qsTr("Year %1. Who remembers the first summer?").arg(y)
                if (v3 === 1) return qsTr("Year %1. We stopped counting.").arg(y)
                return qsTr("Year %1. Same speech. I know the silences by heart.").arg(y)
            }
            if (y < 40) {
                return (y % 2 === 0)
                    ? qsTr("Year %1. Third generation of visitors.").arg(y)
                    : qsTr("Year %1. The cake tastes like every other year.").arg(y)
            }
            return (y % 2 === 0)
                ? qsTr("Year %1. We hold the party indoors now.").arg(y)
                : qsTr("Year %1. She held my hand through the speech.").arg(y)
        }
        if (key.indexOf("static") === 0) {
            var v = parseInt(key.substring(6), 10)
            if (v === 0) return qsTr("Everything is fine.")
            if (v === 1) return qsTr("She asked if I was okay.")
            if (v === 2) return qsTr("The gate creaks. It's the wind.")
            if (v === 3) return qsTr("The books are right. The books are right.")
            if (v === 4) return qsTr("Sleep. I should sleep.")
            if (v === 5) return qsTr("What was it like, at the start?")
            if (v === 6) return qsTr("Don't look at the aviary. Work.")
            if (v === 7) return qsTr("It was a long time ago. It's nothing now.")
            if (v === 8) return qsTr("She trusts me. That's the worst part.")
            if (v === 9) return qsTr("The ledger balances. I checked twice.")
            if (v === 10) return qsTr("Everyone lies a little. Everyone.")
            return qsTr("If I say it now, what's left?")
        }
        return ""
    }

    // The dismiss button IS the narrator's rationalization: the player taps the excuse.
    function narrationButton(key) {
        if (key === "open") return qsTr("Keep going!")
        if (key === "openAgain") return qsTr("It works.")
        if (key === "refound") return qsTr("Back to work.")
        if (key === "refound2") return qsTr("Again.")
        if (key === "refound3") return qsTr("Start.")
        if (key === "bury1") return qsTr("There.")
        if (key === "sit1") return qsTr("It can wait.")
        if (key === "bury5") return qsTr("No.")
        if (key === "sit5") return qsTr("Yes.")
        if (key === "wealth10k") return qsTr("Cheers.")
        if (key === "wealth100k") return qsTr("Well.")
        if (key === "wealth1m") return qsTr("As usual.")
        if (key === "wealth10m") return qsTr("Very well.")
        if (key === "wealth100m") return qsTr("Next.")
        if (key === "wealth1b") return qsTr("Under the photo.")
        if (key === "milestone50") return qsTr("Always.")
        if (key === "milestone100") return qsTr("Moving on.")
        if (key === "milestone200") return qsTr("Never mind.")
        if (key === "milestone400") return qsTr("For us.")
        if (key === "manager_gate") return qsTr("Good idea.")
        if (key === "manager_kiosk") return qsTr("Normal.")
        if (key === "manager_paths") return qsTr("Good.")
        if (key === "manager_aviary") return qsTr("To each their post.")
        if (key === "manager_carousel") return qsTr("Habit.")
        if (key === "manager_pond") return qsTr("That's the point.")
        if (key === "hands_off") return qsTr("Finally.")
        if (key === "echo1") return qsTr("Onward.")
        if (key === "echo2") return qsTr("Good.")
        if (key === "echo3") return qsTr("Anyway.")
        if (key === "echo4") return qsTr("Moving on.")
        if (key === "echo5") return qsTr("It's fine.")
        if (key === "echo6") return qsTr("No time.")
        if (key === "echo7") return qsTr("Nonsense.")
        if (key === "echo8") return qsTr("Right. Go.")
        if (key === "echo9") return qsTr("No choice…")
        if (key === "echo10") return qsTr("Too late anyway.")
        if (key === "first_gate") return qsTr("Open up.")
        if (key === "first_kiosk") return qsTr("Perfect.")
        if (key === "first_paths") return qsTr("Good.")
        if (key === "first_aviary") return qsTr("Okay.")
        if (key === "first_carousel") return qsTr("She'll smile.")
        if (key === "first_pond") return qsTr("Good for us.")
        if (key === "first_milestone") return qsTr("Moving on.")
        if (key === "donut") return qsTr("Let's see.")
        if (key === "creature1") return qsTr("Good.")
        if (key === "creature5") return qsTr("They're fine here.")
        if (key === "creature12") return qsTr("She handles it.")
        if (key === "birthday30") return qsTr("Happy birthday.")
        if (key === "birthday40") return qsTr("She exaggerates.")
        if (key === "birthday50") return qsTr("Still standing.")
        if (key === "birthday60") return qsTr("Already.")
        if (key === "birthday70") return qsTr("One more round.")
        if (key.indexOf("anniv") === 0) {
            var y = parseInt(key.substring(5), 10)
            if (y < 10) {
                var v1 = y % 3
                if (v1 === 0) return qsTr("See you next year.")
                if (v1 === 1) return qsTr("We clap.")
                return qsTr("Pretty.")
            }
            if (y < 20) {
                var v2 = y % 3
                if (v2 === 0) return qsTr("Anyway.")
                if (v2 === 1) return qsTr("Doesn't matter.")
                return qsTr("It grows.")
            }
            if (y < 30) {
                var v3 = y % 3
                if (v3 === 0) return qsTr("Me.")
                if (v3 === 1) return qsTr("We keep going.")
                return qsTr("Anyway.")
            }
            if (y < 40) return (y % 2 === 0) ? qsTr("Already.") : qsTr("Eat.")
            return (y % 2 === 0) ? qsTr("Warmer.") : qsTr("We keep going.")
        }
        if (key.indexOf("static") === 0) {
            var v = parseInt(key.substring(6), 10)
            if (v === 0) return qsTr("Yes.")
            if (v === 1) return qsTr("I'm fine.")
            if (v === 2) return qsTr("Probably.")
            if (v === 3) return qsTr("There.")
            if (v === 4) return qsTr("Later.")
            if (v === 5) return qsTr("Doesn't matter.")
            if (v === 6) return qsTr("Work.")
            if (v === 7) return qsTr("Nothing.")
            if (v === 8) return qsTr("Enough.")
            if (v === 9) return qsTr("Three times.")
            if (v === 10) return qsTr("Everyone.")
            return qsTr("Exactly.")
        }
        return qsTr("Onward.")
    }

    function maybeNarrate() {
        if (narrator.visible || !Game.arrived)
            return
        var key = Game.pendingNarration
        if (key !== "")
            narrator.show(narrationText(key), narrationButton(key))
    }

    initialPage: Component { ParkPage { } }
    cover: Component { CoverPage { } }
    allowedOrientations: defaultAllowedOrientations

    Component.onCompleted: maybeNarrate()

    // Persist pending taps and elapsed production whenever the app goes to the background.
    Connections {
        target: Qt.application
        onActiveChanged: {
            if (!Qt.application.active) {
                Game.flushNow()
            } else {
                Game.appActivated()
                app.maybeNarrate()
            }
        }
    }

    Connections {
        target: Game
        onStateChanged: app.maybeNarrate()
    }

    // Interference: the page underneath glitches while a stray thought passes. Dismissed only
    // by the button below — frantic tapping can't skip a line.
    Item {
        id: narrator
        anchors.fill: parent
        visible: false
        z: 9000

        property real t: 0
        property string line: ""
        property string buttonLine: ""

        function show(text, btn) {
            line = text
            buttonLine = btn
            t = 0
            visible = true
            glitchAnim.restart()
            armTimer.restart()
        }

        function dismiss() {
            if (!visible)
                return
            visible = false
            glitchAnim.stop()
            Game.ackNarration()
        }

        // Swallow every touch that isn't the button.
        MouseArea { anchors.fill: parent }

        // Opaque fallback: even if the shader can't run, the page below goes dark.
        Rectangle {
            anchors.fill: parent
            color: "#10131A"
            opacity: 0.92
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
                    highp float band = floor(uv.y * 48.0);
                    highp float jump = step(0.72, hash(band + floor(time * 9.0)));
                    uv.x += (hash(band + floor(time * 7.0)) - 0.5) * 0.09 * jump;
                    uv.y += (hash(floor(time * 5.0)) - 0.5) * 0.006;
                    lowp vec4 c = texture2D(source, uv);
                    lowp float g = dot(c.rgb, vec3(0.299, 0.587, 0.114));
                    lowp vec3 m = mix(c.rgb, vec3(g), 0.75) * 0.4;
                    lowp float n = (hash(uv.x * 157.0 + uv.y * 311.0 + floor(time * 24.0)) - 0.5) * 0.14;
                    gl_FragColor = vec4(m + vec3(n), 1.0) * qt_Opacity;
                }"
        }

        NumberAnimation {
            id: glitchAnim
            target: narrator
            property: "t"
            from: 0; to: 60
            duration: 60000
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

        // Armed after a beat, so a tap already in flight can't dismiss the thought.
        Timer {
            id: armTimer
            interval: 800
        }

        Button {
            anchors {
                bottom: parent.bottom
                bottomMargin: Theme.itemSizeMedium
                horizontalCenter: parent.horizontalCenter
            }
            text: narrator.buttonLine
            enabled: !armTimer.running
            opacity: armTimer.running ? 0.3 : 1.0
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
