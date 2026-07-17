import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    function genName(id) {
        if (id === "gate") return qsTr("The gate")
        if (id === "kiosk") return qsTr("The kiosk")
        if (id === "paths") return qsTr("The paths")
        if (id === "aviary") return qsTr("The aviary")
        return id
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
            MenuItem {
                text: qsTr("The menagerie")
                onClicked: pageStack.push(Qt.resolvedUrl("MenageriePage.qml"))
            }
            MenuItem {
                text: qsTr("Breakdown")
                onClicked: pageStack.push(Qt.resolvedUrl("BreakdownPage.qml"))
            }
        }

        Column {
            id: column
            width: page.width

            PageHeader { title: qsTr("The park") }

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
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
            }
            // The quiet second ticker, easy to never look at.
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: Game.fmt(Game.soin) + " " + qsTr("care")
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                opacity: 0.6
            }
            Label {
                visible: Game.bonusPercent > 0
                anchors.horizontalCenter: parent.horizontalCenter
                text: "+" + Game.bonusPercent.toFixed(0) + " % " + qsTr("permanent")
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
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

                MouseArea {
                    id: tapArea
                    anchors.fill: parent
                    onClicked: {
                        Game.tap()
                        pulse.restart()
                    }
                }

                SequentialAnimation {
                    id: pulse
                    NumberAnimation { target: tapZone; property: "scale"; to: 0.97; duration: 40 }
                    NumberAnimation { target: tapZone; property: "scale"; to: 1.0; duration: 80 }
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

            SectionHeader { text: qsTr("Attractions") }

            Repeater {
                model: Game.generators

                Item {
                    width: column.width
                    height: Theme.itemSizeMedium

                    Column {
                        x: Theme.horizontalPageMargin
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width * 0.45

                        Label {
                            text: genName(modelData.id)
                                  + (modelData.count > 0 ? "  ×" + modelData.count : "")
                            truncationMode: TruncationMode.Fade
                            width: parent.width
                        }
                        Label {
                            text: modelData.count > 0
                                  ? "+" + Game.fmt(modelData.rate) + "/s"
                                    + (modelData.manager ? "  ·  " + qsTr("managed") : "")
                                  : ""
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: Theme.secondaryColor
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
