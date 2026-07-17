#include "StateProjection.h"
#include "Rng.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>

namespace grain {

namespace {

// Income sources for attribution in the breakdown buckets.
const int kSrcTap = -1;

// Add `base` income from `src`, applying the foundation coefficient on top. The coefficient's
// share always lands in the foundation bucket, whatever the source.
void addIncome(GameState& s, double base, int src)
{
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

double managerCost(int g)
{
    return Balance::kGens[g].managerCost;
}

double tapValue(const GameState& s)
{
    return (Balance::kTapBase + Balance::kTapPerGate * s.gens[Balance::Gate].count) * s.bonusMult;
}

double baseRps(const GameState& s, bool activeSession)
{
    double sum = 0.0;
    for (int g = 0; g < Balance::GenCount; ++g) {
        if (activeSession || s.gens[g].manager)
            sum += s.gens[g].count * Balance::kGens[g].baseRate;
    }
    return sum * s.bonusMult;
}

double totalRps(const GameState& s, bool activeSession)
{
    const double f = s.opened ? Balance::kFoundation : 0.0;
    return baseRps(s, activeSession) * (1.0 + f);
}

double soinPerSec(const GameState& s)
{
    double sum = Balance::kSoinBase;
    for (int g = 0; g < Balance::GenCount; ++g)
        sum += s.gens[g].count * Balance::kGens[g].soinRate;
    return sum;
}

double sitCost(const GameState& s)
{
    const double c = Balance::kSitCostSeconds * totalRps(s, false);
    return c > Balance::kSitCostMin ? c : Balance::kSitCostMin;
}

double spawnThreshold(int index)
{
    return Balance::kSpawnFirst * std::pow(Balance::kSpawnGrowth, index);
}

qint64 momentIntervalMs(const GameState& s, quint64 salt)
{
    int eff = s.buried - s.sat;
    if (eff < 0)
        eff = 0;
    double interval = Balance::kMomentBaseMs * std::pow(Balance::kMomentDecay, eff);
    // Seeded jitter in [0.75, 1.25) so the cadence never feels like a counter.
    const quint64 roll = Rng::mix(salt, static_cast<quint64>(s.buried + s.sat) + Q_UINT64_C(0x51));
    interval *= 0.75 + (roll % 1000) / 2000.0;
    qint64 ms = static_cast<qint64>(interval);
    return ms > Balance::kMomentFloorMs ? ms : Balance::kMomentFloorMs;
}

bool momentDue(const GameState& s, quint64 salt, qint64 nowMs)
{
    if (s.epoch != 0 || !s.opened)
        return false;
    const qint64 anchor = s.lastMomentMs > s.openedAtMs ? s.lastMomentMs : s.openedAtMs;
    return nowMs - anchor >= momentIntervalMs(s, salt);
}

void applyEvent(GameState& s, const Event& e, quint64 salt)
{
    const QJsonObject p = parse(e);
    const qint64 at = static_cast<qint64>(p.value(QLatin1String("at")).toDouble());

    if (e.kind == QLatin1String("tap")) {
        const int n = p.value(QLatin1String("n")).toInt();
        if (n > 0)
            addIncome(s, tapValue(s) * n, kSrcTap);
    } else if (e.kind == QLatin1String("buy")) {
        const int g = p.value(QLatin1String("g")).toInt();
        if (g >= 0 && g < Balance::GenCount) {
            const double cost = genCost(s, g);
            if (s.recette >= cost) {
                s.recette -= cost;
                s.gens[g].count += 1;
            }
        }
    } else if (e.kind == QLatin1String("hire")) {
        const int g = p.value(QLatin1String("g")).toInt();
        if (g >= 0 && g < Balance::GenCount && s.gens[g].count > 0 && !s.gens[g].manager
            && s.recette >= managerCost(g)) {
            s.recette -= managerCost(g);
            s.gens[g].manager = true;
        }
    } else if (e.kind == QLatin1String("open")) {
        if (s.epoch == 0 && !s.opened && s.recette >= Balance::kOpeningCost) {
            s.recette -= Balance::kOpeningCost;
            s.opened = true;
            s.openedAtMs = at;
            // The inauguration bonus itself belongs to the foundation bucket.
            s.recette += Balance::kOpeningInstant;
            s.epochRecette += Balance::kOpeningInstant;
            s.allRecette += Balance::kOpeningInstant;
            s.earnedFoundation += Balance::kOpeningInstant;
        }
    } else if (e.kind == QLatin1String("tick")) {
        qint64 ms = static_cast<qint64>(p.value(QLatin1String("ms")).toDouble());
        const bool active = p.value(QLatin1String("active")).toBool();
        if (ms < 0)
            ms = 0;
        if (ms > Balance::kOfflineCapMs)
            ms = Balance::kOfflineCapMs;
        const double secs = ms / 1000.0;
        for (int g = 0; g < Balance::GenCount; ++g) {
            if (active || s.gens[g].manager)
                addIncome(s, s.gens[g].count * Balance::kGens[g].baseRate * s.bonusMult * secs, g);
        }
        addSoin(s, soinPerSec(s) * secs, salt);
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
    } else if (e.kind == QLatin1String("sit")) {
        const double cost = sitCost(s);
        s.recette = s.recette > cost ? s.recette - cost : 0.0;
        s.sat += 1;
        s.lastMomentMs = at;
        addSoin(s, Balance::kCareLinger, salt);
    } else if (e.kind == QLatin1String("confess")) {
        const int gained = static_cast<int>(
            std::floor(std::sqrt(s.epochRecette / Balance::kPrestigePointDiv)));
        if (gained > 0)
            s.prestigePoints += gained;
        s.bonusMult = 1.0 + Balance::kPrestigePer * s.prestigePoints;
        s.epoch += 1;
        s.recette = 0.0;
        s.epochRecette = 0.0;
        s.opened = false;
        s.openedAtMs = 0;
        for (int g = 0; g < Balance::GenCount; ++g)
            s.gens[g] = GenState();
        // Soin, creatures and the breakdown history persist: the log is never undone.
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
