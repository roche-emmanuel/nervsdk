
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

    ~ResourceUnpacker() override {
        if (_packFile.is_open()) {
            _packFile.close();
        }
    };

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
    void read_from_memory(char* dest, size_t size) {
        NVCHK(_readPosition + size <= _packData.size(),
              "Attempted to read beyond buffer bounds");
        std::memcpy(dest, _packData.data() + _readPosition, size);
        _readPosition += size;
    }

    template <typename T> void read_value(T& value) {
        read_from_memory(reinterpret_cast<char*>(&value), sizeof(T));
    }

    void seek_to(size_t position) {
        NVCHK(position <= _packData.size(),
              "Attempted to seek beyond buffer bounds");
        _readPosition = position;
    }

  protected:
    // Override extraction to use memory instead of file
    auto extract_compressed_data(const String& fileName, U32& fileSize,
                                 U32& checksum) -> U8Vector override {
        const auto& ft = get_file_table();
        auto it = ft.find(fileName);
        NVCHK(it != ft.end(), "File not found in pack: {}", fileName);

        const FileEntry& entry = it->second;

        // Seek to file data
        seek_to(entry.offset);

        // Read encrypted data from memory
        U8Vector encryptedData(entry.encryptedSize);
        read_from_memory(reinterpret_cast<char*>(encryptedData.data()),
                         entry.encryptedSize);

        fileSize = entry.originalSize;
        checksum = entry.checksum;

        // Decrypt data
        return decrypt_data(encryptedData);
    }

    auto get_file_table() -> const UnorderedMap<String, FileEntry>& override {
        if (!_initialized) {
            // Read and verify header
            char magic[5];
            read_from_memory(magic, 5);
            String magicStr(magic, 5);

            NVCHK(magicStr == "NVPCK" || magicStr == "NVPKX",
                  "Invalid pack file format: {}", magicStr);

            bool isV2Format = (magicStr == "NVPKX");

            if (isV2Format) {
                // Read package version
                read_value(packageVersion);

                // Read encrypted metadata
                U32 encryptedMetadataLength;
                read_value(encryptedMetadataLength);

                U8Vector encryptedMetadata(encryptedMetadataLength);
                read_from_memory(
                    reinterpret_cast<char*>(encryptedMetadata.data()),
                    encryptedMetadataLength);

                // Decrypt metadata
                U8Vector decryptedMetadata = decrypt_data(encryptedMetadata);
                metadata =
                    String(decryptedMetadata.begin(), decryptedMetadata.end());

                logDEBUG("Pack version: {}, metadata length: {}",
                         packageVersion, metadata.length());
            } else {
                // Version 1 format - no version/metadata
                packageVersion = 0;
                metadata = "";
                logDEBUG("Loading legacy v1 format pack from memory");
            }

            // Read file count
            U32 fileCount = 0;
            read_value(fileCount);

            logDEBUG("Reading file table with {} entries from memory.",
                     fileCount);

            // Read file table
            for (U32 i = 0; i < fileCount; i++) {
                U32 nameLength = 0;
                read_value(nameLength);

                Vector<char> nameBuffer(nameLength);
                read_from_memory(nameBuffer.data(), nameLength);
                String name(nameBuffer.data(), nameLength);

                FileEntry entry;
                entry.name = name;

                read_value(entry.offset);
                read_value(entry.originalSize);
                read_value(entry.compressedSize);
                read_value(entry.encryptedSize);
                read_value(entry.checksum);

                _fileTable[name] = entry;
            }
            _initialized = true;
        }

        return _fileTable;
    }

  public:
    // Constructor takes memory buffer and virtual filename
    ResourceUnpackerMemory(U8Vector&& data, const String& virtualFilename,
                           const U8Vector& key, const U8Vector& iv)
        : ResourceUnpacker(virtualFilename, key, iv),
          _packData(std::move(data)) {}
};

} // namespace nv

#endif
