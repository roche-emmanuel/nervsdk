
#ifndef NV_RESOURCEPACKER_
#define NV_RESOURCEPACKER_

#include <nvk_common.h>

namespace nv {

// Structure for file entry in the pack
struct FileEntry {
    String name;
    String sourceFile;
    U32 offset;
    U32 originalSize;
    U32 compressedSize;
    U32 encryptedSize;
    U32 checksum;
};

// Simple implementation of a resource packer
class ResourcePacker : public RefObject {
  private:
    // Encryption key - should be embedded in your game engine
    U8Vector AES_KEY; // Should be 32 bytes
    U8Vector AES_IV;  // Should be 16 bytes

    Vector<FileEntry> fileEntries;
    String outputPath;

    I64 packageVersion{0};
    String metadata;

    // Calculate a simple checksum
    auto calculate_checksum(const U8Vector& data) -> U32;

    // Compress data using zlib
    auto compress_data(const U8Vector& input) -> U8Vector;

    // Encrypt data using AES-256
    auto encrypt_data(const U8Vector& input) -> U8Vector;

  public:
    explicit ResourcePacker(const String& outPath, const U8Vector& key,
                            const U8Vector& iv)
        : AES_KEY(key), AES_IV(iv), outputPath(outPath) {}

    // Add a file to the pack
    void add_file(const String& filePath, const String& entryName);

    // Create the pack file
    void pack();

    void set_package_version(I64 version);
    void set_metadata(const String& meta);
};

// Resource unpacker for the game engine
class ResourceUnpacker : public RefObject {
  private:
    std::ifstream packFile;
    UnorderedMap<String, FileEntry> fileTable;
    String _filename;

    U8Vector AES_KEY; // Should be 32 bytes
    U8Vector AES_IV;  // Should be 16 bytes

    I64 packageVersion{0};
    String metadata;

    // Decompress data using zlib
    void decompress_data(const U8Vector& input, U8* destData,
                         size_t originalSize);

    // Decrypt data using AES-256
    auto decrypt_data(const U8Vector& input) -> U8Vector;

    // Calculate a checksum
    template <typename Container>
    auto calculate_checksum(const Container& data) -> U32 {
        U32 checksum = 0;
        for (auto byte : data) {
            // For string, we need to cast char to unsigned to avoid sign
            // extension issues
            checksum = (checksum << 1) ^ static_cast<U8>(byte);
        }
        return checksum;
    }

  public:
    explicit ResourceUnpacker(const String& packFilePath, const U8Vector& key,
                              const U8Vector& iv);

    ~ResourceUnpacker() {
        if (packFile.is_open()) {
            packFile.close();
        }
    }

    auto get_package_version() const -> I64;
    auto get_metadata() const -> const String&;

    // Retrieve the pack filename:
    auto get_filename() const -> const String&;

    // List all files in the pack
    auto list_files() -> Vector<String>;

    // Extract a file from the pack to memory
    auto extract_file(const String& fileName) -> U8Vector;

    auto extract_file_as_string(const String& fileName) -> String;

    // Extract a file and write it to disk
    void extract_file_to_disk(const String& fileName, const String& outputPath);

    // Check if a file exists in the pack
    auto contains_file(const String& fileName) -> bool;

    // Get original size of a file
    auto get_file_size(const String& fileName) -> size_t;

    // Get file metadata
    auto get_file_info(const String& fileName) -> FileEntry;
};

} // namespace nv

#endif
