#include <nvk/resource/ResourcePacker.h>

#include <openssl/aes.h>
#include <openssl/evp.h>
#include <zlib.h>

namespace nv {

auto ResourcePacker::compress_data(const U8Vector& input) -> U8Vector {
    z_stream zs;
    std::memset(&zs, 0, sizeof(zs));

    if (deflateInit(&zs, Z_BEST_COMPRESSION) != Z_OK) {
        THROW_MSG("zlib initialization failed");
    }

    zs.next_in = (Bytef*)input.data();
    zs.avail_in = static_cast<uInt>(input.size());

    int ret;
    char outbuffer[32768];
    U8Vector compressed;

    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);

        ret = deflate(&zs, Z_FINISH);

        if (compressed.size() < zs.total_out) {
            compressed.insert(compressed.end(), outbuffer,
                              outbuffer + (zs.total_out - compressed.size()));
        }
    } while (ret == Z_OK);

    deflateEnd(&zs);

    if (ret != Z_STREAM_END) {
        THROW_MSG("Error during compression");
    }

    return compressed;
}

auto ResourcePacker::encrypt_data(const U8Vector& input) -> U8Vector {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        THROW_MSG("Failed to create OpenSSL cipher context");
    }

    NVCHK(AES_KEY.size() == 32 && AES_IV.size() == 16, "Invalid key size.");

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, AES_KEY.data(),
                           AES_IV.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        THROW_MSG("Failed to initialize encryption");
    }

    U8Vector encrypted(input.size() + AES_BLOCK_SIZE);
    int outlen1 = 0;

    if (EVP_EncryptUpdate(ctx, encrypted.data(), &outlen1, input.data(),
                          input.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        THROW_MSG("Failed to encrypt data");
    }

    int outlen2 = 0;
    if (EVP_EncryptFinal_ex(ctx, encrypted.data() + outlen1, &outlen2) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        THROW_MSG("Failed to finalize encryption");
    }

    encrypted.resize(outlen1 + outlen2);
    EVP_CIPHER_CTX_free(ctx);

    return encrypted;
}

