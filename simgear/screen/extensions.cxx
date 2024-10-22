// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2001 Cesar Blecua Udias

#include "extensions.hxx"

#include <cstring>
#include <osg/GL> // for glGetString

bool SGSearchExtensionsString(const char *extString, const char *extName) {
    // Returns GL_TRUE if the *extName string appears in the *extString string,
    // surrounded by white spaces, or GL_FALSE otherwise.

    const char *p, *end;
    int n, extNameLen;

    if ((extString == NULL) || (extName == NULL))
        return false;

    extNameLen = strlen(extName);

    p=extString;
    end = p + strlen(p);

    while (p < end) {
        n = strcspn(p, " ");
        if ((extNameLen == n) && (strncmp(extName, p, n) == 0))
            return GL_TRUE;

        p += (n + 1);
    }

    return GL_FALSE;
}

bool SGIsOpenGLExtensionSupported(const char *extName) {
   // Returns GL_TRUE if the OpenGL Extension whose name is *extName
   // is supported by the system, or GL_FALSE otherwise.
   //
   // The *extName string must follow the OpenGL extensions naming scheme
   // (ie: "GL_type_extension", like GL_EXT_convolution)

    return SGSearchExtensionsString((const char *)glGetString(GL_EXTENSIONS),extName);
}
