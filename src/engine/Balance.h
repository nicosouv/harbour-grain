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
};

// Owning enough units doubles a generator's output at each of these counts.
static const int kMilestones[] = { 25, 50, 100, 200, 400 };
static const int kMilestoneCount = 5;

// Manual taps.
static const double kTapBase    = 1.0;   // recette per tap
static const double kTapPerGate = 0.25;  // taps scale gently with gates owned

// The grand opening: a one-shot inauguration node, first epoch only.
static const double kOpeningUnlock  = 150.0;  // epoch recette earned before the node shows
static const double kOpeningCost    = 250.0;
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
static const qint64 kMomentFirstMs = Q_INT64_C(1800000);     // ~30 min after the opening
static const qint64 kMomentBaseMs  = Q_INT64_C(10800000);    // then ~3 h between moments
static const double kMomentDecay   = 0.85;                   // interval factor per net bury
static const qint64 kMomentFloorMs = Q_INT64_C(2700000);     // never more often than 45 min
static const double kSitCostSeconds = 900.0;                 // sit costs ~15 min of income
static const double kSitCostMin     = 50.0;

// Refound (prestige): banked permanent bonus, classic idle reset. Offered only when it pays.
static const double kPrestigePer      = 0.10;     // +10% income per point
static const double kPrestigePointDiv = 100000.0; // points = floor(sqrt(epochRecette / div))

// Ticks.
static const qint64 kOfflineCapMs = Q_INT64_C(604800000);    // 7 days of offline progress
static const qint64 kFlushMs      = Q_INT64_C(30000);        // active tick materialization period

} // namespace Balance
} // namespace grain

#endif // GRAIN_BALANCE_H
