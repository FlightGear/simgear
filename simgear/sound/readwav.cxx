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

#include <simgear_config.h>

#include "readwav.hxx"

#include <cassert>
#include <cstdlib>

#include <stdio.h> // snprintf
#include <zlib.h> // for gzXXX functions

#include <simgear/misc/sg_path.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/stdint.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/debug/ErrorReportingCallback.hxx>

#include "sample.hxx"

namespace 
{
  class Buffer {
  public:
    ALvoid* data;
    unsigned int format;
    unsigned int block_align;
    ALsizei length;
    ALfloat frequency;
    SGPath path;
      
    Buffer() : data(nullptr), format(AL_NONE), length(0), frequency(0.0f) {}
    
    ~Buffer()
    {
      if (data) {
        free(data);
      }
    }
  };
  
  unsigned int formatConstruct(ALint numChannels, ALint bitsPerSample, bool compressed, const SGPath& path)
  {
    unsigned int rv = 0;
    if (!compressed) {
      if (numChannels == 1 && bitsPerSample == 16) rv = SG_SAMPLE_MONO16;
      else if (numChannels == 1 && bitsPerSample == 8) rv = SG_SAMPLE_MONO8;
      else if (numChannels == 2 && bitsPerSample == 16) rv = SG_SAMPLE_STEREO16;
      else if (numChannels == 2 && bitsPerSample == 8) rv = SG_SAMPLE_STEREO8;
      else {
        char msg[81];
        snprintf(msg, 80, "Unsupported audio format: tracks: %i, bits/sample: %i", numChannels, bitsPerSample);
          throw sg_exception(msg, {}, path, false);
      }
    } else {
      if (numChannels == 1 && bitsPerSample == 4) rv = SG_SAMPLE_ADPCM; 
      else if (numChannels == 1 && bitsPerSample == 8) rv = SG_SAMPLE_MULAW;
      else {
        char msg[81];
        snprintf(msg, 80, "Unsupported compressed audio format: tracks: %i, bits/sample: %i", numChannels, bitsPerSample);
          throw sg_exception(msg, {}, path, false);
      }
    }
    return rv;
  }
  
// function prototype for decoding audio data
  typedef void Codec(Buffer* buf);
  
  void codecLinear(Buffer* /*buf*/)
  {
  }
  
