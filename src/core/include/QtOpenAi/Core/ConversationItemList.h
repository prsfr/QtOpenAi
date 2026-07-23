// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ResponseOutputItem.h>

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class ConversationItemListData;

// A cursor-paginated `list` of conversation items. Items reuse the Responses
// item model (ResponseOutputItem): messages, function calls, reasoning, ...
class QTOPENAI_CORE_EXPORT ConversationItemList
{
public:
    ConversationItemList();
    ConversationItemList(const ConversationItemList &other);
    ConversationItemList(ConversationItemList &&other) noexcept;
    ConversationItemList &operator=(const ConversationItemList &other);
    ConversationItemList &operator=(ConversationItemList &&other) noexcept;
    ~ConversationItemList();

    void swap(ConversationItemList &other) noexcept { d.swap(other.d); }

    QList<ResponseOutputItem> items() const;
    void setItems(const QList<ResponseOutputItem> &items);

    // Cursor of the first / last item in this page (for `after`/`before`).
    QString firstId() const;
    void setFirstId(const QString &id);

    QString lastId() const;
    void setLastId(const QString &id);

    // Whether more items exist beyond this page.
    bool hasMore() const;
    void setHasMore(bool hasMore);

    QJsonObject toJson() const;
    static ConversationItemList fromJson(const QJsonObject &json);

    bool operator==(const ConversationItemList &other) const;
    bool operator!=(const ConversationItemList &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ConversationItemListData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::ConversationItemList)
