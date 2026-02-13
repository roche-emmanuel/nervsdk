
#include <nvk/resource/ResourceManager.h>
#include <nvk/utils.h>

#include <yaml-cpp/yaml.h>

#include <charconv>
#include <codecvt>

namespace YAML {
#if !NV_USE_STD_MEMORY
template <> struct convert<nv::String> {
    static auto encode(const nv::String& rhs) -> Node {
        Node node(rhs.c_str());
        return node;
    }

    static auto decode(const Node& node, nv::String& rhs) -> bool {
        if (!node.IsScalar()) {
            return false;
        }
        rhs = node.as<std::string>().c_str();
        return true;
    }
};
#endif
} // namespace YAML

namespace fs = std::filesystem;

#define BUFSIZE 4096

namespace nv {

// Base64 encoding table
static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "0123456789+/";

// Helper function to check if a character is base64
namespace {

auto is_base64(unsigned char c) -> bool {
    return ((isalnum(c) != 0) || (c == '+') || (c == '/'));
}

} // namespace

auto base64_decode(const String& encoded_string) -> Vector<U8> {
    size_t in_len = encoded_string.size();
    size_t i = 0;
    size_t j = 0;
    size_t in_ = 0;
    unsigned char char_array_4[4];
    unsigned char char_array_3[3];
    Vector<U8> ret;

    while (in_len-- && (encoded_string[in_] != '=') &&
           is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_];
        in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++) {
                char_array_4[i] = static_cast<unsigned char>(
                    std::strchr(base64_chars, char_array_4[i]) - base64_chars);
            }

            char_array_3[0] =
                (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) +
                              ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; i < 3; i++) {
                ret.push_back(char_array_3[i]);
            }
            i = 0;
        }
    }

    if (i) {
        for (j = 0; j < i; j++) {
            char_array_4[j] = static_cast<unsigned char>(
                std::strchr(base64_chars, char_array_4[j]) - base64_chars);
        }

        char_array_3[0] =
            (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] =
            ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

        for (j = 0; j < i - 1; j++) {
            ret.push_back(char_array_3[j]);
        }
    }

    return ret;
}

auto base64_encode(const U8* data, size_t len) -> String {
    String ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (len--) {
        char_array_3[i++] = *(data++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) +
                              ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) +
                              ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++) {
                ret += base64_chars[char_array_4[i]];
            }
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++) {
            char_array_3[j] = '\0';
        }

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] =
            ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] =
            ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; j < i + 1; j++) {
            ret += base64_chars[char_array_4[j]];
        }

        while (i++ < 3) {
            ret += '=';
        }
    }

    return ret;
}

// Convenience overload for Vector<U8>
auto base64_encode(const Vector<U8>& data) -> String {
    return base64_encode(data.data(), data.size());
}

void sleep_s(U32 secs) {
    std::this_thread::sleep_for(std::chrono::seconds(secs));
}

void sleep_ms(U32 msecs) {
    std::this_thread::sleep_for(std::chrono::milliseconds(msecs));
}

void sleep_us(U32 usecs) {
    std::this_thread::sleep_for(std::chrono::microseconds(usecs));
}

auto system_dir_exists(const char* path) -> bool {
    // Check if path exists and is a directory
    return std::filesystem::exists(path) && std::filesystem::is_directory(path);
}

auto system_file_exists(const char* fname) -> bool {
    // cf.
    // http://stackoverflow.com/questions/268023/what-s-the-best-way-to-check-if-a-file-exists-in-c-cross-platform
    return std::filesystem::exists(fname) &&
           std::filesystem::is_regular_file(fname);
}

auto read_virtual_file(const String& fname, bool forceAllowSystem) -> String {
    return ResourceManager::instance().read_virtual_file(fname,
                                                         forceAllowSystem);
}

auto read_virtual_binary_file(const String& fname, bool forceAllowSystem)
    -> U8Vector {
    return ResourceManager::instance().read_virtual_binary_file(
        fname, forceAllowSystem);
}

