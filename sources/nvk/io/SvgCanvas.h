// Minimal SVG canvas used by dump_junction_svg(): fits world-cm points into a
// fixed-width viewport (Y axis flipped: world Y-up -> SVG Y-down) and
// accumulates shapes as SVG markup.

#include <nvk_common.h>

namespace nv {

struct SvgCanvas {
    F64 minX{0.0};
    F64 maxY{0.0};
    F64 scale{1.0};
    F64 widthPx{1400.0};
    F64 heightPx{0.0};
    F64 marginPx{50.0};
    std::ostringstream body;

    void fit(const Vector<Vec2d>& pts, F64 targetWidthPx);

    [[nodiscard]] auto map(const Vec2d& p) const -> Vec2d;

    void polyline(const Vector<Vec2d>& pts, const char* color, F64 strokePx,
                  bool dashed = false);

    void line(const Vec2d& a, const Vec2d& b, const char* color, F64 strokePx,
              bool dashed = false);

    void dot(const Vec2d& c, F64 radiusPx, const char* color,
             bool filled = true);

    void circle_world(const Vec2d& c, F64 radiusCm, const char* color);

    void cross(const Vec2d& c, F64 sizePx, const char* color);

    void text(const Vec2d& pos, const String& str, const char* color,
              F64 sizePx);

    // Screen-space text (px), for the info footer.
    void text_px(F64 x, F64 y, const String& str, const char* color,
                 F64 sizePx);

    [[nodiscard]] auto finalize() const -> String;

    void write_file(const String& fpath) const;
};

} // namespace nv
