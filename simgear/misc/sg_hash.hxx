// Hash functions with simgear API
//
// Copyright (C) 2016  James Turner <zakalawe@mac.com>
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
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#include <stdint.h>
#include <sys/types.h>

namespace simgear
{

  /* header */

  #define HASH_LENGTH 20
  #define BLOCK_LENGTH 64

  typedef struct sha1nfo {
  	uint32_t buffer[BLOCK_LENGTH/4];
  	uint32_t state[HASH_LENGTH/4];
  	uint32_t byteCount;
  	uint8_t bufferOffset;
  	uint8_t keyBuffer[BLOCK_LENGTH];
  	uint8_t innerHash[HASH_LENGTH];
  } sha1nfo;

  /* public API - prototypes - TODO: doxygen*/

  /**
   */
  void sha1_init(sha1nfo *s);
  /**
   */
  void sha1_writebyte(sha1nfo *s, uint8_t data);
  /**
   */
  void sha1_write(sha1nfo *s, const char *data, size_t len);
  /**
   */
  uint8_t* sha1_result(sha1nfo *s);
  /**
   */
  void sha1_initHmac(sha1nfo *s, const uint8_t* key, int keyLength);
  /**
   */
  uint8_t* sha1_resultHmac(sha1nfo *s);


}
