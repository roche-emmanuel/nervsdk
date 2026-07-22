
#include <nvk/resource/ResourceManager.h>
#include <nvk/utils.h>

#include <yaml-cpp/yaml.h>

#include <charconv>
#include <codecvt>

#include <external/stb/stb_image.h>

#include <cstddef>

#if defined(__EMSCRIPTEN__)
#include <emscripten/heap.h>
#include <malloc.h>
#endif

#if defined(_WIN32)
#include <psapi.h>
#include <windows.h>
#pragma comment(lib, "psapi.lib")

#elif defined(__APPLE__)
#include <mach/mach.h>

#elif defined(__linux__)
#include <fstream>
#include <string>

#endif

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

auto get_current_rss() -> U64 {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    pmc.cb = sizeof(pmc);
    if (!GetProcessMemoryInfo(GetCurrentProcess(),
                              reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                              sizeof(pmc)))
        return 0;
    return pmc.WorkingSetSize;

#elif defined(__APPLE__)
    mach_task_basic_info info{};
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  reinterpret_cast<task_info_t>(&info), &count) != KERN_SUCCESS)
        return 0;
    return info.resident_size;

#elif defined(__EMSCRIPTEN__)
    // return static_cast<U64>(emscripten_get_heap_size() -
    //                         emscripten_get_free_buffer_size());

    struct mallinfo mi = mallinfo();
    return static_cast<U64>(mi.uordblks);

#elif defined(__linux__)
    // /proc/self/smaps_rollup gives PSS — proportional set size.
    // Unlike RSS, shared pages (libc, etc.) are divided by the number of
    // processes sharing them, so this reflects your process's true RAM cost.
    // smaps_rollup is a pre-summed single-read file (kernel 4.14+);
    // it's far cheaper than parsing the full /proc/self/smaps.
    std::ifstream f("/proc/self/smaps_rollup");
    std::string line;
    while (std::getline(f, line)) {
        if (line.compare(0, 4, "Pss:") == 0)
            return std::stoul(line.substr(4)) * 1024; // kB → bytes
    }
    return 0;

#else
    return 0;
#endif
}

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

std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty())
        return std::string();

    int size_needed = WideCharToMultiByte(
        CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0],
                        size_needed, NULL, NULL);
    return strTo;
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

