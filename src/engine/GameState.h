// The projection target: plain value struct, no behaviour. State is only ever produced by
// folding the event log (see StateProjection); nothing writes it directly.
#ifndef GRAIN_GAMESTATE_H
#define GRAIN_GAMESTATE_H

#include <QStringList>
#include "Balance.h"

namespace grain {

struct GenState {
    int  count = 0;
    bool manager = false;
};

struct GameState {
    int    epoch = 0;            // incremented by each refound; 0 is the first life

    double recette = 0.0;        // spendable balance
    double soin = 0.0;           // quiet balance (not spent in v0.1)
    double epochRecette = 0.0;   // earned this epoch (unlock gates, prestige points)
    double allRecette = 0.0;     // earned across all epochs
    double soinEarned = 0.0;     // cumulative soin (drives menagerie thresholds)

    bool   opened = false;       // grand opening done (only reachable in epoch 0)
    qint64 openedAtMs = 0;

    int    buried = 0;           // cover moment outcomes
    int    sat = 0;
    qint64 lastMomentMs = 0;     // when the last cover moment was resolved

    qint64 lastFeedMs = 0;       // cover care gesture cooldowns
    qint64 lastLingerMs = 0;

    qint64 lastSeenMs = 0;       // latest absolute instant seen in any payload (offline accrual)

    int    prestigePoints = 0;
    double bonusMult = 1.0;      // banked refound multiplier on all income

    GenState gens[Balance::GenCount];

    QStringList creatures;       // species ids, in arrival order (persists across epochs)

    // Cumulative income per source, for the breakdown screen. The foundation bucket collects
    // the coefficient's share of every formula plus the inauguration bonus itself.
    double earnedTap = 0.0;
    double earnedGens[Balance::GenCount] = {};
    double earnedFoundation = 0.0;
};

} // namespace grain

#endif // GRAIN_GAMESTATE_H
