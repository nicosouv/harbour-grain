import QtQuick 2.6
import Sailfish.Silica 1.0

CoverBackground {
    id: cover

    // The place at a time of day: a warm facet, few numbers.
    function skyColor() {
        var h = new Date().getHours()
        if (h >= 6 && h < 11) return "#3A4A5A"    // morning
        if (h >= 11 && h < 18) return "#46586A"   // day
        if (h >= 18 && h < 22) return "#4A3A4A"   // evening
        return "#242A34"                          // night
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: skyColor() }
            GradientStop { position: 1.0; color: "#1A1E26" }
        }
    }

    Column {
        anchors.centerIn: parent
        width: parent.width - 2 * Theme.paddingLarge
        spacing: Theme.paddingMedium

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Grain"
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.secondaryColor
        }

        Flow {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width
            spacing: Theme.paddingSmall

            Repeater {
                model: Game.creatures

                Rectangle {
                    width: Theme.paddingLarge
                    height: Theme.paddingLarge
                    radius: width / 2
                    color: app.creatureColor(modelData)
                    opacity: 0.85
                }
            }
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Game.fmt(Game.recette)
            font.pixelSize: Theme.fontSizeMedium
            color: Theme.rgba(Theme.primaryColor, 0.5)
        }
    }

    // Captions for the two actions when a moment is up (icons alone would be ambiguous).
    Row {
        visible: Game.momentActive
        anchors {
            bottom: parent.bottom
            bottomMargin: Theme.itemSizeSmall
            horizontalCenter: parent.horizontalCenter
        }
        spacing: Theme.paddingLarge * 2

        Label {
            text: qsTr("Bury it")
            font.pixelSize: Theme.fontSizeTiny
            color: Theme.secondaryColor
        }
        Label {
            text: qsTr("Sit a while")
            font.pixelSize: Theme.fontSizeTiny
            color: Theme.secondaryColor
        }
    }

    CoverActionList {
        id: coverActions

        CoverAction {
            iconSource: Game.momentActive ? "image://theme/icon-cover-cancel"
                                          : "image://theme/icon-cover-new"
            onTriggered: {
                if (Game.momentActive)
                    Game.bury()
                else if (Game.feedReady)
                    Game.care("feed")
            }
        }

        CoverAction {
            iconSource: Game.momentActive ? "image://theme/icon-cover-pause"
                                          : "image://theme/icon-cover-refresh"
            onTriggered: {
                if (Game.momentActive)
                    Game.sit()
                else if (Game.lingerReady)
                    Game.care("linger")
            }
        }
    }
}
