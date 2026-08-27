// File: nvk/sim/TerrainField.cpp

#include <nvk/sim/TerrainField.h>

namespace nv {

namespace {

/// Level 0 is never coarser than this, so a pathological config cannot ask
/// for a one-texel pyramid.
constexpr U32 kMinRes = 4;

} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

void MaxPyramidTerrain::build(Vector<F32>&& heights, U32 res, F64 sizeM) {
    NVCHK(res >= kMinRes, "MaxPyramidTerrain: resolution {} is too small", res);
    NVCHK(sizeM > 0.0, "MaxPyramidTerrain: invalid world size {}", sizeM);
    NVCHK(heights.size() == size_t(res) * size_t(res),
          "MaxPyramidTerrain: expected {} samples, got {}",
          size_t(res) * size_t(res), heights.size());

    _res = res;
    _sizeM = sizeM;
    _pixelM = sizeM / F64(res);

    _levels.clear();
    _levelRes.clear();
    _levels.push_back(std::move(heights));
    _levelRes.push_back(res);

    reduce();
}

void MaxPyramidTerrain::build_from_fn(
    U32 res, F64 sizeM, const std::function<F64(const Vec2d&)>& fn) {
    NVCHK(bool(fn), "MaxPyramidTerrain: null sampling function");

    Vector<F32> heights(size_t(res) * size_t(res), 0.0F);
    const F64 pixelM = sizeM / F64(res);

    for (U32 row = 0; row < res; ++row) {
        const F64 posY = (F64(row) + 0.5) * pixelM;
        for (U32 col = 0; col < res; ++col) {
            const F64 posX = (F64(col) + 0.5) * pixelM;
            heights[size_t(row) * res + col] = F32(fn({posX, posY}));
        }
    }

    build(std::move(heights), res, sizeM);
}

void MaxPyramidTerrain::build_from_block_max(const Vector<F32>& srcHeights,
                                             U32 srcRes, U32 res, F64 sizeM) {
    NVCHK(srcRes >= res,
          "MaxPyramidTerrain: source resolution {} is below the target {}",
          srcRes, res);
    NVCHK(srcHeights.size() == size_t(srcRes) * size_t(srcRes),
          "MaxPyramidTerrain: source has {} samples, expected {}",
          srcHeights.size(), size_t(srcRes) * size_t(srcRes));

    Vector<F32> heights(size_t(res) * size_t(res),
                        -std::numeric_limits<F32>::max());

    // Source is node-convention: srcRes samples spanning [0, sizeM]
    // inclusive, so sample k sits at k * sizeM / (srcRes - 1).
    const F64 srcStepM = sizeM / F64(srcRes - 1);
    const F64 dstPixelM = sizeM / F64(res);

    for (U32 row = 0; row < res; ++row) {
        // Inclusive on both ends: a ridge that falls exactly on a texel
        // boundary must be seen by both neighbours, or it disappears from
        // one of them and the ceiling under-reports.
        const auto r0 = I32(std::floor(F64(row) * dstPixelM / srcStepM));
        const auto r1 = I32(std::ceil(F64(row + 1) * dstPixelM / srcStepM));

        for (U32 col = 0; col < res; ++col) {
            const auto c0 = I32(std::floor(F64(col) * dstPixelM / srcStepM));
            const auto c1 = I32(std::ceil(F64(col + 1) * dstPixelM / srcStepM));

            F32 best = -std::numeric_limits<F32>::max();

            for (I32 sr = r0; sr <= r1; ++sr) {
                if (sr < 0 || sr >= I32(srcRes)) {
                    continue;
                }
                const size_t rowBase = size_t(sr) * srcRes;
                for (I32 sc = c0; sc <= c1; ++sc) {
                    if (sc < 0 || sc >= I32(srcRes)) {
                        continue;
                    }
                    best = std::max(best, srcHeights[rowBase + size_t(sc)]);
                }
            }

            if (best == -std::numeric_limits<F32>::max()) {
                best = 0.0F;
            }
            heights[size_t(row) * res + col] = best;
        }
    }

    build(std::move(heights), res, sizeM);
}

void MaxPyramidTerrain::raise_level0(U32 row, U32 col, F32 heightM) {
    NVCHK(is_valid(), "MaxPyramidTerrain: raise_level0 on an empty pyramid");
    if (row >= _res || col >= _res) {
        return;
    }
    F32& dst = _levels[0][size_t(row) * _res + col];
    dst = std::max(dst, heightM);
}

void MaxPyramidTerrain::reduce() {
    NVCHK(!_levels.empty(), "MaxPyramidTerrain: reduce with no level 0");

    _levels.resize(1);
    _levelRes.resize(1);

    U32 res = _res;
    while (res > 1) {
        const U32 nextRes = res / 2;
        const Vector<F32>& src = _levels.back();
        Vector<F32> dst(size_t(nextRes) * size_t(nextRes), 0.0F);

        for (U32 row = 0; row < nextRes; ++row) {
            const size_t srcRow0 = size_t(row * 2) * res;
            const size_t srcRow1 = size_t(row * 2 + 1) * res;
            for (U32 col = 0; col < nextRes; ++col) {
                const U32 c0 = col * 2;
                const U32 c1 = c0 + 1;
                const F32 val =
                    std::max(std::max(src[srcRow0 + c0], src[srcRow0 + c1]),
                             std::max(src[srcRow1 + c0], src[srcRow1 + c1]));
                dst[size_t(row) * nextRes + col] = val;
            }
        }

        _levels.push_back(std::move(dst));
        _levelRes.push_back(nextRes);
        res = nextRes;
    }

    refresh_extents();
}

void MaxPyramidTerrain::refresh_extents() {
    _minM = std::numeric_limits<F64>::max();
    _maxM = -std::numeric_limits<F64>::max();

    for (const F32 val : _levels[0]) {
        _minM = std::min(_minM, F64(val));
        _maxM = std::max(_maxM, F64(val));
    }

    if (_levels[0].empty()) {
        _minM = 0.0;
        _maxM = 0.0;
    }
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

auto MaxPyramidTerrain::texel(U32 level, I32 row, I32 col) const -> F32 {
    const auto res = I32(_levelRes[level]);
    const I32 rr = std::clamp(row, 0, res - 1);
    const I32 cc = std::clamp(col, 0, res - 1);
    return _levels[level][size_t(rr) * size_t(res) + size_t(cc)];
}

auto MaxPyramidTerrain::level_for_span(F64 spanM) const -> U32 {
    if (spanM <= _pixelM) {
        return 0;
    }

    // Largest k with pixelM * 2^k <= spanM, capped by the pyramid depth.
    const F64 ratio = spanM / _pixelM;
    auto lvl = U32(std::floor(std::log2(ratio)));
    return std::min(lvl, U32(_levels.size()) - 1);
}

auto MaxPyramidTerrain::height_at(const Vec2d& posM) const -> F64 {
    if (!is_valid()) {
        return 0.0;
    }

    const auto col = I32(std::floor(posM.x() / _pixelM));
    const auto row = I32(std::floor(posM.y() / _pixelM));

    return F64(texel(0, row, col));
}

auto MaxPyramidTerrain::max_height_in(const Vec2d& posM, F64 radiusM) const
    -> F64 {
    if (!is_valid()) {
        return 0.0;
    }

    if (radiusM <= 0.0) {
        return height_at(posM);
    }

    // Pick the level whose texel is no larger than the query diameter, so
    // the bounding box below spans at most three texels per axis. Using the
    // diameter rather than the radius is what bounds the loop: a level whose
    // texel matched the radius would need up to four texels a side, which
    // costs the same but reads worse.
    const U32 lvl = level_for_span(radiusM * 2.0);
    const F64 texelM = _pixelM * F64(1U << lvl);

    const auto c0 = I32(std::floor((posM.x() - radiusM) / texelM));
    const auto c1 = I32(std::floor((posM.x() + radiusM) / texelM));
    const auto r0 = I32(std::floor((posM.y() - radiusM) / texelM));
    const auto r1 = I32(std::floor((posM.y() + radiusM) / texelM));

    F64 best = -std::numeric_limits<F64>::max();
    for (I32 row = r0; row <= r1; ++row) {
        for (I32 col = c0; col <= c1; ++col) {
            best = std::max(best, F64(texel(lvl, row, col)));
        }
    }

    return best;
}

// ---------------------------------------------------------------------------
// Packing
// ---------------------------------------------------------------------------

auto MaxPyramidTerrain::level_row_offset(U32 level) const -> U32 {
    U32 offset = 0;
    for (U32 i = 0; i < level; ++i) {
        offset += _levelRes[i];
    }
    return offset;
}

auto MaxPyramidTerrain::total_rows() const -> U32 {
    return level_row_offset(U32(_levels.size()));
}

auto MaxPyramidTerrain::pack_rows() const -> Vector<F32> {
    NVCHK(is_valid(), "MaxPyramidTerrain: pack_rows on an empty pyramid");

    const U32 rows = total_rows();
    Vector<F32> out(size_t(rows) * size_t(_res), 0.0F);

    for (U32 lvl = 0; lvl < U32(_levels.size()); ++lvl) {
        const U32 res = _levelRes[lvl];
        const U32 rowOff = level_row_offset(lvl);
        for (U32 row = 0; row < res; ++row) {
            const size_t src = size_t(row) * res;
            const size_t dst = size_t(rowOff + row) * _res;
            std::copy(_levels[lvl].begin() + src,
                      _levels[lvl].begin() + src + res, out.begin() + dst);
        }
    }

    return out;
}

void MaxPyramidTerrain::unpack_rows(const Vector<F32>& packed, U32 res,
                                    F64 sizeM) {
    NVCHK(res >= kMinRes, "MaxPyramidTerrain: resolution {} is too small", res);
    NVCHK(sizeM > 0.0, "MaxPyramidTerrain: invalid world size {}", sizeM);

    _res = res;
    _sizeM = sizeM;
    _pixelM = sizeM / F64(res);

    _levels.clear();
    _levelRes.clear();

    U32 levelRes = res;
    U32 rowOff = 0;
    while (levelRes >= 1) {
        Vector<F32> lvl(size_t(levelRes) * size_t(levelRes), 0.0F);
        for (U32 row = 0; row < levelRes; ++row) {
            const size_t src = size_t(rowOff + row) * res;
            NVCHK(src + levelRes <= packed.size(),
                  "MaxPyramidTerrain: packed buffer is truncated at level "
                  "res {}",
                  levelRes);
            std::copy(packed.begin() + src, packed.begin() + src + levelRes,
                      lvl.begin() + size_t(row) * levelRes);
        }

        _levels.push_back(std::move(lvl));
        _levelRes.push_back(levelRes);

        rowOff += levelRes;
        if (levelRes == 1) {
            break;
        }
        levelRes /= 2;
    }

    refresh_extents();
}

} // namespace nv
