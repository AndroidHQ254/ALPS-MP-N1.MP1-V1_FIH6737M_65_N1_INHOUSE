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

#ifndef ART_COMPILER_OPTIMIZING_CODE_GENERATOR_ARM64_H_
#define ART_COMPILER_OPTIMIZING_CODE_GENERATOR_ARM64_H_

#include "arch/arm64/quick_method_frame_info_arm64.h"
#include "code_generator.h"
#include "common_arm64.h"
#include "dex/compiler_enums.h"
#include "driver/compiler_options.h"
#include "nodes.h"
#include "parallel_move_resolver.h"
#include "utils/arm64/assembler_arm64.h"
#include "utils/string_reference.h"
#include "vixl/a64/disasm-a64.h"
#include "vixl/a64/macro-assembler-a64.h"

namespace art {
namespace arm64 {

class CodeGeneratorARM64;

// Use a local definition to prevent copying mistakes.
static constexpr size_t kArm64WordSize = kArm64PointerSize;

static const vixl::Register kParameterCoreRegisters[] = {
  vixl::x1, vixl::x2, vixl::x3, vixl::x4, vixl::x5, vixl::x6, vixl::x7
};
static constexpr size_t kParameterCoreRegistersLength = arraysize(kParameterCoreRegisters);
static const vixl::FPRegister kParameterFPRegisters[] = {
  vixl::d0, vixl::d1, vixl::d2, vixl::d3, vixl::d4, vixl::d5, vixl::d6, vixl::d7
};
static constexpr size_t kParameterFPRegistersLength = arraysize(kParameterFPRegisters);

const vixl::Register tr = vixl::x19;                        // Thread Register
static const vixl::Register kArtMethodRegister = vixl::x0;  // Method register on invoke.

const vixl::CPURegList vixl_reserved_core_registers(vixl::ip0, vixl::ip1);
const vixl::CPURegList vixl_reserved_fp_registers(vixl::d31);

const vixl::CPURegList runtime_reserved_core_registers(tr, vixl::lr);

// Callee-saved registers AAPCS64 (without x19 - Thread Register)
const vixl::CPURegList callee_saved_core_registers(vixl::CPURegister::kRegister,
                                                   vixl::kXRegSize,
                                                   vixl::x20.code(),
                                                   vixl::x30.code());
const vixl::CPURegList callee_saved_fp_registers(vixl::CPURegister::kFPRegister,
                                                 vixl::kDRegSize,
                                                 vixl::d8.code(),
                                                 vixl::d15.code());
Location ARM64ReturnLocation(Primitive::Type return_type);

class SlowPathCodeARM64 : public SlowPathCode {
 public:
  explicit SlowPathCodeARM64(HInstruction* instruction)
      : SlowPathCode(instruction), entry_label_(), exit_label_() {}

  vixl::Label* GetEntryLabel() { return &entry_label_; }
  vixl::Label* GetExitLabel() { return &exit_label_; }

  void SaveLiveRegisters(CodeGenerator* codegen, LocationSummary* locations) OVERRIDE;
  void RestoreLiveRegisters(CodeGenerator* codegen, LocationSummary* locations) OVERRIDE;

 private:
  vixl::Label entry_label_;
  vixl::Label exit_label_;

  DISALLOW_COPY_AND_ASSIGN(SlowPathCodeARM64);
};

class JumpTableARM64 : public DeletableArenaObject<kArenaAllocSwitchTable> {
 public:
  explicit JumpTableARM64(HPackedSwitch* switch_instr)
    : switch_instr_(switch_instr), table_start_() {}

  vixl::Label* GetTableStartLabel() { return &table_start_; }

  void EmitTable(CodeGeneratorARM64* codegen);

 private:
  HPackedSwitch* const switch_instr_;
  vixl::Label table_start_;

  DISALLOW_COPY_AND_ASSIGN(JumpTableARM64);
};

static const vixl::Register kRuntimeParameterCoreRegisters[] =
    { vixl::x0, vixl::x1, vixl::x2, vixl::x3, vixl::x4, vixl::x5, vixl::x6, vixl::x7 };
static constexpr size_t kRuntimeParameterCoreRegistersLength =
    arraysize(kRuntimeParameterCoreRegisters);
static const vixl::FPRegister kRuntimeParameterFpuRegisters[] =
    { vixl::d0, vixl::d1, vixl::d2, vixl::d3, vixl::d4, vixl::d5, vixl::d6, vixl::d7 };
static constexpr size_t kRuntimeParameterFpuRegistersLength =
    arraysize(kRuntimeParameterCoreRegisters);

class InvokeRuntimeCallingConvention : public CallingConvention<vixl::Register, vixl::FPRegister> {
 public:
  static constexpr size_t kParameterCoreRegistersLength = arraysize(kParameterCoreRegisters);

  InvokeRuntimeCallingConvention()
      : CallingConvention(kRuntimeParameterCoreRegisters,
                          kRuntimeParameterCoreRegistersLength,
                          kRuntimeParameterFpuRegisters,
                          kRuntimeParameterFpuRegistersLength,
                          kArm64PointerSize) {}

  Location GetReturnLocation(Primitive::Type return_type);

 private:
  DISALLOW_COPY_AND_ASSIGN(InvokeRuntimeCallingConvention);
};

class InvokeDexCallingConvention : public CallingConvention<vixl::Register, vixl::FPRegister> {
 public:
  InvokeDexCallingConvention()
      : CallingConvention(kParameterCoreRegisters,
                          kParameterCoreRegistersLength,
                          kParameterFPRegisters,
                          kParameterFPRegistersLength,
                          kArm64PointerSize) {}

