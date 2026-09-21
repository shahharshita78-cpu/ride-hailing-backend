#include "CryptoUtils.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include <random>
#include <vector>

namespace utils {
namespace crypto {

std::string generateSalt(size_t length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 255);
    
    std::stringstream ss;
    for (size_t i = 0; i < length; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << dist(gen);
    }
    return ss.str();
}

std::string hashPassword(const std::string& password, const std::string& salt) {
    std::string saltedPassword = password + salt;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (context != nullptr) {
        if (EVP_DigestInit_ex(context, EVP_sha256(), nullptr)) {
            if (EVP_DigestUpdate(context, saltedPassword.c_str(), saltedPassword.length())) {
                EVP_DigestFinal_ex(context, hash, nullptr);
            }
        }
        EVP_MD_CTX_free(context);
    }
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return salt + ":" + ss.str();
}

bool verifyPassword(const std::string& password, const std::string& storedHash) {
    size_t delimiterPos = storedHash.find(':');
    if (delimiterPos == std::string::npos) return false;
    
    std::string salt = storedHash.substr(0, delimiterPos);
    std::string expectedHash = hashPassword(password, salt);
    
    return expectedHash == storedHash;
}

}
}
