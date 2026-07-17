// The engine's one and only source of "now". Injectable so tests can pin time and assert exact,
// deterministic behaviour (offline accrual, cover-action cadence). No other engine code may call
// wall-clock time directly; events carry their own timestamps in the payload so the fold stays pure.
#ifndef GRAIN_CLOCK_H
#define GRAIN_CLOCK_H

#include <QDateTime>

namespace grain {

class Clock {
public:
    virtual ~Clock() = default;

    // Current instant as unix milliseconds (UTC).
    virtual qint64 nowMs() const = 0;
};

// Production clock: the real system time.
class SystemClock : public Clock {
public:
    qint64 nowMs() const override { return QDateTime::currentMSecsSinceEpoch(); }
};

// Test clock: a fixed, settable instant.
class FixedClock : public Clock {
public:
    explicit FixedClock(qint64 ms) : m_now(ms) {}
    qint64 nowMs() const override { return m_now; }
    void set(qint64 ms) { m_now = ms; }
    void advanceMs(qint64 ms) { m_now += ms; }
private:
    qint64 m_now;
};

} // namespace grain

#endif // GRAIN_CLOCK_H