  Location GetReturnLocation(Primitive::Type return_type) const {
    return ARM64ReturnLocation(return_type);
  }


 private:
  DISALLOW_COPY_AND_ASSIGN(InvokeDexCallingConvention);
};

class InvokeDexCallingConventionVisitorARM64 : public InvokeDexCallingConventionVisitor {
 public:
  InvokeDexCallingConventionVisitorARM64() {}
  virtual ~InvokeDexCallingConventionVisitorARM64() {}

  Location GetNextLocation(Primitive::Type type) OVERRIDE;
  Location GetReturnLocation(Primitive::Type return_type) const OVERRIDE {
    return calling_convention.GetReturnLocation(return_type);
  }
  Location GetMethodLocation() const OVERRIDE;

 private:
  InvokeDexCallingConvention calling_convention;

  DISALLOW_COPY_AND_ASSIGN(InvokeDexCallingConventionVisitorARM64);
};

class FieldAccessCallingConventionARM64 : public FieldAccessCallingConvention {
 public:
  FieldAccessCallingConventionARM64() {}

  Location GetObjectLocation() const OVERRIDE {
    return helpers::LocationFrom(vixl::x1);
  }
  Location GetFieldIndexLocation() const OVERRIDE {
    return helpers::LocationFrom(vixl::x0);
  }
  Location GetReturnLocation(Primitive::Type type ATTRIBUTE_UNUSED) const OVERRIDE {
    return helpers::LocationFrom(vixl::x0);
  }
  Location GetSetValueLocation(Primitive::Type type, bool is_instance) const OVERRIDE {
    return Primitive::Is64BitType(type)
        ? helpers::LocationFrom(vixl::x2)
        : (is_instance
            ? helpers::LocationFrom(vixl::x2)
            : helpers::LocationFrom(vixl::x1));
  }
  Location GetFpuLocation(Primitive::Type type ATTRIBUTE_UNUSED) const OVERRIDE {
    return helpers::LocationFrom(vixl::d0);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(FieldAccessCallingConventionARM64);
};

#ifdef MTK_ART_COMMON
class Arm64AsmWrapper {
 public:
  explicit Arm64AsmWrapper(CodeGeneratorARM64* codegen)
      : codegen_(codegen), wrapper_status_(0) {}