// write file content as string:
void write_file(const char* fname, const String& content, bool createFolders) {
    if (createFolders) {
        auto folder = get_parent_folder(fname);
        NVCHK(create_folders(folder), "Could not create folder {}", folder);
    }
    std::ofstream t(fname, std::ios::binary);
    NVCHK(t.is_open(), "Cannot write file {}", fname);

    t.write(content.data(), static_cast<std::streamsize>(content.size()));
}

void write_binary_file(const char* fname, const U8Vector& content,
                       bool createFolders) {
    if (createFolders) {
        auto folder = get_parent_folder(fname);
        NVCHK(create_folders(folder), "Could not create folder {}", folder);
    }
    std::ofstream t(fname, std::ios::out | std::ios::binary);
    NVCHK(t.is_open(), "Cannot write file {}", fname);

    t.write(reinterpret_cast<const char*>(content.data()), content.size());
    t.close();
}

void remove_file(const char* fname) {
    if (!system_file_exists(fname)) {
        logWARN("Cannot remove non existing file {}", fname);
        return;
    }

    int ret = remove(fname);
    NVCHK(ret == 0, "Could not remove file {} properly.", fname);
}

void replace_all(String& str, const String& old_value,
                 const String& new_value) {
    size_t pos = 0;
    while ((pos = str.find(old_value, pos)) != String::npos) {
        str.replace(pos, old_value.length(), new_value);
        pos += new_value.length();
    }
}

auto is_absolute_path(const String& path) -> bool {
    return std::filesystem::path(path).is_absolute();
}

auto get_absolute_path(const String& path) -> String {
#ifdef _WIN32
    DWORD retval = 0;
    std::array<char, BUFSIZE> buffer{0};
    char** lppPart = {nullptr};

    retval = GetFullPathName(path.c_str(), BUFSIZE, buffer.data(), lppPart);

    NVCHK(retval != 0, "Cannot build fullpath variable.");

    String result = buffer.data();
    return result;
#else
    char resolved_path[PATH_MAX];
    realpath(path.c_str(), resolved_path);
    return String(resolved_path);
#endif
}

// Read file content as string:
auto read_system_file(const char* fname) -> String {
    std::ifstream t(fname, std::ios::in | std::ios::binary);
    String res;
    NVCHK(t.is_open(), "File {} doesn't exist.", fname);

    t.seekg(0, std::ios::end);
    auto size = t.tellg();
    // res.reserve(size); // Should not use reserve but resize instead here.
    res.resize(size);
    t.seekg(0, std::ios::beg);

    // logDEBUG("Reading file "<<fname<<", size: "<<size);
    t.read(res.data(), size);
    return res;
}

auto read_system_binary_file(const char* fname) -> U8Vector {
    std::ifstream t(fname, std::ios::in | std::ios::binary);
    U8Vector res;
    NVCHK(t.is_open(), "File {} doesn't exist.", fname);

    t.seekg(0, std::ios::end);
    auto size = t.tellg();
    res.resize(size);
    t.seekg(0, std::ios::beg);

    // logDEBUG("Reading file "<<fname<<", size: "<<size);
    t.read(reinterpret_cast<char*>(res.data()), size);
    return res;
}

// Expand brace patterns like {yml,json} into multiple patterns
static auto expand_braces(const String& pattern) -> Vector<String> {
    Vector<String> results;

    // Find first opening brace
    size_t brace_start = pattern.find('{');
    if (brace_start == String::npos) {
        // No braces, return pattern as-is
        results.push_back(pattern);
        return results;
    }

    // Find matching closing brace
    size_t brace_end = pattern.find('}', brace_start);
    if (brace_end == String::npos) {
        // Unmatched brace, return pattern as-is
        logWARN("Unmatched brace in pattern: {}", pattern);
        results.push_back(pattern);
        return results;
    }

    // Extract the parts
    String prefix = pattern.substr(0, brace_start);
    String suffix = pattern.substr(brace_end + 1);
    String options =
        pattern.substr(brace_start + 1, brace_end - brace_start - 1);

    // Split options by comma
    Vector<String> parts;
    size_t start = 0;
    size_t pos = 0;

    while ((pos = options.find(',', start)) != String::npos) {
        parts.push_back(options.substr(start, pos - start));
        start = pos + 1;
    }
    parts.push_back(options.substr(start));

    // Generate all combinations
    for (const auto& part : parts) {
        String expanded = prefix + part + suffix;

        // Recursively expand in case there are more braces
        auto sub_results = expand_braces(expanded);
        results.insert(results.end(), sub_results.begin(), sub_results.end());
    }

    return results;
}

