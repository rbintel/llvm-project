//===- DXILResource.cpp - Representations of DXIL resources ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Analysis/DXILResource.h"
#include "llvm/ADT/APInt.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/InitializePasses.h"

#define DEBUG_TYPE "dxil-resource"

using namespace llvm;
using namespace dxil;

bool ResourceInfo::isUAV() const { return RC == ResourceClass::UAV; }

bool ResourceInfo::isCBuffer() const { return RC == ResourceClass::CBuffer; }

bool ResourceInfo::isSampler() const { return RC == ResourceClass::Sampler; }

bool ResourceInfo::isStruct() const {
  return Kind == ResourceKind::StructuredBuffer;
}

bool ResourceInfo::isTyped() const {
  switch (Kind) {
  case ResourceKind::Texture1D:
  case ResourceKind::Texture2D:
  case ResourceKind::Texture2DMS:
  case ResourceKind::Texture3D:
  case ResourceKind::TextureCube:
  case ResourceKind::Texture1DArray:
  case ResourceKind::Texture2DArray:
  case ResourceKind::Texture2DMSArray:
  case ResourceKind::TextureCubeArray:
  case ResourceKind::TypedBuffer:
    return true;
  case ResourceKind::RawBuffer:
  case ResourceKind::StructuredBuffer:
  case ResourceKind::FeedbackTexture2D:
  case ResourceKind::FeedbackTexture2DArray:
  case ResourceKind::CBuffer:
  case ResourceKind::Sampler:
  case ResourceKind::TBuffer:
  case ResourceKind::RTAccelerationStructure:
    return false;
  case ResourceKind::Invalid:
  case ResourceKind::NumEntries:
    llvm_unreachable("Invalid resource kind");
  }
  llvm_unreachable("Unhandled ResourceKind enum");
}

bool ResourceInfo::isFeedback() const {
  return Kind == ResourceKind::FeedbackTexture2D ||
         Kind == ResourceKind::FeedbackTexture2DArray;
}

bool ResourceInfo::isMultiSample() const {
  return Kind == ResourceKind::Texture2DMS ||
         Kind == ResourceKind::Texture2DMSArray;
}

ResourceInfo ResourceInfo::SRV(Value *Symbol, StringRef Name,
                               ElementType ElementTy, uint32_t ElementCount,
                               ResourceKind Kind) {
  ResourceInfo RI(ResourceClass::SRV, Kind, Symbol, Name);
  assert(RI.isTyped() && !(RI.isStruct() || RI.isMultiSample()) &&
         "Invalid ResourceKind for SRV constructor.");
  RI.setTyped(ElementTy, ElementCount);
  return RI;
}

ResourceInfo ResourceInfo::RawBuffer(Value *Symbol, StringRef Name) {
  ResourceInfo RI(ResourceClass::SRV, ResourceKind::RawBuffer, Symbol, Name);
  return RI;
}

ResourceInfo ResourceInfo::StructuredBuffer(Value *Symbol, StringRef Name,
                                            uint32_t Stride,
                                            MaybeAlign Alignment) {
  ResourceInfo RI(ResourceClass::SRV, ResourceKind::StructuredBuffer, Symbol,
                  Name);
  RI.setStruct(Stride, Alignment);
  return RI;
}

ResourceInfo ResourceInfo::Texture2DMS(Value *Symbol, StringRef Name,
                                       ElementType ElementTy,
                                       uint32_t ElementCount,
                                       uint32_t SampleCount) {
  ResourceInfo RI(ResourceClass::SRV, ResourceKind::Texture2DMS, Symbol, Name);
  RI.setTyped(ElementTy, ElementCount);
  RI.setMultiSample(SampleCount);
  return RI;
}

ResourceInfo ResourceInfo::Texture2DMSArray(Value *Symbol, StringRef Name,
                                            ElementType ElementTy,
                                            uint32_t ElementCount,
                                            uint32_t SampleCount) {
  ResourceInfo RI(ResourceClass::SRV, ResourceKind::Texture2DMSArray, Symbol,
                  Name);
  RI.setTyped(ElementTy, ElementCount);
  RI.setMultiSample(SampleCount);
  return RI;
}