  void codecPCM16BE (Buffer* buf)
  {
    uint16_t *d = (uint16_t *) buf->data;
    size_t i, l = buf->length / 2;
    for (i = 0; i < l; i++) {
      *d = sg_bswap_16(*d); ++d;
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
    if (buf == nullptr)
      throw sg_exception("malloc failed decoing ULaw WAV file");
   
    for (ALsizei i = 0; i < b->length; i++) {
      buf[i] = mulaw2linear(d[i]);
    }
    
    free(b->data);
    b->data = buf;
    b->length = newLength;
  }

  int16_t ima2linear (uint8_t nibble, int16_t *val, uint8_t *idx)
  {
    const int16_t _ima4_index_table[16] =
    {
       -1, -1, -1, -1, 2, 4, 6, 8,
       -1, -1, -1, -1, 2, 4, 6, 8
    };
    const int16_t _ima4_step_table[89] =
    {
         7,     8,     9,    10,    11,    12,    13,    14,    16,    17,
        19,    21,    23,    25,    28,    31,    34,    37,    41,    45,
        50,    55,    60,    66,    73,    80,    88,    97,   107,   118,
       130,   143,   157,   173,   190,   209,   230,   253,   279,   307,
       337,   371,   408,   449,   494,   544,   598,   658,   724,   796,
       876,   963,  1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
      2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,
      5894,  6484,  7132,  7845,  8630,  9493, 10442, 11487, 12635, 13899,
     15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
    };
    int32_t predictor;
    int16_t diff, step;
    int8_t delta, sign;
    int8_t index;

    index = *idx;
    if (index > 88) index = 88;
    else if (index < 0) index = 0;

    predictor = *val;
    step = _ima4_step_table[index];

    sign = nibble & 0x8;
    delta = nibble & 0x7;

    diff = 0;
    if (delta & 4) diff += step;
    if (delta & 2) diff += (step >> 1);
    if (delta & 1) diff += (step >> 2);
    diff += (step >> 3);

    if (sign) predictor -= diff;
    else predictor += diff;

    index += _ima4_index_table[nibble];
    if (index > 88) index = 88;
    else if (index < 0) index = 0;
    *idx = index;

    if (predictor < -32768) predictor = -32768;
    else if (predictor > 32767) predictor = 32767;
    *val = predictor;

    return *val;
  }

  void codecIMA4 (Buffer* b)
  {
    uint8_t *d = (uint8_t *) b->data;
    unsigned int block_align = b->block_align;
    size_t blocks = b->length/block_align;
    size_t newLength = block_align * blocks * 4;
    int16_t *buf = (int16_t *) malloc ( newLength );
    if (buf == nullptr)
      throw sg_exception("malloc failed decoing IMA4 WAV file");

    int16_t *ptr = buf;
    for (size_t i = 0; i < blocks; i++)
    {
      int16_t predictor;
      uint8_t index;

      predictor = *d++;
      predictor |= *d++ << 8;
      index = *d++;
      d++;

      for (size_t j = 0; j < block_align; j += 4)
      {
        for (unsigned int q=0; q<4; q++)
        {
          uint8_t nibble = *d++;
          *ptr++ = ima2linear(nibble & 0xF, &predictor, &index);
          *ptr++ = ima2linear(nibble >> 4, &predictor, &index);
        }
      }
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
    assert(b->data == nullptr);
    
    bool found_header = false;
    bool compressed = false;
    uint16_t bitsPerSample = 8;
    uint16_t numChannels = 1;
    uint32_t chunkLength;
    int32_t magic;
    uint16_t audioFormat;
    uint32_t samplesPerSecond;
    uint32_t byteRate;
    uint16_t blockAlign;
    Codec *codec = codecLinear;

    if (!wavReadBE(fd, magic))
        throw sg_io_exception("corrupt or truncated WAV data", b->path, {}, false);
    
    if (magic != WAV_RIFF_4CC) {
      throw sg_io_exception("not a .wav file", b->path, {}, false);
    }

    if (!wavReadLE(fd, chunkLength) || !wavReadBE(fd, magic))
      throw sg_io_exception("corrupt or truncated WAV data", b->path, {}, false);

    if (magic != WAV_WAVE_4CC)      /* "WAVE" */
    {
        throw sg_io_exception("unrecognized WAV magic", b->path, {}, false);
    }

    while (1) {
        if (!wavReadBE(fd, magic) || !wavReadLE(fd, chunkLength))
            throw sg_io_exception("corrupt or truncated WAV data", b->path, {}, false);

        if (magic == WAV_FORMAT_4CC)  /* "fmt " */
        {
            found_header = true;
            if (chunkLength < 16) {
              throw sg_io_exception("corrupt or truncated WAV data", b->path, {}, false);
            }

            if (!wavReadLE (fd, audioFormat) ||
                !wavReadLE (fd, numChannels) ||
                !wavReadLE (fd, samplesPerSecond) ||
                !wavReadLE (fd, byteRate) ||
                !wavReadLE (fd, blockAlign) ||
                !wavReadLE (fd, bitsPerSample))
            {
                throw sg_io_exception("corrupt or truncated WAV data", b->path, {}, false);
            }

            if (!gzSkip(fd, chunkLength - 16))
                throw sg_io_exception("corrupt or truncated WAV data", b->path, {}, false);

            switch (audioFormat)
              {
              case 1:            /* PCM */
                codec = (bitsPerSample == 8 || sgIsLittleEndian()) ? codecLinear : codecPCM16BE;
                break;
              case 7:            /* uLaw */
                if (alIsExtensionPresent((ALchar *)"AL_EXT_mulaw")) {
                  compressed = true;
                  codec = codecLinear;
                } else {
                  bitsPerSample *= 2; /* uLaw is 16-bit packed into 8 bits */
                  codec = codecULaw;
               }
                break;
              case 17:		/* IMA4 ADPCM */
                if (alIsExtensionPresent((ALchar *)"AL_EXT_ima4") &&
                    (alIsExtensionPresent((ALchar *)"AL_SOFT_block_alignment")
                     || blockAlign == 65)) {
                  compressed = true;
                  codec = codecLinear;
                } else {
                  bitsPerSample *= 4; /* adpcm is 16-bit packed into 4 bits */
                  codec = codecIMA4;
                  
                }
                break;
              default:
                throw sg_io_exception("unsupported WAV encoding:" + std::to_string(audioFormat), b->path, {}, false);
              }
              
              b->block_align = blockAlign;
              b->frequency = samplesPerSecond;
              b->format = formatConstruct(numChannels, bitsPerSample, compressed, b->path);
        } else if (magic == WAV_DATA_4CC) {
            if (!found_header) {
                /* ToDo: A bit wrong to check here, fmt chunk could come later... */
                throw sg_io_exception("corrupt or truncated WAV data", b->path, {}, false);
            }
            
            b->data = malloc(chunkLength);
            b->length = chunkLength;
            size_t read = gzread(fd, b->data, chunkLength);
            if (read != chunkLength) {
                throw sg_io_exception("insufficent data reading WAV file", b->path, {}, false);
            }
            
            break;
        } else {
            if (!gzSkip(fd, chunkLength))
              throw sg_io_exception("corrupt or truncated WAV data", b->path, {}, false);
        }

        if ((chunkLength & 1) && !gzeof(fd) && !gzSkip(fd, 1))
          throw sg_io_exception("corrupt or truncated WAV data", b->path, {}, false);
      } // of file chunk parser loop
      
      codec(b); // might throw if something really bad occurs
  } // of loadWav function
  
} // of anonymous namespace

namespace simgear
{

ALvoid* loadWAVFromFile(const SGPath& path, unsigned int& format, ALsizei& size, ALfloat& freqf, unsigned int& block_align)
{
  if (!path.exists()) {
      simgear::reportFailure(simgear::LoadFailure::NotFound, simgear::ErrorCode::AudioFX, "loadWAVFromFile: not found", path);
    return nullptr;
  }

  Buffer b;
  b.path = path;

  gzFile fd;
#if defined(SG_WINDOWS)
	std::wstring ws = path.wstr();
	fd = gzopen_w(ws.c_str(), "rb");
#else
  std::string ps = path.utf8Str();
  fd = gzopen(ps.c_str(), "rb");
#endif
  if (!fd) {
      simgear::reportFailure(simgear::LoadFailure::IOError, simgear::ErrorCode::AudioFX, "loadWAVFromFile: unable to open file", path);
    return nullptr;
  }

  try {
      loadWavFile(fd, &b);
  } catch (sg_exception& e) {
      simgear::reportFailure(simgear::LoadFailure::IOError, simgear::ErrorCode::AudioFX, "loadWAVFromFile: unable to read file:"
                             + e.getFormattedMessage(), e.getLocation());
      return nullptr;
  }

  ALvoid* data = b.data;
  b.data = nullptr; // don't free when Buffer does out of scope
  format = b.format;
  block_align = b.block_align;
  size = b.length;
  freqf = b.frequency;
  
  gzclose(fd);
  return data;
}

} // of namespace simgear
