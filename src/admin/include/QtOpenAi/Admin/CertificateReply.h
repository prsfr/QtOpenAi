// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Admin/GlobalAdmin.h>
#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Certificate.h>

namespace QtOpenAi {

namespace Client {
// Declared here so the friendships below can name it; including Client.h would
// pull ~70 reply headers into every user of this one.
class Client;
} // namespace Client

namespace Admin {

// An asynchronous handle for one certificate (POST /organization/certificates,
// GET/POST/DELETE /organization/certificates/{certificate_id}). See
// Client::RestReplyBase for the shared lifecycle.
//
// The reply types of this module derive from the same TypedReply<T> as every
// other endpoint's, and are created through the same request path -- see
// Organization. The friend declaration names Client::Client because that is
// where replies are constructed, so that a reply still cannot be created from
// outside the library.
//
// The deletion acknowledgement decodes into Core::Certificate as well, keeping
// the id and reporting the object as "certificate.deleted".
class QTOPENAI_ADMIN_EXPORT CertificateReply : public Client::TypedReply<Core::Certificate>
{
    Q_OBJECT
public:
    Core::Certificate certificate() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Certificate &certificate);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::Certificate &certificate) override
    {
        Q_EMIT finished(certificate);
    }
};

// An asynchronous handle for several certificates at once: a page of them from
// either scope's listing, and the batch that an activation or deactivation
// reports back. One reply for both because the API answers both with `data` --
// see Core::CertificateList.
class QTOPENAI_ADMIN_EXPORT CertificateListReply : public Client::TypedReply<Core::CertificateList>
{
    Q_OBJECT
public:
    Core::CertificateList certificates() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::CertificateList &certificates);

private:
    friend class QtOpenAi::Client::Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::CertificateList &certificates) override
    {
        Q_EMIT finished(certificates);
    }
};

} // namespace Admin
} // namespace QtOpenAi
