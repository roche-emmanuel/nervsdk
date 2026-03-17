
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
    String _filename;
    std::ifstream _packFile;

    U8Vector AES_KEY; // Should be 32 bytes
    U8Vector AES_IV;  // Should be 16 bytes

  protected:
    I64 packageVersion{0};
    String metadata;
    UnorderedMap<String, FileEntry> _fileTable;
    bool _initialized{false};

    virtual auto get_file_table() -> const UnorderedMap<String, FileEntry>&;

    // Decompress data using zlib
    void decompress_data(const U8Vector& input, U8* destData,
                         size_t originalSize);

    // Decrypt data using AES-256
    auto decrypt_data(const U8Vector& input) -> U8Vector;

  public:
    explicit ResourceUnpacker(const String& packFilePath, const U8Vector& key,
                              const U8Vector& iv);

    ~ResourceUnpacker() override;

    auto get_package_version() const -> I64;
    auto get_metadata() const -> const String&;

    // Retrieve the pack filename:
    auto get_filename() const -> const String&;

    // List all files in the pack
    auto list_files() -> Vector<String>;

    virtual auto extract_compressed_data(const String& fileName, U32& fileSize,
                                         U32& checksum) -> U8Vector;

    // Extract a file and write it to disk
    void extract_file_to_disk(const String& fileName, const String& outputPath);

    // Check if a file exists in the pack
    auto contains_file(const String& fileName) -> bool;

    // Get original size of a file
    auto get_file_size(const String& fileName) -> size_t;

    // Get file metadata
    auto get_file_info(const String& fileName) -> FileEntry;

    template <typename T = U8Vector>
    auto extract_file(const String& fileName) -> T {
        U32 originalSize = 0;
        U32 entryChecksum = 0;
        auto compressedData =
            extract_compressed_data(fileName, originalSize, entryChecksum);

        // Decompress data
        T originalData(originalSize, typename T::value_type{});
        decompress_data(compressedData, (U8*)originalData.data(), originalSize);

        // Verify checksum
        U32 checksum = compute_data_checksum(originalData);

        NVCHK(checksum == entryChecksum,
              "Checksum verification failed for file: {}", fileName);

        return originalData;
    }
};

class ResourceUnpackerMemory : public ResourceUnpacker {
  private:
    U8Vector _packData;
    size_t _readPosition{0};

    // Memory-based reading helpers
    void read_from_memory(char* dest, size_t size);

    template <typename T> void read_value(T& value) {
        read_from_memory(reinterpret_cast<char*>(&value), sizeof(T));
    }

    void seek_to(size_t position);

  protected:
    // Override extraction to use memory instead of file
    auto extract_compressed_data(const String& fileName, U32& fileSize,
                                 U32& checksum) -> U8Vector override;

    auto get_file_table() -> const UnorderedMap<String, FileEntry>& override;

  public:
    // Constructor takes memory buffer and virtual filename
    ResourceUnpackerMemory(U8Vector&& data, const String& virtualFilename,
                           const U8Vector& key, const U8Vector& iv);
};

} // namespace nv

#endif
