import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: page.width

            PageHeader { title: qsTr("The menagerie") }

            Flow {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                spacing: Theme.paddingMedium

                Repeater {
                    model: Game.creatures

                    Rectangle {
                        width: Theme.itemSizeSmall
                        height: Theme.itemSizeSmall
                        radius: width / 2
                        color: app.creatureColor(modelData)
                        opacity: 0.9
                    }
                }
            }

            Item { width: 1; height: Theme.paddingLarge }

            Label {
                x: Theme.horizontalPageMargin
                text: Game.fmt(Game.soin) + " " + qsTr("care")
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
            }

            Label {
                visible: Game.creatures.length > 0
                x: Theme.horizontalPageMargin
                text: "+" + Game.creatureBonusPercent.toFixed(1) + " % " + qsTr("to the park")
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
            }
        }

        ViewPlaceholder {
            enabled: Game.creatures.length === 0
            text: qsTr("It's quiet here.")
        }

        VerticalScrollDecorator { }
    }
}
