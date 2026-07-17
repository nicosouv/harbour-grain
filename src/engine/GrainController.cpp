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

    m_state = fold(m_store.events(), m_salt);

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

double GrainController::liveAccrualRecette() const
{
    const qint64 now = m_clock.nowMs();
    const double secs = (now - m_lastFlushMs) / 1000.0;
    const double f = m_state.opened ? Balance::kFoundation : 0.0;
    return totalRps(m_state, true) * (secs > 0 ? secs : 0)
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
double GrainController::recettePerSec() const { return totalRps(m_state, true); }
double GrainController::soinPerSecQ() const { return grain::soinPerSec(m_state); }
int GrainController::epoch() const { return m_state.epoch; }

QVariantList GrainController::generators() const
{
    QVariantList out;
    for (int g = 0; g < Balance::GenCount; ++g) {
        QVariantMap row;
        row.insert(QStringLiteral("index"), g);
        row.insert(QStringLiteral("id"), QLatin1String(Balance::kGens[g].id));
        row.insert(QStringLiteral("count"), m_state.gens[g].count);
        row.insert(QStringLiteral("rate"),
                   m_state.gens[g].count * Balance::kGens[g].baseRate * m_state.bonusMult);
        row.insert(QStringLiteral("cost"), genCost(m_state, g));
        row.insert(QStringLiteral("manager"), m_state.gens[g].manager);
        row.insert(QStringLiteral("managerCost"), managerCost(g));
        out.append(row);
    }
    return out;
}

QStringList GrainController::creatures() const { return m_state.creatures; }

bool GrainController::openingVisible() const
{
    return m_state.epoch == 0 && !m_state.opened
        && m_state.epochRecette >= Balance::kOpeningUnlock;
}

bool GrainController::openingDone() const { return m_state.opened; }
double GrainController::openingCost() const { return Balance::kOpeningCost; }

bool GrainController::refoundVisible() const
{
    return m_state.epochRecette >= Balance::kPrestigeUnlock;
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
    if (m_clock.nowMs() - m_lastFlushMs >= Balance::kFlushMs)
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
    if (m_state.recette < genCost(m_state, g))
        return;
    QJsonObject p;
    p.insert(QLatin1String("g"), g);
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

void GrainController::inaugurate()
{
    flushNow();
    if (!(m_state.epoch == 0 && !m_state.opened && m_state.recette >= Balance::kOpeningCost))
        return;
    QJsonObject p;
    p.insert(QLatin1String("at"), static_cast<double>(m_clock.nowMs()));
    appendAndApply(QLatin1String("open"),
                   QString::fromUtf8(QJsonDocument(p).toJson(QJsonDocument::Compact)));
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
    QJsonObject p;
    p.insert(QLatin1String("at"), static_cast<double>(m_clock.nowMs()));
    appendAndApply(QLatin1String("bury"),
                   QString::fromUtf8(QJsonDocument(p).toJson(QJsonDocument::Compact)));
    emit stateChanged();
    emit liveChanged();
}

void GrainController::sit()
{
    if (!momentActive())
        return;
    flushNow();
    QJsonObject p;
    p.insert(QLatin1String("at"), static_cast<double>(m_clock.nowMs()));
    appendAndApply(QLatin1String("sit"),
                   QString::fromUtf8(QJsonDocument(p).toJson(QJsonDocument::Compact)));
    emit stateChanged();
    emit liveChanged();
}

void GrainController::refound()
{
    flushNow();
    if (!refoundVisible())
        return;
    QJsonObject p;
    p.insert(QLatin1String("at"), static_cast<double>(m_clock.nowMs()));
    appendAndApply(QLatin1String("confess"),
                   QString::fromUtf8(QJsonDocument(p).toJson(QJsonDocument::Compact)));
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
    const bool active = true;

    QVariantMap tapRow;
    tapRow.insert(QStringLiteral("id"), QStringLiteral("tap"));
    tapRow.insert(QStringLiteral("total"), m_state.earnedTap);
    tapRow.insert(QStringLiteral("share"), total > 0 ? m_state.earnedTap / total : 0.0);
    tapRow.insert(QStringLiteral("perSec"), 0.0);
    out.append(tapRow);

    for (int g = 0; g < Balance::GenCount; ++g) {
        QVariantMap row;
        row.insert(QStringLiteral("id"), QLatin1String(Balance::kGens[g].id));
        row.insert(QStringLiteral("total"), m_state.earnedGens[g]);
        row.insert(QStringLiteral("share"), total > 0 ? m_state.earnedGens[g] / total : 0.0);
        row.insert(QStringLiteral("perSec"),
                   m_state.gens[g].count * Balance::kGens[g].baseRate * m_state.bonusMult
                       * (active || m_state.gens[g].manager ? 1.0 : 0.0));
        out.append(row);
    }

    if (m_state.earnedFoundation > 0.0) {
        QVariantMap row;
        row.insert(QStringLiteral("id"), QStringLiteral("opening"));
        row.insert(QStringLiteral("total"), m_state.earnedFoundation);
        row.insert(QStringLiteral("share"), total > 0 ? m_state.earnedFoundation / total : 0.0);
        row.insert(QStringLiteral("perSec"),
                   baseRps(m_state, true) * (m_state.opened ? Balance::kFoundation : 0.0));
        out.append(row);
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
    m_lastFlushMs = m_clock.nowMs();
    emit stateChanged();
    emit liveChanged();
}

} // namespace grain
