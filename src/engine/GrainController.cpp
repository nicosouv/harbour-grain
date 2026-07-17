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

    // Fold the whole log, capturing the chart histories along the way.
    m_state = GameState();
    const QVector<Event> events = m_store.events();
    for (int i = 0; i < events.size(); ++i) {
        applyEvent(m_state, events.at(i), m_salt);
        recordHistory(events.at(i).tsMs);
        if (events.at(i).kind == QLatin1String("bury")
            || events.at(i).kind == QLatin1String("sit")) {
            m_founderHistory.append(qMakePair(events.at(i).tsMs, founderSleep(m_state)));
            QVariantMap d;
            d.insert(QStringLiteral("t"), static_cast<double>(events.at(i).tsMs));
            d.insert(QStringLiteral("b"), m_state.buried);
            d.insert(QStringLiteral("s"), m_state.sat);
            m_decisionHistory.append(d);
        }
    }

    // Offline progress: one coarse tick covering the whole absence, managers only. A real
    // absence earns a recap on top.
    const qint64 now = m_clock.nowMs();
    if (m_state.lastSeenMs > 0 && now - m_state.lastSeenMs > 5000) {
        qint64 ms = now - m_state.lastSeenMs;
        if (ms > Balance::kOfflineCapMs)
            ms = Balance::kOfflineCapMs;
        const double before = m_state.recette;
        QJsonObject p;
        p.insert(QLatin1String("ms"), static_cast<double>(ms));
        p.insert(QLatin1String("active"), false);
        p.insert(QLatin1String("at"), static_cast<double>(now));
        appendAndApply(QLatin1String("tick"),
                       QString::fromUtf8(QJsonDocument(p).toJson(QJsonDocument::Compact)));
        m_welcomeGain = m_state.recette - before;
        m_welcomeMs = ms;
        m_welcomePending = ms > Q_INT64_C(1800000);
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
    m_soinHistory.append(qMakePair(at, m_state.soinEarned));
    // Bound memory: decimate when the series grow too long (keeps the shape).
    if (m_history.size() > 4000) {
        QVector<QPair<qint64, double> > slim;
        slim.reserve(m_history.size() / 2 + 1);
        for (int i = 0; i < m_history.size(); i += 2)
            slim.append(m_history.at(i));
        m_history = slim;
    }
    if (m_soinHistory.size() > 4000) {
        QVector<QPair<qint64, double> > slim;
        slim.reserve(m_soinHistory.size() / 2 + 1);
        for (int i = 0; i < m_soinHistory.size(); i += 2)
            slim.append(m_soinHistory.at(i));
        m_soinHistory = slim;
    }
}

