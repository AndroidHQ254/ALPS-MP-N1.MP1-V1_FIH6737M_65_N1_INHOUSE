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

#include "asm_support_arm.S"

#include "arch/quick_alloc_entrypoints.S"

    /* double pow(double a, double b) */
    .extern pow
ENTRY art_quick_pow
    push  {lr}
    .cfi_adjust_cfa_offset 4
    .cfi_rel_offset lr, 0
    sub   sp, #4
    .cfi_adjust_cfa_offset 4
    vmov  r0, r1, d0
    vmov  r2, r3, d1
    bl    pow
    vmov  d0, r0, r1
    add   sp, #4
    .cfi_adjust_cfa_offset -4
    pop   {pc}
END art_quick_pow

    /* float powf(float a, float b) */
     .extern powf
ENTRY art_quick_powf
    push  {lr}
    .cfi_adjust_cfa_offset 4
    .cfi_rel_offset lr, 0
    sub   sp, #4
    .cfi_adjust_cfa_offset 4
    vmov  r0, r1, d0
    bl    powf
    vmov  s0, r0
    add   sp, #4
    .cfi_adjust_cfa_offset -4
    pop   {pc}
END art_quick_pow
