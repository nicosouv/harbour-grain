// Thin QObject facade exposed to QML. Owns storage + clock, appends events, and folds them into
// the in-memory projection. Between materialized ticks it shows a live derived value (state at
// last event + rates x elapsed) without touching the log, then trues up with a coarse 'tick'
// event, so the log stays bounded and replay stays exact.
#ifndef GRAIN_GRAINCONTROLLER_H
#define GRAIN_GRAINCONTROLLER_H

#include <QObject>
#include <QPair>
#include <QSettings>
#include <QTimer>
#include <QVariantList>
#include <QVector>
#include "EventStore.h"
#include "GameState.h"
#include "Clock.h"

namespace grain {

class GrainController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)

    // Live values, refreshed by a UI timer between materialized ticks.
    Q_PROPERTY(double recette READ recette NOTIFY liveChanged)
    Q_PROPERTY(double soin READ soin NOTIFY liveChanged)
    Q_PROPERTY(double recettePerSec READ recettePerSec NOTIFY stateChanged)
    Q_PROPERTY(double soinPerSec READ soinPerSecQ NOTIFY stateChanged)
    Q_PROPERTY(double nowMs READ nowMsQ NOTIFY liveChanged)
    Q_PROPERTY(double tapPower READ tapPower NOTIFY stateChanged)

    Q_PROPERTY(int epoch READ epoch NOTIFY stateChanged)
    Q_PROPERTY(bool arrived READ arrived NOTIFY stateChanged)
    Q_PROPERTY(QVariantList echoes READ echoes NOTIFY stateChanged)
    Q_PROPERTY(bool founderVisible READ founderVisible NOTIFY stateChanged)
    Q_PROPERTY(QString pendingNarration READ pendingNarration NOTIFY stateChanged)
    Q_PROPERTY(double sleepPercent READ sleepPercent NOTIFY stateChanged)
    Q_PROPERTY(double focusPercent READ focusPercent NOTIFY stateChanged)
    Q_PROPERTY(int founderAge READ founderAgeQ NOTIFY liveChanged)
    Q_PROPERTY(QVariantList generators READ generators NOTIFY stateChanged)
    Q_PROPERTY(QStringList creatures READ creatures NOTIFY stateChanged)
    Q_PROPERTY(double creatureBonusPercent READ creatureBonusPercent NOTIFY stateChanged)
    Q_PROPERTY(int buyAmount READ buyAmount WRITE setBuyAmount NOTIFY stateChanged)

    Q_PROPERTY(bool openingVisible READ openingVisible NOTIFY stateChanged)
    Q_PROPERTY(bool openingDone READ openingDone NOTIFY stateChanged)
    Q_PROPERTY(double openingCost READ openingCost CONSTANT)

    Q_PROPERTY(bool refoundVisible READ refoundVisible NOTIFY stateChanged)
    Q_PROPERTY(int refoundGain READ refoundGain NOTIFY stateChanged)
    Q_PROPERTY(double refoundGainPercent READ refoundGainPercent NOTIFY stateChanged)
    Q_PROPERTY(double bonusPercent READ bonusPercent NOTIFY stateChanged)

    // Cover state.
    Q_PROPERTY(bool momentActive READ momentActive NOTIFY liveChanged)
    Q_PROPERTY(double sitCostNow READ sitCostNow NOTIFY stateChanged)
    Q_PROPERTY(bool feedReady READ feedReady NOTIFY liveChanged)
    Q_PROPERTY(bool lingerReady READ lingerReady NOTIFY liveChanged)
    Q_PROPERTY(double careFeedValue READ careFeedValue CONSTANT)
    Q_PROPERTY(double careLingerValue READ careLingerValue CONSTANT)

public:
    explicit GrainController(QObject* parent = nullptr);
    ~GrainController() override;

    QString appVersion() const;
    QString language() const;
    void setLanguage(const QString& code);

    double recette() const;
    double soin() const;
    double recettePerSec() const;
    double soinPerSecQ() const;
    double nowMsQ() const;
    double tapPower() const;
    int epoch() const;
    QVariantList generators() const;
    QStringList creatures() const;
    double creatureBonusPercent() const;
    bool arrived() const;
    QVariantList echoes() const;
    bool founderVisible() const;
    QString pendingNarration() const;
    double sleepPercent() const;
    double focusPercent() const;
    int founderAgeQ() const;
    int buyAmount() const;
    void setBuyAmount(int n);

    bool openingVisible() const;
    bool openingDone() const;
    double openingCost() const;

    bool refoundVisible() const;
    int refoundGain() const;
    double refoundGainPercent() const;
    double bonusPercent() const;

    bool momentActive() const;
    double sitCostNow() const;
    bool feedReady() const;
    bool lingerReady() const;
    double careFeedValue() const;
    double careLingerValue() const;

    // Player actions. Each flushes pending accrual first, then appends exactly one event.
    Q_INVOKABLE void tap();
    Q_INVOKABLE void buy(int g);          // buys `buyAmount` units, all-or-nothing
    Q_INVOKABLE void hire(int g);
    Q_INVOKABLE void run(int g);          // start a manual production cycle
    Q_INVOKABLE void arrive();            // step past the intro
    Q_INVOKABLE void buyEcho(int i);      // one-shot improvement
    Q_INVOKABLE void ackNarration();      // mark the pending narration as shown
    Q_INVOKABLE void inaugurate();
    Q_INVOKABLE void care(const QString& kind);   // "feed" | "linger"
    Q_INVOKABLE void bury();
    Q_INVOKABLE void sit();
    Q_INVOKABLE void refound();

    // Persist pending taps + elapsed production now (called on app deactivation too).
    Q_INVOKABLE void flushNow();

    // Income breakdown rows: { id, total, share, perSec }.
    Q_INVOKABLE QVariantList breakdown() const;

    // Epoch recette over time, downsampled for the chart: [{ t, v }].
    Q_INVOKABLE QVariantList history() const;

    // Founder sleep readout over time (one point per resolved moment): [{ t, v }].
    Q_INVOKABLE QVariantList sleepHistory() const;

    // Short human formatting for big numbers ("12,340", "1.24 M").
    Q_INVOKABLE QString fmt(double value) const;

    // Wipe the log (Settings > clear data).
    Q_INVOKABLE void clearData();

signals:
    void stateChanged();   // an event was appended (lists, unlocks, rates)
    void liveChanged();    // timer heartbeat (balances, cooldowns)
    void languageChanged();

private:
    void appendAndApply(const QString& kind, const QString& payload);
    void appendSimple(const QString& kind, qint64 at);           // {at}-only payload
    void recordHistory(qint64 at);
    double liveAccrualRecette() const;   // un-persisted production since the last tick event
    double liveAccrualSoin() const;
    void onUiTick();

    SystemClock m_clock;
    EventStore  m_store;
    QSettings   m_settings;   // device preferences only (language) — never game state
    GameState   m_state;      // the projection: fold of the event log
    quint64     m_salt = 0;

    qint64 m_lastFlushMs = 0; // instant the projection is up to date with (last tick event)
    int    m_pendingTaps = 0;
    int    m_buyAmount = 1;
    QTimer m_uiTimer;

    QVector<QPair<qint64, double> > m_history;  // (instant, epochRecette) for the chart
    int m_historyEpoch = 0;
    QVector<QPair<qint64, double> > m_founderHistory;  // (instant, sleep) at each moment
};

} // namespace grain

#endif // GRAIN_GRAINCONTROLLER_H
