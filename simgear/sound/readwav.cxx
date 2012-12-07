// Copyright (C) 2012  James Turner - zakalawe@mac.com
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

// adapted from the freealut sources, especially alutBufferData.c, alutLoader.c
// and alutCodec.c (freealut is also LGPL licensed)

#include "readwav.hxx"

#include <cassert>
#include <cstdlib>

#include <zlib.h> // for gzXXX functions

#include <simgear/misc/sg_path.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/stdint.hxx>
#include <simgear/structure/exception.hxx>

namespace 
{
  class Buffer {
  public:
    ALvoid* data;
    ALenum format;
    ALsizei length;
    ALfloat frequency;
    SGPath path;
      
    Buffer() : data(NULL), format(AL_NONE), length(0), frequency(0.0f) {}
    
    ~Buffer()
    {
      if (data) {
        free(data);
      }
    }
  };
  
  ALenum formatConstruct(ALint numChannels, ALint bitsPerSample)
  {
    switch (numChannels)
      {
      case 1:
        switch (bitsPerSample) {
          case 8: return  AL_FORMAT_MONO8;
          case 16: return AL_FORMAT_MONO16;
        }
        break;
      case 2:
        switch (bitsPerSample) {
          case 8: return AL_FORMAT_STEREO8;
          case 16: return AL_FORMAT_STEREO16;
          }
        break;
      }
    return AL_NONE;
  }
  
// function prototype for decoding audio data
  typedef void Codec(Buffer* buf);
  
  void codecLinear(Buffer* /*buf*/)
  {
  }
  
  void codecPCM16 (Buffer* buf)
  {
    // always byte-swaps here; is this a good idea?
    uint16_t *d = (uint16_t *) buf->data;
    size_t i, l = buf->length / 2;
    for (i = 0; i < l; i++) {
      *d = sg_bswap_16(*d);
    }
  }
  
 /*
  * From: http://www.multimedia.cx/simpleaudio.html#tth_sEc6.1
  */
int16_t mulaw2linear (uint8_t mulawbyte)
 {
   static const int16_t exp_lut[8] = {
     0, 132, 396, 924, 1980, 4092, 8316, 16764
   };
   int16_t sign, exponent, mantissa, sample;
   mulawbyte = ~mulawbyte;
   sign = (mulawbyte & 0x80);
   exponent = (mulawbyte >> 4) & 0x07;
   mantissa = mulawbyte & 0x0F;
   sample = exp_lut[exponent] + (mantissa << (exponent + 3));
   return sign ? -sample : sample;
 }

 void codecULaw (Buffer* b)
 {
   uint8_t *d = (uint8_t *) b->data;
   size_t newLength = b->length * 2;
   int16_t *buf = (int16_t *) malloc(newLength);
   if (buf == NULL)
     throw sg_exception("malloc failed decoing ULaw WAV file");
   
   for (ALsizei i = 0; i < b->length; i++) {
       buf[i] = mulaw2linear(d[i]);
    }
    
   free(b->data);
   b->data = buf;
   b->length = newLength;
 }
 
  bool gzSkip(gzFile fd, int skipCount)
  {
      int r = gzseek(fd, skipCount, SEEK_CUR);
      return (r >= 0);
  }
  
  const int32_t WAV_RIFF_4CC = 0x52494646; // 'RIFF'
  const int32_t WAV_WAVE_4CC = 0x57415645; // 'WAVE'
  const int32_t WAV_DATA_4CC = 0x64617461; // 'data'
  const int32_t WAV_FORMAT_4CC = 0x666d7420; // 'fmt '
  
  template<class T>
  bool wavReadBE(gzFile fd, T& value)
  {
    if (gzread(fd, &value, sizeof(T)) != sizeof(T))
      return false;
    
    if (sgIsLittleEndian()) 
      sgEndianSwap(&value);
    
    return true;
  }

  template<class T>
  bool wavReadLE(gzFile fd, T& value)
  {
    if (gzread(fd, &value, sizeof(T)) != sizeof(T))
      return false;
    
    if (sgIsBigEndian()) 
      sgEndianSwap(&value);
    
    return true;
  }
  
