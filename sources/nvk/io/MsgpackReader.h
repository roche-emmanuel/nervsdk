// File: nvk/io/MsgpackReader.h
// A minimal, self-contained MessagePack reader operating on a read-only byte
// span.  No external dependencies beyond nvk_types.h and the C++20 STL.
//
// Handles the subset of MessagePack produced by Python's msgpack library with
// use_bin_type=True:
//   fixmap / map16 / map32
//   fixstr / str8  / str16 / str32
//   fixarray / array16 / array32
//   positive fixint / negative fixint
//   uint8 / uint16 / uint32 / uint64
//   int8  / int16  / int32  / int64
//   float32
//   bin8  / bin16  / bin32   (raw bytes)
//   nil / false / true       (skippable, not explicitly returned)
//
// Usage:
//   std::vector<U8> bytes = ...;
//   nv::MsgpackReader rdr(bytes.data(), bytes.size());
//
//   U32 nKeys = rdr.readMapSize();
//   for (U32 i = 0; i < nKeys; ++i) {
//       std::string key = rdr.readString();
//       if (key == "count")       { I64 v = rdr.readInt(); }
//       else if (key == "value")  { F32 v = rdr.readFloat(); }
//       else if (key == "blob")   { auto b = rdr.readBin(); }
//       else                      { rdr.skipValue(); }
//   }

#pragma once

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include <nvk_common.h> // NVCHK, THROW_MSG
#include <nvk_types.h>

namespace nv {

class MsgpackReader {
  public:
    // Construct from a raw byte span.  The caller owns the buffer and must
    // keep it alive for the lifetime of this reader.
    MsgpackReader(const U8* data, size_t size) noexcept
        : _p(data), _end(data + size) {}

    explicit MsgpackReader(const std::vector<U8>& buf) noexcept
        : MsgpackReader(buf.data(), buf.size()) {}

    // ── Position queries ─────────────────────────────────────────────────────

    [[nodiscard]] auto atEnd() const noexcept -> bool { return _p >= _end; }
    [[nodiscard]] auto remaining() const noexcept -> size_t {
        return static_cast<size_t>(_end - _p);
    }

    // ── High-level typed readers
    // ──────────────────────────────────────────────

    // Read a map header → number of key-value pairs.
    auto readMapSize() -> U32 {
        const U8 b = _readU8();
        if ((b & 0xf0u) == 0x80u)
            return b & 0x0fu; // fixmap
        if (b == 0xde)
            return _readU16BE(); // map16
        if (b == 0xdf)
            return _readU32BE(); // map32
        THROW_MSG("MsgpackReader::readMapSize: unexpected byte 0x{:02x}.", b);
    }

    // Read an array header → number of elements.
    auto readArraySize() -> U32 {
        const U8 b = _readU8();
        if ((b & 0xf0u) == 0x90u)
            return b & 0x0fu; // fixarray
        if (b == 0xdc)
            return _readU16BE(); // array16
        if (b == 0xdd)
            return _readU32BE(); // array32
        THROW_MSG("MsgpackReader::readArraySize: unexpected byte 0x{:02x}.", b);
    }

    // Read a string (fixstr / str8 / str16 / str32).
    auto readString() -> std::string {
        const U8 b = _readU8();
        U32 len = 0;
        if ((b & 0xe0u) == 0xa0u)
            len = b & 0x1fu; // fixstr
        else if (b == 0xd9)
            len = _readU8(); // str8
        else if (b == 0xda)
            len = _readU16BE(); // str16
        else if (b == 0xdb)
            len = _readU32BE(); // str32
        else
            THROW_MSG("MsgpackReader::readString: unexpected byte 0x{:02x}.",
                      b);
        return _readRawString(len);
    }

    // Read a binary blob (bin8 / bin16 / bin32) → vector of bytes.
    auto readBin() -> std::vector<U8> {
        const U8 b = _readU8();
        U32 len = 0;
        if (b == 0xc4)
            len = _readU8(); // bin8
        else if (b == 0xc5)
            len = _readU16BE(); // bin16
        else if (b == 0xc6)
            len = _readU32BE(); // bin32
        else
            THROW_MSG("MsgpackReader::readBin: unexpected byte 0x{:02x}.", b);
        return _readRawBytes(len);
    }

    // Read any integer variant (positive/negative fixint, u/int 8–64) as I64.
    auto readInt() -> I64 {
        const U8 b = _peekU8();
        if (b <= 0x7fu) {
            _p++;
            return static_cast<I64>(b);
        } // positive fixint
        if (b >= 0xe0u) {
            _p++;
            return static_cast<I64>(static_cast<I8>(b));
        } // negative fixint
        _p++;
        switch (b) {
        case 0xcc:
            return static_cast<I64>(_readU8());
        case 0xcd:
            return static_cast<I64>(_readU16BE());
        case 0xce:
            return static_cast<I64>(_readU32BE());
        case 0xcf:
            return static_cast<I64>(_readU64BE());
        case 0xd0:
            return static_cast<I64>(static_cast<I8>(_readU8()));
        case 0xd1:
            return static_cast<I64>(static_cast<I16>(_readU16BE()));
        case 0xd2:
            return static_cast<I64>(static_cast<I32>(_readU32BE()));
        case 0xd3:
            return static_cast<I64>(_readU64BE());
        default:
            THROW_MSG("MsgpackReader::readInt: unexpected byte 0x{:02x}.", b);
        }
    }

    // Read a float32 value.
    auto readFloat() -> F32 {
        const U8 b = _readU8();
        NVCHK(
            b == 0xca,
            "MsgpackReader::readFloat: expected float32 (0xca), got 0x{:02x}.",
            b);
        return _readF32BE();
    }