  #define MACRO_LIST(V)  \
    V(adrp, 2, const vixl::Register&, rd, int, imm21)  \
    V(Adr, 2, const vixl::Register&, rd, vixl::Label*, label)  \
    V(B, 1, vixl::Label*, label)  \
    V(B, 2, vixl::Label*, label, vixl::Condition, cond)  \
    V(B, 2, vixl::Condition, cond, vixl::Label*, label)  \
    V(Cbnz, 2, const vixl::Register&, rt, vixl::Label*, label)  \
    V(Cbz, 2, const vixl::Register&, rt, vixl::Label*, label)  \
    V(Tbnz, 3, const vixl::Register&, rt, unsigned, bit_pos, vixl::Label*, label)  \
    V(Tbz, 3, const vixl::Register&, rt, unsigned, bit_pos, vixl::Label*, label)  \
    V(bl, 1, int, imm26)  \
    V(Bl, 1, vixl::Label*, label)  \
    V(Blr, 1, const vixl::Register&, xn)  \
    V(Br, 1, const vixl::Register&, xn)  \
    V(Ret, 0)  \
    V(ldr, 2, const vixl::CPURegister&, rt, const vixl::MemOperand&, src)  \
    V(Ldr, 2, const vixl::CPURegister&, rt, const vixl::MemOperand&, addr)  \
    V(Ldr, 2, const vixl::CPURegister&, rt, vixl::RawLiteral*, literal)  \
    V(Ldrb, 2, const vixl::Register&, rt, const vixl::MemOperand&, addr)  \
    V(Ldrsb, 2, const vixl::Register&, rt, const vixl::MemOperand&, addr)  \
    V(Ldrh, 2, const vixl::Register&, rt, const vixl::MemOperand&, addr)  \
    V(Ldrsh, 2, const vixl::Register&, rt, const vixl::MemOperand&, addr)  \
    V(Ldrsw, 2, const vixl::Register&, rt, const vixl::MemOperand&, addr)  \
    V(Str, 2, const vixl::CPURegister&, rt, const vixl::MemOperand&, addr)  \
    V(Strb, 2, const vixl::Register&, rt, const vixl::MemOperand&, addr)  \
    V(Strh, 2, const vixl::Register&, rt, const vixl::MemOperand&, addr)  \
    V(Ldar, 2, const vixl::Register&, rt, const vixl::MemOperand&, src)  \
    V(Ldarb, 2, const vixl::Register&, rt, const vixl::MemOperand&, src)  \
    V(Ldarh, 2, const vixl::Register&, rt, const vixl::MemOperand&, src)  \
    V(Stlr, 2, const vixl::Register&, rt, const vixl::MemOperand&, dst)  \
    V(Stlrb, 2, const vixl::Register&, rt, const vixl::MemOperand&, dst)  \
    V(Stlrh, 2, const vixl::Register&, rt, const vixl::MemOperand&, dst)  \
    V(LoadCPURegList, 2, vixl::CPURegList, registers, const vixl::MemOperand&, src)  \
    V(StoreCPURegList, 2, vixl::CPURegList, registers, const vixl::MemOperand&, dst)  \
    V(add, 3, const vixl::Register&, rd, const vixl::Register&, rn, const vixl::Operand&, operand)  \
    V(Add, 3, const vixl::Register&, rd, const vixl::Register&, rn, const vixl::Operand&, operand)  \
    V(Sub, 3, const vixl::Register&, rd, const vixl::Register&, rn, const vixl::Operand&, operand)  \
    V(Subs, 3, const vixl::Register&, rd, const vixl::Register&, rn, const vixl::Operand&, operand)  \
    V(Mul, 3, const vixl::Register&, rd, const vixl::Register&, rn, const vixl::Register&, rm)  \
    V(Neg, 2, const vixl::Register&, rd, const vixl::Operand&, operand)  \
    V(And, 3, const vixl::Register&, rd, const vixl::Register&, rn, const vixl::Operand&, operand)  \
    V(Orr, 3, const vixl::Register&, rd, const vixl::Register&, rn, const vixl::Operand&, operand)  \
    V(Eor, 3, const vixl::Register&, rd, const vixl::Register&, rn, const vixl::Operand&, operand)  \
    V(Bic, 3, const vixl::Register&, rd, const vixl::Register&, rn, const vixl::Operand&, operand)  \
    V(Orn, 3, const vixl::Register&, rd, const vixl::Register&, rn, const vixl::Operand&, operand)  \
    V(Eon, 3, const vixl::Register&, rd, const vixl::Register&, rn, const vixl::Operand&, operand)  \
    V(Lsl, 3, const vixl::Register&, rd, const vixl::Register&, rn, unsigned, shift)  \
    V(Lsl, 3, const vixl::Register&, rd, const vixl::Register&, rn, const vixl::Register&, rm)  \
    V(Lsr, 3, const vixl::Register&, rd, const vixl::Register&, rn, unsigned, shift)  \
    V(Lsr, 3, const vixl::Register&, rd, const vixl::Register&, rn, const vixl::Register&, rm)  \
    V(Asr, 3, const vixl::Register&, rd, const vixl::Register&, rn, unsigned, shift)  \
    V(Asr, 3, const vixl::Register&, rd, const vixl::Register&, rn, const vixl::Register&, rm)  \
    V(Ror, 3, const vixl::Register&, rd, const vixl::Register&, rs, unsigned, shift)  \
    V(Ror, 3, const vixl::Register&, rd, const vixl::Register&, rn, const vixl::Register&, rm)  \
    V(Sdiv, 3, const vixl::Register&, rd, const vixl::Register&, rn, const vixl::Register&, rm)  \
    V(Smulh, 3, const vixl::Register&, xd, const vixl::Register&, xn, const vixl::Register&, xm)  \
    V(Smull, 3, const vixl::Register&, rd, const vixl::Register&, rn, const vixl::Register&, rm)  \
    V(Mneg, 3, const vixl::Register&, rd, const vixl::Register&, rn, const vixl::Register&, rm)  \
    V(Madd, 4, const vixl::Register&, rd, const vixl::Register&, rn, const vixl::Register&, rm, const vixl::Register&, ra)  \
    V(Msub, 4, const vixl::Register&, rd, const vixl::Register&, rn, const vixl::Register&, rm, const vixl::Register&, ra)  \
    V(Ubfx, 4, const vixl::Register&, rd, const vixl::Register&, rn, unsigned, lsb, unsigned, width)  \
    V(Sbfx, 4, const vixl::Register&, rd, const vixl::Register&, rn, unsigned, lsb, unsigned, width)  \
    V(Fadd, 3, const vixl::VRegister&, vd, const vixl::VRegister&, vn, const vixl::VRegister&, vm)  \
    V(Fsub, 3, const vixl::VRegister&, vd, const vixl::VRegister&, vn, const vixl::VRegister&, vm)  \
    V(Fmul, 3, const vixl::VRegister&, vd, const vixl::VRegister&, vn, const vixl::VRegister&, vm)  \
    V(Fdiv, 3, const vixl::VRegister&, vd, const vixl::VRegister&, vn, const vixl::VRegister&, vm)  \
    V(Fneg, 2, const vixl::VRegister&, vd, const vixl::VRegister&, vn)  \
    V(Fmadd, 4, const vixl::VRegister&, vd, const vixl::VRegister&, vn, const vixl::VRegister&, vm, const vixl::VRegister&, va)\
    V(Cmp, 2, const vixl::Register&, rn, const vixl::Operand&, operand)  \
    V(Csel, 4, const vixl::Register&, rd, const vixl::Register&, rn, const vixl::Operand&, operand, vixl::Condition, cond)  \
    V(Cneg, 3, const vixl::Register&, rd, const vixl::Register&, rn, vixl::Condition, cond)  \
    V(Cset, 2, const vixl::Register&, rd, vixl::Condition, cond)  \
    V(Csetm, 2, const vixl::Register&, rd, vixl::Condition, cond)  \
    V(Fcmp, 2, const vixl::VRegister&, vn, const vixl::VRegister&, vm)  \
    V(Fcmp, 2, const vixl::VRegister&, vn, double, value)  \
    V(Fcsel, 4, const vixl::VRegister&, vd, const vixl::VRegister&, vn, const vixl::VRegister&, vm, vixl::Condition, cond)  \
    V(Fcvt, 2, const vixl::VRegister&, vd, const vixl::VRegister&, vn)  \
    V(Fcvtzs, 2, const vixl::Register&, rd, const vixl::VRegister&, vn)  \
    V(Fcvtzs, 2, const vixl::VRegister&, vd, const vixl::VRegister&, vn)  \
    V(Scvtf, 2, const vixl::VRegister&, vd, const vixl::Register&, rn)  \
    V(Scvtf, 2, const vixl::VRegister&, vd, const vixl::VRegister&, vn)  \
    V(Mov, 2, const vixl::Register&, rd, uint64_t, imm)  \
    V(Mov, 2, const vixl::Register&, rd, const vixl::Register&, rn)  \
    V(Mvn, 2, const vixl::Register&, rd, const vixl::Operand&, operand)  \
    V(Fmov, 2, vixl::VRegister, vd, vixl::VRegister, vn)  \
    V(Fmov, 2, vixl::VRegister, vd, vixl::Register, rn)  \
    V(Fmov, 2, vixl::Register, rd, vixl::VRegister, vn)  \
    V(Fmov, 2, vixl::VRegister, vd, double, imm)  \
    V(Fmov, 2, vixl::VRegister, vd, float, imm)  \
    V(Nop, 0)  \
    V(nop, 0)  \
    V(Bind, 1, vixl::Label*, label)  \
    V(Dmb, 2, vixl::BarrierDomain, domain, vixl::BarrierType, type)  \
    V(Drop, 1, const vixl::Operand&, size)  \
    V(place, 1, vixl::RawLiteral*, literal)  \
    V(FinalizeCode, 0)  \
    V(Dup, 2, const vixl::VRegister&, vd, const vixl::Register&, rn)  \
    V(Dup, 3, const vixl::VRegister&, vd, const vixl::VRegister&, vn, int, vn_index)  \
    V(Addv, 2, const vixl::VRegister&, vd, const vixl::VRegister&, vn)  \
    V(Mov, 3, vixl::VRegister, vd, int, vd_index, vixl::Register, rn) \
    V(Add, 3, const vixl::VRegister&, vd, const vixl::VRegister&, vn, const vixl::VRegister&, vm) \
    V(Sub, 3, const vixl::VRegister&, vd, const vixl::VRegister&, vn, const vixl::VRegister&, vm) \
    V(Mul, 3, const vixl::VRegister&, vd, const vixl::VRegister&, vn, const vixl::VRegister&, vm)

