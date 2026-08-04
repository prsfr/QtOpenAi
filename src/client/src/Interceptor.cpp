// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/Interceptor.h"

namespace QtOpenAi {
namespace Client {

Interceptor::Interceptor(QObject *parent)
    : QObject(parent)
{ }

Interceptor::~Interceptor() = default;

// Both hooks default to doing nothing, so a subclass overrides only the one it
// came for: a logger wants afterResponse(), a header injector beforeRequest().
std::optional<InterceptedResponse> Interceptor::beforeRequest(InterceptedRequest &)
{
    return std::nullopt;
}

void Interceptor::afterResponse(const InterceptedResponse &) { }

} // namespace Client
} // namespace QtOpenAi
