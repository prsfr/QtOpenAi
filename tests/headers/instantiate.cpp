// SPDX-License-Identifier: MIT
//
// A class template's members are compiled only when it is instantiated, so a
// header compiled on its own says nothing about them. Every class template an
// installed header defines is instantiated here, with a type the library uses
// it with; scripts/checks/45-headers.sh fails when one is missing.
#include <QtOpenAi/Admin/UsageQuery.h>
#include <QtOpenAi/Admin/UsageReply.h>
#include <QtOpenAi/Client/FileListReply.h>
#include <QtOpenAi/Client/PageWalker.h>
#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/BucketPage.h>
#include <QtOpenAi/Core/CursorPage.h>
#include <QtOpenAi/Core/Eval.h>
#include <QtOpenAi/Core/FileObject.h>
#include <QtOpenAi/Core/Group.h>
#include <QtOpenAi/Core/ListPage.h>
#include <QtOpenAi/Core/Model.h>
#include <QtOpenAi/Core/OrganizationUsage.h>

template struct QtOpenAi::Core::ListPage<QtOpenAi::Core::Eval>;
template struct QtOpenAi::Core::CursorPage<QtOpenAi::Core::Group>;
template struct QtOpenAi::Core::Bucket<QtOpenAi::Core::UsageResult>;
template struct QtOpenAi::Core::BucketPage<QtOpenAi::Core::UsageResult>;
template class QtOpenAi::Client::TypedReply<QtOpenAi::Core::Model>;
// Both cursor routes: the default ListParams, and a query type of its own.
template class QtOpenAi::Client::PageWalker<QtOpenAi::Client::FileListReply,
                                            QtOpenAi::Core::FileList>;
template class QtOpenAi::Client::PageWalker<QtOpenAi::Admin::UsageReply, QtOpenAi::Core::UsagePage,
                                            QtOpenAi::Admin::UsageQuery>;
