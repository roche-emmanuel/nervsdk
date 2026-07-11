#include <nvk/io/SvgCanvas.h>

namespace nv {

void SvgCanvas::fit(const Vector<Vec2d>& pts, F64 targetWidthPx) {
    body << std::fixed << std::setprecision(1);
    widthPx = targetWidthPx;
    F64 maxX = std::numeric_limits<F64>::lowest();
    F64 minY = std::numeric_limits<F64>::max();
    minX = std::numeric_limits<F64>::max();
    maxY = std::numeric_limits<F64>::lowest();
    for (const auto& p : pts) {
        minX = std::min(minX, p.x());
        maxX = std::max(maxX, p.x());
        minY = std::min(minY, p.y());
        maxY = std::max(maxY, p.y());
    }
    const F64 spanX = std::max(maxX - minX, 1.0);
    const F64 spanY = std::max(maxY - minY, 1.0);
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
void SvgCanvas::line(const Vec2d& a, const Vec2d& b, const char* color,
                     F64 strokePx, bool dashed) {
    polyline({a, b}, color, strokePx, dashed);
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
         << str << "</text>\n";
}
void SvgCanvas::text_px(F64 x, F64 y, const String& str, const char* color,
                        F64 sizePx) {
    body << "<text x=\"" << x << "\" y=\"" << y << "\" fill=\"" << color
         << "\" font-size=\"" << sizePx << "\" font-family=\"monospace\">"
         << str << "</text>\n";
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