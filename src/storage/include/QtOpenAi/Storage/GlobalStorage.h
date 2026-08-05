// SPDX-License-Identifier: MIT
#pragma once

#include <QtCore/qglobal.h>

// Export/import macro for the QtOpenAi::Storage module.
#if defined(QTOPENAI_STORAGE_STATIC)
#define QTOPENAI_STORAGE_EXPORT
#else
#if defined(QTOPENAI_STORAGE_LIBRARY)
#define QTOPENAI_STORAGE_EXPORT Q_DECL_EXPORT
#else
#define QTOPENAI_STORAGE_EXPORT Q_DECL_IMPORT
#endif
#endif
