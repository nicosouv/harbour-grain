// The pure projection: current state = fold of the event log through applyEvent(). Deterministic
// and unit-tested — no I/O, no clock, no globals. Events carry their own timestamps in the
// payload; `salt` is the per-install seed for menagerie picks and cover-moment jitter.
#ifndef GRAIN_STATEPROJECTION_H
#define GRAIN_STATEPROJECTION_H

#include "GameState.h"
#include "EventStore.h"

namespace grain {

// Derived quantities, all pure. These are the formulas the whole game runs on.
double genCost(const GameState& s, int g);          // next-unit cost (exponential wall)
double bulkCost(const GameState& s, int g, int n);  // cost of the next n units (geometric sum)
double managerCost(int g);
double genMultiplier(int count);                    // x2 at production milestones (25/100/400)
qint64 genCycleMs(const GameState& s, int g);       // manual cycle length; halves at 50/200
int    nextMilestoneAt(int count);                  // next milestone count, 0 if past the last
double softCapFactor(const GameState& s);           // income factor past the tier ceiling (<=1)
double creatureMult(const GameState& s);            // small bonus from the menagerie's residents
double echoMult(const GameState& s);                // small bonus from owned improvements
bool   echoOwned(const GameState& s, int i);
int    momentsResolved(const GameState& s);         // buried + sat (unlock counter for echoes)
double founderSleep(const GameState& s);            // readouts, 0..1 — and they feed back:
double founderFocus(const GameState& s);
double sleepFactor(const GameState& s);             // scales taps and manual cycles
double focusFactor(const GameState& s);             // scales the managed flow
int    founderAge(const GameState& s);              // starts at 20, advances with actions only
bool   herGone(const GameState& s);                 // true after the first confession, forever
int    flowerStage(const GameState& s);             // her flowerbed: grows with her, fades after
double repairCost(const GameState& s, int g);       // fixing a broken attraction
double tierCeiling(const GameState& s);             // soft cap on epoch income; <0 = none
bool   raiseReady(const GameState& s, qint64 nowMs); // next funding round can be closed
qint64 raiseCooldownLeftMs(const GameState& s, qint64 nowMs);
double genRate(const GameState& s, int g);          // full per-second output of a generator
double cyclePayout(const GameState& s, int g);      // one manual cycle's payout (pre-foundation)
double tapValue(const GameState& s);                // base recette per tap (pre-foundation)
double baseRps(const GameState& s);                 // summed managed flow (unmanaged runs cycles)
double totalRps(const GameState& s);                // baseRps with foundation applied
double soinPerSec(const GameState& s);
double sitCost(const GameState& s);
double spawnThreshold(int index);                   // cumulative soin needed for creature #index
qint64 momentIntervalMs(const GameState& s, quint64 salt); // cover-moment cadence, seeded jitter

// True when a cover moment should currently be offered (pure in state + now).
bool momentDue(const GameState& s, quint64 salt, qint64 nowMs);

// Fold one event into the state (in place). Unaffordable/invalid events degrade to no-ops so a
// replayed log can never diverge. Matured manual cycles are settled first, at the event's instant.
void applyEvent(GameState& s, const Event& e, quint64 salt);

// Fold a whole log.
GameState fold(const QVector<Event>& events, quint64 salt);

} // namespace grain

#endif // GRAIN_STATEPROJECTION_H