auto glob_to_regex(const String& pattern) -> String {
    String regex_pattern;
    regex_pattern.reserve(pattern.size() * 2);

    for (size_t i = 0; i < pattern.size(); ++i) {
        char c = pattern[i];
        switch (c) {
        case '*':
            // Check if it's ** (for recursive matching)
            if (i + 1 < pattern.size() && pattern[i + 1] == '*') {
                // Check if ** is surrounded by path separators or boundaries
                // Pattern like "/**/file" or "dir/**/*.ext"
                bool prev_is_sep = (i > 0 && (pattern[i - 1] == '/' ||
                                              pattern[i - 1] == '\\'));
                bool next_is_sep =
                    (i + 2 < pattern.size() &&
                     (pattern[i + 2] == '/' || pattern[i + 2] == '\\'));

                if (prev_is_sep && next_is_sep) {
                    // /**/ should match zero or more directories
                    // Use (|.*/) to match either nothing or any path with
                    // trailing /
                    regex_pattern += "(|.*/?)";
                    ++i; // Skip the second *
                    ++i; // Skip the separator after **
                } else {
                    // Just ** without proper separators, treat as .*
                    regex_pattern += ".*";
                    ++i; // Skip the second *
                }
            } else {
                regex_pattern +=
                    "[^/\\\\]*"; // Match anything except path separators
            }
            break;
        case '?':
            regex_pattern +=
                "[^/\\\\]"; // Match single char except path separators
            break;
        case '.':
        case '+':
        case '^':
        case '$':
        case '(':
        case ')':
        case '[':
        case ']':
        case '{':
        case '}':
        case '|':
        case '\\':
            regex_pattern += '\\';
            regex_pattern += c;
            break;
        default:
            regex_pattern += c;
            break;
        }
    }

    return regex_pattern;
}

static auto expand_single_wildcard(const String& sourceDir,
                                   const String& pattern) -> Set<String> {
    Set<String> matched_files;

    // Normalize path separators to forward slashes
    String base_dir = sourceDir;
    std::ranges::replace(base_dir, '\\', '/');

    String normalized_pattern = get_path(base_dir, pattern);
    std::ranges::replace(normalized_pattern, '\\', '/');

    // Check if pattern contains wildcards
    if (normalized_pattern.find('*') == String::npos &&
        normalized_pattern.find('?') == String::npos) {
        // No wildcards, return as-is
        matched_files.insert(pattern);
        return matched_files;
    }

    // Determine if we should use recursive search
    bool recursive = (normalized_pattern.find("**") != String::npos);

    // Find base directory - the last static path before any wildcard
    size_t first_wildcard = normalized_pattern.find_first_of("*?");

    if (first_wildcard != String::npos) {
        size_t dir_separator =
            normalized_pattern.find_last_of('/', first_wildcard);
        if (dir_separator != String::npos) {
            base_dir = normalized_pattern.substr(0, dir_separator);
        }
    }

    // Convert the full pattern to regex for matching
    String regex_str = glob_to_regex(normalized_pattern);
    std::regex pattern_regex(regex_str);

    auto process_entry = [&](const fs::directory_entry& entry) {
        if (entry.is_regular_file()) {
            String file_path = entry.path().string();
            // Normalize path separators
            std::ranges::replace(file_path, '\\', '/');

            // Match against the pattern
            if (std::regex_match(file_path, pattern_regex)) {
                matched_files.insert(file_path);
            }
        }
    };

    try {
        // Check if base directory exists
        if (!fs::exists(base_dir)) {
            logWARN("Directory {} does not exist for pattern {}", base_dir,
                    pattern);
            return matched_files;
        }

        // For recursive patterns, we need to also check files in the base
        // directory
        if (recursive) {
            // First check files directly in base_dir
            try {
                for (const auto& entry : fs::directory_iterator(base_dir)) {
                    process_entry(entry);
                }
            } catch (const fs::filesystem_error& e) {
                // Continue even if we can't read base directory
            }

            // Then check subdirectories recursively
            try {
                for (const auto& entry :
                     fs::recursive_directory_iterator(base_dir)) {
                    // Skip the base directory itself as we already processed it
                    if (entry.path() != fs::path(base_dir)) {
                        process_entry(entry);
                    }
                }
            } catch (const fs::filesystem_error& e) {
                logERROR("Filesystem error while expanding pattern {}: {}",
                         pattern, e.what());
            }
        } else {
            // Non-recursive: just iterate the base directory
            for (const auto& entry : fs::directory_iterator(base_dir)) {
                process_entry(entry);
            }
        }

        if (matched_files.empty()) {
            logDEBUG("No files matched pattern: {}", pattern);
        }
        // else {
        //     logDEBUG("Pattern {} matched {} files", pattern,
        //              matched_files.size());
        // }

    } catch (const fs::filesystem_error& e) {
        logERROR("Filesystem error while expanding pattern {}: {}", pattern,
                 e.what());
    }

    return matched_files;
}

