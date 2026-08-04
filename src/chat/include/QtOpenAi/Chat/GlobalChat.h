// SPDX-License-Identifier: MIT
#pragma once

#include <QtCore/qglobal.h>

// Export/import macro for the QtOpenAi::Chat module.
#if defined(QTOPENAI_CHAT_STATIC)
#define QTOPENAI_CHAT_EXPORT
#else
#if defined(QTOPENAI_CHAT_LIBRARY)
#define QTOPENAI_CHAT_EXPORT Q_DECL_EXPORT
#else
#define QTOPENAI_CHAT_EXPORT Q_DECL_IMPORT
#endif
#endif
