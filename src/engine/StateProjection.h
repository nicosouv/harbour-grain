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
double managerCost(int g);
double tapValue(const GameState& s);                // base recette per tap (pre-foundation)
double baseRps(const GameState& s, bool activeSession); // summed base rate; managers only when idle
double totalRps(const GameState& s, bool activeSession); // baseRps with foundation applied
double soinPerSec(const GameState& s);
double sitCost(const GameState& s);
double spawnThreshold(int index);                   // cumulative soin needed for creature #index
qint64 momentIntervalMs(const GameState& s, quint64 salt); // cover-moment cadence, seeded jitter

// True when a cover moment should currently be offered (pure in state + now).
bool momentDue(const GameState& s, quint64 salt, qint64 nowMs);

// Fold one event into the state (in place). Unaffordable/invalid events degrade to no-ops so a
// replayed log can never diverge.
void applyEvent(GameState& s, const Event& e, quint64 salt);

// Fold a whole log.
GameState fold(const QVector<Event>& events, quint64 salt);

} // namespace grain

#endif // GRAIN_STATEPROJECTION_H
