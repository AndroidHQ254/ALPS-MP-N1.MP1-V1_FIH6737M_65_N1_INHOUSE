/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#ifdef MTK_ART_COMMON
#include "base/logging.h"
#endif

namespace art {

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
#endif

int CmplFloat(float a, float b) {
  if (a == b) {
    return 0;
  } else if (a < b) {
    return -1;
  } else if (a > b) {
    return 1;
  }
  return -1;
}

int CmpgFloat(float a, float b) {
  if (a == b) {
    return 0;
  } else if (a < b) {
    return -1;
  } else if (a > b) {
    return 1;
  }
  return 1;
}

int CmpgDouble(double a, double b) {
  if (a == b) {
    return 0;
  } else if (a < b) {
    return -1;
  } else if (a > b) {
    return 1;
  }
  return 1;
}

int CmplDouble(double a, double b) {
  if (a == b) {
    return 0;
  } else if (a < b) {
    return -1;
  } else if (a > b) {
    return 1;
  }
  return -1;
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

extern "C" int64_t artLmul(int64_t a, int64_t b) {
  return a * b;
}

extern "C" int64_t artLdiv(int64_t a, int64_t b) {
  return a / b;
}

extern "C" int64_t artLmod(int64_t a, int64_t b) {
  return a % b;
}

#ifdef MTK_ART_COMMON
// Below integer power() doesn't support negative exponent.
extern "C" int64_t artLpow64(int64_t a, int64_t b) {
  DCHECK_GE(b, 0);
  int64_t result = 1;
  while (b != 0) {
    if ((b & 1) == 1)
      result *= a;
    b >>= 1;
    a *= a;
  }
  return result;
}

extern "C" int32_t artLpow32(int32_t a, int32_t b) {
  DCHECK_GE(b, 0);
  int32_t result = 1;
  while (b != 0) {
    if ((b & 1) == 1)
      result *= a;
    b >>= 1;
    a *= a;
  }
  return result;
}
#endif

}  // namespace art
