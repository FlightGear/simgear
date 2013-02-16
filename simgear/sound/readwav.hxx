

#ifndef SG_SOUND_READWAV_HXX
#define SG_SOUND_READWAV_HXX

#if defined( __APPLE__ )
# include <OpenAL/al.h>
#elif defined(OPENALSDK)
# include <al.h>
#else
# include <AL/al.h>
#endif

// forward decls
class SGPath;

namespace simgear
{
  ALvoid* loadWAVFromFile(const SGPath& path, ALenum& format, ALsizei& size, ALfloat& freqf);
  
  ALuint createBufferFromFile(const SGPath& path);
}

#endif // of SG_SOUND_READWAV_HXX
