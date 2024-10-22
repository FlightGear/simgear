// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2001 Cesar Blecua Udias

#ifndef __SG_EXTENSIONS_HXX
#define __SG_EXTENSIONS_HXX 1


#include <simgear/compiler.h>

bool SGSearchExtensionsString(const char *extString, const char *extName);
bool SGIsOpenGLExtensionSupported(const char *extName);

#endif // !__SG_EXTENSIONS_HXX

