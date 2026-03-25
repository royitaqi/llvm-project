//===-- Mips.h -----------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLDB_SOURCE_PLUGINS_ABI_MIPS_ABIMIPS_H
#define LLDB_SOURCE_PLUGINS_ABI_MIPS_ABIMIPS_H

/// Utility class for MIPS ABI plugin initialization.
///
/// Provides static methods to initialize and terminate all MIPS ABI plugins.
class ABIMips {
public:
  static void Initialize();
  static void Terminate();
};
#endif