// Main entry point that handles brace expansion first
auto expand_files_wildcard(const String& sourceDir, const String& pattern)
    -> Set<String> {
    Set<String> all_matched_files;

    // First expand braces
    auto expanded_patterns = expand_braces(pattern);

    // Then expand wildcards for each pattern
    for (const auto& expanded_pattern : expanded_patterns) {
        auto matched = expand_single_wildcard(sourceDir, expanded_pattern);
        all_matched_files.insert(matched.begin(), matched.end());
    }

    return all_matched_files;
}

namespace {
auto remove_comments(const String& input) -> String {
    String cleaned = input;

    // Remove C++ style comments
    // cleaned = std::regex_replace(cleaned, std::regex(R"(//[^\n]*\n)"), "\n");
    cleaned = std::regex_replace(cleaned, std::regex("//.*?(\\r?\\n|$)"), "\n");

    // Remove C style comments
    // cleaned = std::regex_replace(cleaned, std::regex(R"(/\*.*?\*/)"), " ");
    // below with multilines support:
    cleaned = std::regex_replace(cleaned, std::regex(R"(/\*[\s\S]*?\*/)"), " ");

    // Remove trailing commas
    cleaned =
        std::regex_replace(cleaned, std::regex(R"(,(\s*)([\]\}]))"), "$1$2");

    return cleaned;
};
} // namespace

auto read_json_string(const String& content) -> Json {
    Json data;
    auto cleaned = remove_comments(content);

    try {
        // Parse
        data = Json::parse(cleaned, nullptr, true, false);
    } catch (Json::parse_error& e) {
        logERROR("Error parsing JSON content:\n{}", cleaned);
        THROW_MSG("JSON parse error: {}", e.what());
    }

    return data;
};

auto read_json_file(const String& fname, bool forceAllowSystem) -> Json {
    auto content = read_virtual_file(fname, forceAllowSystem);
    return read_json_string(content);
};

auto read_ordered_json_string(const String& content) -> OrderedJson {
    OrderedJson data;
    auto cleaned = remove_comments(content);

    try {
        // Parse
        data = OrderedJson::parse(cleaned, nullptr, true, false);
    } catch (OrderedJson::parse_error& e) {
        logERROR("Error parsing JSON content:\n{}", cleaned);
        THROW_MSG("JSON parse error: {}", e.what());
    }
    return data;
}

auto read_ordered_json_file(const String& fname, bool forceAllowSystem)
    -> OrderedJson {
    auto content = read_virtual_file(fname, forceAllowSystem);
    return read_ordered_json_string(content);
}