ResourceInfo ResourceInfo::UAV(Value *Symbol, StringRef Name,
                               ElementType ElementTy, uint32_t ElementCount,
                               bool GloballyCoherent, bool IsROV,
                               ResourceKind Kind) {
  ResourceInfo RI(ResourceClass::UAV, Kind, Symbol, Name);
  assert(RI.isTyped() && !(RI.isStruct() || RI.isMultiSample()) &&
         "Invalid ResourceKind for UAV constructor.");
  RI.setTyped(ElementTy, ElementCount);
  RI.setUAV(GloballyCoherent, /*HasCounter=*/false, IsROV);
  return RI;
}

ResourceInfo ResourceInfo::RWRawBuffer(Value *Symbol, StringRef Name,
                                       bool GloballyCoherent, bool IsROV) {
  ResourceInfo RI(ResourceClass::UAV, ResourceKind::RawBuffer, Symbol, Name);
  RI.setUAV(GloballyCoherent, /*HasCounter=*/false, IsROV);
  return RI;
}

ResourceInfo ResourceInfo::RWStructuredBuffer(Value *Symbol, StringRef Name,
                                              uint32_t Stride,
                                              MaybeAlign Alignment,
                                              bool GloballyCoherent, bool IsROV,
                                              bool HasCounter) {
  ResourceInfo RI(ResourceClass::UAV, ResourceKind::StructuredBuffer, Symbol,
                  Name);
  RI.setStruct(Stride, Alignment);
  RI.setUAV(GloballyCoherent, HasCounter, IsROV);
  return RI;
}

ResourceInfo ResourceInfo::RWTexture2DMS(Value *Symbol, StringRef Name,
                                         ElementType ElementTy,
                                         uint32_t ElementCount,
                                         uint32_t SampleCount,
                                         bool GloballyCoherent) {
  ResourceInfo RI(ResourceClass::UAV, ResourceKind::Texture2DMS, Symbol, Name);
  RI.setTyped(ElementTy, ElementCount);
  RI.setUAV(GloballyCoherent, /*HasCounter=*/false, /*IsROV=*/false);
  RI.setMultiSample(SampleCount);
  return RI;
}

ResourceInfo ResourceInfo::RWTexture2DMSArray(Value *Symbol, StringRef Name,
                                              ElementType ElementTy,
                                              uint32_t ElementCount,
                                              uint32_t SampleCount,
                                              bool GloballyCoherent) {
  ResourceInfo RI(ResourceClass::UAV, ResourceKind::Texture2DMSArray, Symbol,
                  Name);
  RI.setTyped(ElementTy, ElementCount);
  RI.setUAV(GloballyCoherent, /*HasCounter=*/false, /*IsROV=*/false);
  RI.setMultiSample(SampleCount);
  return RI;
}

ResourceInfo ResourceInfo::FeedbackTexture2D(Value *Symbol, StringRef Name,
                                             SamplerFeedbackType FeedbackTy) {
  ResourceInfo RI(ResourceClass::UAV, ResourceKind::FeedbackTexture2D, Symbol,
                  Name);
  RI.setUAV(/*GloballyCoherent=*/false, /*HasCounter=*/false, /*IsROV=*/false);
  RI.setFeedback(FeedbackTy);
  return RI;
}

ResourceInfo
ResourceInfo::FeedbackTexture2DArray(Value *Symbol, StringRef Name,
                                     SamplerFeedbackType FeedbackTy) {
  ResourceInfo RI(ResourceClass::UAV, ResourceKind::FeedbackTexture2DArray,
                  Symbol, Name);
  RI.setUAV(/*GloballyCoherent=*/false, /*HasCounter=*/false, /*IsROV=*/false);
  RI.setFeedback(FeedbackTy);
  return RI;
}

ResourceInfo ResourceInfo::CBuffer(Value *Symbol, StringRef Name,
                                   uint32_t Size) {
  ResourceInfo RI(ResourceClass::CBuffer, ResourceKind::CBuffer, Symbol, Name);
  RI.setCBuffer(Size);
  return RI;
}

ResourceInfo ResourceInfo::Sampler(Value *Symbol, StringRef Name,
                                   SamplerType SamplerTy) {
  ResourceInfo RI(ResourceClass::Sampler, ResourceKind::Sampler, Symbol, Name);
  RI.setSampler(SamplerTy);
  return RI;
}

