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

    initialPage: Component { ParkPage { } }
    cover: Component { CoverPage { } }
    allowedOrientations: defaultAllowedOrientations

    // Persist pending taps and elapsed production whenever the app goes to the background.
    Connections {
        target: Qt.application
        onActiveChanged: {
            if (!Qt.application.active)
                Game.flushNow()
        }
    }
}
