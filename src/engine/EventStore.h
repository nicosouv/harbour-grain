// The single source of truth: an append-only event log over SQLite. Never UPDATE or DELETE rows
// in `events` — current state is a fold of this log (see StateProjection), so the whole engine is
// reproducible and testable. Also owns install_meta (the per-install salt that seeds every
// deterministic roll). The only erasure path is clearAll (Settings > clear data).
#ifndef GRAIN_EVENTSTORE_H
#define GRAIN_EVENTSTORE_H

#include <QString>
#include <QVector>
#include <QtGlobal>

namespace grain {

struct Event {
    qint64  seq = 0;      // monotonic primary key (append order), never reused
    int     epoch = 0;    // epoch the event belongs to
    qint64  tsMs = 0;     // unix millis at append time
    QString kind;         // 'tap' | 'buy' | 'hire' | 'open' | 'tick' | 'care' | 'bury' | 'sit' | 'confess'
    QString payload;      // kind-specific JSON; carries its own clock data so the fold stays pure
};

class EventStore {
public:
    explicit EventStore(const QString& connectionName = QStringLiteral("grain_main"));
    ~EventStore();

    // Open the database at `path` (use ":memory:" for tests). Returns false on failure.
    bool open(const QString& path);
    void close();
    bool isOpen() const;

    // Create tables if absent and ensure the single install_meta row exists. If it's a fresh
    // install, `saltIfNew` becomes the permanent install salt (production passes an entropy mix;
    // tests pass a fixed one). Idempotent — safe to call on every launch.
    bool bootstrap(quint64 saltIfNew);

    // The permanent per-install salt that seeds all deterministic RNG. 0 if not bootstrapped.
    quint64 installSalt() const;

    // Append one event. Returns the new seq, or -1 on failure.
    qint64 appendEvent(const QString& kind, int epoch, qint64 tsMs, const QString& payload);

    // All events in append order. This is what the projection folds over.
    QVector<Event> events() const;

    // Cheap count of the log.
    int eventCount() const;

    // Wipe the log. Keeps install_meta. The one ordinary door (Settings > clear data).
    bool clearAll();

private:
    bool exec(const QString& sql);

    QString m_connectionName;
    bool    m_open = false;
};

} // namespace grain

#endif // GRAIN_EVENTSTORE_H
