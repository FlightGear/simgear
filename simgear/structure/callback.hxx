// SPDX-License-Identifier: LGPL-2.1-or-later

/**
 * @file
 * @brief Declaration for simgear callback
*/

#include <functional>

#ifndef _SG_CALLBACK_HXX
#define _SG_CALLBACK_HXX

namespace simgear {
    using Callback = std::function<void()>;
}

#endif // _SG_CALLBACK_HXX
