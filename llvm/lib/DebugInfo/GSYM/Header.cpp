//===- Header.cpp -----------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/DebugInfo/GSYM/Header.h"
#include "llvm/DebugInfo/GSYM/FileWriter.h"
#include "llvm/Support/DataExtractor.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"

#define HEX8(v) llvm::format_hex(v, 4)
#define HEX16(v) llvm::format_hex(v, 6)
#define HEX32(v) llvm::format_hex(v, 10)
#define HEX64(v) llvm::format_hex(v, 18)

using namespace llvm;
using namespace gsym;

raw_ostream &llvm::gsym::operator<<(raw_ostream &OS, const Header &H) {
  OS << "Header:\n";
  OS << "  Magic              = " << HEX32(H.Magic) << "\n";
  OS << "  Version            = " << HEX16(H.Version) << '\n';
  OS << "  AddrOffSize        = " << HEX8(H.AddrOffSize) << '\n';
  OS << "  UUIDSize           = " << HEX8(H.UUIDSize) << '\n';
  OS << "  BaseAddress        = " << HEX64(H.BaseAddress) << '\n';
  OS << "  StrtabOffset       = " << HEX64(H.StrtabOffset) << '\n';
  OS << "  StrtabSize         = " << HEX64(H.StrtabSize) << '\n';
  OS << "  NumAddresses       = " << HEX32(H.NumAddresses) << '\n';
  OS << "  FuncInfoOffsetSize = " << HEX8(H.FuncInfoOffsetSize) << '\n';
  OS << "  StringOffsetSize   = " << HEX8(H.StringOffsetSize) << '\n';
  OS << "  UUID               = ";
  for (uint8_t I = 0; I < H.UUIDSize; ++I)
    OS << format_hex_no_prefix(H.UUID[I], 2);
  OS << '\n';
  return OS;
}

/// Check the header and detect any errors.
llvm::Error Header::checkForError() const {
  if (Magic != GSYM_MAGIC)
    return createStringError(std::errc::invalid_argument,
                             "invalid GSYM magic 0x%8.8x", Magic);
  if (Version != GSYM_VERSION_1 && Version != GSYM_VERSION_2)
    return createStringError(std::errc::invalid_argument,
                             "unsupported GSYM version %u", Version);
  switch (AddrOffSize) {
    case 1: break;
    case 2: break;
    case 4: break;
    case 8: break;
    default:
        return createStringError(std::errc::invalid_argument,
                                 "invalid address offset size %u",
                                 AddrOffSize);
  }
  if (UUIDSize > GSYM_MAX_UUID_SIZE)
    return createStringError(std::errc::invalid_argument,
                             "invalid UUID size %u", UUIDSize);
  if (FuncInfoOffsetSize < 1 || FuncInfoOffsetSize > 8)
    return createStringError(std::errc::invalid_argument,
                             "invalid function info offset size %u",
                             FuncInfoOffsetSize);
  if (StringOffsetSize < 1 || StringOffsetSize > 8)
    return createStringError(std::errc::invalid_argument,
                             "invalid string offset size %u",
                             StringOffsetSize);
  return Error::success();
}

llvm::Expected<Header> Header::decode(DataExtractor &Data) {
  uint64_t Offset = 0;
  Header H;
  memset(&H, 0, sizeof(H));
  // Read the common fields first (Magic, Version, AddrOffSize, UUIDSize,
  // BaseAddress) which are the same in v1 and v2.
  if (!Data.isValidOffsetForDataOfSize(Offset, 16))
    return createStringError(std::errc::invalid_argument,
                             "not enough data for a gsym::Header");
  H.Magic = Data.getU32(&Offset);
  H.Version = Data.getU16(&Offset);
  H.AddrOffSize = Data.getU8(&Offset);
  H.UUIDSize = Data.getU8(&Offset);
  H.BaseAddress = Data.getU64(&Offset);

  if (H.Version == GSYM_VERSION_1) {
    // V1 layout after BaseAddress:
    //   NumAddresses(u32), StrtabOffset(u32), StrtabSize(u32), UUID[20].
    //   Total=48.
    if (!Data.isValidOffsetForDataOfSize(Offset, 12 + GSYM_MAX_UUID_SIZE))
      return createStringError(std::errc::invalid_argument,
                               "not enough data for a v1 gsym::Header");
    H.NumAddresses = Data.getU32(&Offset);
    H.FuncInfoOffsetSize = 4;
    H.StringOffsetSize = 4;
    H.StrtabOffset = Data.getU32(&Offset);
    H.StrtabSize = Data.getU32(&Offset);
    Data.getU8(&Offset, H.UUID, GSYM_MAX_UUID_SIZE);
  } else {
    // V2 layout after BaseAddress:
    //   StrtabOffset(u64), StrtabSize(u64), NumAddresses(u32),
    //   FuncInfoOffsetSize(u8), StringOffsetSize(u8), UUID[20], Pad[6].
    //   Total=64.
    if (!Data.isValidOffsetForDataOfSize(Offset,
            48)) // 64 - 16 common bytes
      return createStringError(std::errc::invalid_argument,
                               "not enough data for a v2 gsym::Header");
    H.StrtabOffset = Data.getU64(&Offset);
    H.StrtabSize = Data.getU64(&Offset);
    H.NumAddresses = Data.getU32(&Offset);
    H.FuncInfoOffsetSize = Data.getU8(&Offset);
    H.StringOffsetSize = Data.getU8(&Offset);
    Data.getU8(&Offset, H.UUID, GSYM_MAX_UUID_SIZE);
    Offset += 6; // Skip reserved padding bytes.
  }

  // Validate the header before using UUIDSize to avoid buffer overflow.
  if (llvm::Error Err = H.checkForError())
    return std::move(Err);
  memset(H.UUID + H.UUIDSize, 0, GSYM_MAX_UUID_SIZE - H.UUIDSize);
  return H;
}

