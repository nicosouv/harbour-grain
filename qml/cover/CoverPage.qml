import QtQuick 2.6
import Sailfish.Silica 1.0
import "../components"

CoverBackground {
    id: cover

    // The place at a time of day: the warm facet. The park itself lives here.
    function skyColor() {
        var h = new Date().getHours()
        if (h >= 6 && h < 11) return "#3A4A5A"    // morning
        if (h >= 11 && h < 18) return "#46586A"   // day
        if (h >= 18 && h < 22) return "#4A3A4A"   // evening
        return "#242A34"                          // night
    }

    function lightColor() {
        var h = new Date().getHours()
        if (h >= 6 && h < 18) return "#E8DFA0"    // the sun
        return "#D8D8E8"                          // the moon
    }

    Rectangle {
        id: sky
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: parent.height * 0.30
        color: skyColor()

        Rectangle {
            width: Theme.paddingLarge
            height: Theme.paddingLarge
            radius: width / 2
            x: parent.width * 0.68
            y: parent.height * 0.30
            color: lightColor()
            opacity: 0.9
        }

        Label {
            anchors {
                left: parent.left
                leftMargin: Theme.paddingMedium
                top: parent.top
                topMargin: Theme.paddingMedium
            }
            text: "Grain"
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.secondaryColor
        }
    }

    ParkView {
        anchors {
            top: sky.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        age: Game.founderAge
        herGone: Game.herGone
        flowerStage: Game.flowerStage
    }

    Label {
        anchors {
            top: sky.bottom
            topMargin: Theme.paddingSmall
            horizontalCenter: parent.horizontalCenter
        }
        text: Game.fmt(Game.recette)
        font.pixelSize: Theme.fontSizeMedium
        font.family: "Monospace"
        color: Theme.rgba(Theme.primaryColor, 0.6)
    }

    // Feedback when a gesture lands.
    Label {
        id: flashLabel
        anchors.centerIn: parent
        opacity: 0
        font.pixelSize: Theme.fontSizeLarge
        color: Theme.highlightColor

        SequentialAnimation {
            id: flashAnim
            NumberAnimation { target: flashLabel; property: "opacity"; to: 1; duration: 120 }
            PauseAnimation { duration: 700 }
            NumberAnimation { target: flashLabel; property: "opacity"; to: 0; duration: 500 }
        }

        function show(text) {
            flashLabel.text = text
            flashAnim.restart()
        }
    }

    // What the two actions below do right now; dimmed while a gesture rests.
    Row {
        anchors {
            bottom: parent.bottom
            bottomMargin: Theme.itemSizeSmall + Theme.paddingSmall
            horizontalCenter: parent.horizontalCenter
        }
        spacing: Theme.paddingLarge * 2

        Label {
            text: Game.momentActive ? qsTr("Bury it") : qsTr("Feed")
            font.pixelSize: Theme.fontSizeTiny
            color: Theme.primaryColor
            opacity: Game.momentActive || Game.feedReady ? 1.0 : 0.35
        }
        Label {
            text: Game.momentActive ? qsTr("Sit a while") : qsTr("Linger")
            font.pixelSize: Theme.fontSizeTiny
            color: Theme.primaryColor
            opacity: Game.momentActive || Game.lingerReady ? 1.0 : 0.35
        }
    }

    CoverActionList {
        id: coverActions

        CoverAction {
            iconSource: Game.momentActive ? "image://theme/icon-cover-cancel"
                                          : "image://theme/icon-cover-new"
            onTriggered: {
                if (Game.momentActive) {
                    Game.bury()
                } else if (Game.feedReady) {
                    Game.care("feed")
                    flashLabel.show("+" + Game.careFeedValue + " " + qsTr("care"))
                }
            }
        }

        CoverAction {
            iconSource: Game.momentActive ? "image://theme/icon-cover-pause"
                                          : "image://theme/icon-cover-refresh"
            onTriggered: {
                if (Game.momentActive) {
                    Game.sit()
                } else if (Game.lingerReady) {
                    Game.care("linger")
                    flashLabel.show("+" + Game.careLingerValue + " " + qsTr("care"))
                }
            }
        }
    }
}