bool ResourceInfo::operator==(const ResourceInfo &RHS) const {
  if (std::tie(Symbol, Name, Binding, RC, Kind) !=
      std::tie(RHS.Symbol, RHS.Name, RHS.Binding, RHS.RC, RHS.Kind))
    return false;
  if (isCBuffer())
    return CBufferSize == RHS.CBufferSize;
  if (isSampler())
    return SamplerTy == RHS.SamplerTy;
  if (isUAV() && UAVFlags != RHS.UAVFlags)
    return false;

  if (isStruct())
    return Struct == RHS.Struct;
  if (isFeedback())
    return Feedback == RHS.Feedback;
  if (isTyped() && Typed != RHS.Typed)
    return false;

  if (isMultiSample())
    return MultiSample == RHS.MultiSample;

  assert((Kind == ResourceKind::RawBuffer) && "Unhandled resource kind");
  return true;
}

MDTuple *ResourceInfo::getAsMetadata(LLVMContext &Ctx) const {
  SmallVector<Metadata *, 11> MDVals;

  Type *I32Ty = Type::getInt32Ty(Ctx);
  Type *I1Ty = Type::getInt1Ty(Ctx);
  auto getIntMD = [&I32Ty](uint32_t V) {
    return ConstantAsMetadata::get(
        Constant::getIntegerValue(I32Ty, APInt(32, V)));
  };
  auto getBoolMD = [&I1Ty](uint32_t V) {
    return ConstantAsMetadata::get(
        Constant::getIntegerValue(I1Ty, APInt(1, V)));
  };

  MDVals.push_back(getIntMD(Binding.UniqueID));
  MDVals.push_back(ValueAsMetadata::get(Symbol));
  MDVals.push_back(MDString::get(Ctx, Name));
  MDVals.push_back(getIntMD(Binding.Space));
  MDVals.push_back(getIntMD(Binding.LowerBound));
  MDVals.push_back(getIntMD(Binding.Size));

  if (isCBuffer()) {
    MDVals.push_back(getIntMD(CBufferSize));
    MDVals.push_back(nullptr);
  } else if (isSampler()) {
    MDVals.push_back(getIntMD(llvm::to_underlying(SamplerTy)));
    MDVals.push_back(nullptr);
  } else {
    MDVals.push_back(getIntMD(llvm::to_underlying(Kind)));

    if (isUAV()) {
      MDVals.push_back(getBoolMD(UAVFlags.GloballyCoherent));
      MDVals.push_back(getBoolMD(UAVFlags.HasCounter));
      MDVals.push_back(getBoolMD(UAVFlags.IsROV));
    } else {
      // All SRVs include sample count in the metadata, but it's only meaningful
      // for multi-sampled textured. Also, UAVs can be multisampled in SM6.7+,
      // but this just isn't reflected in the metadata at all.
      uint32_t SampleCount = isMultiSample() ? MultiSample.Count : 0;
      MDVals.push_back(getIntMD(SampleCount));
    }

    // Further properties are attached to a metadata list of tag-value pairs.
    SmallVector<Metadata *> Tags;
    if (isStruct()) {
      Tags.push_back(
          getIntMD(llvm::to_underlying(ExtPropTags::StructuredBufferStride)));
      Tags.push_back(getIntMD(Struct.Stride));
    } else if (isTyped()) {
      Tags.push_back(getIntMD(llvm::to_underlying(ExtPropTags::ElementType)));
      Tags.push_back(getIntMD(llvm::to_underlying(Typed.ElementTy)));
    } else if (isFeedback()) {
      Tags.push_back(
          getIntMD(llvm::to_underlying(ExtPropTags::SamplerFeedbackKind)));
      Tags.push_back(getIntMD(llvm::to_underlying(Feedback.Type)));
    }
    MDVals.push_back(Tags.empty() ? nullptr : MDNode::get(Ctx, Tags));
  }

  return MDNode::get(Ctx, MDVals);
}

