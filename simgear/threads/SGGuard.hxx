// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once
#pragma message(": warning<deprecated> use std::lock_guard and std::mutex.")

// backwards compatibility, just needs a recompile
#define SGGuard         std::lock_guard
