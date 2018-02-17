/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef ART_COMPILER_OPTIMIZING_GVN_H_
#define ART_COMPILER_OPTIMIZING_GVN_H_

#include "nodes.h"
#include "optimization.h"

namespace art {

class SideEffectsAnalysis;

class GVNOptimization : public HOptimization {
 public:
#ifdef MTK_ART_COMMON
  GVNOptimization(HGraph* graph,
                  const SideEffectsAnalysis& side_effects,
                  const char* pass_name = kGlobalValueNumberingPassName,
                  OptimizingCompilerStats* stats = nullptr)
      : HOptimization(graph, pass_name, stats), side_effects_(side_effects) {}
#else
  GVNOptimization(HGraph* graph,
                  const SideEffectsAnalysis& side_effects,
                  const char* pass_name = kGlobalValueNumberingPassName)
      : HOptimization(graph, pass_name), side_effects_(side_effects) {}
#endif

  void Run() OVERRIDE;

  static constexpr const char* kGlobalValueNumberingPassName = "GVN";

#ifndef MTK_ART_COMMON
 private:
#else
 protected:
#endif
  const SideEffectsAnalysis& side_effects_;

  DISALLOW_COPY_AND_ASSIGN(GVNOptimization);
};

}  // namespace art

#endif  // ART_COMPILER_OPTIMIZING_GVN_H_
