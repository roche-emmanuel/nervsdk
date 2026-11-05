#ifndef _NV_TEXTUREFOLDERCONFIG_H_
#define _NV_TEXTUREFOLDERCONFIG_H_

#include <nvk/pcg/CellTextureAtlas.h>

namespace nv {

// ---------------------------------------------------------------------------
// Folder based texture atlas configuration (cells_material, ground_material).
//
//   ground_material:
//     slot_size: 256
//     grid_xsize: 16
//     grid_ysize: 16
//     defaults:                  # lowest priority, applies to every entry
//       size: [1024, 1024]
//       dimsM: [2, 2]
//       type: ground
//       category: surface
//       disp_scale: 2.0
//       max_res: 2k              # optional: never pick a map above this resolution
//     content:
//       - folder: Ground/terrain # every texture found in the folder
//         subtype: terrain       # any CellTextureEntry key overrides the defaults
//       - folder: Roads
//         category: sidewalk
//         recursive: false       # true: also scan the sub folders
//         custom:                # per texture overrides, keyed by texture id
//           w_F3_B3_sep:
//             dimsM: [1, 1]
//       - file: Test/pattern01.png   # the explicit form still works
//         type: test
//         ...
//
// Texture id = folder path + "/" + texture name (+ the sub folder path when
// recursive), e.g. "Roads/w_F3_B3_sep". The keys of `custom` are relative to
// the folder ("w_F3_B3_sep"); the full id is accepted too. Map files given in a
// custom entry (file, normal, arm, ao, rough, metal, reflect, disp) are
// relative to the folder as well.
//
// Priority of a field, lowest first: defaults < folder entry < custom entry.
//
// The files of a folder are grouped by texture name: the name of a file is its
// stem minus the map suffix and the resolution suffix, case insensitive:
//   diffuse : _diff _diffuse _color _colour _albedo _basecolor _col
//   normal  : _nor_gl _normalgl _normal_gl _normal _nor _nrm _norm
//             (_nor_dx _normaldx _normal_dx too, but the GL ones win)
//   arm     : _arm _orm
//   ao      : _ao _ambientocclusion _ambient_occlusion _occlusion
//   rough   : _rough _roughness
//   metal   : _metal _metallic _metalness
//   reflect : _refl _reflect _reflection _spec _specular
//   disp    : _disp _displacement _height
//   (ignored, not textures: _opacity _alpha _mask _bump _emissive _gloss ...)
// followed by an optional resolution: _1k _2k _4k _8k _16k, or the AmbientCG
// form "Name_2K-PNG_Color.png". A file with no recognised map suffix is a
// diffuse map named after the whole stem ("<texture_id>.png"). When several
// resolutions exist the highest one (up to max_res) is used. Extensions:
// png jpg jpeg exr tga bmp hdr tif tiff webp.
//
// The result is deterministic (textures sorted by id), as the atlas layout is
// computed from the order of the content: PCGen and ArgusWorldBuilder must get
// the same one from the same folders.
// ---------------------------------------------------------------------------

enum class TextureMapKind : U8 {
    diffuse,
    normal,
    arm,
    ao,
    rough,
    metal,
    reflect,
    disp,
    ignored, // known map that is not used (opacity, bump...)
};

struct TextureFileName {
    String baseId;                 // texture name, without map and resolution suffix
    TextureMapKind kind{TextureMapKind::diffuse};
    bool recognized{false};        // a map suffix was found
    I32 resolutionK{0};            // 0 when the name has none
    I32 rank{0};                   // among the files of a kind: GL > plain > DX normals
};

// Splits a file name (extension excluded) as described above.
[[nodiscard]] auto parse_texture_file_name(const String& stem) -> TextureFileName;

// Lists the files of a folder, as paths relative to it with '/' separators.
// The default one walks the file system; a host without std::filesystem
// support can provide its own.
using TextureFileLister =
    std::function<Vector<String>(const String& folder, bool recursive)>;
[[nodiscard]] auto list_texture_folder_files(const String& folder, bool recursive)
    -> Vector<String>;

// Returns `cfg` with the `defaults` applied and every `folder` entry replaced
// by the explicit entries found in textureDir/<folder>: the format
// CellTextureAtlasDesc reads.
[[nodiscard]] auto
expand_texture_atlas_config(const Json& cfg, const String& textureDir,
                            const TextureFileLister& lister = list_texture_folder_files)
    -> Json;

// expand_texture_atlas_config + the conversion to a CellTextureAtlasDesc. Use
// this where `CellTextureAtlasDesc desc = cfg["cells_material"]` was used.
[[nodiscard]] auto
load_texture_atlas_desc(const Json& cfg, const String& textureDir,
                        const TextureFileLister& lister = list_texture_folder_files)
    -> CellTextureAtlasDesc;

} // namespace nv

#endif
