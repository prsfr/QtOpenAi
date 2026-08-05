// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Chat/Transcript.h>
#include <QtOpenAi/Storage/GlobalStorage.h>

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>

#include <functional>

namespace QtOpenAi {

namespace Client {
class MetricsCollector;
}

namespace Storage {

class Store;
class AutosavePrivate;

// Writes to a Store on a timer instead of on every change.
//
// The two obvious policies are both wrong. Saving on every change is a file
// write -- or a transaction -- per streamed fragment. Saving on exit is the
// save that is missing after the crash, which is the case persistence exists
// for. So this sits between them: something says state changed, and the store
// is written at most once per intervalMs().
//
//     Storage::Autosave autosave(&store);
//     autosave.setConversation(QStringLiteral("chat-1"), [&] { return transcript; });
//     connect(&agent, &Chat::Agent::finished, &autosave, &Storage::Autosave::touch);
//
// The conversation is fetched through a callback at save time rather than
// copied in here, because the application goes on editing its transcript and
// would otherwise have to hand over a fresh copy at every change -- which is
// the per-change work this class exists to avoid.
//
// Metrics need no touch(): a MetricsCollector announces every request it
// records, so setMetrics() connects to that itself.
//
// **The destructor does not save.** It is tempting -- the last interval's work
// is exactly what a crash costs -- but the conversation source is a callback
// into the application, and calling it while that application is being torn
// down reads objects that may already be gone. Call flush() at shutdown, where
// the caller still knows what is alive.
class QTOPENAI_STORAGE_EXPORT Autosave : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int intervalMs READ intervalMs WRITE setIntervalMs)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)
    Q_PROPERTY(bool dirty READ isDirty NOTIFY dirtyChanged)
public:
    // The store is not owned and must outlive this object.
    explicit Autosave(Store *store, QObject *parent = nullptr);
    ~Autosave() override;

    Store *store() const;

    // What to save, and where the current version comes from. Passing an empty
    // id, or no source, stops the conversation being saved.
    void setConversation(const QString &id, std::function<Chat::Transcript()> source);
    QString conversationId() const;

    // The same for a collector's snapshot. Connects to the collector's own
    // requestRecorded signal, so recording a request is what marks it dirty.
    void setMetrics(const QString &id, Client::MetricsCollector *collector);
    QString metricsId() const;

    // At most one save per this many milliseconds. Default 5000. Setting it to
    // 0 makes every touch() save immediately, which is the right answer for a
    // test and rarely for anything else.
    int intervalMs() const;
    void setIntervalMs(int ms);

    // A disabled autosave still tracks that something changed, so enabling it
    // again saves what happened in between rather than losing it.
    bool isEnabled() const;
    void setEnabled(bool enabled);

    bool isDirty() const;

public Q_SLOTS:
    // Something changed: save within the interval. Connect it to whatever the
    // application has that says so.
    void touch();

    // Save now, whatever the interval. Returns false only when a write failed
    // -- a flush with nothing to save has done what was asked.
    bool flush();

Q_SIGNALS:
    void saved();
    // The store's lastError(), forwarded at the moment it happened: a failing
    // autosave is silent by construction, and silence here means data loss.
    void failed(const QString &error);
    void dirtyChanged();

private:
    Q_DECLARE_PRIVATE(Autosave)
    QScopedPointer<AutosavePrivate> d_ptr;
};

} // namespace Storage
} // namespace QtOpenAi