void ResourcePacker::add_file(const String& filePath, const String& entryName) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return;
    }

    // Read file content
    U8Vector content((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
    file.close();

    // Prepare file entry
    FileEntry entry;
    // entry.name = std::filesystem::path(filePath).filename().string();
    entry.name = entryName;
    entry.sourceFile = filePath;
    entry.originalSize = content.size();

    // Compress the data
    U8Vector compressed = compress_data(content);
    entry.compressedSize = compressed.size();

    // Encrypt the compressed data
    U8Vector encrypted = encrypt_data(compressed);
    entry.encryptedSize = encrypted.size();
    // logDEBUG("Packer: file {}: compressedSize={}, encryptedSize={}",
    // filePath,
    //          entry.compressedSize, entry.encryptedSize);

    // Calculate checksum (on the original data)
    entry.checksum = compute_data_checksum(content);

    // Store file info
    entry.offset = 0; // Will be set during pack()
    fileEntries.push_back(entry);
}

void ResourcePacker::pack() {
    std::ofstream out(outputPath, std::ios::binary);
    if (!out) {
        std::cerr << "Failed to create output file: " << outputPath
                  << std::endl;
        return;
    }

    // Write header: magic number (v2 format with metadata support)
    const char* magic = "NVPKX";
    out.write(magic, 5);

    // Write package version
    out.write(reinterpret_cast<const char*>(&packageVersion),
              sizeof(packageVersion));

    // Encrypt and write metadata
    U8Vector metadataBytes(metadata.begin(), metadata.end());
    U8Vector encryptedMetadata = encrypt_data(metadataBytes);

    U32 metadataLength = encryptedMetadata.size();
    out.write(reinterpret_cast<const char*>(&metadataLength),
              sizeof(metadataLength));
    out.write(reinterpret_cast<const char*>(encryptedMetadata.data()),
              metadataLength);

    // Write file count
    U32 fileCount = fileEntries.size();
    out.write(reinterpret_cast<const char*>(&fileCount), sizeof(fileCount));

    // Calculate offset for the first file data block
    U32 currentOffset = 5 + // magic
                        sizeof(packageVersion) + sizeof(metadataLength) +
                        metadataLength + sizeof(fileCount);

    // Compute the total offset for the file table:
    for (auto& entry : fileEntries) {
        U32 nameLength = entry.name.length();
        currentOffset += sizeof(nameLength) + nameLength +
                         sizeof(entry.offset) + sizeof(entry.originalSize) +
                         sizeof(entry.compressedSize) +
                         sizeof(entry.encryptedSize) + sizeof(entry.checksum);
    }

    // Write file table entries (name, offset, sizes, checksum)
    // Update offsets and write headers
    for (auto& entry : fileEntries) {
        U32 nameLength = entry.name.length();
        out.write(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
        out.write(entry.name.c_str(), nameLength);

        entry.offset = currentOffset;
        out.write(reinterpret_cast<char*>(&entry.offset), sizeof(entry.offset));
        out.write(reinterpret_cast<char*>(&entry.originalSize),
                  sizeof(entry.originalSize));
        out.write(reinterpret_cast<char*>(&entry.compressedSize),
                  sizeof(entry.compressedSize));
        out.write(reinterpret_cast<char*>(&entry.encryptedSize),
                  sizeof(entry.encryptedSize));
        out.write(reinterpret_cast<char*>(&entry.checksum),
                  sizeof(entry.checksum));

        currentOffset += entry.encryptedSize;
    }

    // Write file data
    U32 tsize = 0;
    for (const auto& entry : fileEntries) {
        // Read original file
        NVCHK(system_file_exists(entry.sourceFile.c_str()),
              "Invalid source file for pack entry: {}",
              entry.sourceFile.c_str());
        std::ifstream file(entry.sourceFile, std::ios::binary);
        U8Vector content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

        // Compress and encrypt
        U8Vector compressed = compress_data(content);
        U8Vector encrypted = encrypt_data(compressed);

        // Write to pack file
        out.write(reinterpret_cast<const char*>(encrypted.data()),
                  encrypted.size());
        tsize += encrypted.size();
    }

    out.close();

    logDEBUG("Created resource pack: {} with {} files (dataSize={})",
             outputPath, fileEntries.size(), tsize);
}

void ResourceUnpacker::decompress_data(const U8Vector& input, U8* destData,
                                       size_t originalSize) {
    z_stream zs;
    std::memset(&zs, 0, sizeof(zs));

    if (inflateInit(&zs) != Z_OK) {
        THROW_MSG("zlib initialization failed");
    }

    zs.next_in = (Bytef*)input.data();
    zs.avail_in = static_cast<uInt>(input.size());

    zs.next_out = (Bytef*)destData;
    zs.avail_out = originalSize;

    int ret = inflate(&zs, Z_FINISH);
    inflateEnd(&zs);

    if (ret != Z_STREAM_END) {
        THROW_MSG("Error during decompression");
    }
}

auto ResourceUnpacker::decrypt_data(const U8Vector& input) -> U8Vector {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        THROW_MSG("Failed to create OpenSSL cipher context");
    }

    NVCHK(AES_KEY.size() == 32 && AES_IV.size() == 16, "Invalid key size.");

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, AES_KEY.data(),
                           AES_IV.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        THROW_MSG("Failed to initialize decryption");
    }

    U8Vector decrypted(input.size());
    int outlen1 = 0;

    if (EVP_DecryptUpdate(ctx, decrypted.data(), &outlen1, input.data(),
                          input.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        THROW_MSG("Failed to decrypt data");
    }

    int outlen2 = 0;
    if (EVP_DecryptFinal_ex(ctx, decrypted.data() + outlen1, &outlen2) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        THROW_MSG("Failed to finalize decryption");
    }

    decrypted.resize(outlen1 + outlen2);
    EVP_CIPHER_CTX_free(ctx);

    return decrypted;
}

ResourceUnpacker::ResourceUnpacker(const String& packFilePath,
                                   const U8Vector& key, const U8Vector& iv)
    : _filename(packFilePath), AES_KEY(key), AES_IV(iv) {}

auto ResourceUnpacker::get_file_table()
    -> const UnorderedMap<String, FileEntry>& {
    if (!_initialized) {
        _packFile.open(_filename, std::ios::binary);
        NVCHK(_packFile.is_open(), "Failed to open pack file {}", _filename);

        // Read and verify header
        char magic[5];
        _packFile.read(magic, 5);
        String magicStr(magic, 5);

        NVCHK(magicStr == "NVPCK" || magicStr == "NVPKX",
              "Invalid pack file format: {}", magicStr);

        bool isV2Format = (magicStr == "NVPKX");

        if (isV2Format) {
            // Read package version
            _packFile.read(reinterpret_cast<char*>(&packageVersion),
                           sizeof(packageVersion));

            // Read encrypted metadata
            U32 encryptedMetadataLength;
            _packFile.read(reinterpret_cast<char*>(&encryptedMetadataLength),
                           sizeof(encryptedMetadataLength));

            U8Vector encryptedMetadata(encryptedMetadataLength);
            _packFile.read(reinterpret_cast<char*>(encryptedMetadata.data()),
                           encryptedMetadataLength);

            // Decrypt metadata
            U8Vector decryptedMetadata = decrypt_data(encryptedMetadata);
            metadata =
                String(decryptedMetadata.begin(), decryptedMetadata.end());

            logDEBUG("Pack version: {}, metadata length: {}", packageVersion,
                     metadata.length());
        } else {
            // Version 1 format - no version/metadata
            packageVersion = 0;
            metadata = "";
            logDEBUG("Loading legacy v1 format pack");
        }

        // Read file count
        U32 fileCount;
        _packFile.read(reinterpret_cast<char*>(&fileCount), sizeof(fileCount));

        logDEBUG("Reading file table with {} entries.", fileCount);

        // Read file table (same for both versions)
        for (U32 i = 0; i < fileCount; i++) {
            U32 nameLength;
            _packFile.read(reinterpret_cast<char*>(&nameLength),
                           sizeof(nameLength));

            Vector<char> nameBuffer(nameLength);
            _packFile.read(nameBuffer.data(), nameLength);
            String name(nameBuffer.data(), nameLength);

            FileEntry entry;
            entry.name = name;

            _packFile.read(reinterpret_cast<char*>(&entry.offset),
                           sizeof(entry.offset));
            _packFile.read(reinterpret_cast<char*>(&entry.originalSize),
                           sizeof(entry.originalSize));
            _packFile.read(reinterpret_cast<char*>(&entry.compressedSize),
                           sizeof(entry.compressedSize));
            _packFile.read(reinterpret_cast<char*>(&entry.encryptedSize),
                           sizeof(entry.encryptedSize));
            _packFile.read(reinterpret_cast<char*>(&entry.checksum),
                           sizeof(entry.checksum));

            _fileTable[name] = entry;
        }

        _initialized = true;
    }

    return _fileTable;
}

auto ResourceUnpacker::get_file_info(const String& fileName) -> FileEntry {
    const auto& ft = get_file_table();
    auto it = ft.find(fileName);
    NVCHK(it != ft.end(), "File not found in pack {}", fileName);
    return it->second;
}

auto ResourceUnpacker::get_file_size(const String& fileName) -> size_t {
    return get_file_info(fileName).originalSize;
}

auto ResourceUnpacker::contains_file(const String& fileName) -> bool {
    const auto& ft = get_file_table();
    return ft.find(fileName) != ft.end();
}

void ResourceUnpacker::extract_file_to_disk(const String& fileName,
                                            const String& outputPath) {
    U8Vector data = extract_file(fileName);

    std::ofstream outFile(outputPath, std::ios::binary);
    NVCHK(outFile.is_open(), "Failed to create output file: {}", outputPath);

    outFile.write(reinterpret_cast<const char*>(data.data()), data.size());
    outFile.close();
}

auto ResourceUnpacker::list_files() -> Vector<String> {
    Vector<String> files;
    const auto& ft = get_file_table();
    for (const auto& entry : ft) {
        files.push_back(entry.first);
    }
    return files;
}

auto ResourceUnpacker::get_filename() const -> const String& {
    return _filename;
}
void ResourcePacker::set_package_version(I64 version) {
    packageVersion = version;
}
void ResourcePacker::set_metadata(const String& meta) { metadata = meta; }
auto ResourceUnpacker::get_package_version() const -> I64 {
    return packageVersion;
}
auto ResourceUnpacker::get_metadata() const -> const String& {
    return metadata;
}
auto ResourceUnpacker::extract_compressed_data(const String& fileName,
                                               U32& fileSize, U32& checksum)
    -> U8Vector {
    const auto& ft = get_file_table();
    auto it = ft.find(fileName);
    NVCHK(it != ft.end(), "File not found in pack: {}", fileName);

    const FileEntry& entry = it->second;

    // Seek to file data
    _packFile.seekg(entry.offset);

    // Read encrypted data
    U8Vector encryptedData(entry.encryptedSize);
    _packFile.read(reinterpret_cast<char*>(encryptedData.data()),
                   entry.encryptedSize);

    fileSize = entry.originalSize;
    checksum = entry.checksum;

    // Decrypt data
    return decrypt_data(encryptedData);
};

ResourceUnpacker::~ResourceUnpacker() {
    if (_packFile.is_open()) {
        _packFile.close();
    }
};
void ResourceUnpackerMemory::read_from_memory(char* dest, size_t size) {
    NVCHK(_readPosition + size <= _packData.size(),
          "Attempted to read beyond buffer bounds");
    std::memcpy(dest, _packData.data() + _readPosition, size);
    _readPosition += size;
}
void ResourceUnpackerMemory::seek_to(size_t position) {
    NVCHK(position <= _packData.size(),
          "Attempted to seek beyond buffer bounds");
    _readPosition = position;
}
auto ResourceUnpackerMemory::extract_compressed_data(const String& fileName,
                                                     U32& fileSize,
                                                     U32& checksum)
    -> U8Vector {
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
auto ResourceUnpackerMemory::get_file_table()
    -> const UnorderedMap<String, FileEntry>& {
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
            read_from_memory(reinterpret_cast<char*>(encryptedMetadata.data()),
                             encryptedMetadataLength);

            // Decrypt metadata
            U8Vector decryptedMetadata = decrypt_data(encryptedMetadata);
            metadata =
                String(decryptedMetadata.begin(), decryptedMetadata.end());

            logDEBUG("Pack version: {}, metadata length: {}", packageVersion,
                     metadata.length());
        } else {
            // Version 1 format - no version/metadata
            packageVersion = 0;
            metadata = "";
            logDEBUG("Loading legacy v1 format pack from memory");
        }

        // Read file count
        U32 fileCount = 0;
        read_value(fileCount);

        logDEBUG("Reading file table with {} entries from memory.", fileCount);

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
ResourceUnpackerMemory::ResourceUnpackerMemory(U8Vector&& data,
                                               const String& virtualFilename,
                                               const U8Vector& key,
                                               const U8Vector& iv)
    : ResourceUnpacker(virtualFilename, key, iv), _packData(std::move(data)) {}
void ResourceUnpacker::read_file_async(const String& fileName,
                                       ReadCallback callback) {
    String err;
    callback(read_file(fileName), err);
};
auto ResourceUnpacker::get_file_metadata(const String& fileName) -> Json {
    return {};
};
auto ResourceUnpacker::get_version() const -> I64 {
    return get_package_version();
}
auto ResourceUnpacker::get_name() const -> String { return get_filename(); }
auto ResourceUnpacker::supports_sync_read() const -> bool { return true; }
} // namespace nv
