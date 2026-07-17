#include "StateProjection.h"
#include "Rng.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>

namespace grain {

namespace {

// Income sources for attribution in the breakdown buckets.
const int kSrcTap = -1;

void addAge(GameState& s, int years)
{
    s.age += years;
    if (s.age > Balance::kMaxAge)
        s.age = Balance::kMaxAge;
}

// Add `base` income from `src`, applying the foundation coefficient on top. The coefficient's
// share always lands in the foundation bucket, whatever the source. Past the current funding
// tier's ceiling the whole flow shrinks (plateau, never a halt). Wealth marks cost years.
void addIncome(GameState& s, double base, int src)
{
    base *= softCapFactor(s);
    const double f = s.opened ? Balance::kFoundation : 0.0;
    const double total = base * (1.0 + f);
    s.recette += total;
    s.epochRecette += total;
    s.allRecette += total;
    s.earnedFoundation += base * f;
    if (src == kSrcTap)
        s.earnedTap += base;
    else if (src >= 0 && src < Balance::GenCount)
        s.earnedGens[src] += base;
    while (s.wealthMarks < 6 && s.epochRecette >= Balance::kWealthMark[s.wealthMarks]) {
        s.wealthMarks += 1;
        addAge(s, Balance::kAgeWealth);
    }
}

// Grant soin and settle new creatures for every threshold crossed (seeded, deterministic).
void addSoin(GameState& s, double amount, quint64 salt)
{
    s.soin += amount;
    s.soinEarned += amount;
    while (s.creatures.size() < Balance::kCreatureCap
           && s.soinEarned >= spawnThreshold(s.creatures.size())) {
        const quint64 roll = Rng::mix(salt, static_cast<quint64>(s.creatures.size()));
        const int species = static_cast<int>(roll % static_cast<quint64>(Balance::kSpeciesCount));
        s.creatures.append(QLatin1String(Balance::kSpecies[species]));
    }
}

// Pay out any manual cycle that has matured by instant `at`, and let managers quietly fix
// what broke on their watch.
void settleCycles(GameState& s, qint64 at)
{
    for (int g = 0; g < Balance::GenCount; ++g) {
        if (s.gens[g].broken && s.gens[g].manager
            && at - s.gens[g].brokenAtMs >= Balance::kAutoRepairMs) {
            s.gens[g].broken = false;
        }
        if (s.gens[g].runningUntilMs > 0 && at >= s.gens[g].runningUntilMs) {
            addIncome(s, cyclePayout(s, g), g);
            s.gens[g].runningUntilMs = 0;
        }
    }
}

// Wear and tear: at most one attraction down at a time; odds grow as focus frays and age climbs.
void rollIncident(GameState& s, qint64 ms, qint64 at, quint64 salt)
{
    int owned = 0;
    for (int g = 0; g < Balance::GenCount; ++g) {
        if (s.gens[g].broken)
            return;
        if (s.gens[g].count > 0)
            ++owned;
    }
    if (owned == 0)
        return;
    const double hours = ms / 3600000.0;
    double p = (Balance::kIncidentPerHourBase
                + Balance::kIncidentPerHourFocus * (1.0 - founderFocus(s))
                + (s.age > Balance::kIncidentOldAge ? Balance::kIncidentPerHourOld : 0.0))
             * hours;
    if (p > 0.9)
        p = 0.9;
    const double roll =
        (Rng::mix(salt, static_cast<quint64>(at)) % Q_UINT64_C(100000)) / 100000.0;
    if (roll >= p)
        return;
    int pick = static_cast<int>(
        Rng::mix(salt, static_cast<quint64>(at) ^ Q_UINT64_C(0x9E37)) % static_cast<quint64>(owned));
    for (int g = 0; g < Balance::GenCount; ++g) {
        if (s.gens[g].count > 0 && pick-- == 0) {
            s.gens[g].broken = true;
            s.gens[g].brokenAtMs = at;
            s.gens[g].runningUntilMs = 0;   // a cycle in flight is lost
            s.incidents += 1;
            return;
        }
    }
}

QJsonObject parse(const Event& e)
{
    return QJsonDocument::fromJson(e.payload.toUtf8()).object();
}

void touch(GameState& s, qint64 atMs)
{
    if (atMs > s.lastSeenMs)
        s.lastSeenMs = atMs;
}

} // namespace

double genCost(const GameState& s, int g)
{
    const Balance::GenDef& d = Balance::kGens[g];
    return d.baseCost * std::pow(d.costGrowth, s.gens[g].count);
}

double bulkCost(const GameState& s, int g, int n)
{
    if (n <= 1)
        return genCost(s, g);
    const double r = Balance::kGens[g].costGrowth;
    return genCost(s, g) * (std::pow(r, n) - 1.0) / (r - 1.0);
}

double managerCost(int g)
{
    return Balance::kGens[g].managerCost;
}

double genMultiplier(int count)
{
    double m = 1.0;
    for (int i = 0; i < 3; ++i) {
        if (count >= Balance::kProdMilestones[i])
            m *= 2.0;
    }
    return m;
}

qint64 genCycleMs(const GameState& s, int g)
{
    qint64 ms = Balance::kGens[g].cycleMs;
    for (int i = 0; i < 2; ++i) {
        if (s.gens[g].count >= Balance::kSpeedMilestones[i])
            ms /= 2;
    }
    return ms;
}

double softCapFactor(const GameState& s)
{
    const double ceiling = tierCeiling(s);
    if (ceiling <= 0.0 || s.epochRecette <= ceiling)
        return 1.0;
    return ceiling / (ceiling + (s.epochRecette - ceiling));
}

int nextMilestoneAt(int count)
{
    for (int i = 0; i < Balance::kMilestoneCount; ++i) {
        if (count < Balance::kMilestones[i])
            return Balance::kMilestones[i];
    }
    return 0;
}

double creatureMult(const GameState& s)
{
    return 1.0 + Balance::kCreaturePer * s.creatures.size();
}

bool echoOwned(const GameState& s, int i)
{
    return (s.echoes & (1u << i)) != 0;
}

double echoMult(const GameState& s)
{
    double sum = 0.0;
    for (int i = 0; i < Balance::kEchoCount; ++i) {
        if (echoOwned(s, i))
            sum += Balance::kEchoBonus[i];
    }
    return 1.0 + sum;
}

int momentsResolved(const GameState& s)
{
    return s.buried + s.sat;
}

double founderSleep(const GameState& s)
{
    const double v = 1.0 - Balance::kSleepPerBury * s.buried + Balance::kSleepPerSit * s.sat;
    return v < Balance::kSleepFloor ? Balance::kSleepFloor : (v > 1.0 ? 1.0 : v);
}

double founderFocus(const GameState& s)
{
    const double v = 1.0 - Balance::kFocusPerBury * s.buried + Balance::kFocusPerSit * s.sat;
    return v < Balance::kFocusFloor ? Balance::kFocusFloor : (v > 1.0 ? 1.0 : v);
}

int founderAge(const GameState& s)
{
    return s.age;
}

bool herGone(const GameState& s)
{
    return s.epoch >= 1;
}

int flowerStage(const GameState& s)
{
    if (!herGone(s)) {
        // She plants something every couple of years. Nobody says so.
        const int stage = (s.age - Balance::kStartAge) / 2;
        return stage > 6 ? 6 : stage;
    }
    if (s.flowerFrozen <= 0)
        return 0;
    const int faded = s.flowerFrozen - (s.age - s.departureAge) / 2;
    return faded > 0 ? faded : 0;
}

double sleepFactor(const GameState& s)
{
    const double t = (founderSleep(s) - Balance::kSleepFloor) / (1.0 - Balance::kSleepFloor);
    return Balance::kSleepWorkFloor + (1.0 - Balance::kSleepWorkFloor) * t;
}

double focusFactor(const GameState& s)
{
    const double t = (founderFocus(s) - Balance::kFocusFloor) / (1.0 - Balance::kFocusFloor);
    return Balance::kFocusFlowFloor + (1.0 - Balance::kFocusFlowFloor) * t;
}

double repairCost(const GameState& s, int g)
{
    const double c = Balance::kRepairCostSecs * genRate(s, g);
    return c > Balance::kSitCostMin ? c : Balance::kSitCostMin;
}

double tierCeiling(const GameState& s)
{
    if (s.raised >= Balance::kTierCount)
        return -1.0;
    return Balance::kCeilingFactor * Balance::kTierThreshold[s.raised];
}

bool raiseReady(const GameState& s, qint64 nowMs)
{
    if (s.raised >= Balance::kTierCount || s.age >= Balance::kMaxAge)
        return false;
    if (s.epochRecette < Balance::kTierThreshold[s.raised])
        return false;
    return raiseCooldownLeftMs(s, nowMs) == 0;
}

qint64 raiseCooldownLeftMs(const GameState& s, qint64 nowMs)
{
    if (s.raised >= Balance::kTierCount || s.lastRaiseMs == 0)
        return 0;
    const qint64 readyAt = s.lastRaiseMs + Balance::kTierCooldownMs[s.raised];
    return nowMs >= readyAt ? 0 : readyAt - nowMs;
}

double genRate(const GameState& s, int g)
{
    return s.gens[g].count * Balance::kGens[g].baseRate
         * genMultiplier(s.gens[g].count) * s.bonusMult * creatureMult(s) * echoMult(s);
}

double cyclePayout(const GameState& s, int g)
{
    if (s.gens[g].broken)
        return 0.0;
    return genRate(s, g) * (genCycleMs(s, g) / 1000.0) * sleepFactor(s);
}

double tapValue(const GameState& s)
{
    const double manual = (Balance::kTapBase + Balance::kTapPerGate * s.gens[Balance::Gate].count)
                        * s.bonusMult * creatureMult(s) * echoMult(s);
    // Keep tapping meaningful once managers carry the park: a tap tracks the ticker.
    const double share = Balance::kTapRpsShare * baseRps(s);
    return (manual > share ? manual : share) * sleepFactor(s);
}

double baseRps(const GameState& s)
{
    double sum = 0.0;
    for (int g = 0; g < Balance::GenCount; ++g) {
        if (s.gens[g].manager && !s.gens[g].broken)
            sum += genRate(s, g);
    }
    return sum * focusFactor(s);
}

double totalRps(const GameState& s)
{
    const double f = s.opened ? Balance::kFoundation : 0.0;
    return baseRps(s) * (1.0 + f);
}

double soinPerSec(const GameState& s)
{
    double sum = Balance::kSoinBase;
    for (int g = 0; g < Balance::GenCount; ++g) {
        if (!s.gens[g].broken)
            sum += s.gens[g].count * Balance::kGens[g].soinRate;
    }
    // She was the one at the aviary. The trickle barely survives her.
    if (herGone(s))
        sum *= Balance::kAbsenceSoinScale;
    return sum;
}

double sitCost(const GameState& s)
{
    const double c = Balance::kSitCostSeconds * totalRps(s);
    return c > Balance::kSitCostMin ? c : Balance::kSitCostMin;
}

double spawnThreshold(int index)
{
    return Balance::kSpawnFirst * std::pow(Balance::kSpawnGrowth, index);
}

qint64 momentIntervalMs(const GameState& s, quint64 salt)
{
    int eff = s.buried - s.sat - s.eased;
    if (eff < 0)
        eff = 0;
    // The very first moment lands within the opening session; later ones are episodic.
    double interval = (s.buried + s.sat == 0)
        ? double(Balance::kMomentFirstMs)
        : Balance::kMomentBaseMs * std::pow(Balance::kMomentDecay, eff);
    // Seeded jitter in [0.75, 1.25) so the cadence never feels like a counter.
    const quint64 roll = Rng::mix(salt, static_cast<quint64>(s.buried + s.sat) + Q_UINT64_C(0x51));
    interval *= 0.75 + (roll % 1000) / 2000.0;
    qint64 ms = static_cast<qint64>(interval);
    if (s.buried + s.sat == 0)
        return ms;
    return ms > Balance::kMomentFloorMs ? ms : Balance::kMomentFloorMs;
}

bool momentDue(const GameState& s, quint64 salt, qint64 nowMs)
{
    if (!s.opened)
        return false;
    const qint64 anchor = s.lastMomentMs > s.openedAtMs ? s.lastMomentMs : s.openedAtMs;
    return nowMs - anchor >= momentIntervalMs(s, salt);
}

void applyEvent(GameState& s, const Event& e, quint64 salt)
{
    const QJsonObject p = parse(e);
    const qint64 at = static_cast<qint64>(p.value(QLatin1String("at")).toDouble());

    // Matured manual cycles pay out first, at this event's instant, whatever the event is.
    settleCycles(s, at);

    if (e.kind == QLatin1String("arrive")) {
        if (!s.arrived) {
            s.arrived = true;
            s.arrivedAtMs = at;
        }
    } else if (e.kind == QLatin1String("improve")) {
        const int i = p.value(QLatin1String("i")).toInt(-1);
        if (i >= 0 && i < Balance::kEchoCount && !echoOwned(s, i)
            && momentsResolved(s) >= i + 1 && s.recette >= Balance::kEchoCost[i]) {
            s.recette -= Balance::kEchoCost[i];
            s.echoes |= (1u << i);
        }
    } else if (e.kind == QLatin1String("tap")) {
        const int n = p.value(QLatin1String("n")).toInt();
        if (n > 0)
            addIncome(s, tapValue(s) * n, kSrcTap);
    } else if (e.kind == QLatin1String("buy")) {
        const int g = p.value(QLatin1String("g")).toInt();
        int n = p.value(QLatin1String("n")).toInt();
        if (n <= 0)
            n = 1;
        if (g >= 0 && g < Balance::GenCount) {
            const double cost = bulkCost(s, g, n);
            if (s.recette >= cost) {
                s.recette -= cost;
                s.gens[g].count += n;
            }
        }
    } else if (e.kind == QLatin1String("hire")) {
        const int g = p.value(QLatin1String("g")).toInt();
        if (g >= 0 && g < Balance::GenCount && s.gens[g].count > 0 && !s.gens[g].manager
            && s.recette >= managerCost(g)) {
            s.recette -= managerCost(g);
            s.gens[g].manager = true;
            s.gens[g].runningUntilMs = 0;   // the manager takes over; no cycle overlap
        }
    } else if (e.kind == QLatin1String("run")) {
        const int g = p.value(QLatin1String("g")).toInt();
        if (g >= 0 && g < Balance::GenCount && s.gens[g].count > 0 && !s.gens[g].manager
            && !s.gens[g].broken && s.gens[g].runningUntilMs == 0) {
            s.gens[g].runningUntilMs = at + genCycleMs(s, g);
        }
    } else if (e.kind == QLatin1String("repair")) {
        const int g = p.value(QLatin1String("g")).toInt();
        if (g >= 0 && g < Balance::GenCount && s.gens[g].broken
            && s.recette >= repairCost(s, g)) {
            s.recette -= repairCost(s, g);
            s.gens[g].broken = false;
        }
    } else if (e.kind == QLatin1String("raise")) {
        const int i = p.value(QLatin1String("i")).toInt(-1);
        const bool fastChoice = p.value(QLatin1String("fast")).toBool();
        if (i == s.raised && i >= 0 && i < Balance::kTierCount
            && s.epochRecette >= Balance::kTierThreshold[i]
            && (s.lastRaiseMs == 0 || at - s.lastRaiseMs >= Balance::kTierCooldownMs[i])) {
            s.raised += 1;
            s.lastRaiseMs = at;
            addAge(s, Balance::kAgeRaise);
            if (fastChoice) {
                s.raisedFast += 1;
            } else {
                s.raisedSlow += 1;
                s.eased += 1;
                addSoin(s, Balance::kRaiseSlowSoin * (i + 1), salt);
            }
        }
    } else if (e.kind == QLatin1String("open")) {
        // Available once per epoch: every refounded park gets its own inauguration.
        if (!s.opened && s.recette >= Balance::kOpeningCost) {
            s.recette -= Balance::kOpeningCost;
            s.opened = true;
            s.openedAtMs = at;
            addAge(s, Balance::kAgeOpen);
            // The inauguration bonus itself belongs to the foundation bucket.
            s.recette += Balance::kOpeningInstant;
            s.epochRecette += Balance::kOpeningInstant;
            s.allRecette += Balance::kOpeningInstant;
            s.earnedFoundation += Balance::kOpeningInstant;
        }
    } else if (e.kind == QLatin1String("tick")) {
        qint64 ms = static_cast<qint64>(p.value(QLatin1String("ms")).toDouble());
        if (ms < 0)
            ms = 0;
        if (ms > Balance::kOfflineCapMs)
            ms = Balance::kOfflineCapMs;
        const double secs = ms / 1000.0;
        const double flowFactor = focusFactor(s);
        for (int g = 0; g < Balance::GenCount; ++g) {
            if (s.gens[g].manager && !s.gens[g].broken)
                addIncome(s, genRate(s, g) * flowFactor * secs, g);
        }
        addSoin(s, soinPerSec(s) * secs, salt);
        rollIncident(s, ms, at, salt);
    } else if (e.kind == QLatin1String("care")) {
        const QString k = p.value(QLatin1String("k")).toString();
        if (k == QLatin1String("feed")) {
            if (at - s.lastFeedMs >= Balance::kCareCooldownMs) {
                addSoin(s, Balance::kCareFeed, salt);
                s.lastFeedMs = at;
            }
        } else if (k == QLatin1String("linger")) {
            if (at - s.lastLingerMs >= Balance::kCareCooldownMs) {
                addSoin(s, Balance::kCareLinger, salt);
                s.lastLingerMs = at;
            }
        }
    } else if (e.kind == QLatin1String("bury")) {
        s.buried += 1;
        s.lastMomentMs = at;
        if ((s.buried + s.sat) % Balance::kAgeMomentsPer == 0)
            addAge(s, 1);
    } else if (e.kind == QLatin1String("sit")) {
        const double cost = sitCost(s);
        s.recette = s.recette > cost ? s.recette - cost : 0.0;
        s.sat += 1;
        s.lastMomentMs = at;
        addSoin(s, Balance::kCareLinger, salt);
        if ((s.buried + s.sat) % Balance::kAgeMomentsPer == 0)
            addAge(s, 1);
    } else if (e.kind == QLatin1String("confess")) {
        const int stageBefore = flowerStage(s);
        const int gained = static_cast<int>(
            std::floor(std::pow(s.epochRecette / Balance::kPrestigePointDiv,
                                Balance::kPrestigeExp)));
        if (gained > 0)
            s.prestigePoints += gained;
        s.epoch += 1;
        // Each lived life makes a banked point worth a little more.
        s.bonusMult = 1.0 + Balance::kPrestigePer * s.prestigePoints
                          * (1.0 + Balance::kPrestigeEpochBoost * s.epoch);
        s.recette = 0.0;
        s.epochRecette = 0.0;
        s.opened = false;
        s.openedAtMs = 0;
        s.wealthMarks = 0;
        s.raised = 0;
        s.lastRaiseMs = 0;
        addAge(s, Balance::kAgeConfess);
        if (s.epoch == 1) {
            // She leaves. The flowerbed stops changing, then fades over the years.
            s.flowerFrozen = stageBefore;
            s.departureAge = s.age;
        }
        for (int g = 0; g < Balance::GenCount; ++g)
            s.gens[g] = GenState();
        // Soin, creatures, the years and the breakdown history persist: nothing is undone.
    }

    touch(s, at);
}

GameState fold(const QVector<Event>& events, quint64 salt)
{
    GameState s;
    for (int i = 0; i < events.size(); ++i)
        applyEvent(s, events.at(i), salt);
    return s;
}

} // namespace grain
