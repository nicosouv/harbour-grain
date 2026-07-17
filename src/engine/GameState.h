// The projection target: plain value struct, no behaviour. State is only ever produced by
// folding the event log (see StateProjection); nothing writes it directly.
#ifndef GRAIN_GAMESTATE_H
#define GRAIN_GAMESTATE_H

#include <QStringList>
#include "Balance.h"

namespace grain {

struct GenState {
    int    count = 0;
    bool   manager = false;
    bool   broken = false;       // out of order until repaired; produces nothing
    qint64 brokenAtMs = 0;       // when it failed (a manager quietly fixes it after a while)
    qint64 runningUntilMs = 0;   // manual cycle in flight; pays out when an event passes this instant
};

struct GameState {
    int    epoch = 0;            // incremented by each refound; 0 is the first life
    bool   arrived = false;      // the player has read the intro and stepped in
    qint64 arrivedAtMs = 0;      // when this life started (age anchor; survives refounds)

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

    quint32 echoes = 0;          // bitmask of owned improvements (persists across epochs)

    int    age = Balance::kStartAge;  // advances with actions only; never resets
    int    wealthMarks = 0;      // wealth thresholds crossed this epoch (each adds years)

    int    departureAge = 0;     // how old the founder was when she left
    int    flowerFrozen = -1;    // her flowerbed's stage at that moment (then it fades)

    int    raised = 0;           // funding tiers closed this epoch
    qint64 lastRaiseMs = 0;      // real-time cooldown anchor for the next raise
    int    raisedFast = 0;       // the lifetime cursor: fast vs slow choices
    int    raisedSlow = 0;
    int    eased = 0;            // cadence relief earned by slow raises

    int    incidents = 0;        // lifetime breakdown count

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
