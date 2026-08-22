// SPDX-License-Identifier: MIT
//
// Open and control a Realtime call -- a session with a phone or a browser on
// the other end (the /realtime/calls endpoints):
//
//   client.createRealtimeCall(sdpOffer, config); // open one over WebRTC
//   client.acceptRealtimeCall(callId, config);   // answer it, with a session
//   client.rejectRealtimeCall(callId, 486);      // decline; 0 -> the API's 603
//   client.referRealtimeCall(callId, target);    // transfer it elsewhere
//   client.hangupRealtimeCall(callId);           // end it
//
// **A call id comes from one of two places, neither of them this library.** An
// inbound SIP call announces itself in a `realtime.call.incoming` webhook, which
// OpenAI POSTs to an HTTPS endpoint the application publishes -- a web server,
// not an API client. The other way is to open a call yourself with `create`
// below, which answers with the id.
//
// **`create` is the signalling half of a WebRTC handshake and nothing more.**
// The SDP offer has to come from a peer-connection stack and the answer has to
// go back into it; Qt ships neither, so the media path belongs in the
// application. This example posts a canned offer to show the exchange -- it
// will be refused by a real server, which is the honest demonstration: what the
// library does here is HTTP, and the part that makes audio flow is yours.
//
// **Answering is a decision with a deadline.** The caller is listening to a
// ringing phone while this runs, so a handler that waits on a slow lookup
// before deciding is one the caller hangs up on. Accept first with a safe
// configuration and adjust the session afterwards over the channel.
//
// **The four control verbs are the only endpoints in the library with nothing
// to decode.** The API acknowledges and returns no object, so
// `RealtimeCallReply::finished()` carries no payload -- success *is* the result.
// `create` is the opposite extreme: its result is split between an
// application/sdp body and the `Location` header, and both halves are needed.
//
// Usage:
//   export OPENAI_API_KEY=sk-...
//   ./realtime_calls create offer.sdp       # open a call, print the answer
//   ./realtime_calls accept rtc_abc123      # answer, with a concierge session
//   ./realtime_calls reject rtc_abc123      # decline (SIP 486 Busy Here)
//   ./realtime_calls refer  rtc_abc123 tel:+14155550123
//   ./realtime_calls hangup rtc_abc123

#include <QtOpenAi/Client/Client.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
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
        << "  " << program << " create <offer.sdp-file>\n"
        << "  " << program << " accept <call-id>\n"
        << "  " << program << " reject <call-id>\n"
        << "  " << program << " refer  <call-id> <target-uri>\n"
        << "  " << program << " hangup <call-id>\n"
        << "\n"
        << "A call id comes from a realtime.call.incoming webhook, or from\n"
        << "`create` -- see the comment at the top of this file.\n";
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
    // For `create` the second argument is a file holding the SDP offer; for
    // everything else it is the id of an existing call.
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

    // The concierge the caller is bridged into, configured before they hear
    // anything. Audio in both directions is the point of a phone call, so this
    // is one of the few places {"audio"} is not a choice.
    const auto conciergeSession = [&model] {
        Core::RealtimeSessionConfig session;
        session.setModel(model);
        session.setInstructions(
                QStringLiteral("You are Alex, a friendly concierge for Example Corp. "
                               "Keep answers to one or two sentences."));
        session.setOutputModalities({QStringLiteral("audio")});
        return session;
    };

    // `create` answers with an SDP body and a call id rather than an
    // acknowledgement, so it has its own reply type and its own branch.
    if (action == QLatin1String("create")) {
        QFile offerFile(callId);
        if (!offerFile.open(QIODevice::ReadOnly)) {
            out << "Cannot read " << offerFile.fileName() << "\n"
                << "Pass a file holding an SDP offer from your peer-connection stack.\n";
            return 1;
        }
        const QByteArray sdpOffer = offerFile.readAll();

        // Passing a session means the credential is an API key. With an
        // ephemeral client secret you would call createRealtimeCall(sdpOffer)
        // with no session -- the token carries one, and a bare application/sdp
        // body is the only form the endpoint accepts from it.
        Client::RealtimeCallCreateReply *call
                = client->createRealtimeCall(sdpOffer, conciergeSession());
        QObject::connect(call, &Client::RealtimeCallCreateReply::failed,
                         [&](const Client::ClientError &error) {
                             out << "Error: " << error.message() << "\n";
                             out << "A canned or stale offer is refused here; the offer has to\n"
                                 << "come from a live peer connection.\n";
                             app.exit(1);
                         });
        QObject::connect(call, &Client::RealtimeCallCreateReply::finished,
                         [&](const QString &newCallId, const QByteArray &sdpAnswer) {
                             // Both halves matter: the answer completes the peer
                             // connection, the id is how the call is controlled.
                             out << "Call " << newCallId << " created.\n";
                             out << "SDP answer (" << sdpAnswer.size()
                                 << " bytes) -- hand this to\n"
                                 << "the peer connection as its remote description.\n";
                             out << "End it with:\n"
                                 << "  " << args.value(0) << " hangup " << newCallId << "\n";
                             app.quit();
                         });
        return app.exec();
    }

    Client::RealtimeCallReply *reply = nullptr;

    if (action == QLatin1String("accept")) {
        reply = client->acceptRealtimeCall(callId, conciergeSession());

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