  #define DECLARE_ASM_WRAPPER_FUNC_0(NAME, ...)                           \
    void NAME();
  #define DECLARE_ASM_WRAPPER_FUNC_1(NAME, T1, V1)                         \
    void NAME(T1 V1);
  #define DECLARE_ASM_WRAPPER_FUNC_2(NAME, T1, V1, T2, V2)                   \
    void NAME(T1 V1, T2 V2);
  #define DECLARE_ASM_WRAPPER_FUNC_3(NAME, T1, V1, T2, V2, T3, V3)             \
    void NAME(T1 V1, T2 V2, T3 V3);
  #define DECLARE_ASM_WRAPPER_FUNC_4(NAME, T1, V1, T2, V2, T3, V3, T4, V4)       \
    void NAME(T1 V1, T2 V2, T3 V3, T4 V4);
  #define DECLARE_ASM_WRAPPER_FUNC_5(NAME, T1, V1, T2, V2, T3, V3, T4, V4, T5, V5) \
    void NAME(T1 V1, T2 V2, T3 V3, T4 V4, T5 V5);

  #define DECLARE_ASM_WRAPPER_FUNC(NAME, NUMARG, ...)                              \
    DECLARE_ASM_WRAPPER_FUNC_##NUMARG(NAME, __VA_ARGS__)
    MACRO_LIST(DECLARE_ASM_WRAPPER_FUNC)
  #undef DECLARE_ASM_WRAPPER_FUNC

  // template functions
  template<typename T>
  vixl::Literal<T>* CreateLiteralDestroyedWithPool(T value);

  void SetWrapperStatus(int status) { wrapper_status_ = status; }
  int GetWrapperStatus() { return wrapper_status_; }

private:
  CodeGeneratorARM64* const codegen_;
  int wrapper_status_;
};
#endif

class InstructionCodeGeneratorARM64 : public InstructionCodeGenerator {
 public:
  InstructionCodeGeneratorARM64(HGraph* graph, CodeGeneratorARM64* codegen);

#define DECLARE_VISIT_INSTRUCTION(name, super) \
  void Visit##name(H##name* instr) OVERRIDE;

  FOR_EACH_CONCRETE_INSTRUCTION_COMMON(DECLARE_VISIT_INSTRUCTION)
  FOR_EACH_CONCRETE_INSTRUCTION_ARM64(DECLARE_VISIT_INSTRUCTION)
  FOR_EACH_CONCRETE_INSTRUCTION_SHARED(DECLARE_VISIT_INSTRUCTION)

#undef DECLARE_VISIT_INSTRUCTION

  void VisitInstruction(HInstruction* instruction) OVERRIDE {
    LOG(FATAL) << "Unreachable instruction " << instruction->DebugName()
               << " (id " << instruction->GetId() << ")";
  }