  void loadWavFile(gzFile fd, Buffer* b)
  {
    assert(b->data == NULL);
    
    bool found_header = false;
    uint32_t chunkLength;
    int32_t magic;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t samplesPerSecond;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    Codec *codec = codecLinear;

    if (!wavReadBE(fd, magic))
      throw sg_io_exception("corrupt or truncated WAV data", b->path);
    
    if (magic != WAV_RIFF_4CC) {
      throw sg_io_exception("not a .wav file", b->path);
    }

    if (!wavReadLE(fd, chunkLength) || !wavReadBE(fd, magic))
      throw sg_io_exception("corrupt or truncated WAV data", b->path);

    if (magic != WAV_WAVE_4CC)      /* "WAVE" */
    {
        throw sg_io_exception("unrecognized WAV magic", b->path);
    }

    while (1) {
        if (!wavReadBE(fd, magic) || !wavReadLE(fd, chunkLength))
            throw sg_io_exception("corrupt or truncated WAV data", b->path);

        if (magic == WAV_FORMAT_4CC)  /* "fmt " */
        {
            found_header = true;
            if (chunkLength < 16) {
              throw sg_io_exception("corrupt or truncated WAV data", b->path);
            }

            if (!wavReadLE (fd, audioFormat) ||
                !wavReadLE (fd, numChannels) ||
                !wavReadLE (fd, samplesPerSecond) ||
                !wavReadLE (fd, byteRate) ||
                !wavReadLE (fd, blockAlign) ||
                !wavReadLE (fd, bitsPerSample))
              {
                throw sg_io_exception("corrupt or truncated WAV data", b->path);
              }

            if (!gzSkip(fd, chunkLength - 16))
                throw sg_io_exception("corrupt or truncated WAV data", b->path);

            switch (audioFormat)
              {
              case 1:            /* PCM */
                codec = (bitsPerSample == 8 || sgIsLittleEndian()) ? codecLinear : codecPCM16;
                break;
              case 7:            /* uLaw */
                bitsPerSample *= 2;       /* ToDo: ??? */
                codec = codecULaw;
                break;
              default:
                throw sg_io_exception("unsupported WAV encoding", b->path);
              }
              
              b->frequency = samplesPerSecond;
              b->format = formatConstruct(numChannels, bitsPerSample);
        } else if (magic == WAV_DATA_4CC) {
            if (!found_header) {
                /* ToDo: A bit wrong to check here, fmt chunk could come later... */
                throw sg_io_exception("corrupt or truncated WAV data", b->path);
            }
            
            b->data = malloc(chunkLength);
            b->length = chunkLength;
            size_t read = gzread(fd, b->data, chunkLength);
            if (read != chunkLength) {
                throw sg_io_exception("insufficent data reading WAV file", b->path);
            }
            
            break;
        } else {
            if (!gzSkip(fd, chunkLength))
              throw sg_io_exception("corrupt or truncated WAV data", b->path);
        }

        if ((chunkLength & 1) && !gzeof(fd) && !gzSkip(fd, 1))
          throw sg_io_exception("corrupt or truncated WAV data", b->path);
      } // of file chunk parser loop
      
      codec(b); // might throw if something really bad occurs
  } // of loadWav function
  
} // of anonymous namespace

namespace simgear
{

ALvoid* loadWAVFromFile(const SGPath& path, ALenum& format, ALsizei& size, ALfloat& freqf)
{
  if (!path.exists()) {
    throw sg_io_exception("loadWAVFromFile: file not found", path);
  }
  
  Buffer b;
    b.path = path;
    
  gzFile fd;
  fd = gzopen(path.c_str(), "rb");
  if (!fd) {
    throw sg_io_exception("loadWAVFromFile: unable to open file", path);
  }
  
    loadWavFile(fd, &b);
  ALvoid* data = b.data;
    b.data = NULL; // don't free when Buffer does out of scope
    format = b.format;
    size = b.length;
    freqf = b.frequency;
  
  gzclose(fd);
  return data;
}

ALuint createBufferFromFile(const SGPath& path)
{
  ALuint buffer = -1;
#ifdef ENABLE_SOUND
  ALenum format;
  ALsizei size;
  ALfloat sampleFrequency;
  ALvoid* data = loadWAVFromFile(path, format, size, sampleFrequency);
  assert(data);
  
  alGenBuffers(1, &buffer);
  if (alGetError() != AL_NO_ERROR) {
    free(data);
    throw sg_io_exception("OpenAL buffer allocation failed", sg_location(path.str()));
  }
    
  alBufferData (buffer, format, data, size, (ALsizei) sampleFrequency);
  if (alGetError() != AL_NO_ERROR) {
    alDeleteBuffers(1, &buffer);
    free(data);
    throw sg_io_exception("OpenAL setting buffer data failed", sg_location(path.str()));
  }
#endif 
  return buffer;
}

} // of namespace simgear
