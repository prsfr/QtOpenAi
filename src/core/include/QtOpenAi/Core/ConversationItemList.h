// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>
#include <QtOpenAi/Core/ResponseOutputItem.h>

#include <QtCore/QMetaType>

namespace QtOpenAi {
namespace Core {

// A cursor-paginated `list` of conversation items. Items reuse the Responses
// item model (ResponseOutputItem): messages, function calls, reasoning, ...
//
// This was once a hand-written type with its own accessors, which made it the
// only list in the library that PageWalker could not iterate. It is now the
// shared ListPage like every other list endpoint, so /conversations/{id}/items
// and /responses/{id}/input_items paginate exactly like the rest.
using ConversationItemList = ListPage<ResponseOutputItem>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_METATYPE(QtOpenAi::Core::ConversationItemList)
