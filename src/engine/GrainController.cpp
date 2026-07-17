#include "GrainController.h"
#include "AppId.h"
#include "Balance.h"
#include "Rng.h"
#include "StateProjection.h"

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QStandardPaths>
#include <QVariantMap>
#include <cmath>

namespace grain {

GrainController::GrainController(QObject* parent)
    : QObject(parent)
    , m_settings(QLatin1String(AppId::kOrganization), QLatin1String(AppId::kApplication))
{
    const QString dir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    m_store.open(dir + QLatin1String("/") + QLatin1String(AppId::kDatabaseFile));

    // Fresh installs get a permanent salt from a cheap entropy mix (Qt 5.6 has no
    // QRandomGenerator); determinism only matters after the salt exists.
    const quint64 entropy = Rng::mix(static_cast<quint64>(m_clock.nowMs()),
                                     reinterpret_cast<quintptr>(this));
    m_store.bootstrap(entropy != 0 ? entropy : Q_UINT64_C(0xC0FFEE));
    m_salt = m_store.installSalt();

    // Fold the whole log, capturing the chart history along the way.
    m_state = GameState();
    const QVector<Event> events = m_store.events();
    for (int i = 0; i < events.size(); ++i) {
        applyEvent(m_state, events.at(i), m_salt);
        recordHistory(events.at(i).tsMs);
    }

    // Offline progress: one coarse tick covering the whole absence, managers only.
    const qint64 now = m_clock.nowMs();
    if (m_state.lastSeenMs > 0 && now - m_state.lastSeenMs > 5000) {
        qint64 ms = now - m_state.lastSeenMs;
        if (ms > Balance::kOfflineCapMs)
            ms = Balance::kOfflineCapMs;
        QJsonObject p;
        p.insert(QLatin1String("ms"), static_cast<double>(ms));
        p.insert(QLatin1String("active"), false);
        p.insert(QLatin1String("at"), static_cast<double>(now));
        appendAndApply(QLatin1String("tick"),
                       QString::fromUtf8(QJsonDocument(p).toJson(QJsonDocument::Compact)));
    }
    m_lastFlushMs = now;

    connect(&m_uiTimer, &QTimer::timeout, this, &GrainController::onUiTick);
    m_uiTimer.start(500);
}

GrainController::~GrainController()
{
    flushNow();
}

QString GrainController::appVersion() const
{
#ifdef APP_VERSION
    return QLatin1String(APP_VERSION);
#else
    return QLatin1String("dev");
#endif
}

QString GrainController::language() const
{
    return m_settings.value(QStringLiteral("language")).toString();
}

void GrainController::setLanguage(const QString& code)
{
    if (language() == code)
        return;
    m_settings.setValue(QStringLiteral("language"), code);
    emit languageChanged();
}

void GrainController::recordHistory(qint64 at)
{
    if (m_state.epoch != m_historyEpoch) {
        m_history.clear();
        m_historyEpoch = m_state.epoch;
    }
    m_history.append(qMakePair(at, m_state.epochRecette));
    // Bound memory: decimate when the series grows too long (keeps the shape).
    if (m_history.size() > 4000) {
        QVector<QPair<qint64, double> > slim;
        slim.reserve(m_history.size() / 2 + 1);
        for (int i = 0; i < m_history.size(); i += 2)
            slim.append(m_history.at(i));
        m_history = slim;
    }
}

double GrainController::liveAccrualRecette() const
{
    const qint64 now = m_clock.nowMs();
    const double secs = (now - m_lastFlushMs) / 1000.0;
    const double f = m_state.opened ? Balance::kFoundation : 0.0;
    return totalRps(m_state) * (secs > 0 ? secs : 0)
         + tapValue(m_state) * (1.0 + f) * m_pendingTaps;
}

double GrainController::liveAccrualSoin() const
{
    const qint64 now = m_clock.nowMs();
    const double secs = (now - m_lastFlushMs) / 1000.0;
    return grain::soinPerSec(m_state) * (secs > 0 ? secs : 0);
}

double GrainController::recette() const { return m_state.recette + liveAccrualRecette(); }
double GrainController::soin() const { return m_state.soin + liveAccrualSoin(); }
double GrainController::recettePerSec() const { return totalRps(m_state); }
double GrainController::soinPerSecQ() const { return grain::soinPerSec(m_state); }
double GrainController::nowMsQ() const { return static_cast<double>(m_clock.nowMs()); }

double GrainController::tapPower() const
{
    const double f = m_state.opened ? Balance::kFoundation : 0.0;
    return tapValue(m_state) * (1.0 + f);
}

int GrainController::epoch() const { return m_state.epoch; }

QVariantList GrainController::generators() const
{
    QVariantList out;
    for (int g = 0; g < Balance::GenCount; ++g) {
        // Progressive reveal: a generator shows once the previous one is owned; the first
        // still-locked row is shown dimmed as the next goal, later ones stay hidden.
        const bool unlocked = g == 0 || m_state.gens[g - 1].count > 0 || m_state.gens[g].count > 0;
        QVariantMap row;
        row.insert(QStringLiteral("index"), g);
        row.insert(QStringLiteral("id"), QLatin1String(Balance::kGens[g].id));
        row.insert(QStringLiteral("count"), m_state.gens[g].count);
        row.insert(QStringLiteral("rate"), genRate(m_state, g));
        row.insert(QStringLiteral("cost"), bulkCost(m_state, g, m_buyAmount));
        row.insert(QStringLiteral("manager"), m_state.gens[g].manager);
        row.insert(QStringLiteral("managerCost"), managerCost(g));
        row.insert(QStringLiteral("payout"), cyclePayout(m_state, g));
        row.insert(QStringLiteral("cycleMs"), static_cast<double>(Balance::kGens[g].cycleMs));
        row.insert(QStringLiteral("runningUntil"),
                   static_cast<double>(m_state.gens[g].runningUntilMs));
        row.insert(QStringLiteral("nextAt"), nextMilestoneAt(m_state.gens[g].count));
        row.insert(QStringLiteral("locked"), !unlocked);
        out.append(row);
        if (!unlocked)
            break;   // show one locked goal, hide the rest
    }
    return out;
}

QStringList GrainController::creatures() const { return m_state.creatures; }

double GrainController::creatureBonusPercent() const
{
    return (creatureMult(m_state) - 1.0) * 100.0;
}

int GrainController::buyAmount() const { return m_buyAmount; }

void GrainController::setBuyAmount(int n)
{
    if (n != 1 && n != 10 && n != 100)
        n = 1;
    if (n == m_buyAmount)
        return;
    m_buyAmount = n;
    emit stateChanged();
}

bool GrainController::openingVisible() const
{
    return !m_state.opened && m_state.epochRecette >= Balance::kOpeningUnlock;
}

bool GrainController::openingDone() const { return m_state.opened; }
double GrainController::openingCost() const { return Balance::kOpeningCost; }

bool GrainController::refoundVisible() const
{
    return refoundGain() >= 1;
}

int GrainController::refoundGain() const
{
    return static_cast<int>(
        std::floor(std::sqrt(m_state.epochRecette / Balance::kPrestigePointDiv)));
}

double GrainController::refoundGainPercent() const
{
    return refoundGain() * Balance::kPrestigePer * 100.0;
}

double GrainController::bonusPercent() const
{
    return (m_state.bonusMult - 1.0) * 100.0;
}

bool GrainController::momentActive() const
{
    return momentDue(m_state, m_salt, m_clock.nowMs());
}

double GrainController::sitCostNow() const { return sitCost(m_state); }

bool GrainController::feedReady() const
{
    return m_clock.nowMs() - m_state.lastFeedMs >= Balance::kCareCooldownMs;
}

bool GrainController::lingerReady() const
{
    return m_clock.nowMs() - m_state.lastLingerMs >= Balance::kCareCooldownMs;
}

double GrainController::careFeedValue() const { return Balance::kCareFeed; }
double GrainController::careLingerValue() const { return Balance::kCareLinger; }

void GrainController::appendAndApply(const QString& kind, const QString& payload)
{
    const qint64 now = m_clock.nowMs();
    const qint64 seq = m_store.appendEvent(kind, m_state.epoch, now, payload);
    if (seq < 0)
        return;
    Event e;
    e.seq = seq;
    e.epoch = m_state.epoch;
    e.tsMs = now;
    e.kind = kind;
    e.payload = payload;
    applyEvent(m_state, e, m_salt);
    recordHistory(now);
}

void GrainController::appendSimple(const QString& kind, qint64 at)
{
    QJsonObject p;
    p.insert(QLatin1String("at"), static_cast<double>(at));
    appendAndApply(kind, QString::fromUtf8(QJsonDocument(p).toJson(QJsonDocument::Compact)));
}

void GrainController::flushNow()
{
    const qint64 now = m_clock.nowMs();
    const qint64 ms = now - m_lastFlushMs;
    if (ms >= 1000) {
        QJsonObject p;
        p.insert(QLatin1String("ms"), static_cast<double>(ms));
        p.insert(QLatin1String("active"), true);
        p.insert(QLatin1String("at"), static_cast<double>(now));
        appendAndApply(QLatin1String("tick"),
                       QString::fromUtf8(QJsonDocument(p).toJson(QJsonDocument::Compact)));
        m_lastFlushMs = now;
    }
    if (m_pendingTaps > 0) {
        QJsonObject p;
        p.insert(QLatin1String("n"), m_pendingTaps);
        p.insert(QLatin1String("at"), static_cast<double>(now));
        appendAndApply(QLatin1String("tap"),
                       QString::fromUtf8(QJsonDocument(p).toJson(QJsonDocument::Compact)));
        m_pendingTaps = 0;
    }
    emit stateChanged();
    emit liveChanged();
}

void GrainController::onUiTick()
{
    const qint64 now = m_clock.nowMs();
    bool matured = false;
    for (int g = 0; g < Balance::GenCount; ++g) {
        if (m_state.gens[g].runningUntilMs > 0 && now >= m_state.gens[g].runningUntilMs) {
            matured = true;
            break;
        }
    }
    if (matured || now - m_lastFlushMs >= Balance::kFlushMs)
        flushNow();
    emit liveChanged();
}

void GrainController::tap()
{
    m_pendingTaps += 1;
    emit liveChanged();
}

void GrainController::buy(int g)
{
    if (g < 0 || g >= Balance::GenCount)
        return;
    flushNow();
    if (m_state.recette < bulkCost(m_state, g, m_buyAmount))
        return;
    QJsonObject p;
    p.insert(QLatin1String("g"), g);
    p.insert(QLatin1String("n"), m_buyAmount);
    p.insert(QLatin1String("at"), static_cast<double>(m_clock.nowMs()));
    appendAndApply(QLatin1String("buy"),
                   QString::fromUtf8(QJsonDocument(p).toJson(QJsonDocument::Compact)));
    emit stateChanged();
    emit liveChanged();
}

void GrainController::hire(int g)
{
    if (g < 0 || g >= Balance::GenCount)
        return;
    flushNow();
    if (m_state.gens[g].count <= 0 || m_state.gens[g].manager
        || m_state.recette < managerCost(g))
        return;
    QJsonObject p;
    p.insert(QLatin1String("g"), g);
    p.insert(QLatin1String("at"), static_cast<double>(m_clock.nowMs()));
    appendAndApply(QLatin1String("hire"),
                   QString::fromUtf8(QJsonDocument(p).toJson(QJsonDocument::Compact)));
    emit stateChanged();
    emit liveChanged();
}

void GrainController::run(int g)
{
    if (g < 0 || g >= Balance::GenCount)
        return;
    flushNow();
    if (m_state.gens[g].count <= 0 || m_state.gens[g].manager
        || m_state.gens[g].runningUntilMs != 0)
        return;
    QJsonObject p;
    p.insert(QLatin1String("g"), g);
    p.insert(QLatin1String("at"), static_cast<double>(m_clock.nowMs()));
    appendAndApply(QLatin1String("run"),
                   QString::fromUtf8(QJsonDocument(p).toJson(QJsonDocument::Compact)));
    emit stateChanged();
    emit liveChanged();
}

void GrainController::inaugurate()
{
    flushNow();
    if (m_state.opened || m_state.recette < Balance::kOpeningCost)
        return;
    appendSimple(QLatin1String("open"), m_clock.nowMs());
    emit stateChanged();
    emit liveChanged();
}

void GrainController::care(const QString& kind)
{
    if (kind != QLatin1String("feed") && kind != QLatin1String("linger"))
        return;
    flushNow();
    QJsonObject p;
    p.insert(QLatin1String("k"), kind);
    p.insert(QLatin1String("at"), static_cast<double>(m_clock.nowMs()));
    appendAndApply(QLatin1String("care"),
                   QString::fromUtf8(QJsonDocument(p).toJson(QJsonDocument::Compact)));
    emit stateChanged();
    emit liveChanged();
}

void GrainController::bury()
{
    if (!momentActive())
        return;
    flushNow();
    appendSimple(QLatin1String("bury"), m_clock.nowMs());
    emit stateChanged();
    emit liveChanged();
}

void GrainController::sit()
{
    if (!momentActive())
        return;
    flushNow();
    appendSimple(QLatin1String("sit"), m_clock.nowMs());
    emit stateChanged();
    emit liveChanged();
}

void GrainController::refound()
{
    flushNow();
    if (!refoundVisible())
        return;
    appendSimple(QLatin1String("confess"), m_clock.nowMs());
    emit stateChanged();
    emit liveChanged();
}

QVariantList GrainController::breakdown() const
{
    // One list, plain buckets: taps, each attraction, and (once present) the opening line.
    double total = m_state.earnedTap + m_state.earnedFoundation;
    for (int g = 0; g < Balance::GenCount; ++g)
        total += m_state.earnedGens[g];

    QVariantList out;

    QVariantMap tapRow;
    tapRow.insert(QStringLiteral("id"), QStringLiteral("tap"));
    tapRow.insert(QStringLiteral("total"), m_state.earnedTap);
    tapRow.insert(QStringLiteral("share"), total > 0 ? m_state.earnedTap / total : 0.0);
    tapRow.insert(QStringLiteral("perSec"), 0.0);
    out.append(tapRow);

    for (int g = 0; g < Balance::GenCount; ++g) {
        if (m_state.gens[g].count == 0 && m_state.earnedGens[g] <= 0.0)
            continue;
        QVariantMap row;
        row.insert(QStringLiteral("id"), QLatin1String(Balance::kGens[g].id));
        row.insert(QStringLiteral("total"), m_state.earnedGens[g]);
        row.insert(QStringLiteral("share"), total > 0 ? m_state.earnedGens[g] / total : 0.0);
        row.insert(QStringLiteral("perSec"),
                   m_state.gens[g].manager ? genRate(m_state, g) : 0.0);
        out.append(row);
    }

    if (m_state.earnedFoundation > 0.0) {
        QVariantMap row;
        row.insert(QStringLiteral("id"), QStringLiteral("opening"));
        row.insert(QStringLiteral("total"), m_state.earnedFoundation);
        row.insert(QStringLiteral("share"), total > 0 ? m_state.earnedFoundation / total : 0.0);
        row.insert(QStringLiteral("perSec"),
                   baseRps(m_state) * (m_state.opened ? Balance::kFoundation : 0.0));
        out.append(row);
    }
    return out;
}

QVariantList GrainController::history() const
{
    QVariantList out;
    const int n = m_history.size();
    if (n == 0)
        return out;
    const int step = n > 120 ? (n + 119) / 120 : 1;
    for (int i = 0; i < n; i += step) {
        QVariantMap pt;
        pt.insert(QStringLiteral("t"), static_cast<double>(m_history.at(i).first));
        pt.insert(QStringLiteral("v"), m_history.at(i).second);
        out.append(pt);
    }
    if ((n - 1) % step != 0) {
        QVariantMap pt;
        pt.insert(QStringLiteral("t"), static_cast<double>(m_history.at(n - 1).first));
        pt.insert(QStringLiteral("v"), m_history.at(n - 1).second);
        out.append(pt);
    }
    return out;
}

QString GrainController::fmt(double value) const
{
    static const char* const kSuffix[] = { "", " k", " M", " B", " T", " P", " E" };
    double a = value < 0 ? -value : value;
    int i = 0;
    while (a >= 1000.0 && i < 6) {
        a /= 1000.0;
        value /= 1000.0;
        ++i;
    }
    QLocale loc;
    if (i == 0)
        return loc.toString(value, 'f', 0);
    return loc.toString(value, 'f', 2) + QLatin1String(kSuffix[i]);
}

void GrainController::clearData()
{
    m_store.clearAll();
    m_state = GameState();
    m_pendingTaps = 0;
    m_history.clear();
    m_historyEpoch = 0;
    m_lastFlushMs = m_clock.nowMs();
    emit stateChanged();
    emit liveChanged();
}

} // namespace grain
