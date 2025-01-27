

#ifndef SG_SOUND_READWAV_HXX
#define SG_SOUND_READWAV_HXX

#include <al.h>

// forward decls
class SGPath;

#define DEFAULT_IMA4_BLOCKSIZE 		36
#define BLOCKSIZE_TO_SMP(a)		((a) > 1) ? (((a)-4)*2) : 1

namespace simgear
{
  ALvoid* loadWAVFromFile(const SGPath& path, unsigned int& format, ALsizei& size, ALfloat& freqf, unsigned int& block_align);
}

#endif // of SG_SOUND_READWAV_HXX
