// Every tuning constant in one header. Numbers here are gameplay balance, expected to move
// freely during playtests; nothing else in the engine hardcodes a curve.
#ifndef GRAIN_BALANCE_H
#define GRAIN_BALANCE_H

#include <QtGlobal>

namespace grain {
namespace Balance {

// Generator identities. Index order is frozen: it is the storage format of per-generator state.
enum Gen {
    Gate = 0,
    Kiosk,
    Paths,
    Aviary,
    Carousel,
    Pond,
    Wheel,
    Greenhouse,
    Museum,
    GenCount
};

struct GenDef {
    const char* id;       // stable string id (events, breakdown rows, QML lookup)
    double baseCost;      // cost of the first unit
    double costGrowth;    // cost multiplier per unit owned (the exponential wall)
    double baseRate;      // recette per second per unit (managed flow / cycle payout basis)
    double managerCost;   // one-time hire that automates the generator
    double soinRate;      // soin per second per unit (the quiet side)
    qint64 cycleMs;       // manual cycle length when unmanaged (tap to run, paid on completion)
};

static const GenDef kGens[GenCount] = {
    { "gate",        15.0, 1.13,    0.6,      600.0, 0.0,  Q_INT64_C(3000)   },
    { "kiosk",      120.0, 1.14,    4.0,     6000.0, 0.0,  Q_INT64_C(6000)   },
    { "paths",     1500.0, 1.15,   30.0,    60000.0, 0.0,  Q_INT64_C(15000)  },
    { "aviary",   18000.0, 1.16,  160.0,   500000.0, 0.05, Q_INT64_C(30000)  },
    { "carousel", 240000.0, 1.15, 1000.0,  3000000.0, 0.0,  Q_INT64_C(60000)  },
    { "pond",    3200000.0, 1.16, 6500.0, 40000000.0, 0.1,  Q_INT64_C(120000) },
    { "wheel",   45000000.0, 1.15, 45000.0, 500000000.0, 0.0, Q_INT64_C(240000) },
    { "greenhouse", 620000000.0, 1.16, 300000.0, 7000000000.0, 0.4, Q_INT64_C(480000) },
    { "museum",  8500000000.0, 1.17, 2000000.0, 90000000000.0, 0.0, Q_INT64_C(960000) },
};

// Owning enough units doubles a generator's output at each of these counts.
static const int kMilestones[] = { 25, 50, 100, 200, 400 };
static const int kMilestoneCount = 5;

// Manual taps.
static const double kTapBase    = 1.0;   // recette per tap
static const double kTapPerGate = 0.25;  // taps scale gently with gates owned
static const double kTapRpsShare = 0.01; // late game: a tap is worth >= 1% of managed income

// The grand opening: a one-shot inauguration node, offered once per epoch.
static const double kOpeningUnlock  = 100.0;  // epoch recette earned before the node shows
static const double kOpeningCost    = 150.0;
static const double kOpeningInstant = 500.0;  // immediate bonus on inauguration
static const double kFoundation     = 0.8;    // permanent coefficient applied to all income after it

// Soin: the slow, quiet currency.
static const double kSoinBase        = 0.005; // passive trickle per second
static const double kCareFeed        = 4.0;   // cover gesture rewards
static const double kCareLinger      = 6.0;
static const qint64 kCareCooldownMs  = Q_INT64_C(1800000);   // 30 min per gesture

// Menagerie: creatures settle in at cumulative-soin thresholds (seeded pick from the table).
static const double kSpawnFirst  = 12.0;
static const double kSpawnGrowth = 2.2;
static const int    kCreatureCap = 64;
static const double kCreaturePer = 0.005;   // each settled creature adds +0.5% to all income
static const int    kSpeciesCount = 8;
static const char* const kSpecies[kSpeciesCount] = {
    "sparrow", "rabbit", "hedgehog", "duck", "squirrel", "turtle", "frog", "koi"
};

// Cover moments: the first lands within the opening session, then episodic; cadence tightens
// with each 'bury'.
static const qint64 kMomentFirstMs = Q_INT64_C(600000);      // ~10 min after the opening
static const qint64 kMomentBaseMs  = Q_INT64_C(3600000);     // then ~1 h between moments
static const double kMomentDecay   = 0.85;                   // interval factor per net bury
static const qint64 kMomentFloorMs = Q_INT64_C(1500000);     // never more often than 25 min
static const double kSitCostSeconds = 900.0;                 // sit costs ~15 min of income
static const double kSitCostMin     = 50.0;

// Refound (prestige): banked permanent bonus, classic idle reset. Offered only when it pays.
static const double kPrestigePer      = 0.10;     // +10% income per point
static const double kPrestigePointDiv = 100000.0; // points = floor(sqrt(epochRecette / div))

// Improvements: one-shot upgrades, one unlocked per resolved cover moment. Labels live in QML;
// here they are only an index, a price and a small permanent bonus.
static const int kEchoCount = 20;
static const double kEchoCost[kEchoCount] = {
    400.0, 2400.0, 14000.0, 86000.0, 520000.0,
    3100000.0, 19000000.0, 110000000.0, 670000000.0, 4000000000.0,
    2.4e10, 1.4e11, 8.6e11, 5.2e12, 3.1e13,
    1.9e14, 1.1e15, 6.7e15, 4.0e16, 2.4e17
};
static const double kEchoBonus[kEchoCount] = {
    0.02, 0.02, 0.02, 0.03, 0.03, 0.03, 0.04, 0.04, 0.05, 0.05,
    0.05, 0.05, 0.06, 0.06, 0.06, 0.07, 0.07, 0.08, 0.08, 0.10
};

// Funding rounds: the mid-game pacing gate. Production softly plateaus at the current tier's
// ceiling; the next tier needs a raise (a choice) and sits behind a real-time cooldown.
static const int kTierCount = 12;
static const double kTierThreshold[kTierCount] = {
    1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16
};
static const qint64 kTierCooldownMs[kTierCount] = {
    Q_INT64_C(7200000),  Q_INT64_C(14400000),  Q_INT64_C(28800000),  Q_INT64_C(43200000),
    Q_INT64_C(64800000), Q_INT64_C(86400000),  Q_INT64_C(86400000),  Q_INT64_C(129600000),
    Q_INT64_C(129600000), Q_INT64_C(172800000), Q_INT64_C(172800000), Q_INT64_C(259200000)
};
static const double kCeilingFactor = 2.0;   // earn up to 2x the next raise's bar, then...
static const double kSoftCapScale  = 0.1;   // ...income shrinks to 10% (never halts)
static const double kRaiseSlowSoin = 30.0;  // slow/honest raise: care granted per tier index+1

// Age advances with actions, never with the wall clock. Rebuilding costs years too.
static const int kAgeOpen       = 1;   // the grand opening
static const int kAgeWealth     = 2;   // per wealth mark crossed (per epoch)
static const int kAgeRaise      = 3;   // per funding round closed
static const int kAgeConfess    = 2;   // per refound
static const int kAgeMomentsPer = 10;  // +1 year per this many resolved cover moments
static const double kWealthMark[6] = { 1e4, 1e5, 1e6, 1e7, 1e8, 1e9 };

// Founder condition feeds back into the park (the malus side).
static const double kSleepWorkFloor = 0.70;  // taps/cycles factor at minimum sleep
static const double kFocusFlowFloor = 0.80;  // managed flow factor at minimum focus

// Breakdowns: one attraction at a time can fail; repairing costs a slice of its output.
static const double kIncidentPerHourBase  = 0.02;
static const double kIncidentPerHourFocus = 0.06;  // x (1 - focus)
static const double kIncidentPerHourOld   = 0.02;  // added past 60
static const int    kIncidentOldAge       = 60;
static const double kRepairCostSecs       = 300.0;

// After she is gone, the quiet side barely feeds itself.
static const double kAbsenceSoinScale = 0.25;

// Founder readouts: drift per cover choice, clamped. Age never resets — refounding rebuilds
// the park, not the years.
static const int kStartAge = 20;
static const int kMaxAge   = 79;
static const double kSleepPerBury = 0.05;
static const double kSleepPerSit  = 0.02;
static const double kSleepFloor   = 0.35;
static const double kFocusPerBury = 0.04;
static const double kFocusPerSit  = 0.03;
static const double kFocusFloor   = 0.30;

// Ticks.
static const qint64 kOfflineCapMs = Q_INT64_C(604800000);    // 7 days of offline progress
static const qint64 kFlushMs      = Q_INT64_C(30000);        // active tick materialization period

} // namespace Balance
} // namespace grain

#endif // GRAIN_BALANCE_H
