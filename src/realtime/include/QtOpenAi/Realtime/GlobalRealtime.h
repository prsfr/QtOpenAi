// SPDX-License-Identifier: MIT
#pragma once

#include <QtCore/qglobal.h>

// Export/import macro for the QtOpenAi::Realtime module.
#if defined(QTOPENAI_REALTIME_STATIC)
#define QTOPENAI_REALTIME_EXPORT
#else
#if defined(QTOPENAI_REALTIME_LIBRARY)
#define QTOPENAI_REALTIME_EXPORT Q_DECL_EXPORT
#else
#define QTOPENAI_REALTIME_EXPORT Q_DECL_IMPORT
#endif
#endif
