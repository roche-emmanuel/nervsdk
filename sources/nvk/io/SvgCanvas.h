#ifndef _SVG_CANVAS_H_
#define _SVG_CANVAS_H_

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
    bool flipVertical{false};
    std::ostringstream body;

    void fit(const Vector<Vec2d>& pts, F64 targetWidthPx);

    // When enabled, mirrors every mapped (world-space) point about the
    // horizontal midline of the canvas, on top of whatever flip map()
    // already applies. Useful when the caller's world convention already
    // has Y growing downward (eg. raster-convention data), so the default
    // Y-up -> Y-down flip in map() ends up inverted relative to what the
    // caller expects.
    void set_flip_vertical(bool flip) { flipVertical = flip; }

    // Same as fit(), but takes an explicit world-space bounding box instead
    // of a point list. Handy when the bbox is already known (eg. accumulated
    // incrementally) and building a Vector<Vec2d> just to call fit() would be
    // wasteful.
    void fit_bounds(F64 minXIn, F64 minYIn, F64 maxXIn, F64 maxYIn,
                    F64 targetWidthPx, bool flipVertical = false);

    [[nodiscard]] auto map(const Vec2d& p) const -> Vec2d;

    void polyline(const Vector<Vec2d>& pts, const char* color, F64 strokePx,
                  bool dashed = false);

    // Closed, optionally filled shape (eg. junction fan triangles, overlap
    // regions). fillColor may be nullptr for an unfilled (stroke-only)
    // polygon.
    void polygon(const Vector<Vec2d>& pts, const char* fillColor,
                 const char* strokeColor, F64 strokePx, F64 fillOpacity = 1.0);

    // Several closed rings rendered as a single filled path, resolved with
    // the even-odd rule. That is what a raster-traced coastline needs:
    // an island inside a lake inside an island comes out right without the
    // caller having to classify rings as outer or hole. fillColor may be
    // nullptr for a stroke-only outline, strokeColor may be nullptr for a
    // fill with no outline.
    void polygons(const Vector<Vector<Vec2d>>& rings, const char* fillColor,
                  const char* strokeColor, F64 strokePx, F64 fillOpacity = 1.0);

    void line(const Vec2d& a, const Vec2d& b, const char* color, F64 strokePx,
              bool dashed = false);

    // Same as line(), but draws a small filled arrowhead at b. Useful for
    // debugging directionality (connector orientation, road direction, ...).
    void line_arrow(const Vec2d& a, const Vec2d& b, const char* color,
                    F64 strokePx, F64 headSizePx = 8.0);

    void dot(const Vec2d& c, F64 radiusPx, const char* color,
             bool filled = true);

    void circle_world(const Vec2d& c, F64 radiusCm, const char* color);

    void cross(const Vec2d& c, F64 sizePx, const char* color);

    void text(const Vec2d& pos, const String& str, const char* color,
              F64 sizePx);

    // Screen-space text (px), for the info footer.
    void text_px(F64 x, F64 y, const String& str, const char* color,
                 F64 sizePx);

    // Resets accumulated markup so the same canvas (and its current fit) can
    // be reused for another drawing pass.
    void clear();

    [[nodiscard]] auto finalize() const -> String;

    void write_file(const String& fpath) const;
};

} // namespace nv

#endif
