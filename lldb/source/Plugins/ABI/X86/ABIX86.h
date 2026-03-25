//===-- ABIX86.h ------------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLDB_SOURCE_PLUGINS_ABI_X86_ABIX86_H
#define LLDB_SOURCE_PLUGINS_ABI_X86_ABIX86_H

#include "lldb/Target/ABI.h"
#include "lldb/lldb-private.h"

/// Base ABI plugin for x86 architectures.
///
/// Provides common functionality for x86 calling conventions including
/// register augmentation and shared utilities for both 32-bit and 64-bit variants.
class ABIX86 : public lldb_private::MCBasedABI {
public:
  static void Initialize();
  static void Terminate();

protected:
  void AugmentRegisterInfo(
      std::vector<lldb_private::DynamicRegisterInfo::Register> &regs) override;

private:
  using lldb_private::MCBasedABI::MCBasedABI;
};

#endif