llvm::Error Header::encode(FileWriter &O) const {
  // Users must verify the Header is valid prior to calling this funtion.
  if (llvm::Error Err = checkForError())
    return Err;
  O.writeU32(Magic);
  O.writeU16(Version);
  O.writeU8(AddrOffSize);
  O.writeU8(UUIDSize);
  O.writeU64(BaseAddress);
  if (Version == GSYM_VERSION_1) {
    O.writeU32(NumAddresses);
    O.writeU32(static_cast<uint32_t>(StrtabOffset));
    O.writeU32(static_cast<uint32_t>(StrtabSize));
  } else {
    O.writeU64(StrtabOffset);
    O.writeU64(StrtabSize);
    O.writeU32(NumAddresses);
    O.writeU8(FuncInfoOffsetSize);
    O.writeU8(StringOffsetSize);
  }
  O.writeData(llvm::ArrayRef<uint8_t>(UUID));
  if (Version != GSYM_VERSION_1) {
    // Write reserved padding bytes for V2.
    const uint8_t Zeros[6] = {0};
    O.writeData(llvm::ArrayRef<uint8_t>(Zeros));
  }
  return Error::success();
}

llvm::Error HeaderV1::checkForError() const {
  if (Magic != GSYM_MAGIC)
    return createStringError(std::errc::invalid_argument,
                             "invalid GSYM magic 0x%8.8x", Magic);
  if (Version != GSYM_VERSION_1)
    return createStringError(std::errc::invalid_argument,
                             "unsupported GSYM version %u", Version);
  switch (AddrOffSize) {
    case 1: break;
    case 2: break;
    case 4: break;
    case 8: break;
    default:
        return createStringError(std::errc::invalid_argument,
                                 "invalid address offset size %u",
                                 AddrOffSize);
  }
  if (UUIDSize > GSYM_MAX_UUID_SIZE)
    return createStringError(std::errc::invalid_argument,
                             "invalid UUID size %u", UUIDSize);
  return Error::success();
}

llvm::Expected<HeaderV1> HeaderV1::decode(DataExtractor &Data) {
  uint64_t Offset = 0;
  if (!Data.isValidOffsetForDataOfSize(Offset, sizeof(HeaderV1)))
    return createStringError(std::errc::invalid_argument,
                             "not enough data for a v1 gsym::Header");
  HeaderV1 H;
  H.Magic = Data.getU32(&Offset);
  H.Version = Data.getU16(&Offset);
  H.AddrOffSize = Data.getU8(&Offset);
  H.UUIDSize = Data.getU8(&Offset);
  H.BaseAddress = Data.getU64(&Offset);
  H.NumAddresses = Data.getU32(&Offset);
  H.StrtabOffset = Data.getU32(&Offset);
  H.StrtabSize = Data.getU32(&Offset);
  Data.getU8(&Offset, H.UUID, GSYM_MAX_UUID_SIZE);
  if (llvm::Error Err = H.checkForError())
    return std::move(Err);
  memset(H.UUID + H.UUIDSize, 0, GSYM_MAX_UUID_SIZE - H.UUIDSize);
  return H;
}

Header Header::normalize(const HeaderV1 &V1H) {
  Header H;
  memset(&H, 0, sizeof(H));
  H.Magic = V1H.Magic;
  H.Version = V1H.Version;
  H.AddrOffSize = V1H.AddrOffSize;
  H.UUIDSize = V1H.UUIDSize;
  H.BaseAddress = V1H.BaseAddress;
  H.StrtabOffset = V1H.StrtabOffset;
  H.StrtabSize = V1H.StrtabSize;
  H.NumAddresses = V1H.NumAddresses;
  H.FuncInfoOffsetSize = 4;
  H.StringOffsetSize = 4;
  memcpy(H.UUID, V1H.UUID, GSYM_MAX_UUID_SIZE);
  return H;
}

bool llvm::gsym::operator==(const Header &LHS, const Header &RHS) {
  return LHS.Magic == RHS.Magic && LHS.Version == RHS.Version &&
      LHS.AddrOffSize == RHS.AddrOffSize && LHS.UUIDSize == RHS.UUIDSize &&
      LHS.BaseAddress == RHS.BaseAddress &&
      LHS.NumAddresses == RHS.NumAddresses &&
      LHS.FuncInfoOffsetSize == RHS.FuncInfoOffsetSize &&
      LHS.StringOffsetSize == RHS.StringOffsetSize &&
      LHS.StrtabOffset == RHS.StrtabOffset &&
      LHS.StrtabSize == RHS.StrtabSize &&
      memcmp(LHS.UUID, RHS.UUID, LHS.UUIDSize) == 0;
}
