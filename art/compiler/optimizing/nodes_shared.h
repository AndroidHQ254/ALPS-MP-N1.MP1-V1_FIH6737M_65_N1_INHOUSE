/*
 * Copyright (C) 2015 The Android Open Source Project
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

#ifndef ART_COMPILER_OPTIMIZING_NODES_SHARED_H_
#define ART_COMPILER_OPTIMIZING_NODES_SHARED_H_

namespace art {

class HMultiplyAccumulate : public HExpression<3> {
 public:
  HMultiplyAccumulate(Primitive::Type type,
                      InstructionKind op,
                      HInstruction* accumulator,
                      HInstruction* mul_left,
                      HInstruction* mul_right,
                      uint32_t dex_pc = kNoDexPc)
      : HExpression(type, SideEffects::None(), dex_pc), op_kind_(op) {
    SetRawInputAt(kInputAccumulatorIndex, accumulator);
    SetRawInputAt(kInputMulLeftIndex, mul_left);
    SetRawInputAt(kInputMulRightIndex, mul_right);
  }

  static constexpr int kInputAccumulatorIndex = 0;
  static constexpr int kInputMulLeftIndex = 1;
  static constexpr int kInputMulRightIndex = 2;

  bool CanBeMoved() const OVERRIDE { return true; }
  bool InstructionDataEquals(HInstruction* other) const OVERRIDE {
    return op_kind_ == other->AsMultiplyAccumulate()->op_kind_;
  }

  InstructionKind GetOpKind() const { return op_kind_; }

  DECLARE_INSTRUCTION(MultiplyAccumulate);

 private:
  // Indicates if this is a MADD or MSUB.
  const InstructionKind op_kind_;

  DISALLOW_COPY_AND_ASSIGN(HMultiplyAccumulate);
};

class HBitwiseNegatedRight : public HBinaryOperation {
 public:
  HBitwiseNegatedRight(Primitive::Type result_type,
                            InstructionKind op,
                            HInstruction* left,
                            HInstruction* right,
                            uint32_t dex_pc = kNoDexPc)
    : HBinaryOperation(result_type, left, right, SideEffects::None(), dex_pc),
      op_kind_(op) {
    DCHECK(op == HInstruction::kAnd || op == HInstruction::kOr || op == HInstruction::kXor) << op;
  }

  template <typename T, typename U>
  auto Compute(T x, U y) const -> decltype(x & ~y) {
    static_assert(std::is_same<decltype(x & ~y), decltype(x | ~y)>::value &&
                  std::is_same<decltype(x & ~y), decltype(x ^ ~y)>::value,
                  "Inconsistent negated bitwise types");
    switch (op_kind_) {
      case HInstruction::kAnd:
        return x & ~y;
      case HInstruction::kOr:
        return x | ~y;
      case HInstruction::kXor:
        return x ^ ~y;
      default:
        LOG(FATAL) << "Unreachable";
        UNREACHABLE();
    }
  }

  HConstant* Evaluate(HIntConstant* x, HIntConstant* y) const OVERRIDE {
    return GetBlock()->GetGraph()->GetIntConstant(
        Compute(x->GetValue(), y->GetValue()), GetDexPc());
  }
  HConstant* Evaluate(HLongConstant* x, HLongConstant* y) const OVERRIDE {
    return GetBlock()->GetGraph()->GetLongConstant(
        Compute(x->GetValue(), y->GetValue()), GetDexPc());
  }
  HConstant* Evaluate(HFloatConstant* x ATTRIBUTE_UNUSED,
                      HFloatConstant* y ATTRIBUTE_UNUSED) const OVERRIDE {
    LOG(FATAL) << DebugName() << " is not defined for float values";
    UNREACHABLE();
  }
  HConstant* Evaluate(HDoubleConstant* x ATTRIBUTE_UNUSED,
                      HDoubleConstant* y ATTRIBUTE_UNUSED) const OVERRIDE {
    LOG(FATAL) << DebugName() << " is not defined for double values";
    UNREACHABLE();
  }

  InstructionKind GetOpKind() const { return op_kind_; }

  DECLARE_INSTRUCTION(BitwiseNegatedRight);

 private:
  // Specifies the bitwise operation, which will be then negated.
  const InstructionKind op_kind_;

  DISALLOW_COPY_AND_ASSIGN(HBitwiseNegatedRight);
};

#ifdef MTK_ART_COMMON
class HMla : public HTernaryOperation {
 public:
  HMla(Primitive::Type result_type, HInstruction* first, HInstruction* second, HInstruction* third)
      : HTernaryOperation(result_type, first, second, third) {}

  bool IsCommutative() const OVERRIDE { return true; }

  template <typename T> T Compute(T x, T y, T z) const {
    return x * y + z;
  }

  HConstant* Evaluate(HIntConstant* x, HIntConstant* y, HIntConstant* z) const OVERRIDE {
    return GetBlock()->GetGraph()->GetIntConstant(
        Compute(x->GetValue(), y->GetValue(), z->GetValue()), GetDexPc());
  }
  HConstant* Evaluate(HLongConstant* x, HLongConstant* y, HLongConstant* z) const OVERRIDE {
    return GetBlock()->GetGraph()->GetIntConstant(
        Compute(x->GetValue(), y->GetValue(), z->GetValue()), GetDexPc());
  }

  DECLARE_INSTRUCTION(Mla);

 private:
  DISALLOW_COPY_AND_ASSIGN(HMla);
};

class HPow : public HBinaryOperation {
 public:
  HPow(Primitive::Type result_type,
       HInstruction* left,
       HInstruction* right,
       uint32_t dex_pc = kNoDexPc)
      : HBinaryOperation(result_type, left, right, SideEffectsForArchRuntimeCalls(), dex_pc) {}

  template <typename T> T Compute(T x, T y) const {
    T result = 1;
    while (y) {
        if (y & 1)
            result *= x;
        y >>= 1;
        x *= x;
    }
    return result;
  }

  HConstant* Evaluate(HIntConstant* x, HIntConstant* y) const OVERRIDE {
    return GetBlock()->GetGraph()->GetIntConstant(
        Compute(x->GetValue(), y->GetValue()), GetDexPc());
  }
  HConstant* Evaluate(HLongConstant* x, HLongConstant* y) const OVERRIDE {
    return GetBlock()->GetGraph()->GetIntConstant(
        Compute(x->GetValue(), y->GetValue()), GetDexPc());
  }
  HConstant* Evaluate(HFloatConstant* x ATTRIBUTE_UNUSED,
                      HFloatConstant* y ATTRIBUTE_UNUSED) const OVERRIDE {
    LOG(FATAL) << DebugName() << " is not defined for the (float, float) case.";
    UNREACHABLE();
  }
  HConstant* Evaluate(HDoubleConstant* x ATTRIBUTE_UNUSED,
                      HDoubleConstant* y ATTRIBUTE_UNUSED) const OVERRIDE {
    LOG(FATAL) << DebugName() << " is not defined for the (double, double) case.";
    UNREACHABLE();
  }

  static SideEffects SideEffectsForArchRuntimeCalls() {
    // The generated code can use a runtime call.
    return SideEffects::CanTriggerGC();
  }

  DECLARE_INSTRUCTION(Pow);

 private:
  DISALLOW_COPY_AND_ASSIGN(HPow);
};

class HZeroBranch : public HTemplateInstruction<1> {
 public:
  explicit HZeroBranch(HInstruction* value, uint32_t dex_pc)
      : HTemplateInstruction(SideEffects::None(), dex_pc) {
    SetRawInputAt(0, value);
  }

  bool CanBeMoved() const OVERRIDE { return true; }
  bool InstructionDataEquals(HInstruction* other) const OVERRIDE {
    UNUSED(other);
    return true;
  }

  HBasicBlock* IfTrueSuccessor() const {
    return GetBlock()->GetSuccessors()[0];
  }

  HBasicBlock* IfFalseSuccessor() const {
    return GetBlock()->GetSuccessors()[1];
  }

  DECLARE_INSTRUCTION(ZeroBranch);

 private:
  DISALLOW_COPY_AND_ASSIGN(HZeroBranch);
};

// Return a vector value that contains @value broadcasted to @num_elts elements.
class HVectorSplat : public HExpression<1> {
 public:
  explicit HVectorSplat(Primitive::Type type, HInstruction* value)
    : HExpression(type, SideEffects::None(), kNoDexPc) {
    SetRawInputAt(0, value);
  }

  DECLARE_INSTRUCTION(VectorSplat);

 private:
  DISALLOW_COPY_AND_ASSIGN(HVectorSplat);
};

// Instruction does a horizontal addition of the packed elements and then adds it to VR.
class HAddReduce : public HExpression<1> {
 public:
  explicit HAddReduce(Primitive::Type type, HInstruction* source)
    : HExpression(type, SideEffects::None(), kNoDexPc) {
    SetRawInputAt(0, source);
  }

  DECLARE_INSTRUCTION(AddReduce);

 private:
  DISALLOW_COPY_AND_ASSIGN(HAddReduce);
};

class HGetElementPtr : public HExpression<2> {
 public:
  explicit HGetElementPtr(HInstruction* base, HInstruction* offset, Primitive::Type type)
    : HExpression(Primitive::kPrimNot, SideEffects::DependsOnGC(), kNoDexPc),
      component_type_(type) {
    SetRawInputAt(0, base);
    SetRawInputAt(1, offset);
  }

  Primitive::Type GetComponentType() {
    return component_type_;
  }

  DECLARE_INSTRUCTION(GetElementPtr);

 private:
  const Primitive::Type component_type_;
  DISALLOW_COPY_AND_ASSIGN(HGetElementPtr);
};

// Move constant data to a vector register.
class HConstantVector : public HExpression<0> {
 public:
  explicit HConstantVector(Primitive::Type type, std::vector<int> indices)
    : HExpression(type, SideEffects::None(), kNoDexPc), indices_(indices) {
  }

  std::vector<int>* GetIndices() { return &indices_; }

  DECLARE_INSTRUCTION(ConstantVector);

 private:
  std::vector<int> indices_;
  DISALLOW_COPY_AND_ASSIGN(HConstantVector);
};
#endif

}  // namespace art

#endif  // ART_COMPILER_OPTIMIZING_NODES_SHARED_H_
