// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 James Turner <zakalawe@mac.com>

/**
 * @file
 * @brief adapted from the freealut sources, especially alutBufferData.c, alutLoader.c
 *        and alutCodec.c (freealut is also LGPL licensed)
 */

#ifndef SG_SOUND_READWAV_HXX
#define SG_SOUND_READWAV_HXX

#if defined( __APPLE__ ) && !defined(SG_SOUND_USES_OPENALSOFT)
# include <OpenAL/al.h>
#elif defined(OPENALSDK)
# include <al.h>
#else
# include <AL/al.h>
#endif

// forward decls
class SGPath;

#define DEFAULT_IMA4_BLOCKSIZE 		36
#define BLOCKSIZE_TO_SMP(a)		((a) > 1) ? (((a)-4)*2) : 1

namespace simgear
{
  ALvoid* loadWAVFromFile(const SGPath& path, unsigned int& format, ALsizei& size, ALfloat& freqf, unsigned int& block_align);
}

#endif // of SG_SOUND_READWAV_HXX