  Arm64Assembler* GetAssembler() const { return assembler_; }
  vixl::MacroAssembler* GetVIXLAssembler() { return GetAssembler()->vixl_masm_; }
#ifdef MTK_ART_COMMON
  Arm64AsmWrapper* GetArm64AsmWrapper();
  bool SwDivideInstruction(int64_t);
#endif

 private:
  void GenerateClassInitializationCheck(SlowPathCodeARM64* slow_path, vixl::Register class_reg);
  void GenerateSuspendCheck(HSuspendCheck* instruction, HBasicBlock* successor);
  void HandleBinaryOp(HBinaryOperation* instr);

  void HandleFieldSet(HInstruction* instruction,
                      const FieldInfo& field_info,
                      bool value_can_be_null);
  void HandleFieldGet(HInstruction* instruction, const FieldInfo& field_info);
  void HandleCondition(HCondition* instruction);

  // Generate a heap reference load using one register `out`:
  //
  //   out <- *(out + offset)
  //
  // while honoring heap poisoning and/or read barriers (if any).
  //
  // Location `maybe_temp` is used when generating a read barrier and
  // shall be a register in that case; it may be an invalid location
  // otherwise.
  void GenerateReferenceLoadOneRegister(HInstruction* instruction,
                                        Location out,
                                        uint32_t offset,
                                        Location maybe_temp);
  // Generate a heap reference load using two different registers
  // `out` and `obj`:
  //
  //   out <- *(obj + offset)
  //
  // while honoring heap poisoning and/or read barriers (if any).
  //
  // Location `maybe_temp` is used when generating a Baker's (fast
  // path) read barrier and shall be a register in that case; it may
  // be an invalid location otherwise.
  void GenerateReferenceLoadTwoRegisters(HInstruction* instruction,
                                         Location out,
                                         Location obj,
                                         uint32_t offset,
                                         Location maybe_temp);
  // Generate a GC root reference load:
  //
  //   root <- *(obj + offset)
  //
  // while honoring read barriers (if any).
  void GenerateGcRootFieldLoad(HInstruction* instruction,
                               Location root,
                               vixl::Register obj,
                               uint32_t offset,
                               vixl::Label* fixup_label = nullptr);

  // Generate a floating-point comparison.
  void GenerateFcmp(HInstruction* instruction);

  void HandleShift(HBinaryOperation* instr);
  void GenerateTestAndBranch(HInstruction* instruction,
                             size_t condition_input_index,
                             vixl::Label* true_target,
                             vixl::Label* false_target);
  void DivRemOneOrMinusOne(HBinaryOperation* instruction);
  void DivRemByPowerOfTwo(HBinaryOperation* instruction);
  void GenerateDivRemWithAnyConstant(HBinaryOperation* instruction);
  void GenerateDivRemIntegral(HBinaryOperation* instruction);
  void HandleGoto(HInstruction* got, HBasicBlock* successor);

  Arm64Assembler* const assembler_;
  CodeGeneratorARM64* const codegen_;

  DISALLOW_COPY_AND_ASSIGN(InstructionCodeGeneratorARM64);
};

class LocationsBuilderARM64 : public HGraphVisitor {
 public:
  LocationsBuilderARM64(HGraph* graph, CodeGeneratorARM64* codegen)
      : HGraphVisitor(graph), codegen_(codegen) {}

#define DECLARE_VISIT_INSTRUCTION(name, super) \
  void Visit##name(H##name* instr) OVERRIDE;

  FOR_EACH_CONCRETE_INSTRUCTION_COMMON(DECLARE_VISIT_INSTRUCTION)
  FOR_EACH_CONCRETE_INSTRUCTION_ARM64(DECLARE_VISIT_INSTRUCTION)
  FOR_EACH_CONCRETE_INSTRUCTION_SHARED(DECLARE_VISIT_INSTRUCTION)

#undef DECLARE_VISIT_INSTRUCTION

  void VisitInstruction(HInstruction* instruction) OVERRIDE {
    LOG(FATAL) << "Unreachable instruction " << instruction->DebugName()
               << " (id " << instruction->GetId() << ")";
  }
#ifdef MTK_ART_COMMON
  bool SwDivideInstruction(int64_t);
#endif

 private:
  void HandleBinaryOp(HBinaryOperation* instr);
  void HandleFieldSet(HInstruction* instruction);
  void HandleFieldGet(HInstruction* instruction);
  void HandleInvoke(HInvoke* instr);
  void HandleCondition(HCondition* instruction);
  void HandleShift(HBinaryOperation* instr);

  CodeGeneratorARM64* const codegen_;
  InvokeDexCallingConventionVisitorARM64 parameter_visitor_;

  DISALLOW_COPY_AND_ASSIGN(LocationsBuilderARM64);
};

class ParallelMoveResolverARM64 : public ParallelMoveResolverNoSwap {
 public:
  ParallelMoveResolverARM64(ArenaAllocator* allocator, CodeGeneratorARM64* codegen)
      : ParallelMoveResolverNoSwap(allocator), codegen_(codegen), vixl_temps_() {}

 protected:
  void PrepareForEmitNativeCode() OVERRIDE;
  void FinishEmitNativeCode() OVERRIDE;
  Location AllocateScratchLocationFor(Location::Kind kind) OVERRIDE;
  void FreeScratchLocation(Location loc) OVERRIDE;
  void EmitMove(size_t index) OVERRIDE;

 private:
  Arm64Assembler* GetAssembler() const;
  vixl::MacroAssembler* GetVIXLAssembler() const {
    return GetAssembler()->vixl_masm_;
  }

