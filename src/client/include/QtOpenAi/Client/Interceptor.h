// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkRequest>

#include <optional>

namespace QtOpenAi {
namespace Client {

// A request on its way out, as an interceptor may still change it.
//
// The verb travels alongside the QNetworkRequest because a QNetworkRequest does
// not carry one -- and without it `GET /files/x` and `DELETE /files/x` are the
// same request, which a cache would get catastrophically wrong.
struct QTOPENAI_CLIENT_EXPORT InterceptedRequest
{
    // "GET", "POST" or "DELETE".
    QByteArray method;
    // The request as it will go out. Modify it to add or replace headers.
    QNetworkRequest request;
    // The JSON about to be sent. Empty for GET and DELETE, and for the
    // multipart uploads, whose body is assembled per attempt and can be a whole
    // file -- an interceptor that hashes or logs bodies must not assume one.
    QByteArray body;

    QUrl url() const { return request.url(); }
};

// One HTTP exchange, as an interceptor sees it after the fact.
//
// A plain struct: it is a bag of readings with no invariants to protect, and
// making it a d-pointer value type would buy nothing but an allocation.
struct QTOPENAI_CLIENT_EXPORT InterceptedResponse
{
    // The request this answers, as it went out. Without it the two hooks could
    // not be correlated: a client has several requests in flight at once, so an
    // interceptor that remembered "the last request" would attribute responses
    // to the wrong one. A cache needs exactly this to know what to store under.
    InterceptedRequest request;
    QByteArray body;
    // 0 when the request never reached a response at all (DNS, TLS, timeout);
    // `error` says what happened in that case.
    int httpStatus = 0;
    qint64 elapsedMs = 0;
    // True when an interceptor answered the request instead of the network.
    bool fromCache = false;
    ClientError error;

    QUrl url() const { return request.url(); }
    bool isSuccess() const { return httpStatus >= 200 && httpStatus < 400; }
};

// A hook around every request a Client makes.
//
// Subclass it, override one or both hooks, and install it:
//
//     LoggingInterceptor logger;
//     client.addInterceptor(&logger);
//
// Interceptors exist for the things that have to happen on *every* call and
// cannot be written at the call sites: structured logging with the credentials
// redacted, a trace header whose value differs per request, serving a repeated
// request from a cache. A header with a *constant* value is not one of them --
// that is Client::setDefaultHeader(), and routing it through an interceptor
// would only make it harder to find.
//
// Ordering nests, the way middleware conventionally does: beforeRequest() runs
// in installation order, afterResponse() in reverse, so the first interceptor
// installed is the outermost one and sees the exchange whole.
//
// **Nothing is paid for when nothing is installed.** The chain is one empty-list
// check on the path that builds a request.
//
// A note on scope: afterResponse() reports a single response body, so it does
// not fire for the streaming endpoints, whose body arrives as a sequence of
// events and never exists as one object. beforeRequest() fires for those too,
// so header injection and logging still cover them.
//
// Not a value type and not copyable: an interceptor is an identity that the
// Client holds a pointer to. It is a QObject so that installing one does not
// hand the Client a dangling pointer -- a destroyed interceptor removes itself
// from every Client it was installed in. Ownership stays with the caller.
class QTOPENAI_CLIENT_EXPORT Interceptor : public QObject
{
    Q_OBJECT
public:
    explicit Interceptor(QObject *parent = nullptr);
    ~Interceptor() override;

    // Fires once per logical request, before the first attempt -- not once per
    // retry, so a header stamped here is stable across the retries of one call,
    // which is what an idempotency key or a trace id needs.
    //
    // `request` may be modified freely; what the next interceptor sees is what
    // this one leaves behind.
    //
    // Returning a response answers the call from here: no network request is
    // made, the remaining interceptors' beforeRequest() are skipped, and the
    // reply decodes the body returned. Returning nothing lets the call proceed.
    virtual std::optional<InterceptedResponse> beforeRequest(InterceptedRequest &request);

    // Fires once when the request settles, whatever the outcome: a 2xx, an HTTP
    // error, or a transport failure. Retries are already over by then --
    // `response` describes the attempt that ended it.
    virtual void afterResponse(const InterceptedResponse &response);
};

} // namespace Client
} // namespace QtOpenAi
