// SPDX-License-Identifier: MIT
#pragma once

#include <QtCore/qglobal.h>

// Export/import macro for the QtOpenAi::Sql module.
#if defined(QTOPENAI_SQL_STATIC)
#define QTOPENAI_SQL_EXPORT
#else
#if defined(QTOPENAI_SQL_LIBRARY)
#define QTOPENAI_SQL_EXPORT Q_DECL_EXPORT
#else
#define QTOPENAI_SQL_EXPORT Q_DECL_IMPORT
#endif
#endif
