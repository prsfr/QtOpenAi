// SPDX-License-Identifier: MIT
//
// Control a SIP call bridged into a Realtime session (the /realtime/calls
// endpoints):
//
//   client.acceptRealtimeCall(callId, config);   // answer it, with a session
//   client.rejectRealtimeCall(callId, 486);      // decline; 0 -> the API's 603
//   client.referRealtimeCall(callId, target);    // transfer it elsewhere
//   client.hangupRealtimeCall(callId);           // end it
//
// **The call id does not come from this library.** It arrives in a
// `realtime.call.incoming` webhook, which OpenAI POSTs to an HTTPS endpoint the
// application publishes -- a web server, not an API client. So there is no call
// to make here that produces one, and this example takes it on the command line
// as a webhook handler would have taken it out of the payload. The create half,
// `POST /realtime/calls`, is not implemented: it takes a WebRTC SDP offer
// produced by a peer-connection stack, which Qt does not ship.
//
// **Answering is a decision with a deadline.** The caller is listening to a
// ringing phone while this runs, so a handler that waits on a slow lookup
// before deciding is one the caller hangs up on. Accept first with a safe
// configuration and adjust the session afterwards over the channel.
//
// **These four are the only endpoints in the library with nothing to decode.**
// The API acknowledges and returns no object, so `RealtimeCallReply::finished()`
// carries no payload -- success *is* the result.
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./realtime_calls accept rtc_abc123      # answer, with a concierge session
//   ./realtime_calls reject rtc_abc123      # decline (SIP 486 Busy Here)
//   ./realtime_calls refer  rtc_abc123 tel:+14155550123
//   ./realtime_calls hangup rtc_abc123

#include <QtOpenAi/Client/Client.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>

using namespace QtOpenAi;

namespace {

// The SIP response sent when declining. 486 (Busy Here) is the one a caller's
// phone reports as "busy"; the API's own default is 603 (Decline), which most
// networks surface as a hard rejection instead. Which of the two is right is a
// product decision, so it is spelled out here rather than left to the default.
constexpr int kBusyHere = 486;

void usage(QTextStream &out, const QString &program)
{
    out << "Usage:\n"
        << "  " << program << " accept <call-id>\n"
        << "  " << program << " reject <call-id>\n"
        << "  " << program << " refer  <call-id> <target-uri>\n"
        << "  " << program << " hangup <call-id>\n"
        << "\n"
        << "The call id comes from a realtime.call.incoming webhook -- see the\n"
        << "comment at the top of this file.\n";
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString apiKey = env.value(QStringLiteral("OPENAI_API_KEY"));
    const QString baseUrl = env.value(QStringLiteral("OPENAI_BASE_URL"),
                                      QStringLiteral("https://api.openai.com/v1"));
    const QString model = env.value(QStringLiteral("OPENAI_MODEL"), QStringLiteral("gpt-realtime"));

    const QStringList args = app.arguments();
    const QString action = args.value(1);
    const QString callId = args.value(2);
    if (action.isEmpty() || callId.isEmpty()) {
        usage(out, args.value(0));
        return 1;
    }
    if (apiKey.isEmpty()) {
        out << "Set OPENAI_API_KEY to run this example.\n";
        return 1;
    }

    auto *client = new Client::Client(QUrl(baseUrl), apiKey, &app);

    Client::RealtimeCallReply *reply = nullptr;

    if (action == QLatin1String("accept")) {
        // The session the caller is bridged into, configured before they hear
        // anything. Audio in both directions is the point of a phone call, so
        // this is one of the few places {"audio"} is not a choice.
        Core::RealtimeSessionConfig session;
        session.setModel(model);
        session.setInstructions(
                QStringLiteral("You are Alex, a friendly concierge for Example Corp. "
                               "Keep answers to one or two sentences."));
        session.setOutputModalities({QStringLiteral("audio")});

        reply = client->acceptRealtimeCall(callId, session);

    } else if (action == QLatin1String("reject")) {
        reply = client->rejectRealtimeCall(callId, kBusyHere);

    } else if (action == QLatin1String("refer")) {
        const QString target = args.value(3);
        if (target.isEmpty()) {
            out << "refer needs a target, e.g. tel:+14155550123 or sip:agent@example.com\n";
            return 1;
        }
        // A transfer hands the caller to somebody else and ends the model's
        // part in the call; there is no coming back from it.
        reply = client->referRealtimeCall(callId, target);

    } else if (action == QLatin1String("hangup")) {
        reply = client->hangupRealtimeCall(callId);

    } else {
        usage(out, args.value(0));
        return 1;
    }

    QObject::connect(reply, &Client::RealtimeCallReply::failed,
                     [&](const Client::ClientError &error) {
                         // A call id is short-lived: one that has already been
                         // answered, declined or abandoned is gone, and that is
                         // the common failure here rather than a bad key.
                         out << "Error: " << error.message() << "\n";
                         app.exit(1);
                     });
    QObject::connect(reply, &Client::RealtimeCallReply::finished, [&] {
        // Nothing to print but the fact that it worked: the API acknowledges
        // these four and returns no object.
        out << "OK: " << action << " " << callId << "\n";
        if (action == QLatin1String("accept")) {
            // What a real handler does next: attach to the live call to watch
            // it and steer it, using the optional QtOpenAi::Realtime module.
            //
            //   Realtime::RealtimeConnection connection;
            //   connection.setUrl(QUrl("wss://api.openai.com/v1/realtime"
            //                          "?call_id=" + callId));
            //   connection.setApiKey(apiKey);   // leave the model unset --
            //   connection.open();              // accept already chose it
            //
            // Not done here, so that this example needs nothing beyond
            // QtOpenAi::Client. See examples/realtime.cpp for the channel.
            out << "The caller is now talking to the model. Attach to the call with\n"
                << "  wss://api.openai.com/v1/realtime?call_id=" << callId << "\n"
                << "to follow it (see examples/realtime.cpp), or end it with:\n"
                << "  " << args.value(0) << " hangup " << callId << "\n";
        }
        app.quit();
    });

    return app.exec();
}