void write_json_file(const char* fname, const Json& content, I32 indent) {
    // Write JSON to file
    std::ofstream file(fname);
    NVCHK(file.is_open(), "Cannot open file {} for writing", fname);
    // The number 4 specifies indentation for pretty printing:
    // file << content.dump(2);
    // file << content.dump(2);
    file << content.dump(indent);
    file.close();

    // Alternative for better performance (without pretty printing):
    // file << content;
};

static auto yaml_to_json(const YAML::Node& node) -> Json {
    switch (node.Type()) {
    case YAML::NodeType::Null:
        return nullptr;

    case YAML::NodeType::Scalar: {
        bool bool_val;
        long long int_val;
        double double_val;

        // Try conversions in order
        if (YAML::convert<bool>::decode(node, bool_val)) {
            return bool_val;
        } else if (YAML::convert<I64>::decode(node, int_val)) {
            return int_val;
        } else if (YAML::convert<F64>::decode(node, double_val)) {
            return double_val;
        } else {
            // Default to string
            return node.as<std::string>();
        }
    }

    case YAML::NodeType::Sequence: {
        Json result = Json::array();
        result.get_ptr<Json::array_t*>()->reserve(node.size());
        for (const auto& item : node) {
            result.push_back(yaml_to_json(item));
        }
        return result;
    }

    case YAML::NodeType::Map: {
        Json result = Json::object();
        for (const auto& pair : node) {
            result[pair.first.as<std::string>()] = yaml_to_json(pair.second);
        }
        return result;
    }

    case YAML::NodeType::Undefined:
    default:
        return nullptr;
    }
}

auto read_yaml_string(const String& content) -> Json {
    try {
        return yaml_to_json(YAML::Load(content));
    } catch (const std::exception& e) {
        THROW_MSG("read_yaml_file: Failed to load YAML string: {}", e.what());
    }
    return {};
}

auto read_yaml_file(const String& fname, bool forceAllowSystem) -> Json {
    auto content = read_virtual_file(fname, forceAllowSystem);
    return read_yaml_string(content);
};

auto get_file_extension(const char* filename) -> String {
    // Returns the file extension with the "." included, like ".jpg" or ".png"
    if (filename == nullptr)
        return "";

    String name_str(filename);
    size_t last_dot = name_str.find_last_of('.');
    size_t last_sep = name_str.find_last_of("/\\"); // Handle Windows/Unix paths

    // Make sure the dot is after the last slash (i.e., part of the filename,
    // not the path)
    if (last_dot != String::npos &&
        (last_sep == String::npos || last_dot > last_sep)) {
        return name_str.substr(last_dot);
    }
    return "";
}

auto is_json_file(const char* filename) -> bool {
    // Check the extension of the file and returns true if this is ".json"
    auto ext = to_lower(get_file_extension(filename));
    return ext == ".json";
}

auto is_yaml_file(const char* filename) -> bool {
    // Check the extension of the file and returns true if this is ".yaml" or
    // ".yml"
    auto ext = to_lower(get_file_extension(filename));
    return ext == ".yaml" || ext == ".yml";
}

auto is_yaml_file(const String& filename) -> bool {
    return is_yaml_file(filename.c_str());
};

auto is_json_file(const String& filename) -> bool {
    return is_json_file(filename.c_str());
};

auto to_upper(const String& str) -> String {
    String result = str;
    std::ranges::transform(result, result.begin(), ::toupper);
    return result;
}

auto to_lower(const String& str) -> String {
    String result = str;
    std::ranges::transform(result, result.begin(), ::tolower);
    return result;
}

auto read_config_file(const String& fname, bool forceAllowSystem) -> Json {
    if (is_json_file(fname)) {
        return read_json_file(fname, forceAllowSystem);
    }

    if (is_yaml_file(fname)) {
        return read_yaml_file(fname, forceAllowSystem);
    }

    THROW_MSG("Unsupport config file format: {}", fname);
    return {};
}

auto create_folders(const String& fullpath) -> bool {
    if (fullpath.empty() || system_dir_exists(fullpath.c_str())) {
        return true;
    }
    return std::filesystem::create_directories(fullpath.c_str());
}

