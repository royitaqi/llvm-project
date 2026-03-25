//===-- ABIX86_i386.h -------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLDB_SOURCE_PLUGINS_ABI_X86_ABIX86_I386_H
#define LLDB_SOURCE_PLUGINS_ABI_X86_ABIX86_I386_H

#include "Plugins/ABI/X86/ABIX86.h"

/// Base ABI plugin for i386 (32-bit x86) architectures.
///
/// Provides common functionality for 32-bit x86 calling conventions,
/// serving as a base for platform-specific i386 ABI implementations.
class ABIX86_i386 : public ABIX86 {
public:
  uint32_t GetGenericNum(llvm::StringRef name) override;

private:
  using ABIX86::ABIX86;
};

#endif
