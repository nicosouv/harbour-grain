TARGET = harbour-grain

CONFIG += sailfishapp
CONFIG += c++17

# Version is injected by the spec at build time (%qmake5 VERSION=%{version}).
isEmpty(VERSION) {
    VERSION = 0.1.0
}
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

SOURCES += src/main.cpp \
    src/engine/Rng.cpp \
    src/engine/EventStore.cpp \
    src/engine/StateProjection.cpp \
    src/engine/GrainController.cpp

HEADERS += src/engine/AppId.h \
    src/engine/Balance.h \
    src/engine/Clock.h \
    src/engine/Rng.h \
    src/engine/EventStore.h \
    src/engine/GameState.h \
    src/engine/StateProjection.h \
    src/engine/GrainController.h

QT += sql

DISTFILES += qml/harbour-grain.qml \
    qml/cover/CoverPage.qml \
    qml/pages/ParkPage.qml \
    qml/pages/BreakdownPage.qml \
    qml/pages/MenageriePage.qml \
    qml/pages/SettingsPage.qml \
    rpm/harbour-grain.spec \
    harbour-grain.desktop

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

CONFIG += sailfishapp_i18n

TRANSLATIONS += translations/harbour-grain-en.ts \
                translations/harbour-grain-fr.ts