auto get_path_extension(const String& fname) -> String {

    // Find the position of the last dot (.)
    std::size_t dotPos = fname.find_last_of('.');

    // && dotPos != 0
    if (dotPos != String::npos && dotPos < fname.length() - 1) {
        // Extract the substring after the dot (including the dot)
        return fname.substr(dotPos);
    }

    // No extension found
    return "";
}

auto get_parent_folder(const char* fname) -> String {
    std::filesystem::path filePath(fname);
    std::filesystem::path parentPath = filePath.parent_path();

#if NV_USE_STD_MEMORY
    return parentPath.string();
#else
    return parentPath.string<char, std::char_traits<char>,
                             STLAllocator<char, DefaultPoolAllocator>>();
#endif
}

auto get_parent_folder(const String& fname) -> String {
    return get_parent_folder(fname.c_str());
}

auto get_cwd() -> String {
    auto cwd = std::filesystem::current_path();
    return toString(cwd.string());
}

auto toString(const std::wstring& wstr) -> String {
#if NV_USE_STD_MEMORY
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
#else
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t,
                         std::allocator<wchar_t>,
                         STLAllocator<char, DefaultPoolAllocator>>
        converter;
#endif
    return converter.to_bytes(wstr);
}

auto toString(const std::string& str) -> String {
    return {str.data(), str.size()};
}

#ifdef _WIN32
namespace {
auto get_win_home_dir() -> String {
    const char* home_drive = std::getenv("HOMEDRIVE");
    const char* home_path = std::getenv("HOMEPATH");
    NVCHK(home_drive != nullptr && home_path != nullptr,
          "Invalid windows home drive or path");
    return String(home_drive) + String(home_path);
}
} // namespace

#endif

auto get_home_dir() -> String {

    const char* var = std::getenv("HOME");
#ifdef _WIN32
    if (var == nullptr) {
        // We could be in a windows batch environment here:
        return get_win_home_dir();
    }
#endif

    NVCHK(var != nullptr, "Invalid home directory");
    return var;
}

auto copy_file(const char* source_path, const char* dest_path, U32 buffer_size,
               bool createFolders) -> bool {
    // Check if source file exists
    NVCHK(system_file_exists(source_path), "Source file doesn't exist: {}",
          source_path);

    // Open source file in binary mode
    std::ifstream source(source_path, std::ios::binary);
    NVCHK(source.is_open(), "Could not open source file: {}", source_path);

    auto folder = get_parent_folder(dest_path);
    if (!system_dir_exists(folder.c_str())) {
        if (createFolders) {
            if (!create_folders(folder)) {
                logERROR("Could not create folder {}", folder);
                return false;
            };
        } else {
            THROW_MSG("Parent folder {} doesn't exist.", folder);
        }
    }

    // Open destination file in binary mode
    std::ofstream dest(dest_path, std::ios::binary | std::ios::trunc);
    NVCHK(dest.is_open(), "Could not open destination file: {}", dest_path);

    try {
        // Create buffer
        std::vector<char> buffer(buffer_size);

        // Copy data in chunks
        while (!source.eof()) {
            // Read a chunk
            source.read(buffer.data(), buffer.size());
            std::streamsize bytes_read = source.gcount();

            // Write the chunk
            if (bytes_read > 0) {
                dest.write(buffer.data(), bytes_read);
                if (dest.fail()) {
                    THROW_MSG("Failed to write to destination file");
                }
            }
        }

        // Close files
        source.close();
        dest.close();

        // Preserve file attributes and timestamps
        try {
            std::filesystem::permissions(
                dest_path, std::filesystem::status(source_path).permissions(),
                std::filesystem::perm_options::replace);

            std::filesystem::last_write_time(
                dest_path, std::filesystem::last_write_time(source_path));
        } catch (const std::filesystem::filesystem_error& e) {
            logWARN("Could not preserve file attributes: {}", e.what());
            // Not a fatal error, continue
        }

        return true;
    } catch (const std::exception& e) {
        source.close();
        dest.close();

        // Clean up the partially written destination file
        try {
            if (std::filesystem::exists(dest_path)) {
                std::filesystem::remove(dest_path);
            }
        } catch (...) {
            // Ignore cleanup errors
        }

        THROW_MSG("Error during file copy: {}", e.what());
        return false;
    }
}

