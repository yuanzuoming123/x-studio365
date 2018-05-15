//
// Copyright (c) 2014-2018 x-studio365 - All Rights Reserved
//
#ifndef  _ENCRYPTMANAGER_H_
#define  _ENCRYPTMANAGER_H_
#include <string>
#include <unordered_map>

class FileUtilsEncrypt;

class  EncryptManager {
    friend class FileUtilsEncrypt;
public:
    enum ENCRYPT_FLAG
    {
        ENCF_NOFLAGS,
        ENCF_COMPRESS,
        ENCF_SIGNATURE = 2,
    };

    static EncryptManager *getInstance();

    /*
    The default ivec is: nsc::hex2bin("00234b89aa96fecdaf80fbf178a25621")
    @Remark:
    1.If the key/ivec generated by batch encrypt tool, you need use nsc::hex2bin
    to translate the key/ivec hexstring format to binary format.
    And you must check compress option.
    2.If you use x-studio365 to publish assets with specific encrypt key, you do not need translate
    it with nsc::hex2bin.

    Why we are need to compress data before encrypt(picture or script, any other blob data)?
    Because when encrypt with AES-CBC mode, the result data will be few of redundancy data, any compress
    algorithm could not compress the AES-CBC encrypted data again, so we need compress data before encrypt it.
    Then final package(.apk,.ipa,.appx) will small, both of platform app pacakge use zip algorithm.
    */
    void setEncryptEnabled(bool bVal, 
        const std::string& key, 
        const std::string& ivec = "", 
        ENCRYPT_FLAG flags = ENCF_NOFLAGS);
    bool isEncryptEnabled(void) const { return _encryptEnabled; }

    static std::string decryptData(const std::string& encryptedData, const std::string& key, const std::string& ivec = "");

    inline bool isCompressed() const { return _encryptFlags & ENCF_COMPRESS; };
    bool isEncryptedData(const char* data, size_t len) const;
protected:
    void setupHookFuncs();

private:
    bool _encryptEnabled = false;
    ENCRYPT_FLAG _encryptFlags = ENCF_NOFLAGS;
    std::string _encryptKey;
    std::string _encryptIvec; // required by CBC mode.
    std::string _encryptSignature;
    /// file index support, TODO: implement.
public:
    enum class FileIndexFormat {
        Binary,
        Csv,
    };
    void enableFileIndex(const std::string& indexFile, FileIndexFormat format = FileIndexFormat::Binary);
private:
    std::string _indexFilename;
    std::unordered_map<std::string, std::string> _indexFileMap;
};

#endif