std::pair<uint32_t, uint32_t> ResourceInfo::getAnnotateProps() const {
  uint32_t ResourceKind = llvm::to_underlying(Kind);
  uint32_t AlignLog2 = isStruct() ? Struct.AlignLog2 : 0;
  bool IsUAV = isUAV();
  bool IsROV = IsUAV && UAVFlags.IsROV;
  bool IsGloballyCoherent = IsUAV && UAVFlags.GloballyCoherent;
  uint8_t SamplerCmpOrHasCounter = 0;
  if (IsUAV)
    SamplerCmpOrHasCounter = UAVFlags.HasCounter;
  else if (isSampler())
    SamplerCmpOrHasCounter = SamplerTy == SamplerType::Comparison;

  // TODO: Document this format. Currently the only reference is the
  // implementation of dxc's DxilResourceProperties struct.
  uint32_t Word0 = 0;
  Word0 |= ResourceKind & 0xFF;
  Word0 |= (AlignLog2 & 0xF) << 8;
  Word0 |= (IsUAV & 1) << 12;
  Word0 |= (IsROV & 1) << 13;
  Word0 |= (IsGloballyCoherent & 1) << 14;
  Word0 |= (SamplerCmpOrHasCounter & 1) << 15;

  uint32_t Word1 = 0;
  if (isStruct())
    Word1 = Struct.Stride;
  else if (isCBuffer())
    Word1 = CBufferSize;
  else if (isFeedback())
    Word1 = llvm::to_underlying(Feedback.Type);
  else if (isTyped()) {
    uint32_t CompType = llvm::to_underlying(Typed.ElementTy);
    uint32_t CompCount = Typed.ElementCount;
    uint32_t SampleCount = isMultiSample() ? MultiSample.Count : 0;

    Word1 |= (CompType & 0xFF) << 0;
    Word1 |= (CompCount & 0xFF) << 8;
    Word1 |= (SampleCount & 0xFF) << 16;
  }

  return {Word0, Word1};
}

//===----------------------------------------------------------------------===//
// DXILResourceAnalysis and DXILResourcePrinterPass

// Provide an explicit template instantiation for the static ID.
AnalysisKey DXILResourceAnalysis::Key;

DXILResourceMap DXILResourceAnalysis::run(Module &M,
                                          ModuleAnalysisManager &AM) {
  DXILResourceMap Data;
  return Data;
}

PreservedAnalyses DXILResourcePrinterPass::run(Module &M,
                                               ModuleAnalysisManager &AM) {
  DXILResourceMap &Data =
      AM.getResult<DXILResourceAnalysis>(M);

  for (const auto &[Handle, Info] : Data) {
    OS << "Binding for ";
    Handle->print(OS);
    OS << "\n";
    // TODO: Info.print(OS);
    OS << "\n";
  }

  return PreservedAnalyses::all();
}

//===----------------------------------------------------------------------===//
// DXILResourceWrapperPass

DXILResourceWrapperPass::DXILResourceWrapperPass() : ModulePass(ID) {
  initializeDXILResourceWrapperPassPass(*PassRegistry::getPassRegistry());
}

DXILResourceWrapperPass::~DXILResourceWrapperPass() = default;

void DXILResourceWrapperPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

bool DXILResourceWrapperPass::runOnModule(Module &M) {
  ResourceMap.reset(new DXILResourceMap());
  return false;
}

void DXILResourceWrapperPass::releaseMemory() { ResourceMap.reset(); }

void DXILResourceWrapperPass::print(raw_ostream &OS, const Module *) const {
  if (!ResourceMap) {
    OS << "No resource map has been built!\n";
    return;
  }
  for (const auto &[Handle, Info] : *ResourceMap) {
    OS << "Binding for ";
    Handle->print(OS);
    OS << "\n";
    // TODO: Info.print(OS);
    OS << "\n";
  }
}

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
LLVM_DUMP_METHOD
void DXILResourceWrapperPass::dump() const { print(dbgs(), nullptr); }
#endif

INITIALIZE_PASS(DXILResourceWrapperPass, DEBUG_TYPE, "DXIL Resource analysis",
                false, true)
char DXILResourceWrapperPass::ID = 0;

ModulePass *llvm::createDXILResourceWrapperPassPass() {
  return new DXILResourceWrapperPass();
}