auto system_path_exists(const char* path) -> bool {
    // Check if path exists and is a directory
    return std::filesystem::exists(path);
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

auto read_virtual_file_async(const String& fname, bool forceAllowSystem)
    -> Promise<String> {
    return ResourceManager::instance().read_virtual_file_async(
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
    return std::filesystem::absolute(path).string();
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
    if (normalized_pattern.find_first_of("*?") == String::npos) {
        matched_files.insert(normalized_pattern);
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
        I64 int_val;
        F64 double_val;

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
    auto cleanPath = normalized_path(fullpath, true, PATHSEP_NONE);

    if (cleanPath.empty() || system_dir_exists(cleanPath.c_str())) {
        return true;
    }
    return std::filesystem::create_directories(cleanPath.c_str());
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

void normalize_path(String& path, bool isFolder, PathSep sep) {
    if (path.empty()) {
        return;
    }

    // Replace all separators and add trailing if needed
    if (sep == PATHSEP_LINUX) {
        for (char& c : path) {
            if (c == '\\')
                c = '/';
        }
        NVCHK(isFolder || path.back() != '/',
              "Invalid path ending for file: {}", path);

        if (isFolder && path.back() != '/') {
            path += '/';
        }
    } else if (sep == PATHSEP_WIN) {
        for (char& c : path) {
            if (c == '/')
                c = '\\';
        }
        NVCHK(isFolder || path.back() != '\\',
              "Invalid path ending for file: {}", path);

        if (isFolder && path.back() != '\\') {
            path += '\\';
        }
    } else if (sep == PATHSEP_NONE) {
        // Remove trailing separators
        while (!path.empty() && (path.back() == '/' || path.back() == '\\')) {
            path.pop_back();
        }
    }
}

auto get_parent_folder(const char* fname, PathSep sep) -> String {
    std::filesystem::path filePath(fname);
    std::filesystem::path parentPath = filePath.parent_path();

#if NV_USE_STD_MEMORY
    String pathStr = parentPath.string();
#else
    String pathStr =
        parentPath.string<char, std::char_traits<char>,
                          STLAllocator<char, DefaultPoolAllocator>>();
#endif

    normalize_path(pathStr, true, sep);
    return pathStr;
}

auto get_parent_folder(const String& fname, PathSep sep) -> String {
    return get_parent_folder(fname.c_str(), sep);
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
                files.emplace_back(entry.path().string().c_str());
            }
        }
    } else {
        for (const auto& entry :
             std::filesystem::directory_iterator(directory)) {
            if (std::filesystem::is_regular_file(entry)) {
                files.emplace_back(entry.path().string().c_str());
            }
        }
    }

    // Avoid duplicates:
    std::sort(files.begin(), files.end());
    files.erase(std::unique(files.begin(), files.end()), files.end());

    return files;
}

auto get_files(const String& directory, const std::regex& pattern,
               bool recursive) -> Vector<String> {
    Vector<String> files;

    if (!system_dir_exists(directory.c_str())) {
        // Nothing to search:
        return files;
    }

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

    // Avoid duplicates:
    std::sort(files.begin(), files.end());
    files.erase(std::unique(files.begin(), files.end()), files.end());

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

auto get_filename(const String& full_path, bool withExt) -> String {
    auto path = std::filesystem::path(full_path);
    if (withExt) {
        return path.filename().string();
    }
    return path.stem().string();
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

auto get_relative_path(const String& filepath, const String& parent) -> String {
    namespace fs = std::filesystem;

    fs::path file_path(filepath);
    fs::path parent_path(parent);

    // Get relative path
    fs::path relative = fs::relative(file_path, parent_path);

    auto res = relative.string();
    std::ranges::replace(res, '\\', '/');
    return res;
}

auto fluToLLA(const Vec3d& xyz, F64 radius) -> Vec3d {

    Vec3d vec{xyz};
    F64 R = vec.normalize();

    F64 phi = asin(vec.z());
    F64 theta = atan2(-vec.y(), -vec.x());
    return {phi, theta, R - radius};
}

auto llaToFLU(const Vec3d& lla, F64 radius) -> Vec3d {

    F64 sinlat = sin(lla.x());
    F64 coslat = cos(lla.x());
    F64 sinlon = sin(lla.y());
    F64 coslon = cos(lla.y());

    F64 R = radius + lla.z();
    return {-coslat * coslon * R, -coslat * sinlon * R, sinlat * R};
}

auto fruToLLA(const Vec3d& xyz, F64 radius) -> Vec3d {

    Vec3d vec{xyz};
    F64 R = vec.normalize();

    F64 phi = asin(vec.z());
    F64 theta = atan2(vec.y(), -vec.x());
    return {phi, theta, R - radius};
}

auto llaToFRU(const Vec3d& lla, F64 radius) -> Vec3d {

    F64 sinlat = sin(lla.x());
    F64 coslat = cos(lla.x());
    F64 sinlon = sin(lla.y());
    F64 coslon = cos(lla.y());

    F64 R = radius + lla.z();
    return {-coslat * coslon * R, coslat * sinlon * R, sinlat * R};
}

auto get_neu_frame(const Vec3d& pos, const Vec3d& worldUp) -> Mat4d {
    // Compute local up:
    auto up = pos.normalized();

    // Compute east dir:
    auto east = worldUp.cross(up).normalized();

    // Compute north dir:
    auto north = up.cross(east).normalized();

    return {north.x(), east.x(), up.x(),    pos.x(),  north.y(), east.y(),
            up.y(),    pos.y(),  north.z(), east.z(), up.z(),    pos.z(),
            0.0,       0.0,      0.0,       1.0};
};

auto align_element_size(U32 elemSize, U32 alignment) -> U32 {
    return (elemSize + alignment - 1) & ~(alignment - 1);
}

auto color_to_f32(const Vec4f& col) -> F32 {
    // Pack color components into bytes
    U8 bytes[4];
    bytes[0] = static_cast<U8>(std::clamp(col.x() * 255.0F, 0.0F, 255.0F));
    bytes[1] = static_cast<U8>(std::clamp(col.y() * 255.0F, 0.0F, 255.0F));
    bytes[2] = static_cast<U8>(std::clamp(col.z() * 255.0F, 0.0F, 255.0F));
    bytes[3] = static_cast<U8>(std::clamp(col.w() * 255.0F, 0.0F, 255.0F));

    // Safe type punning
    F32 res = NAN;
    std::memcpy(&res, bytes, sizeof(F32));
    return res;
}

auto normalized_path(const String& path, bool isFolder, PathSep sep) -> String {
    String result{path};
    normalize_path(result, isFolder, sep);
    return result;
};

auto get_current_module_path(PathSep sep) -> String {
#ifdef _WIN32
    // Use TCHAR to handle both ANSI and Unicode builds
    TCHAR pBuf[MAX_PATH];

    DWORD bytes = GetModuleFileName(NULL, pBuf, MAX_PATH);

    // Check for failure
    if (bytes == 0) {
        THROW_MSG("Cannot retrieve executable path.");
        return {};
    }

    // Check for truncation (buffer too small)
    if (bytes == MAX_PATH) {
        DWORD error = GetLastError();
        if (error == ERROR_INSUFFICIENT_BUFFER) {
            THROW_MSG("Executable path exceeds MAX_PATH.");
            return {};
        }
    }

    return normalized_path(pBuf, false, sep);
#else
    THROW_MSG("get_current_module_path() not supported.");
    return {};
#endif
}

auto get_current_module_folder(PathSep sep) -> String {
    auto mpath = get_current_module_path(sep);
    NVCHK(!mpath.empty(), "Invalid module path.");
    return get_parent_folder(mpath, sep);
}
auto compute_file_checksum(const String& filename, bool forceAllowSystem)
    -> U32 {
    auto content = read_virtual_file(filename, forceAllowSystem);
    return compute_data_checksum(content);
}

auto matches_pattern(const String& name, const String& pattern) -> bool {
    // Full wildcard short-circuit — avoids building a regex for the common case
    if (pattern == "*")
        return true;

    // Build an anchored regex from the glob pattern and match the full name
    String regexStr = "^" + glob_to_regex(pattern) + "$";
    return std::regex_match(name, std::regex(regexStr));
}

auto matches_any_pattern(const String& name, const Vector<String>& patterns)
    -> bool {
    for (const auto& pattern : patterns) {
        // A pattern starting with '!' is a deny rule: if the name matches
        // the rest of the pattern, immediately return false regardless of
        // any subsequent allow patterns. Order in the list matters.
        if (!pattern.empty() && pattern[0] == '!') {
            if (matches_pattern(name, pattern.substr(1)))
                return false;
        } else {
            if (matches_pattern(name, pattern))
                return true;
        }
    }
    return false;
}

auto fromHexString(const String& hex) -> String {
    nv::U8Vector bytes = nv::fromHex(hex);
    return String(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

auto toHexString(const String& s) -> String {
    return nv::toHex(
        nv::U8Vector(reinterpret_cast<const uint8_t*>(s.data()),
                     reinterpret_cast<const uint8_t*>(s.data()) + s.size()));
}

auto toUpperHexString(const String& s) -> String {
    String hex = toHexString(s);
    std::transform(hex.begin(), hex.end(), hex.begin(), ::toupper);
    return hex;
}

auto is_relative_path(const String& path) -> bool {
    return !is_absolute_path(path);
}

void move_path(const String& src, const String& dest) {
    namespace fs = std::filesystem;

    const fs::path src_path(src);
    const fs::path dest_path(dest);

    // Validate source exists
    std::error_code ec;
    if (!fs::exists(src_path, ec) || ec) {
        THROW_MSG("Source path does not exist: {}", src);
    }

    // Ensure destination parent directory exists
    const auto dest_parent = dest_path.parent_path();
    if (!dest_parent.empty() && !fs::exists(dest_parent, ec)) {
        fs::create_directories(dest_parent, ec);
        if (ec) {
            THROW_MSG("Failed to create destination directories: {}",
                      ec.message());
        }
    }

    // Attempt rename first (fast, same-filesystem move)
    fs::rename(src_path, dest_path, ec);
    if (!ec)
        return;

    // Fallback: cross-device move (copy + remove)
    if (ec.value() == EXDEV || ec == std::errc::cross_device_link) {
        if (fs::is_directory(src_path)) {
            fs::copy(src_path, dest_path,
                     fs::copy_options::recursive |
                         fs::copy_options::copy_symlinks,
                     ec);
        } else {
            fs::copy_file(src_path, dest_path,
                          fs::copy_options::overwrite_existing, ec);
        }
        if (ec) {
            THROW_MSG("Failed to copy during cross-device move: {}",
                      ec.message());
        }

        fs::remove_all(src_path, ec);
        if (ec) {
            // Non-fatal: dest was written successfully, warn or log
            // Optionally throw, depending on your semantics
        }
        return;
    }

    // Any other rename failure
    THROW_MSG("Failed to move '{}' to '{}': {}", src, dest, ec.message());
}
void remove_file(const String& fname) { remove_file(fname.c_str()); }
auto remove_file_if_exists(const String& fname) -> bool {
    if (system_file_exists(fname)) {
        remove_file(fname);
        return true;
    }
    return false;
}
void trim(String& s) {
    // Trim left
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) {
                return !std::isspace(c);
            }));

    // Trim right
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         [](unsigned char c) { return !std::isspace(c); })
                .base(),
            s.end());
}

