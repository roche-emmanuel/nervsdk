
#include <nvk/resource/ResourceManager.h>
#include <nvk/utils.h>

#include <external/RTree.h>
#include <yaml-cpp/yaml.h>


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

static auto expand_single_wildcard(const String& pattern) -> Set<String> {
    Set<String> matched_files;

    // Normalize path separators to forward slashes
    String normalized_pattern = pattern;
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
    String base_dir = ".";
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
auto expand_files_wildcard(const String& pattern) -> Set<String> {
    Set<String> all_matched_files;

    // First expand braces
    auto expanded_patterns = expand_braces(pattern);

    // Then expand wildcards for each pattern
    for (const auto& expanded_pattern : expanded_patterns) {
        auto matched = expand_single_wildcard(expanded_pattern);
        all_matched_files.insert(matched.begin(), matched.end());
    }

    return all_matched_files;
}

auto read_json_string(const String& content) -> Json {
    Json data;

    try {
        // Parse with the JSON5 extension flag
        data = Json::parse(content, nullptr, true, true);
    } catch (Json::parse_error& e) {
        THROW_MSG("JSON parse error: {}", e.what());
    }

    return data;
};

auto read_json_file(const String& fname, bool forceAllowSystem) -> Json {
    auto content = read_virtual_file(fname, forceAllowSystem);
    return read_json_string(content);
};

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

} // namespace nv
