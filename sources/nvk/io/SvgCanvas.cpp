#include <nvk/io/SvgCanvas.h>

namespace nv {

namespace {

// Minimal XML entity escaping so labels containing '&', '<', '>' or quotes
// (eg. connector ids, formatted floats with a stray '<' from debug strings)
// don't corrupt the surrounding markup.
auto escape_xml(const String& str) -> String {
    String out;
    out.reserve(str.size());
    for (char c : str) {
        switch (c) {
        case '&':
            out += "&amp;";
            break;
        case '<':
            out += "&lt;";
            break;
        case '>':
            out += "&gt;";
            break;
        case '"':
            out += "&quot;";
            break;
        default:
            out += c;
            break;
        }
    }
    return out;
}

} // namespace

void SvgCanvas::fit(const Vector<Vec2d>& pts, F64 targetWidthPx) {
    F64 maxX = std::numeric_limits<F64>::lowest();
    F64 minY = std::numeric_limits<F64>::max();
    F64 minXIn = std::numeric_limits<F64>::max();
    F64 maxYIn = std::numeric_limits<F64>::lowest();
    for (const auto& p : pts) {
        minXIn = std::min(minXIn, p.x());
        maxX = std::max(maxX, p.x());
        minY = std::min(minY, p.y());
        maxYIn = std::max(maxYIn, p.y());
    }
    fit_bounds(minXIn, minY, maxX, maxYIn, targetWidthPx);
}
void SvgCanvas::fit_bounds(F64 minXIn, F64 minYIn, F64 maxXIn, F64 maxYIn,
                           F64 targetWidthPx) {
    body << std::fixed << std::setprecision(1);
    widthPx = targetWidthPx;
    minX = minXIn;
    maxY = maxYIn;
    const F64 spanX = std::max(maxXIn - minXIn, 1.0);
    const F64 spanY = std::max(maxYIn - minYIn, 1.0);
    scale = (widthPx - 2.0 * marginPx) / spanX;
    heightPx = spanY * scale + 2.0 * marginPx;
}
auto SvgCanvas::map(const Vec2d& p) const -> Vec2d {
    return {marginPx + (p.x() - minX) * scale,
            marginPx + (maxY - p.y()) * scale};
}
void SvgCanvas::polyline(const Vector<Vec2d>& pts, const char* color,
                         F64 strokePx, bool dashed) {
    if (pts.size() < 2)
        return;
    body << "<polyline fill=\"none\" stroke=\"" << color << "\" stroke-width=\""
         << strokePx << "\"";
    if (dashed)
        body << " stroke-dasharray=\"6 4\"";
    body << " points=\"";
    for (const auto& p : pts) {
        const Vec2d s = map(p);
        body << s.x() << "," << s.y() << " ";
    }
    body << "\"/>\n";
}
void SvgCanvas::polygon(const Vector<Vec2d>& pts, const char* fillColor,
                        const char* strokeColor, F64 strokePx,
                        F64 fillOpacity) {
    if (pts.size() < 3)
        return;
    body << "<polygon fill=\"" << (fillColor ? fillColor : "none") << "\"";
    if (fillColor)
        body << " fill-opacity=\"" << fillOpacity << "\"";
    body << " stroke=\"" << strokeColor << "\" stroke-width=\"" << strokePx
         << "\" points=\"";
    for (const auto& p : pts) {
        const Vec2d s = map(p);
        body << s.x() << "," << s.y() << " ";
    }
    body << "\"/>\n";
}
void SvgCanvas::line(const Vec2d& a, const Vec2d& b, const char* color,
                     F64 strokePx, bool dashed) {
    polyline({a, b}, color, strokePx, dashed);
}
void SvgCanvas::line_arrow(const Vec2d& a, const Vec2d& b, const char* color,
                           F64 strokePx, F64 headSizePx) {
    line(a, b, color, strokePx);
    const Vec2d sa = map(a);
    const Vec2d sb = map(b);
    const F64 dx = sb.x() - sa.x();
    const F64 dy = sb.y() - sa.y();
    const F64 len = std::hypot(dx, dy);
    if (len < 1e-6)
        return;
    const F64 ux = dx / len;
    const F64 uy = dy / len;
    constexpr F64 headAngle = 0.5; // radians, ~29deg half-angle
    const F64 cosA = std::cos(headAngle);
    const F64 sinA = std::sin(headAngle);
    // Rotate the back-pointing direction (-ux,-uy) by +-headAngle to get the
    // two wing points of the arrowhead.
    const F64 backX = -ux;
    const F64 backY = -uy;
    const F64 wing1X = backX * cosA - backY * sinA;
    const F64 wing1Y = backX * sinA + backY * cosA;
    const F64 wing2X = backX * cosA + backY * sinA;
    const F64 wing2Y = -backX * sinA + backY * cosA;
    body << "<polygon fill=\"" << color << "\" points=\"" << sb.x() << ","
         << sb.y() << " " << (sb.x() + wing1X * headSizePx) << ","
         << (sb.y() + wing1Y * headSizePx) << " "
         << (sb.x() + wing2X * headSizePx) << ","
         << (sb.y() + wing2Y * headSizePx) << "\"/>\n";
}
void SvgCanvas::dot(const Vec2d& c, F64 radiusPx, const char* color,
                    bool filled) {
    const Vec2d s = map(c);
    body << "<circle cx=\"" << s.x() << "\" cy=\"" << s.y() << "\" r=\""
         << radiusPx << "\" ";
    if (filled)
        body << "fill=\"" << color << "\"/>\n";
    else
        body << "fill=\"none\" stroke=\"" << color
             << "\" stroke-width=\"2\"/>\n";
}
void SvgCanvas::circle_world(const Vec2d& c, F64 radiusCm, const char* color) {
    const Vec2d s = map(c);
    body << "<circle cx=\"" << s.x() << "\" cy=\"" << s.y() << "\" r=\""
         << radiusCm * scale << "\" fill=\"none\" stroke=\"" << color
         << "\" stroke-width=\"1\" stroke-dasharray=\"3 3\"/>\n";
}
void SvgCanvas::cross(const Vec2d& c, F64 sizePx, const char* color) {
    const Vec2d s = map(c);
    body << "<path fill=\"none\" stroke=\"" << color
         << "\" stroke-width=\"2\" d=\"M " << (s.x() - sizePx) << " "
         << (s.y() - sizePx) << " L " << (s.x() + sizePx) << " "
         << (s.y() + sizePx) << " M " << (s.x() - sizePx) << " "
         << (s.y() + sizePx) << " L " << (s.x() + sizePx) << " "
         << (s.y() - sizePx) << "\"/>\n";
}
void SvgCanvas::text(const Vec2d& pos, const String& str, const char* color,
                     F64 sizePx) {
    const Vec2d s = map(pos);
    body << "<text x=\"" << s.x() << "\" y=\"" << s.y() << "\" fill=\"" << color
         << "\" font-size=\"" << sizePx << "\" font-family=\"monospace\">"
         << escape_xml(str) << "</text>\n";
}
void SvgCanvas::text_px(F64 x, F64 y, const String& str, const char* color,
                        F64 sizePx) {
    body << "<text x=\"" << x << "\" y=\"" << y << "\" fill=\"" << color
         << "\" font-size=\"" << sizePx << "\" font-family=\"monospace\">"
         << escape_xml(str) << "</text>\n";
}
void SvgCanvas::clear() {
    body.str("");
    body.clear();
    body << std::fixed << std::setprecision(1);
}
auto SvgCanvas::finalize() const -> String {
    std::ostringstream out;
    out << std::fixed << std::setprecision(1)
        << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << widthPx
        << "\" height=\"" << heightPx << "\" viewBox=\"0 0 " << widthPx << " "
        << heightPx << "\">\n"
        << "<rect width=\"100%\" height=\"100%\" fill=\"#ffffff\"/>\n"
        << body.str() << "</svg>\n";
    return out.str();
}
void SvgCanvas::write_file(const String& fpath) const {
    auto folder = get_parent_folder(fpath);
    create_folders(folder);
    nv::write_file(fpath.c_str(), finalize());
}
} // namespace nv
