
#include <nvk/resource/ResourceManager.h>
#include <nvk/utils.h>

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

} // namespace nv
