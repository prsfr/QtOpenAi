// SPDX-License-Identifier: MIT
#pragma once

#include <QtCore/qglobal.h>

// Export/import macro for the QtOpenAi::Client module.
#if defined(QTOPENAI_CLIENT_STATIC)
#  define QTOPENAI_CLIENT_EXPORT
#else
#  if defined(QTOPENAI_CLIENT_LIBRARY)
#    define QTOPENAI_CLIENT_EXPORT Q_DECL_EXPORT
#  else
#    define QTOPENAI_CLIENT_EXPORT Q_DECL_IMPORT
#  endif
#endif