    // Read a float64 value.
    auto readDouble() -> F64 {
        const U8 b = _readU8();
        NVCHK(
            b == 0xcb,
            "MsgpackReader::readDouble: expected float64 (0xcb), got 0x{:02x}.",
            b);
        const U64 bits = _readU64BE();
        F64 v;
        std::memcpy(&v, &bits, sizeof(v));
        return v;
    }

    // Skip exactly one complete msgpack value (any type).
    // Used to ignore unknown map keys and their associated values.
    void skipValue() {
        const U8 b = _readU8();
        // positive fixint / negative fixint
        if (b <= 0x7fu || b >= 0xe0u)
            return;
        // fixstr
        if ((b & 0xe0u) == 0xa0u) {
            _advance(b & 0x1fu);
            return;
        }
        // fixmap
        if ((b & 0xf0u) == 0x80u) {
            const U32 n = b & 0x0fu;
            for (U32 i = 0; i < n * 2u; ++i)
                skipValue();
            return;
        }
        // fixarray
        if ((b & 0xf0u) == 0x90u) {
            const U32 n = b & 0x0fu;
            for (U32 i = 0; i < n; ++i)
                skipValue();
            return;
        }
        switch (b) {
        case 0xc0:
        case 0xc2:
        case 0xc3:
            return; // nil, false, true
        case 0xcc:
        case 0xd0:
            _advance(1);
            return; // uint8, int8
        case 0xcd:
        case 0xd1:
            _advance(2);
            return; // uint16, int16
        case 0xce:
        case 0xd2:
        case 0xca:
            _advance(4);
            return; // uint32, int32, float32
        case 0xcf:
        case 0xd3:
        case 0xcb:
            _advance(8);
            return; // uint64, int64, float64
        case 0xd9: {
            _advance(_readU8());
            return;
        } // str8
        case 0xda: {
            _advance(_readU16BE());
            return;
        } // str16
        case 0xdb: {
            _advance(_readU32BE());
            return;
        } // str32
        case 0xc4: {
            _advance(_readU8());
            return;
        } // bin8
        case 0xc5: {
            _advance(_readU16BE());
            return;
        } // bin16
        case 0xc6: {
            _advance(_readU32BE());
            return;
        } // bin32
        case 0xde: {
            const U32 n = _readU16BE();
            for (U32 i = 0; i < n * 2u; ++i)
                skipValue();
            return;
        } // map16
        case 0xdf: {
            const U32 n = _readU32BE();
            for (U32 i = 0; i < n * 2u; ++i)
                skipValue();
            return;
        } // map32
        case 0xdc: {
            const U32 n = _readU16BE();
            for (U32 i = 0; i < n; ++i)
                skipValue();
            return;
        } // array16
        case 0xdd: {
            const U32 n = _readU32BE();
            for (U32 i = 0; i < n; ++i)
                skipValue();
            return;
        } // array32
        default:
            THROW_MSG("MsgpackReader::skipValue: unhandled byte 0x{:02x}.", b);
        }
    }

  private:
    const U8* _p;
    const U8* _end;

    // ── Low-level primitives
    // ──────────────────────────────────────────────────

    void _checkAvailable(size_t n, const char* caller) const {
        NVCHK(
            _p + n <= _end,
            "MsgpackReader::{}: buffer overrun (need {} bytes, {} remaining).",
            caller, n, remaining());
    }

    auto _peekU8() const -> U8 {
        _checkAvailable(1, "peek");
        return *_p;
    }

    auto _readU8() -> U8 {
        _checkAvailable(1, "readU8");
        return *_p++;
    }

    auto _readU16BE() -> U16 {
        _checkAvailable(2, "readU16BE");
        const U16 v = static_cast<U16>((static_cast<U16>(_p[0]) << 8u) | _p[1]);
        _p += 2;
        return v;
    }

    auto _readU32BE() -> U32 {
        _checkAvailable(4, "readU32BE");
        const U32 v = (static_cast<U32>(_p[0]) << 24u) |
                      (static_cast<U32>(_p[1]) << 16u) |
                      (static_cast<U32>(_p[2]) << 8u) | static_cast<U32>(_p[3]);
        _p += 4;
        return v;
    }

    auto _readU64BE() -> U64 {
        _checkAvailable(8, "readU64BE");
        const U64 hi = _readU32BE();
        const U64 lo = _readU32BE();
        return (hi << 32u) | lo;
    }

    // Reads a big-endian IEEE-754 float32 via memcpy — no aliasing UB,
    // same pattern used by RprDecoder in the SDK.
    auto _readF32BE() -> F32 {
        _checkAvailable(4, "readF32BE");
        U32 raw = (static_cast<U32>(_p[0]) << 24u) |
                  (static_cast<U32>(_p[1]) << 16u) |
                  (static_cast<U32>(_p[2]) << 8u) | static_cast<U32>(_p[3]);
        _p += 4;
        F32 v;
        std::memcpy(&v, &raw, sizeof(v));
        return v;
    }

    void _advance(U32 n) {
        _checkAvailable(n, "advance");
        _p += n;
    }

    auto _readRawString(U32 len) -> std::string {
        _checkAvailable(len, "readRawString");
        std::string s(reinterpret_cast<const char*>(_p), len);
        _p += len;
        return s;
    }

    auto _readRawBytes(U32 len) -> std::vector<U8> {
        _checkAvailable(len, "readRawBytes");
        std::vector<U8> out(len);
        std::memcpy(out.data(), _p, len);
        _p += len;
        return out;
    }
};

} // namespace nv