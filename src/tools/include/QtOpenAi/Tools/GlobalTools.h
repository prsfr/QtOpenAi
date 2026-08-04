// SPDX-License-Identifier: MIT
#pragma once

#include <QtCore/qglobal.h>

// Export/import macro for the QtOpenAi::Tools module.
#if defined(QTOPENAI_TOOLS_STATIC)
#define QTOPENAI_TOOLS_EXPORT
#else
#if defined(QTOPENAI_TOOLS_LIBRARY)
#define QTOPENAI_TOOLS_EXPORT Q_DECL_EXPORT
#else
#define QTOPENAI_TOOLS_EXPORT Q_DECL_IMPORT
#endif
#endif
