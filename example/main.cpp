/*
    SPDX-License-Identifier: BSD-3-Clause
    Copyright (C) 2016-2023 Raúl Ramos
    See the LICENSE file in the project root for more information.
*/
#include <iostream>
#include <iomanip>
#include <utility>
#include <vector>

#undef MPML_ENABLED
#define MPML_ENABLED   1
#define MPML_UNIT_TEST 1
#include "mpml.h"

auto main() -> int {
    // Invoke unit tests (The rest of the unit tests are "run" at compile time)

    qcstudio::mpml::unit_testing::execute();

    return 0;
}
