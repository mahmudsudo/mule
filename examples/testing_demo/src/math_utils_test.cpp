#include "math_utils.h"
#include "mule_test.h"

MULE_TEST(add_positive) {
    MULE_ASSERT(add(2, 2) == 4);
}

MULE_TEST(add_negative) {
    MULE_ASSERT(add(-1, -1) == -2);
}