  CodeGeneratorARM64* const codegen_;
  vixl::UseScratchRegisterScope vixl_temps_;

  DISALLOW_COPY_AND_ASSIGN(ParallelMoveResolverARM64);
};

class CodeGeneratorARM64 : public CodeGenerator {
 public:
  CodeGeneratorARM64(HGraph* graph,
                     const Arm64InstructionSetFeatures& isa_features,
                     const CompilerOptions& compiler_options,
                     OptimizingCompilerStats* stats = nullptr);
  virtual ~CodeGeneratorARM64() {}

  void GenerateFrameEntry() OVERRIDE;
  void GenerateFrameExit() OVERRIDE;

  vixl::CPURegList GetFramePreservedCoreRegisters() const;
  vixl::CPURegList GetFramePreservedFPRegisters() const;

  void Bind(HBasicBlock* block) OVERRIDE;

  vixl::Label* GetLabelOf(HBasicBlock* block) {
    block = FirstNonEmptyBlock(block);
    return &(block_labels_[block->GetBlockId()]);
  }

  size_t GetWordSize() const OVERRIDE {
    return kArm64WordSize;
  }

  size_t GetFloatingPointSpillSlotSize() const OVERRIDE {
    // Allocated in D registers, which are word sized.
    return kArm64WordSize;
  }

  uintptr_t GetAddressOf(HBasicBlock* block) OVERRIDE {
    vixl::Label* block_entry_label = GetLabelOf(block);
    DCHECK(block_entry_label->IsBound());
    return block_entry_label->location();
  }

  HGraphVisitor* GetLocationBuilder() OVERRIDE { return &location_builder_; }
  HGraphVisitor* GetInstructionVisitor() OVERRIDE { return &instruction_visitor_; }
  Arm64Assembler* GetAssembler() OVERRIDE { return &assembler_; }
  const Arm64Assembler& GetAssembler() const OVERRIDE { return assembler_; }
  vixl::MacroAssembler* GetVIXLAssembler() { return GetAssembler()->vixl_masm_; }
#ifdef MTK_ART_COMMON
  Arm64AsmWrapper* GetArm64AsmWrapper() { return &asm_wrapper_; }
  const CompilerDriver* GetCompilerDriver() OVERRIDE { return compiler_driver_; }
  void SetCompilerDriver(CompilerDriver* compiler_driver) { compiler_driver_ = compiler_driver; }
#endif

  // Emit a write barrier.
  void MarkGCCard(vixl::Register object, vixl::Register value, bool value_can_be_null);

  void GenerateMemoryBarrier(MemBarrierKind kind);

  // Register allocation.

  void SetupBlockedRegisters() const OVERRIDE;

  size_t SaveCoreRegister(size_t stack_index, uint32_t reg_id) OVERRIDE;
  size_t RestoreCoreRegister(size_t stack_index, uint32_t reg_id) OVERRIDE;
  size_t SaveFloatingPointRegister(size_t stack_index, uint32_t reg_id) OVERRIDE;
  size_t RestoreFloatingPointRegister(size_t stack_index, uint32_t reg_id) OVERRIDE;

  // The number of registers that can be allocated. The register allocator may
  // decide to reserve and not use a few of them.
  // We do not consider registers sp, xzr, wzr. They are either not allocatable
  // (xzr, wzr), or make for poor allocatable registers (sp alignment
  // requirements, etc.). This also facilitates our task as all other registers
  // can easily be mapped via to or from their type and index or code.
  static const int kNumberOfAllocatableRegisters = vixl::kNumberOfRegisters - 1;
  static const int kNumberOfAllocatableFPRegisters = vixl::kNumberOfFPRegisters;
  static constexpr int kNumberOfAllocatableRegisterPairs = 0;

  void DumpCoreRegister(std::ostream& stream, int reg) const OVERRIDE;
  void DumpFloatingPointRegister(std::ostream& stream, int reg) const OVERRIDE;

  InstructionSet GetInstructionSet() const OVERRIDE {
    return InstructionSet::kArm64;
  }

  const Arm64InstructionSetFeatures& GetInstructionSetFeatures() const {
    return isa_features_;
  }

  void Initialize() OVERRIDE {
    block_labels_.resize(GetGraph()->GetBlocks().size());
  }

  JumpTableARM64* CreateJumpTable(HPackedSwitch* switch_instr) {
    jump_tables_.emplace_back(new (GetGraph()->GetArena()) JumpTableARM64(switch_instr));
    return jump_tables_.back().get();
  }

  void Finalize(CodeAllocator* allocator) OVERRIDE;

  // Code generation helpers.
  void MoveConstant(vixl::CPURegister destination, HConstant* constant);
  void MoveConstant(Location destination, int32_t value) OVERRIDE;
  void MoveLocation(Location dst, Location src, Primitive::Type dst_type) OVERRIDE;
  void AddLocationAsTemp(Location location, LocationSummary* locations) OVERRIDE;

  void Load(Primitive::Type type, vixl::CPURegister dst, const vixl::MemOperand& src);
  void Store(Primitive::Type type, vixl::CPURegister src, const vixl::MemOperand& dst);
  void LoadAcquire(HInstruction* instruction,
                   vixl::CPURegister dst,
                   const vixl::MemOperand& src,
                   bool needs_null_check);
  void StoreRelease(Primitive::Type type, vixl::CPURegister src, const vixl::MemOperand& dst);