auto copy_file(const String& source_path, const String& dest_path,
               U32 buffer_size, bool createFolders) -> bool {
    return copy_file(source_path.c_str(), dest_path.c_str(), buffer_size,
                     createFolders);
};

auto get_files(const String& directory, bool recursive) -> Vector<String> {
    Vector<String> files;

    if (recursive) {
        for (const auto& entry :
             std::filesystem::recursive_directory_iterator(directory)) {
            if (std::filesystem::is_regular_file(entry)) {
                files.push_back(entry.path().string().c_str());
            }
        }
    } else {
        for (const auto& entry :
             std::filesystem::directory_iterator(directory)) {
            if (std::filesystem::is_regular_file(entry)) {
                files.push_back(entry.path().string().c_str());
            }
        }
    }

    return files;
}

auto get_files(const String& directory, const std::regex& pattern,
               bool recursive) -> Vector<String> {
    Vector<String> files;

    if (recursive) {
        for (const auto& entry :
             std::filesystem::recursive_directory_iterator(directory)) {
            if (std::filesystem::is_regular_file(entry)) {
                const auto& str = entry.path().string();
                if (std::regex_match(str, pattern)) {
                    files.push_back(str.c_str());
                }
            }
        }
    } else {
        for (const auto& entry :
             std::filesystem::directory_iterator(directory)) {
            if (std::filesystem::is_regular_file(entry)) {
                const auto& str = entry.path().string();
                if (std::regex_match(str, pattern)) {
                    files.push_back(str.c_str());
                }
            }
        }
    }

    return files;
}

auto make_extensions_regex(const Vector<String>& extensions) -> std::regex {
    std::string pattern = ".*(?:";

    for (size_t i = 0; i < extensions.size(); ++i) {
        std::string ext = extensions[i].c_str();
        NVCHK(!ext.empty(), "Cannot handle empty extension");

        // If the extension starts with a dot, escape it for the regex
        if (ext[0] == '.') {
            pattern += "\\" + ext;
        } else {
            pattern += "\\." + ext;
        }

        if (i < extensions.size() - 1) {
            pattern += "|";
        }
    }

    pattern += ")$";

    return std::regex(pattern, std::regex::icase);
}

auto virtual_file_exists(const String& fname, bool forceAllowSystem) -> bool {
    return ResourceManager::instance().virtual_file_exists(fname,
                                                           forceAllowSystem);
}

auto get_file_last_write_time(const String& fname) -> std::time_t {
    return ResourceManager::instance().get_file_last_write_time(fname);
}

auto get_virtual_files(const String& directory, const std::regex& pattern,
                       bool recursive) -> Vector<String> {
    return ResourceManager::instance().get_files(directory, pattern, recursive);
};

auto get_filename(const String& full_path) -> String {
    return std::filesystem::path(full_path).filename().string();
}

auto toHex(const U8Vector& data) -> String {
    static const char hexChars[] = "0123456789abcdef";

    String result;
    result.reserve(data.size() * 2);

    for (U8 byte : data) {
        result.push_back(hexChars[byte >> 4]);
        result.push_back(hexChars[byte & 0x0F]);
    }

    return result;
};

auto fromHex(const String& hex) -> U8Vector {
    U8Vector bytes;

    NVCHK(hex.length() % 2 == 0,
          "Hex string must have an even number of characters");

    bytes.reserve(hex.length() / 2);

    auto hexCharToValue = [](char c) -> int {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        return -1; // Invalid
    };

    for (size_t i = 0; i < hex.length(); i += 2) {
        int high = hexCharToValue(hex[i]);
        int low = hexCharToValue(hex[i + 1]);

        NVCHK(high >= 0 && low >= 0, "Invalid hex character in string");

        auto byte = static_cast<uint8_t>((high << 4) | low);
        bytes.push_back(byte);
    }

    return bytes;
};

} // namespace nv
