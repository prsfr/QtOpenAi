// SPDX-License-Identifier: MIT
//
// The other half of the headless guarantee.
//
// QtOpenAi must not *depend* on a GUI stack — that is checked at configure time
// and again against the built libraries. But "does not depend on" is worthless
// if it also means "does not work with": the library has to be usable from a
// Widgets or Qt Quick application, which is where most of them will be used.
//
// So this links QtOpenAi against Qt6::Widgets (and Qt6::Quick, when the package
// is present), builds a real QApplication, and drives the library's types
// through the mechanisms a GUI application actually uses — signals into a
// widget's slot, and a value type read from QML through the meta-object system.
//
// It is run headless via QT_QPA_PLATFORM=offscreen, so it needs no display.

#include <QtOpenAi/Chat/Transcript.h>
#include <QtOpenAi/Client/Client.h>
#include <QtOpenAi/Client/RateLimiter.h>

#include <QtCore/QUrl>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>

#ifdef QTOPENAI_CONSUMER_HAS_QML
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#endif

using namespace QtOpenAi;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    // A GUI application's own object, connected to the library's signals the
    // ordinary way. If the library forced its own event-loop or application
    // type, this is where it would show.
    QLabel status;
    Client::Client client(QUrl(QStringLiteral("http://localhost:1234/v1")),
                          QStringLiteral("test-key"));

    Client::RateLimiter limiter;
    QObject::connect(
            &limiter, &Client::RateLimiter::queueChanged, &status,
            [&status](int queued, int inFlight) {
                status.setText(QStringLiteral("%1 running, %2 waiting").arg(inFlight).arg(queued));
            });
    client.setRateLimiter(&limiter);
    limiter.setMaxConcurrent(2);

    if (client.rateLimiter() != &limiter)
        return 1;

    // A value type crossing into QVariant, which is how anything reaches a
    // model, a delegate or a property binding.
    Chat::Transcript transcript;
    transcript.addUserMessage(QStringLiteral("hi"));
    const QVariant boxed = QVariant::fromValue(transcript);
    if (!boxed.isValid() || boxed.value<Chat::Transcript>().messages().size() != 1)
        return 1;

#ifdef QTOPENAI_CONSUMER_HAS_QML
    // A QML engine alongside the library, and a gadget read from QML by
    // property name -- the path every QML binding onto a library type takes.
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQml\nQtObject { property int status: 200 }", QUrl());
    const QScopedPointer<QObject> object(component.create());
    if (!object)
        return 1;
#endif

    // Nothing to show; the point was that all of the above compiled, linked and
    // ran inside a QApplication.
    return 0;
}