  // Generate code to invoke a runtime entry point.
  void InvokeRuntime(QuickEntrypointEnum entrypoint,
                     HInstruction* instruction,
                     uint32_t dex_pc,
                     SlowPathCode* slow_path) OVERRIDE;

  void InvokeRuntime(int32_t offset,
                     HInstruction* instruction,
                     uint32_t dex_pc,
                     SlowPathCode* slow_path);

  ParallelMoveResolverARM64* GetMoveResolver() OVERRIDE { return &move_resolver_; }

  bool NeedsTwoRegisters(Primitive::Type type ATTRIBUTE_UNUSED) const OVERRIDE {
    return false;
  }

  // Check if the desired_string_load_kind is supported. If it is, return it,
  // otherwise return a fall-back kind that should be used instead.
  HLoadString::LoadKind GetSupportedLoadStringKind(
      HLoadString::LoadKind desired_string_load_kind) OVERRIDE;

  // Check if the desired_dispatch_info is supported. If it is, return it,
  // otherwise return a fall-back info that should be used instead.
  HInvokeStaticOrDirect::DispatchInfo GetSupportedInvokeStaticOrDirectDispatch(
      const HInvokeStaticOrDirect::DispatchInfo& desired_dispatch_info,
      MethodReference target_method) OVERRIDE;

  void GenerateStaticOrDirectCall(HInvokeStaticOrDirect* invoke, Location temp) OVERRIDE;
  void GenerateVirtualCall(HInvokeVirtual* invoke, Location temp) OVERRIDE;

  void MoveFromReturnRegister(Location trg ATTRIBUTE_UNUSED,
                              Primitive::Type type ATTRIBUTE_UNUSED) OVERRIDE {
    UNIMPLEMENTED(FATAL);
  }

  // Add a new PC-relative string patch for an instruction and return the label
  // to be bound before the instruction. The instruction will be either the
  // ADRP (pass `adrp_label = null`) or the ADD (pass `adrp_label` pointing
  // to the associated ADRP patch label).
  vixl::Label* NewPcRelativeStringPatch(const DexFile& dex_file,
                                        uint32_t string_index,
                                        vixl::Label* adrp_label = nullptr);

  // Add a new PC-relative dex cache array patch for an instruction and return
  // the label to be bound before the instruction. The instruction will be
  // either the ADRP (pass `adrp_label = null`) or the LDR (pass `adrp_label`
  // pointing to the associated ADRP patch label).
  vixl::Label* NewPcRelativeDexCacheArrayPatch(const DexFile& dex_file,
                                               uint32_t element_offset,
                                               vixl::Label* adrp_label = nullptr);

  vixl::Literal<uint32_t>* DeduplicateBootImageStringLiteral(const DexFile& dex_file,
                                                             uint32_t string_index);
  vixl::Literal<uint32_t>* DeduplicateBootImageAddressLiteral(uint64_t address);
  vixl::Literal<uint64_t>* DeduplicateDexCacheAddressLiteral(uint64_t address);

  void EmitLinkerPatches(ArenaVector<LinkerPatch>* linker_patches) OVERRIDE;

  // Fast path implementation of ReadBarrier::Barrier for a heap
  // reference field load when Baker's read barriers are used.
  void GenerateFieldLoadWithBakerReadBarrier(HInstruction* instruction,
                                             Location ref,
                                             vixl::Register obj,
                                             uint32_t offset,
                                             vixl::Register temp,
                                             bool needs_null_check,
                                             bool use_load_acquire);
  // Fast path implementation of ReadBarrier::Barrier for a heap
  // reference array load when Baker's read barriers are used.
  void GenerateArrayLoadWithBakerReadBarrier(HInstruction* instruction,
                                             Location ref,
                                             vixl::Register obj,
                                             uint32_t data_offset,
                                             Location index,
                                             vixl::Register temp,
                                             bool needs_null_check);

  // Generate a read barrier for a heap reference within `instruction`
  // using a slow path.
  //
  // A read barrier for an object reference read from the heap is
  // implemented as a call to the artReadBarrierSlow runtime entry
  // point, which is passed the values in locations `ref`, `obj`, and
  // `offset`:
  //
  //   mirror::Object* artReadBarrierSlow(mirror::Object* ref,
  //                                      mirror::Object* obj,
  //                                      uint32_t offset);
  //
  // The `out` location contains the value returned by
  // artReadBarrierSlow.
  //
  // When `index` is provided (i.e. for array accesses), the offset
  // value passed to artReadBarrierSlow is adjusted to take `index`
  // into account.
  void GenerateReadBarrierSlow(HInstruction* instruction,
                               Location out,
                               Location ref,
                               Location obj,
                               uint32_t offset,
                               Location index = Location::NoLocation());

  // If read barriers are enabled, generate a read barrier for a heap
  // reference using a slow path. If heap poisoning is enabled, also
  // unpoison the reference in `out`.
  void MaybeGenerateReadBarrierSlow(HInstruction* instruction,
                                    Location out,
                                    Location ref,
                                    Location obj,
                                    uint32_t offset,
                                    Location index = Location::NoLocation());

  // Generate a read barrier for a GC root within `instruction` using
  // a slow path.
  //
  // A read barrier for an object reference GC root is implemented as
  // a call to the artReadBarrierForRootSlow runtime entry point,
  // which is passed the value in location `root`:
  //
  //   mirror::Object* artReadBarrierForRootSlow(GcRoot<mirror::Object>* root);
  //
  // The `out` location contains the value returned by
  // artReadBarrierForRootSlow.
  void GenerateReadBarrierForRootSlow(HInstruction* instruction, Location out, Location root);

