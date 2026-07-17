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
