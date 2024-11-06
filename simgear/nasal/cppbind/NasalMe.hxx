// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2018  Thomas Geymayer <tomgey@gmail.com>


#ifndef SG_NASAL_ME_HXX_
#define SG_NASAL_ME_HXX_

#include <simgear/nasal/nasal.h>

namespace nasal
{

  /**
   * Wrap a naRef to indicate it references the self/me object in Nasal method
   * calls.
   */
  struct Me
  {
    naRef _ref;

    explicit Me(naRef ref = naNil()):
      _ref(ref)
    {}

    operator naRef() { return _ref; }
  };

} // namespace nasal

#endif /* SG_NASAL_ME_HXX_ */