auto trimmed(String s) -> String {
    trim(s);
    return s;
}

auto get_executable_path() -> String {
#ifdef __EMSCRIPTEN__
    // Get the application path using EM_ASM
    // String appPath =
    //     (const char*)EM_ASM_INT({ return Module['locateFile'](''); });
    String appPath = "/nvapp";
    // Print the application path
    logDEBUG("Application path: {}", appPath.c_str());
    return appPath;

#elif defined _WIN32
    std::array<char, FILENAME_MAX> pBuf{0};

    int bytes = GetModuleFileName(nullptr, pBuf.data(), FILENAME_MAX);
    if (bytes == 0) {
        logERROR("Cannot retrieve executable path.");
        return {};
    }

    String path = pBuf.data();
    auto index = path.rfind('\\');
    path = path.substr(0, index);

    // logINFO("Found VBSSim3 root path: " + path);
    return path;
#else
    // cf.
    // http://stackoverflow.com/questions/4025370/can-an-executable-discover-its-own-path-linux
    char path[PATH_MAX];
    char dest[PATH_MAX];
    // struct stat info;
    pid_t pid = getpid();
    sprintf(path, "/proc/%d/exe", pid);
    NVCHK(readlink(path, dest, PATH_MAX) != -1, "Readlink error occured.");

    String pstr = String(dest);

    int index = pstr.rfind("/");
    return pstr.substr(0, index);
#endif
}
auto get_str(const Json& obj, const String& key, const String& defVal)
    -> std::string {
    if (!obj.contains(key) || obj[key].is_null())
        return defVal;
    return obj[key].get<String>();
};
auto get_bool(const Json& obj, const String& key, bool defVal) -> bool {
    if (!obj.contains(key) || obj[key].is_null())
        return defVal;
    return obj[key].get<bool>();
};
auto get_i32(const Json& obj, const String& key, I32 defVal) -> I32 {
    if (!obj.contains(key) || obj[key].is_null())
        return defVal;
    return obj[key].get<I32>();
};
auto get_u32(const Json& obj, const String& key, U32 defVal) -> U32 {
    if (!obj.contains(key) || obj[key].is_null())
        return defVal;
    return obj[key].get<U32>();
};
auto get_f32(const Json& obj, const String& key, F32 defVal) -> F32 {
    if (!obj.contains(key) || obj[key].is_null())
        return defVal;
    return obj[key].get<F32>();
};
auto get_f64(const Json& obj, const String& key, F64 defVal) -> F64 {
    if (!obj.contains(key) || obj[key].is_null())
        return defVal;
    return obj[key].get<F64>();
};
void write_json_file(const String& fname, const Json& content, I32 indent) {
    write_json_file(fname.c_str(), content, indent);
}
auto get_image_size(const String& path, I32* numChannels) -> Vec2i {
    int w = 0, h = 0, channels = 0;
    if (stbi_info(path.c_str(), &w, &h, &channels) == 0) {
        THROW_MSG("Cannot read image header for "
                  "'{}': {}",
                  path, stbi_failure_reason());
        return {0, 0};
    }
    if (numChannels != nullptr) {
        *numChannels = channels;
    }
    return {w, h};
}

