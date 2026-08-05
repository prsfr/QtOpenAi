// SPDX-License-Identifier: MIT
#pragma once

#include <QtCore/qglobal.h>

// Export/import macro for the QtOpenAi::Admin module.
#if defined(QTOPENAI_ADMIN_STATIC)
#define QTOPENAI_ADMIN_EXPORT
#else
#if defined(QTOPENAI_ADMIN_LIBRARY)
#define QTOPENAI_ADMIN_EXPORT Q_DECL_EXPORT
#else
#define QTOPENAI_ADMIN_EXPORT Q_DECL_IMPORT
#endif
#endif
