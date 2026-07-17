import QtQuick 2.6
import Sailfish.Silica 1.0

// The park's logbook: small facts of the place, noted as they happen. A plain idle log —
// and where the minor beats live so the interference stays rare.
Page {
    id: page

    property var entries: Game.journalKeys()

    Connections {
        target: Game
        onStateChanged: page.entries = Game.journalKeys()
    }

    SilicaListView {
        anchors.fill: parent
        model: page.entries

        header: PageHeader { title: qsTr("Logbook") }

        delegate: Item {
            width: page.width
            height: entryLabel.height + Theme.paddingLarge

            Label {
                id: entryLabel
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                text: app.narrationText(modelData)
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.primaryColor
            }
        }

        ViewPlaceholder {
            enabled: page.entries.length === 0
            text: qsTr("Nothing noted yet.")
        }

        VerticalScrollDecorator { }
    }
}
