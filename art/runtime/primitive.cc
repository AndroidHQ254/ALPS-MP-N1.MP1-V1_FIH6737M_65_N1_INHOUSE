/*
 * Copyright (C) 2011 The Android Open Source Project
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

#include "primitive.h"

namespace art {

static const char* kTypeNames[] = {
  "PrimNot",
  "PrimBoolean",
  "PrimByte",
  "PrimChar",
  "PrimShort",
  "PrimInt",
  "PrimLong",
  "PrimFloat",
  "PrimDouble",
  "PrimVoid",
#ifdef MTK_ART_COMMON
  "VectorDoublex2",
  "VectorFloatx4",
  "VectorInt32x4",
  "VectorInt16x8",
  "VectorInt8x16",
#endif
};

const char* Primitive::PrettyDescriptor(Primitive::Type type) {
#ifdef MTK_ART_COMMON
  CHECK(Primitive::kPrimNot <= type && type <= Primitive::kVectorInt8x16) << static_cast<int>(type);
#else
  CHECK(Primitive::kPrimNot <= type && type <= Primitive::kPrimVoid) << static_cast<int>(type);
#endif
  return kTypeNames[type];
}

std::ostream& operator<<(std::ostream& os, const Primitive::Type& type) {
  int32_t int_type = static_cast<int32_t>(type);
#ifdef MTK_ART_COMMON
  if (type >= Primitive::kPrimNot && type <= Primitive::kVectorInt8x16) {
#else
  if (type >= Primitive::kPrimNot && type <= Primitive::kPrimVoid) {
#endif
    os << kTypeNames[int_type];
  } else {
    os << "Type[" << int_type << "]";
  }
  return os;
}

}  // namespace art
