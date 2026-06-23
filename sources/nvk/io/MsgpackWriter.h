
#pragma once

#include <nvk_common.h>
#include <nvk_types.h>

namespace nv {
// ---------------------------------------------------------------------------
// MsgpackWriter — minimal hand-rolled msgpack serialiser.
// Produces the subset used by building_placer.py: map, array, str, bin,
// int (positive fixint / uint32), float32.
// ---------------------------------------------------------------------------
class MsgpackWriter {
  public:
    auto take() -> U8Vector { return std::move(_buf); }
    [[nodiscard]] auto data() const -> const U8Vector& { return _buf; }

    void writeMapHeader(U32 n) {
        if (n <= 15) {
            _buf.push_back(0x80U | U8(n));
        } else if (n <= 0xffffU) {
            _buf.push_back(0xde);
            _writeU16BE(U16(n));
        } else {
            _buf.push_back(0xdf);
            _writeU32BE(n);
        }
    }

    void writeArrayHeader(U32 n) {
        if (n <= 15) {
            _buf.push_back(0x90U | U8(n));
        } else if (n <= 0xffffU) {
            _buf.push_back(0xdc);
            _writeU16BE(U16(n));
        } else {
            _buf.push_back(0xdd);
            _writeU32BE(n);
        }
    }

    void writeStr(const std::string& s) {
        U32 len = U32(s.size());
        if (len <= 31) {
            _buf.push_back(0xa0U | U8(len));
        } else if (len <= 0xffU) {
            _buf.push_back(0xd9);
            _buf.push_back(U8(len));
        } else {
            _buf.push_back(0xda);
            _writeU16BE(U16(len));
        }
        _buf.insert(_buf.end(), s.begin(), s.end());
    }

    void writeBin(const U8Vector& b) {
        U32 len = U32(b.size());
        if (len <= 0xffU) {
            _buf.push_back(0xc4);
            _buf.push_back(U8(len));
        } else if (len <= 0xffffU) {
            _buf.push_back(0xc5);
            _writeU16BE(U16(len));
        } else {
            _buf.push_back(0xc6);
            _writeU32BE(len);
        }
        _buf.insert(_buf.end(), b.begin(), b.end());
    }

    void writeInt(I64 v) {
        if (v >= 0 && v <= 127) {
            _buf.push_back(U8(v));
        } else if (v >= 0 && v <= 0xffffU) {
            _buf.push_back(0xcd);
            _writeU16BE(U16(v));
        } else if (v >= 0) {
            _buf.push_back(0xce);
            _writeU32BE(U32(v));
        } else {
            // negative — use int32
            _buf.push_back(0xd2);
            _writeU32BE(U32(I32(v)));
        }
    }

    // Writes a float32 as msgpack float32 (0xca).
    // Note: origin_x/origin_y are stored as float32 bytes in the Python writer.
    void writeFloat32(F32 v) {
        _buf.push_back(0xca);
        U32 bits = 0;
        std::memcpy(&bits, &v, 4);
        _writeU32BE(bits);
    }

    void writeFloat64(F64 v) {
        _buf.push_back(0xcb);
        U64 bits = 0;
        std::memcpy(&bits, &v, 8);
        _writeU64BE(bits);
    }

    // Writes a boolean as msgpack bool (0xc2 = false, 0xc3 = true).
    void writeBool(bool v) { _buf.push_back(v ? 0xc3U : 0xc2U); }

  private:
    U8Vector _buf;

    void _writeU16BE(U16 v) {
        _buf.push_back(U8(v >> 8));
        _buf.push_back(U8(v));
    }
    void _writeU32BE(U32 v) {
        _buf.push_back(U8(v >> 24));
        _buf.push_back(U8(v >> 16));
        _buf.push_back(U8(v >> 8));
        _buf.push_back(U8(v));
    }
    void _writeU64BE(U64 v) {
        _buf.push_back(U8(v >> 56));
        _buf.push_back(U8(v >> 48));
        _buf.push_back(U8(v >> 40));
        _buf.push_back(U8(v >> 32));
        _buf.push_back(U8(v >> 24));
        _buf.push_back(U8(v >> 16));
        _buf.push_back(U8(v >> 8));
        _buf.push_back(U8(v));
    }
};

} // namespace nv