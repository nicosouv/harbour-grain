// Engine unit tests: the projection is pure, so everything is asserted by folding hand-built
// event vectors. Runs against plain Qt5 (no Sailfish SDK needed).
#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QPair>
#include <QVector>
#include <cmath>
#include <initializer_list>

#include "engine/Balance.h"
#include "engine/EventStore.h"
#include "engine/GameState.h"
#include "engine/Rng.h"
#include "engine/StateProjection.h"

using namespace grain;

namespace {

const quint64 kSalt = Q_UINT64_C(0xA5A5A5A5DEADBEEF);

QString json(std::initializer_list<QPair<QString, QJsonValue>> pairs)
{
    QJsonObject o;
    for (const auto& p : pairs)
        o.insert(p.first, p.second);
    return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

Event ev(const QString& kind, const QString& payload)
{
    Event e;
    e.kind = kind;
    e.payload = payload;
    return e;
}

Event tap(int n, qint64 at = 1000) { return ev("tap", json({{"n", n}, {"at", double(at)}})); }
Event buy(int g, qint64 at = 1000) { return ev("buy", json({{"g", g}, {"at", double(at)}})); }
Event hire(int g, qint64 at = 1000) { return ev("hire", json({{"g", g}, {"at", double(at)}})); }
Event open(qint64 at = 1000) { return ev("open", json({{"at", double(at)}})); }
Event tick(qint64 ms, bool active, qint64 at = 1000)
{
    return ev("tick", json({{"ms", double(ms)}, {"active", active}, {"at", double(at)}}));
}
Event run(int g, qint64 at) { return ev("run", json({{"g", g}, {"at", double(at)}})); }
Event buyN(int g, int n, qint64 at = 1000)
{
    return ev("buy", json({{"g", g}, {"n", n}, {"at", double(at)}}));
}
Event care(const QString& k, qint64 at) { return ev("care", json({{"k", k}, {"at", double(at)}})); }
Event bury(qint64 at) { return ev("bury", json({{"at", double(at)}})); }
Event sit(qint64 at) { return ev("sit", json({{"at", double(at)}})); }
Event confess(qint64 at = 1000) { return ev("confess", json({{"at", double(at)}})); }

// Enough taps to afford `amount` from scratch (1 recette per tap, no gates).
QVector<Event> richStart(int amount)
{
    QVector<Event> v;
    v.append(tap(amount));
    return v;
}

} // namespace

class TstGrain : public QObject
{
    Q_OBJECT

private slots:
    void rngDeterminism();
    void rngMixPure();
    void storeRoundtrip();
    void storeClearKeepsSalt();
    void foldTap();
    void foldBuyExponentialCost();
    void foldBuyUnaffordableIsNoop();
    void foldBulkBuy();
    void foldMilestoneDoubles();
    void foldCycleRunAndSettle();
    void foldManagerAutomation();
    void foldOpeningOneShot();
    void foldFoundationShare();
    void foldOfflineTickManagedOnly();
    void foldOfflineCap();
    void foldCareCooldown();
    void foldSpawnsDeterministic();
    void foldBuryTightensCadence();
    void foldSitEasesAndCosts();
    void foldConfessResetsEpoch();
    void foldReplayDeterministic();
};

void TstGrain::rngDeterminism()
{
    Rng a(42), b(42);
    for (int i = 0; i < 100; ++i)
        QCOMPARE(a.next(), b.next());
    Rng c(43);
    QVERIFY(Rng(42).next() != c.next());
}

void TstGrain::rngMixPure()
{
    QCOMPARE(Rng::mix(1, 2), Rng::mix(1, 2));
    QVERIFY(Rng::mix(1, 2) != Rng::mix(2, 1));
}

void TstGrain::storeRoundtrip()
{
    EventStore store(QStringLiteral("t_roundtrip"));
    QVERIFY(store.open(QStringLiteral(":memory:")));
    QVERIFY(store.bootstrap(kSalt));
    QCOMPARE(store.installSalt(), kSalt);

    QVERIFY(store.appendEvent("tap", 0, 123, "{\"n\":1,\"at\":123}") > 0);
    QVERIFY(store.appendEvent("buy", 0, 456, "{\"g\":0,\"at\":456}") > 0);

    const QVector<Event> events = store.events();
    QCOMPARE(events.size(), 2);
    QCOMPARE(events[0].kind, QStringLiteral("tap"));
    QCOMPARE(events[0].tsMs, Q_INT64_C(123));
    QCOMPARE(events[1].kind, QStringLiteral("buy"));
    QVERIFY(events[1].seq > events[0].seq);
    QCOMPARE(store.eventCount(), 2);
}

void TstGrain::storeClearKeepsSalt()
{
    EventStore store(QStringLiteral("t_clear"));
    QVERIFY(store.open(QStringLiteral(":memory:")));
    QVERIFY(store.bootstrap(kSalt));
    store.appendEvent("tap", 0, 1, "{}");
    QVERIFY(store.clearAll());
    QCOMPARE(store.eventCount(), 0);
    QCOMPARE(store.installSalt(), kSalt);
}

void TstGrain::foldTap()
{
    const GameState s = fold(QVector<Event>() << tap(5), kSalt);
    QCOMPARE(s.recette, 5.0 * Balance::kTapBase);
    QCOMPARE(s.earnedTap, 5.0 * Balance::kTapBase);
    QCOMPARE(s.epochRecette, s.recette);
}

void TstGrain::foldBuyExponentialCost()
{
    QVector<Event> v = richStart(1000);
    v.append(buy(Balance::Gate));
    v.append(buy(Balance::Gate));
    const GameState s = fold(v, kSalt);

    QCOMPARE(s.gens[Balance::Gate].count, 2);
    const double c0 = Balance::kGens[Balance::Gate].baseCost;
    const double c1 = c0 * Balance::kGens[Balance::Gate].costGrowth;
    QVERIFY(std::fabs(s.recette - (1000.0 - c0 - c1)) < 1e-9);
    // The next unit is dearer again: the wall.
    QVERIFY(genCost(s, Balance::Gate) > c1);
}

void TstGrain::foldBuyUnaffordableIsNoop()
{
    QVector<Event> v;
    v.append(tap(1));
    v.append(buy(Balance::Aviary));
    const GameState s = fold(v, kSalt);
    QCOMPARE(s.gens[Balance::Aviary].count, 0);
    QCOMPARE(s.recette, Balance::kTapBase);
}

void TstGrain::foldBulkBuy()
{
    QVector<Event> v = richStart(100000);
    v.append(buyN(Balance::Gate, 10));
    const GameState s = fold(v, kSalt);
    QCOMPARE(s.gens[Balance::Gate].count, 10);
    const GameState empty;
    QVERIFY(std::fabs(s.recette - (100000.0 - bulkCost(empty, Balance::Gate, 10))) < 1e-6);

    // An unaffordable bundle is a no-op, not a partial buy.
    QVector<Event> v2;
    v2.append(tap(1));
    v2.append(buyN(Balance::Aviary, 10));
    QCOMPARE(fold(v2, kSalt).gens[Balance::Aviary].count, 0);
}

void TstGrain::foldMilestoneDoubles()
{
    QCOMPARE(genMultiplier(24), 1.0);
    QCOMPARE(genMultiplier(25), 2.0);
    QCOMPARE(genMultiplier(100), 8.0);
    QCOMPARE(nextMilestoneAt(0), 25);
    QCOMPARE(nextMilestoneAt(400), 0);

    QVector<Event> v = richStart(100000);
    v.append(buyN(Balance::Gate, 25));
    v.append(hire(Balance::Gate));
    v.append(tick(10000, false, 2000));
    const GameState s = fold(v, kSalt);
    const double expected = 25 * Balance::kGens[Balance::Gate].baseRate * 2.0 * 10.0;
    QVERIFY(std::fabs(s.earnedGens[Balance::Gate] - expected) < 1e-6);
}

void TstGrain::foldCycleRunAndSettle()
{
    QVector<Event> v = richStart(1000);
    v.append(buy(Balance::Gate, 1000));
    v.append(run(Balance::Gate, 2000));
    GameState s = fold(v, kSalt);
    QCOMPARE(s.gens[Balance::Gate].runningUntilMs,
             Q_INT64_C(2000) + Balance::kGens[Balance::Gate].cycleMs);
    QCOMPARE(s.earnedGens[Balance::Gate], 0.0);

    // A second run while one is in flight is ignored.
    QVector<Event> v2 = v;
    v2.append(run(Balance::Gate, 2500));
    QCOMPARE(fold(v2, kSalt).gens[Balance::Gate].runningUntilMs,
             s.gens[Balance::Gate].runningUntilMs);

    // Any later event settles the matured payout.
    v.append(tick(1000, true, 2000 + Balance::kGens[Balance::Gate].cycleMs + 1));
    s = fold(v, kSalt);
    const double expected = Balance::kGens[Balance::Gate].baseRate
                          * (Balance::kGens[Balance::Gate].cycleMs / 1000.0);
    QVERIFY(std::fabs(s.earnedGens[Balance::Gate] - expected) < 1e-9);
    QCOMPARE(s.gens[Balance::Gate].runningUntilMs, Q_INT64_C(0));
}

void TstGrain::foldManagerAutomation()
{
    QVector<Event> v = richStart(10000);
    v.append(buy(Balance::Gate));
    // Idle tick before hiring: nothing accrues.
    v.append(tick(10000, false));
    GameState s = fold(v, kSalt);
    QCOMPARE(s.earnedGens[Balance::Gate], 0.0);

    v.append(hire(Balance::Gate));
    v.append(tick(10000, false));
    s = fold(v, kSalt);
    QVERIFY(s.gens[Balance::Gate].manager);
    const double expected = Balance::kGens[Balance::Gate].baseRate * 10.0;
    QVERIFY(std::fabs(s.earnedGens[Balance::Gate] - expected) < 1e-9);
}

void TstGrain::foldOpeningOneShot()
{
    QVector<Event> v = richStart(1000);
    v.append(open(5000));
    GameState s = fold(v, kSalt);
    QVERIFY(s.opened);
    QCOMPARE(s.openedAtMs, Q_INT64_C(5000));
    const double expected = 1000.0 - Balance::kOpeningCost + Balance::kOpeningInstant;
    QVERIFY(std::fabs(s.recette - expected) < 1e-9);
    QCOMPARE(s.earnedFoundation, Balance::kOpeningInstant);

    // A second 'open' is a no-op.
    v.append(open(6000));
    const GameState s2 = fold(v, kSalt);
    QVERIFY(std::fabs(s2.recette - s.recette) < 1e-9);
}

void TstGrain::foldFoundationShare()
{
    // After the opening, every formula carries the same constant coefficient.
    QVector<Event> v = richStart(100000);
    v.append(open());
    v.append(buy(Balance::Gate));
    v.append(hire(Balance::Gate));
    const double before = fold(v, kSalt).earnedFoundation;
    v.append(tick(100000, true));
    const GameState s = fold(v, kSalt);

    const double base = Balance::kGens[Balance::Gate].baseRate * 100.0;
    QVERIFY(std::fabs((s.earnedGens[Balance::Gate]) - base) < 1e-6);
    QVERIFY(std::fabs((s.earnedFoundation - before) - base * Balance::kFoundation) < 1e-6);

    // Taps carry it too.
    const double tapBefore = s.recette;
    QVector<Event> v2 = v;
    v2.append(tap(10));
    const GameState s3 = fold(v2, kSalt);
    const double gain = s3.recette - tapBefore;
    const double expected = tapValue(s) * 10 * (1.0 + Balance::kFoundation);
    QVERIFY(std::fabs(gain - expected) < 1e-6);
}

void TstGrain::foldOfflineTickManagedOnly()
{
    QVector<Event> v = richStart(100000);
    v.append(buy(Balance::Gate));
    v.append(buy(Balance::Kiosk));
    v.append(hire(Balance::Kiosk));
    v.append(tick(60000, false));
    const GameState s = fold(v, kSalt);
    QCOMPARE(s.earnedGens[Balance::Gate], 0.0);   // no manager, app closed
    const double expected = Balance::kGens[Balance::Kiosk].baseRate * 60.0;
    QVERIFY(std::fabs(s.earnedGens[Balance::Kiosk] - expected) < 1e-9);
}

void TstGrain::foldOfflineCap()
{
    QVector<Event> v = richStart(100000);
    v.append(buy(Balance::Gate));
    v.append(hire(Balance::Gate));
    v.append(tick(Balance::kOfflineCapMs * 10, false));
    const GameState s = fold(v, kSalt);
    const double capped = Balance::kGens[Balance::Gate].baseRate
                        * (Balance::kOfflineCapMs / 1000.0);
    QVERIFY(std::fabs(s.earnedGens[Balance::Gate] - capped) < 1e-3);
}

void TstGrain::foldCareCooldown()
{
    QVector<Event> v;
    v.append(care("feed", Balance::kCareCooldownMs + 1));
    v.append(care("feed", Balance::kCareCooldownMs + 2));  // inside cooldown: ignored
    GameState s = fold(v, kSalt);
    QCOMPARE(s.soinEarned, Balance::kCareFeed);

    v.append(care("feed", 2 * Balance::kCareCooldownMs + 2));
    s = fold(v, kSalt);
    QCOMPARE(s.soinEarned, 2 * Balance::kCareFeed);
}

void TstGrain::foldSpawnsDeterministic()
{
    // Enough care to cross the first two thresholds.
    QVector<Event> v;
    qint64 at = Balance::kCareCooldownMs + 1;
    for (int i = 0; i < 12; ++i) {
        v.append(care("feed", at));
        at += Balance::kCareCooldownMs + 1;
    }
    const GameState a = fold(v, kSalt);
    const GameState b = fold(v, kSalt);
    QVERIFY(a.creatures.size() >= 2);
    QCOMPARE(a.creatures, b.creatures);

    // A different install sees a different menagerie (with overwhelming probability).
    const GameState c = fold(v, kSalt + 1);
    QCOMPARE(c.creatures.size(), a.creatures.size());
}

void TstGrain::foldBuryTightensCadence()
{
    QVector<Event> v = richStart(1000);
    v.append(open(1000));

    // The very first moment lands quickly after the opening.
    const GameState s0 = fold(v, kSalt);
    QVERIFY(momentIntervalMs(s0, kSalt) <= Balance::kMomentFirstMs * 5 / 4 + 1);

    QVector<Event> v1 = v;
    v1.append(bury(2000));
    const GameState s1 = fold(v1, kSalt);
    const qint64 one = momentIntervalMs(s1, kSalt);

    QVector<Event> v6 = v1;
    for (int i = 0; i < 5; ++i)
        v6.append(bury(3000 + i));
    const GameState s6 = fold(v6, kSalt);
    const qint64 six = momentIntervalMs(s6, kSalt);

    QVERIFY(six < one);
    QVERIFY(six >= Balance::kMomentFloorMs);

    // Due only once the interval has elapsed since the last resolution.
    QVERIFY(!momentDue(s6, kSalt, Q_INT64_C(3004) + six - 1));
    QVERIFY(momentDue(s6, kSalt, Q_INT64_C(3004) + six + 1));
}

void TstGrain::foldSitEasesAndCosts()
{
    QVector<Event> v = richStart(100000);
    v.append(open(1000));
    for (int i = 0; i < 4; ++i)
        v.append(bury(2000 + i));
    GameState buried = fold(v, kSalt);

    QVector<Event> v2 = v;
    v2.append(sit(3000));
    GameState eased = fold(v2, kSalt);

    QCOMPARE(eased.sat, 1);
    QVERIFY(eased.recette < buried.recette);                    // sitting costs recette
    QVERIFY(momentIntervalMs(eased, kSalt) >= Balance::kMomentFloorMs);
    QVERIFY(eased.soinEarned > buried.soinEarned);              // and quietly feeds soin
}

void TstGrain::foldConfessResetsEpoch()
{
    QVector<Event> v = richStart(100000);
    v.append(open(1000));
    v.append(buy(Balance::Gate));
    v.append(hire(Balance::Gate));
    v.append(care("feed", Balance::kCareCooldownMs + 1));
    const GameState before = fold(v, kSalt);
    QVERIFY(before.opened);

    v.append(confess(9000));
    const GameState s = fold(v, kSalt);

    QCOMPARE(s.epoch, 1);
    QCOMPARE(s.recette, 0.0);
    QCOMPARE(s.epochRecette, 0.0);
    QVERIFY(!s.opened);
    QCOMPARE(s.gens[Balance::Gate].count, 0);
    QVERIFY(s.prestigePoints > 0);
    QVERIFY(s.bonusMult > 1.0);
    QCOMPARE(s.soinEarned, before.soinEarned);       // the quiet side persists
    QCOMPARE(s.creatures, before.creatures);
    QVERIFY(s.allRecette > 0.0);

    // The coefficient is gone: income is base only, and 'open' can't recur in epoch 1.
    QVector<Event> v2 = v;
    v2.append(tap(10, 9500));
    v2.append(open(9600));
    const GameState s2 = fold(v2, kSalt);
    QVERIFY(!s2.opened);
    QVERIFY(std::fabs(s2.recette - tapValue(s) * 10) < 1e-9);
}

void TstGrain::foldReplayDeterministic()
{
    QVector<Event> v = richStart(100000);
    v.append(buy(Balance::Gate));
    v.append(open(1000));
    v.append(hire(Balance::Gate));
    v.append(tick(123456, true, 2000));
    v.append(care("feed", Balance::kCareCooldownMs + 1));
    v.append(bury(5000));
    v.append(confess(6000));
    v.append(tap(3, 7000));

    const GameState a = fold(v, kSalt);
    const GameState b = fold(v, kSalt);

    QCOMPARE(a.recette, b.recette);
    QCOMPARE(a.soin, b.soin);
    QCOMPARE(a.epoch, b.epoch);
    QCOMPARE(a.allRecette, b.allRecette);
    QCOMPARE(a.earnedFoundation, b.earnedFoundation);
    QCOMPARE(a.creatures, b.creatures);
    QCOMPARE(a.prestigePoints, b.prestigePoints);
    QCOMPARE(a.lastSeenMs, b.lastSeenMs);
}

QTEST_GUILESS_MAIN(TstGrain)
#include "tst_main.moc"