auto mix_bits64(U64 x) -> U64 {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}

auto hash_id_with_seed(U64 id, U32 seed) -> U32 {
    // Fold the seed into the 64-bit id so both influence every output bit.
    // Multiplying the seed by a large odd constant before XOR-ing spreads
    // its bits across the full 64-bit range instead of just the low 32 bits.
    U64 state = id ^ (static_cast<U64>(seed) * 0x9e3779b97f4a7c15ULL);

    U64 mixed = mix_bits64(state);

    // Fold down to 32 bits by XOR-ing the halves rather than truncating,
    // so entropy from the high bits isn't discarded.
    return static_cast<U32>(mixed ^ (mixed >> 32));
}

auto build_connected_chains(const Vector<std::pair<I32, I32>>& links)
    -> Vector<Vector<I32>> {
    // ── Map each distinct I32 value to a dense index [0..n) ────────────────
    UnorderedMap<I32, U32> valueToIdx;
    Vector<I32> idxToValue;
    valueToIdx.reserve(links.size() * 2);
    idxToValue.reserve(links.size() * 2);

    auto internValue = [&](I32 v) -> U32 {
        auto it = valueToIdx.find(v);
        if (it != valueToIdx.end())
            return it->second;
        U32 idx = U32(idxToValue.size());
        idxToValue.push_back(v);
        valueToIdx.emplace(v, idx);
        return idx;
    };

    for (const auto& [a, b] : links) {
        internValue(a);
        internValue(b);
    }

    // ── Disjoint set over dense indices ────────────────────────────────────
    struct disjointSet {
        Vector<U32> parent;
        Vector<U32> rank;

        explicit disjointSet(U32 n) {
            parent.resize(n);
            rank.resize(n, 0);
            for (U32 i = 0; i < n; ++i)
                parent[i] = i;
        }

        auto find(U32 x) -> U32 {
            while (parent[x] != x) {
                parent[x] = parent[parent[x]]; // path halving
                x = parent[x];
            }
            return x;
        }

        void unite(U32 a, U32 b) {
            U32 ra = find(a);
            U32 rb = find(b);
            if (ra == rb)
                return;
            if (rank[ra] < rank[rb])
                std::swap(ra, rb);
            parent[rb] = ra;
            if (rank[ra] == rank[rb])
                ++rank[ra];
        }
    };

    disjointSet ds(U32(idxToValue.size()));

    for (const auto& [a, b] : links)
        ds.unite(valueToIdx[a], valueToIdx[b]);

    // ── Group members by root ───────────────────────────────────────────────
    UnorderedMap<U32, Vector<I32>> clusters;
    clusters.reserve(idxToValue.size());
    for (U32 i = 0; i < idxToValue.size(); ++i)
        clusters[ds.find(i)].push_back(idxToValue[i]);

    Vector<Vector<I32>> chains;
    chains.reserve(clusters.size());
    for (auto& [root, members] : clusters)
        chains.push_back(std::move(members));

    return chains;
}

} // namespace nv