  void GenerateNop();

  void GenerateImplicitNullCheck(HNullCheck* instruction);
  void GenerateExplicitNullCheck(HNullCheck* instruction);

 private:
  // Factored implementation of GenerateFieldLoadWithBakerReadBarrier
  // and GenerateArrayLoadWithBakerReadBarrier.
  void GenerateReferenceLoadWithBakerReadBarrier(HInstruction* instruction,
                                                 Location ref,
                                                 vixl::Register obj,
                                                 uint32_t offset,
                                                 Location index,
                                                 vixl::Register temp,
                                                 bool needs_null_check,
                                                 bool use_load_acquire);

  using Uint64ToLiteralMap = ArenaSafeMap<uint64_t, vixl::Literal<uint64_t>*>;
  using Uint32ToLiteralMap = ArenaSafeMap<uint32_t, vixl::Literal<uint32_t>*>;
  using MethodToLiteralMap = ArenaSafeMap<MethodReference,
                                          vixl::Literal<uint64_t>*,
                                          MethodReferenceComparator>;
  using BootStringToLiteralMap = ArenaSafeMap<StringReference,
                                              vixl::Literal<uint32_t>*,
                                              StringReferenceValueComparator>;

  vixl::Literal<uint32_t>* DeduplicateUint32Literal(uint32_t value, Uint32ToLiteralMap* map);
  vixl::Literal<uint64_t>* DeduplicateUint64Literal(uint64_t value);
  vixl::Literal<uint64_t>* DeduplicateMethodLiteral(MethodReference target_method,
                                                    MethodToLiteralMap* map);
  vixl::Literal<uint64_t>* DeduplicateMethodAddressLiteral(MethodReference target_method);
  vixl::Literal<uint64_t>* DeduplicateMethodCodeLiteral(MethodReference target_method);

  // The PcRelativePatchInfo is used for PC-relative addressing of dex cache arrays
  // and boot image strings. The only difference is the interpretation of the offset_or_index.
  struct PcRelativePatchInfo {
    PcRelativePatchInfo(const DexFile& dex_file, uint32_t off_or_idx)
        : target_dex_file(dex_file), offset_or_index(off_or_idx), label(), pc_insn_label() { }

    const DexFile& target_dex_file;
    // Either the dex cache array element offset or the string index.
    uint32_t offset_or_index;
    vixl::Label label;
    vixl::Label* pc_insn_label;
  };

  vixl::Label* NewPcRelativePatch(const DexFile& dex_file,
                                  uint32_t offset_or_index,
                                  vixl::Label* adrp_label,
                                  ArenaDeque<PcRelativePatchInfo>* patches);

  void EmitJumpTables();

  // Labels for each block that will be compiled.
  // We use a deque so that the `vixl::Label` objects do not move in memory.
  ArenaDeque<vixl::Label> block_labels_;  // Indexed by block id.
  vixl::Label frame_entry_label_;
  ArenaVector<std::unique_ptr<JumpTableARM64>> jump_tables_;

  LocationsBuilderARM64 location_builder_;
  InstructionCodeGeneratorARM64 instruction_visitor_;
  ParallelMoveResolverARM64 move_resolver_;
  Arm64Assembler assembler_;
#ifdef MTK_ART_COMMON
  Arm64AsmWrapper asm_wrapper_;
  CompilerDriver* compiler_driver_;
#endif
  const Arm64InstructionSetFeatures& isa_features_;

  // Deduplication map for 32-bit literals, used for non-patchable boot image addresses.
  Uint32ToLiteralMap uint32_literals_;
  // Deduplication map for 64-bit literals, used for non-patchable method address, method code
  // or string dex cache address.
  Uint64ToLiteralMap uint64_literals_;
  // Method patch info, map MethodReference to a literal for method address and method code.
  MethodToLiteralMap method_patches_;
  MethodToLiteralMap call_patches_;
  // Relative call patch info.
  // Using ArenaDeque<> which retains element addresses on push/emplace_back().
  ArenaDeque<MethodPatchInfo<vixl::Label>> relative_call_patches_;
  // PC-relative DexCache access info.
  ArenaDeque<PcRelativePatchInfo> pc_relative_dex_cache_patches_;
  // Deduplication map for boot string literals for kBootImageLinkTimeAddress.
  BootStringToLiteralMap boot_image_string_patches_;
  // PC-relative String patch info.
  ArenaDeque<PcRelativePatchInfo> pc_relative_string_patches_;
  // Deduplication map for patchable boot image addresses.
  Uint32ToLiteralMap boot_image_address_patches_;

  DISALLOW_COPY_AND_ASSIGN(CodeGeneratorARM64);
};

inline Arm64Assembler* ParallelMoveResolverARM64::GetAssembler() const {
  return codegen_->GetAssembler();
}
#ifdef MTK_ART_COMMON
inline Arm64AsmWrapper* InstructionCodeGeneratorARM64::GetArm64AsmWrapper() {
  return down_cast<CodeGeneratorARM64*>(codegen_)->GetArm64AsmWrapper();
}
#endif

}  // namespace arm64
}  // namespace art

#endif  // ART_COMPILER_OPTIMIZING_CODE_GENERATOR_ARM64_H_