double GrainController::liveAccrualRecette() const
{
    const qint64 now = m_clock.nowMs();
    const double secs = (now - m_lastFlushMs) / 1000.0;
    const double f = m_state.opened ? Balance::kFoundation : 0.0;
    double accrual = totalRps(m_state) * (secs > 0 ? secs : 0)
                   + tapValue(m_state) * (1.0 + f) * m_pendingTaps;
    // Mirror the fold's tier plateau, so the live number never runs ahead of what a flush
    // will actually materialize.
    return accrual * softCapFactor(m_state);
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
        row.insert(QStringLiteral("buyN"), buyCountFor(g));
        row.insert(QStringLiteral("cost"), bulkCost(m_state, g, buyCountFor(g)));
        row.insert(QStringLiteral("manager"), m_state.gens[g].manager);
        row.insert(QStringLiteral("managerCost"), managerCost(g));
        row.insert(QStringLiteral("payout"), cyclePayout(m_state, g));
        row.insert(QStringLiteral("broken"), m_state.gens[g].broken);
        row.insert(QStringLiteral("repairCost"), repairCost(m_state, g));
        row.insert(QStringLiteral("cycleMs"), static_cast<double>(genCycleMs(m_state, g)));
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

bool GrainController::arrived() const { return m_state.arrived; }

void GrainController::arrive()
{
    if (m_state.arrived)
        return;
    appendSimple(QLatin1String("arrive"), m_clock.nowMs());
    emit stateChanged();
    emit liveChanged();
}

QVariantList GrainController::echoes() const
{
    // One row per unlocked improvement (one unlocks per resolved cover moment).
    QVariantList out;
    const int unlocked = momentsResolved(m_state) < Balance::kEchoCount
        ? momentsResolved(m_state) : Balance::kEchoCount;
    for (int i = 0; i < unlocked; ++i) {
        QVariantMap row;
        row.insert(QStringLiteral("index"), i);
        row.insert(QStringLiteral("cost"), Balance::kEchoCost[i]);
        row.insert(QStringLiteral("bonus"), Balance::kEchoBonus[i] * 100.0);
        row.insert(QStringLiteral("owned"), echoOwned(m_state, i));
        out.append(row);
    }
    return out;
}

void GrainController::buyEcho(int i)
{
    if (i < 0 || i >= Balance::kEchoCount)
        return;
    flushNow();
    if (echoOwned(m_state, i) || momentsResolved(m_state) < i + 1
        || m_state.recette < Balance::kEchoCost[i])
        return;
    QJsonObject p;
    p.insert(QLatin1String("i"), i);
    p.insert(QLatin1String("at"), static_cast<double>(m_clock.nowMs()));
    appendAndApply(QLatin1String("improve"),
                   QString::fromUtf8(QJsonDocument(p).toJson(QJsonDocument::Compact)));
    emit stateChanged();
    emit liveChanged();
}

bool GrainController::founderVisible() const
{
    return momentsResolved(m_state) >= 1;
}

double GrainController::sleepPercent() const { return founderSleep(m_state) * 100.0; }
double GrainController::focusPercent() const { return founderFocus(m_state) * 100.0; }

int GrainController::founderAgeQ() const
{
    return founderAge(m_state);
}

bool GrainController::herGoneQ() const { return herGone(m_state); }
int GrainController::flowerStageQ() const { return flowerStage(m_state); }
int GrainController::raiseTier() const { return m_state.raised; }

bool GrainController::raiseReadyQ() const
{
    return raiseReady(m_state, m_clock.nowMs());
}

bool GrainController::raisePending() const
{
    // The bar is met but the cooldown still runs — worth telling the player when to come back.
    return m_state.raised < Balance::kTierCount
        && m_state.age < Balance::kMaxAge
        && m_state.epochRecette >= Balance::kTierThreshold[m_state.raised]
        && raiseCooldownLeftMs(m_state, m_clock.nowMs()) > 0;
}

double GrainController::raiseThresholdQ() const
{
    if (m_state.raised >= Balance::kTierCount)
        return 0.0;
    return Balance::kTierThreshold[m_state.raised];
}

double GrainController::raiseCooldownLeftQ() const
{
    return static_cast<double>(raiseCooldownLeftMs(m_state, m_clock.nowMs()));
}

bool GrainController::plateaued() const
{
    const double ceiling = tierCeiling(m_state);
    return ceiling > 0.0 && m_state.epochRecette >= ceiling;
}

bool GrainController::decisionsVisible() const
{
    return m_state.buried + m_state.sat >= 15;
}

void GrainController::closeRaise(bool fastChoice)
{
    flushNow();
    if (!raiseReady(m_state, m_clock.nowMs()))
        return;
    QJsonObject p;
    p.insert(QLatin1String("i"), m_state.raised);
    p.insert(QLatin1String("fast"), fastChoice);
    p.insert(QLatin1String("at"), static_cast<double>(m_clock.nowMs()));
    appendAndApply(QLatin1String("raise"),
                   QString::fromUtf8(QJsonDocument(p).toJson(QJsonDocument::Compact)));
    emit stateChanged();
    emit liveChanged();
}

void GrainController::repair(int g)
{
    if (g < 0 || g >= Balance::GenCount)
        return;
    flushNow();
    if (!m_state.gens[g].broken || m_state.recette < repairCost(m_state, g))
        return;
    QJsonObject p;
    p.insert(QLatin1String("g"), g);
    p.insert(QLatin1String("at"), static_cast<double>(m_clock.nowMs()));
    appendAndApply(QLatin1String("repair"),
                   QString::fromUtf8(QJsonDocument(p).toJson(QJsonDocument::Compact)));
    emit stateChanged();
    emit liveChanged();
}

bool GrainController::beatSeen(const QString& key) const
{
    if (m_settings.value(QStringLiteral("narr/b_") + key).toBool())
        return true;
    // Pre-0.4 installs tracked open/refound/echo differently; honour those marks.
    if (key == QStringLiteral("open%1").arg(m_state.epoch))
        return m_settings.value(QStringLiteral("narr/open_%1").arg(m_state.epoch)).toBool();
    if (key == QStringLiteral("refound%1").arg(m_state.epoch))
        return m_settings.value(QStringLiteral("narr/refound_%1").arg(m_state.epoch)).toBool();
    if (key.startsWith(QLatin1String("echo")))
        return key.mid(4).toInt()
            <= m_settings.value(QStringLiteral("narr/level"), 0).toInt();
    return false;
}

QString GrainController::pendingNarration() const
{
    if (!m_state.arrived)
        return QString();
    const qint64 now = m_clock.nowMs();

    if (m_state.epoch > 0 && !beatSeen(QStringLiteral("refound%1").arg(m_state.epoch))) {
        if (m_state.epoch == 1)
            return QStringLiteral("refound");
        return m_state.epoch == 2 ? QStringLiteral("refound2") : QStringLiteral("refound3");
    }
    if (m_state.opened && !beatSeen(QStringLiteral("open%1").arg(m_state.epoch)))
        return m_state.epoch == 0 ? QStringLiteral("open") : QStringLiteral("openAgain");

    // Each closed funding round tells its bit of the story.
    for (int i = 0; i < m_state.raised && i < Balance::kTierCount; ++i) {
        if (!beatSeen(QStringLiteral("raise%1").arg(i)))
            return QStringLiteral("raise%1").arg(i);
    }

    if (m_state.incidents >= 1 && !beatSeen(QStringLiteral("panne1")))
        return QStringLiteral("panne1");

    // The cover choices themselves, first times and repetitions.
    if (m_state.buried >= 1 && !beatSeen(QStringLiteral("bury1")))
        return QStringLiteral("bury1");
    if (m_state.sat >= 1 && !beatSeen(QStringLiteral("sit1")))
        return QStringLiteral("sit1");
    const bool gone = herGone(m_state);
    if (m_state.buried >= 5 && !beatSeen(QStringLiteral("bury5")))
        return QStringLiteral("bury5");
    if (!gone && m_state.sat >= 5 && !beatSeen(QStringLiteral("sit5")))
        return QStringLiteral("sit5");

    // Decade birthdays, oldest missed first.
    const int age = founderAge(m_state);
    for (int decade = 30; decade <= 70; decade += 10) {
        if (age >= decade && !beatSeen(QStringLiteral("birthday%1").arg(decade)))
            return QStringLiteral("birthday%1").arg(decade);
    }

    // The park's anniversary: one beat per year of its life (a year is a real day).
    const int year = parkYear();
    if (year >= 1 && !beatSeen(QStringLiteral("anniv%1").arg(year)))
        return QStringLiteral("anniv%1").arg(year);

    // One improvement echo per resolved cover moment.
    const int resolved = momentsResolved(m_state) < Balance::kEchoCount
        ? momentsResolved(m_state) : Balance::kEchoCount;
    for (int n = 1; n <= resolved; ++n) {
        if (gone && n == 4)
            continue;   // her speech: only while she is here
        if (!beatSeen(QStringLiteral("echo%1").arg(n)))
            return QStringLiteral("echo%1").arg(n);
    }

    // Wealth marks (epoch recette).
    static const double kWealth[] = { 1e4, 1e5, 1e6, 1e7, 1e8, 1e9 };
    static const char* const kWealthKey[] = {
        "wealth10k", "wealth100k", "wealth1m", "wealth10m", "wealth100m", "wealth1b"
    };
    for (int w = 0; w < 6; ++w) {
        if (m_state.epochRecette >= kWealth[w]
            && !beatSeen(QLatin1String(kWealthKey[w])))
            return QLatin1String(kWealthKey[w]);
    }

    // Count milestones, whichever attraction crosses them first.
    static const char* const kMilestoneKey[] = {
        "first_milestone", "milestone50", "milestone100", "milestone200", "milestone400"
    };
    for (int m = 0; m < Balance::kMilestoneCount; ++m) {
        if (gone && m == 3)
            continue;   // "she stopped counting with me" needs her here
        for (int g = 0; g < Balance::GenCount; ++g) {
            if (m_state.gens[g].count >= Balance::kMilestones[m]
                && !beatSeen(QLatin1String(kMilestoneKey[m])))
                return QLatin1String(kMilestoneKey[m]);
        }
    }

    // Progression firsts, and each hand-over to a manager.
    for (int g = 0; g < Balance::GenCount; ++g) {
        if (gone && (g == Balance::Aviary || g == Balance::Carousel))
            continue;   // those firsts speak of her
        if (m_state.gens[g].count > 0) {
            const QString key = QStringLiteral("first_") + QLatin1String(Balance::kGens[g].id);
            if (!beatSeen(key))
                return key;
        }
    }
    int managers = 0;
    for (int g = 0; g < Balance::GenCount; ++g) {
        if (m_state.gens[g].manager) {
            ++managers;
            if (gone && g == Balance::Aviary)
                continue;
            const QString key = QStringLiteral("manager_") + QLatin1String(Balance::kGens[g].id);
            if (!beatSeen(key))
                return key;
        }
    }
    if (managers >= 4 && !beatSeen(QStringLiteral("hands_off")))
        return QStringLiteral("hands_off");
    if (donutVisible() && !beatSeen(QStringLiteral("donut")))
        return QStringLiteral("donut");

    if (m_state.creatures.size() >= 1 && !beatSeen(QStringLiteral("creature1")))
        return QStringLiteral("creature1");
    if (m_state.creatures.size() >= 5 && !beatSeen(QStringLiteral("creature5")))
        return QStringLiteral("creature5");
    if (!gone && m_state.creatures.size() >= 12 && !beatSeen(QStringLiteral("creature12")))
        return QStringLiteral("creature12");

    // Cameos: a cat that watches, and a face that reads faces. Once (or twice) each.
    if (m_state.opened && m_state.gens[Balance::Aviary].count >= 1
        && !beatSeen(QStringLiteral("cat1")))
        return QStringLiteral("cat1");
    if (m_state.raised >= 7 && !beatSeen(QStringLiteral("nami1")))
        return QStringLiteral("nami1");
    if ((m_state.raised >= 10 || m_state.age >= 65)
        && beatSeen(QStringLiteral("nami1")) && !beatSeen(QStringLiteral("nami2")))
        return QStringLiteral("nami2");

    // The late confession's epilogue, then the absence, then the ambient hum — one per visit.
    if (m_staticArmed && gone) {
        if (m_state.age >= 70) {
            for (int i = 0; i < 6; ++i) {
                if (!beatSeen(QStringLiteral("epi%1").arg(i)))
                    return QStringLiteral("epi%1").arg(i);
            }
        }
        for (int i = 0; i < 4; ++i) {
            if (!beatSeen(QStringLiteral("abs%1").arg(i)))
                return QStringLiteral("abs%1").arg(i);
        }
    }

    // Ambient interference: repeatable, once per activation, quota rising with net buries.
    const int pressure = m_state.buried - m_state.sat;
    if (m_staticArmed && (pressure >= 3 || gone)) {
        const qint64 day = now / Q_INT64_C(86400000);
        int quotaToday = pressure - 2 < 3 ? pressure - 2 : 3;
        if (gone && quotaToday < 1)
            quotaToday = 1;
        if (m_state.age >= 70)
            quotaToday = 1;   // the last decade goes quiet
        const qint64 markedDay = m_settings.value(QStringLiteral("narr/sday"), 0).toLongLong();
        const int shown = markedDay == day
            ? m_settings.value(QStringLiteral("narr/scount"), 0).toInt() : 0;
        if (shown < quotaToday) {
            const bool hasCat = m_state.gens[Balance::Aviary].count > 0;
            if (gone) {
                const int variant = static_cast<int>(
                    Rng::mix(m_salt, static_cast<quint64>(day * 7 + shown))
                        % (hasCat ? 9 : 8));
                return QStringLiteral("astatic%1").arg(variant);
            }
            // Deep pressure pulls from the darker half of the pool; the cat drifts through both.
            const int base = pressure >= 6 ? 6 : 0;
            const int idx = static_cast<int>(
                Rng::mix(m_salt, static_cast<quint64>(day * 7 + shown)) % (hasCat ? 7 : 6));
            if (idx == 6)
                return pressure >= 6 ? QStringLiteral("static13") : QStringLiteral("static12");
            return QStringLiteral("static%1").arg(base + idx);
        }
    }
    return QString();
}

void GrainController::ackNarration()
{
    const QString pending = pendingNarration();
    if (pending.isEmpty())
        return;
    if (pending.startsWith(QLatin1String("epi")) || pending.startsWith(QLatin1String("abs"))) {
        m_settings.setValue(QStringLiteral("narr/b_") + pending, true);
        m_staticArmed = false;
    } else if (pending.startsWith(QLatin1String("static"))
               || pending.startsWith(QLatin1String("astatic"))) {
        const qint64 day = m_clock.nowMs() / Q_INT64_C(86400000);
        const qint64 markedDay = m_settings.value(QStringLiteral("narr/sday"), 0).toLongLong();
        const int shown = markedDay == day
            ? m_settings.value(QStringLiteral("narr/scount"), 0).toInt() : 0;
        m_settings.setValue(QStringLiteral("narr/sday"), day);
        m_settings.setValue(QStringLiteral("narr/scount"), shown + 1);
        m_staticArmed = false;
    } else if (pending.startsWith(QLatin1String("open"))) {
        m_settings.setValue(
            QStringLiteral("narr/b_open%1").arg(m_state.epoch), true);
    } else if (pending.startsWith(QLatin1String("refound"))) {
        m_settings.setValue(
            QStringLiteral("narr/b_refound%1").arg(m_state.epoch), true);
    } else {
        m_settings.setValue(QStringLiteral("narr/b_") + pending, true);
    }
    emit stateChanged();
}

void GrainController::appActivated()
{
    m_staticArmed = true;
    emit stateChanged();
}

int GrainController::parkYear() const
{
    return m_state.age - Balance::kStartAge;
}

bool GrainController::donutVisible() const
{
    return m_state.epochRecette >= 1000.0;
}

int GrainController::buyAmount() const { return m_buyAmount; }

void GrainController::setBuyAmount(int n)
{
    if (n != 1 && n != 10 && n != 100 && n != -1 && n != -2)
        n = 1;
    if (n == m_buyAmount)
        return;
    m_buyAmount = n;
    emit stateChanged();
}

int GrainController::buyCountFor(int g) const
{
    if (m_buyAmount > 0)
        return m_buyAmount;
    if (m_buyAmount == -2) {
        const int next = nextMilestoneAt(m_state.gens[g].count);
        if (next > m_state.gens[g].count)
            return next - m_state.gens[g].count;
    }
    // Max: the largest affordable batch (closed-form inverse of the geometric cost sum).
    const double r = Balance::kGens[g].costGrowth;
    const double first = genCost(m_state, g);
    if (m_state.recette < first)
        return 1;
    const int n = static_cast<int>(
        std::floor(std::log(m_state.recette * (r - 1.0) / first + 1.0) / std::log(r)));
    return n > 1 ? n : 1;
}

bool GrainController::welcomePending() const { return m_welcomePending; }
double GrainController::welcomeGain() const { return m_welcomeGain; }
double GrainController::welcomeMs() const { return static_cast<double>(m_welcomeMs); }

void GrainController::ackWelcome()
{
    if (!m_welcomePending)
        return;
    m_welcomePending = false;
    emit stateChanged();
}

bool GrainController::reduceFx() const
{
    return m_settings.value(QStringLiteral("reduceFx"), false).toBool();
}

void GrainController::setReduceFx(bool on)
{
    m_settings.setValue(QStringLiteral("reduceFx"), on);
    emit prefsChanged();
}

bool GrainController::notifyRaises() const
{
    return m_settings.value(QStringLiteral("notifyRaises"), false).toBool();
}

void GrainController::setNotifyRaises(bool on)
{
    m_settings.setValue(QStringLiteral("notifyRaises"), on);
    emit prefsChanged();
}

namespace {
// Beats that read as a park logbook line, not an interference.
const char* const kMinorBeats[] = {
    "first_gate", "first_kiosk", "first_paths", "first_pond", "first_wheel",
    "first_greenhouse", "first_milestone", "milestone50", "milestone100",
    "milestone200", "milestone400", "creature5"
};
const int kMinorBeatCount = 12;
}

bool GrainController::isMinorBeat(const QString& key) const
{
    for (int i = 0; i < kMinorBeatCount; ++i) {
        if (key == QLatin1String(kMinorBeats[i]))
            return true;
    }
    return false;
}

QStringList GrainController::journalKeys() const
{
    QStringList out;
    for (int i = 0; i < kMinorBeatCount; ++i) {
        if (beatSeen(QLatin1String(kMinorBeats[i])))
            out.append(QLatin1String(kMinorBeats[i]));
    }
    return out;
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
    if (kind == QLatin1String("bury") || kind == QLatin1String("sit")) {
        m_founderHistory.append(qMakePair(now, founderSleep(m_state)));
        QVariantMap d;
        d.insert(QStringLiteral("t"), static_cast<double>(now));
        d.insert(QStringLiteral("b"), m_state.buried);
        d.insert(QStringLiteral("s"), m_state.sat);
        m_decisionHistory.append(d);
    }
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
    const int n = buyCountFor(g);
    if (n < 1 || m_state.recette < bulkCost(m_state, g, n))
        return;
    QJsonObject p;
    p.insert(QLatin1String("g"), g);
    p.insert(QLatin1String("n"), n);
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

QVariantList GrainController::sleepHistory() const
{
    QVariantList out;
    for (int i = 0; i < m_founderHistory.size(); ++i) {
        QVariantMap pt;
        pt.insert(QStringLiteral("t"), static_cast<double>(m_founderHistory.at(i).first));
        pt.insert(QStringLiteral("v"), m_founderHistory.at(i).second);
        out.append(pt);
    }
    return out;
}

QVariantList GrainController::soinHistory() const
{
    QVariantList out;
    const int n = m_soinHistory.size();
    if (n == 0)
        return out;
    const int step = n > 120 ? (n + 119) / 120 : 1;
    for (int i = 0; i < n; i += step) {
        QVariantMap pt;
        pt.insert(QStringLiteral("t"), static_cast<double>(m_soinHistory.at(i).first));
        pt.insert(QStringLiteral("v"), m_soinHistory.at(i).second);
        out.append(pt);
    }
    return out;
}

QVariantList GrainController::decisionsHistory() const
{
    return m_decisionHistory;
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
    m_founderHistory.clear();
    m_soinHistory.clear();
    m_decisionHistory.clear();
    // A new life: forget the shown narrations along with everything else.
    m_settings.remove(QStringLiteral("narr"));
    m_lastFlushMs = m_clock.nowMs();
    emit stateChanged();
    emit liveChanged();
}

} // namespace grain
