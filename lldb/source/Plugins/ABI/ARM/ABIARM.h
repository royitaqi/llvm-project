//===-- ARM.h -------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLDB_SOURCE_PLUGINS_ABI_ARM_ABIARM_H
#define LLDB_SOURCE_PLUGINS_ABI_ARM_ABIARM_H

/// Utility class for ARM ABI plugin initialization.
///
/// Provides static methods to initialize and terminate all ARM ABI plugins.
class ABIARM {
public:
  static void Initialize();
  static void Terminate();
};
#endif
