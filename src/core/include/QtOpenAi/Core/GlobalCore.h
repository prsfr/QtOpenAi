// SPDX-License-Identifier: MIT
#pragma once

#include <QtCore/qglobal.h>

// Export/import macro for the QtOpenAi::Core module.
#if defined(QTOPENAI_CORE_STATIC)
#define QTOPENAI_CORE_EXPORT
#else
#if defined(QTOPENAI_CORE_LIBRARY)
#define QTOPENAI_CORE_EXPORT Q_DECL_EXPORT
#else
#define QTOPENAI_CORE_EXPORT Q_DECL_IMPORT
#endif
#endif
