// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/ChatCompletionResponse.h>
#include <QtOpenAi/Core/ListPage.h>
#include <QtOpenAi/Core/Message.h>

#include <QtCore/QMetaType>

namespace QtOpenAi {
namespace Core {

// A page of stored chat completions (GET /chat/completions).
using ChatCompletionList = ListPage<ChatCompletionResponse>;

// A page of a stored completion's input messages
// (GET /chat/completions/{id}/messages).
using ChatCompletionMessageList = ListPage<Message>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_METATYPE(QtOpenAi::Core::ChatCompletionList)
Q_DECLARE_METATYPE(QtOpenAi::Core::ChatCompletionMessageList)
