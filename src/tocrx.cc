
#include <string>
#include <vector>

#include "base/base64.h"
#include "base/crypto/rsa_private_key.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "base/string_util.h"

namespace {

const int kRSAKeySize = 1024;

// KEY MARKERS
const char kKeyBeginHeaderMarker[] = "-----BEGIN";
const char kKeyBeginFooterMarker[] = "-----END";
const char kKeyInfoEndMarker[] = "KEY-----";
const char kPublic[] = "PUBLIC";
const char kPrivate[] = "PRIVATE";

bool ValidateInput(const FilePath& extension_dir,
                   const FilePath& private_key_path,
                   const FilePath& private_key_output_path) {
  if (extension_dir.value().empty() ||
      !file_util::DirectoryExists(extension_dir)) {
    // TODO(knorton): Error message.
    return false;
  }
    
  if (!private_key_path.value().empty() &&
      !file_util::PathExists(private_key_path)) {
    // TODO(knorton): Error message.
    return false;
  }
    
  if (private_key_path.value().empty() &&
      !private_key_output_path.value().empty() &&
      file_util::PathExists(private_key_output_path)) {
    // TODO(knorton): Error message.
    return false;
  }
  // TODO(knorton): ExtensionCreator loads the extension at this point to get
  // additional checking. It would be nice to replicate some of that checking.
  return true;
}
  
bool ParsePEMKeyBytes(const std::string& input, std::string* output) {
  DCHECK(output);
  if (!output)
    return false;
  if (input.length() == 0)
    return false;
    
  std::string working = input;
  if (StartsWithASCII(working, kKeyBeginHeaderMarker, true)) {
    working = CollapseWhitespaceASCII(working, true);
    size_t header_pos = working.find(kKeyInfoEndMarker,
        sizeof(kKeyBeginHeaderMarker) - 1);
    if (header_pos == std::string::npos)
      return false;
    size_t start_pos = header_pos + sizeof(kKeyInfoEndMarker) - 1;
    size_t end_pos = working.rfind(kKeyBeginFooterMarker);
    if (end_pos == std::string::npos)
      return false;
    if (start_pos >= end_pos)
      return false;

    working = working.substr(start_pos, end_pos - start_pos);
    if (working.length() == 0)
      return false;
  }
  
  return base::Base64Decode(working, output);
}
  
base::RSAPrivateKey* ReadyInputKey(const FilePath& private_key_path) {
  if (!file_util::PathExists(private_key_path)) {
    // TODO(knorton): Error message.
    return NULL;
  }
    
  std::string private_key_contents;
  if (!file_util::ReadFileToString(private_key_path, &private_key_contents)) {
    // TODO(knorton): Error message.
    return NULL;
  }
    
  std::string private_key_bytes;
  if (!ParsePEMKeyBytes(private_key_contents, &private_key_bytes)) {
    // TODO(knorton): Error message.
    return NULL;
  }
  
  return base::RSAPrivateKey::CreateFromPrivateKeyInfo(
      std::vector<uint8>(private_key_bytes.begin(), private_key_bytes.end()));
}

bool ProducePEM(const std::string& input, std::string* output) {
  CHECK(output);
  if (input.length() == 0)
    return false;

  return base::Base64Encode(input, output);
}

bool FormatPEMForFileOutput(const std::string input,
                            std::string* output,
                            bool is_public) {
  CHECK(output);
  if (input.length() == 0)
    return false;

  *output = "";
  output->append(kKeyBeginHeaderMarker);
  output->append(" ");
}

base::RSAPrivateKey* GenerateKey(const FilePath& output_private_key_path) {
  scoped_ptr<base:RSAPrivateKey> key_pair(base::RSAPrivateKey::Create(
      kRSAKeySize));
  if (!key_pair.get()) {
    // TODO(knorton): Error message.
    return NULL;
  }

  std::vector<uint8> private_key_vector;
  if (!key_pair->ExportPrivateKey(&private_key_vector)) {
    // TODO(knorton): Error message.
    return NULL;
  }

  std::string private_key_bytes(
      reinterpret_cast<char*>(&private_key_vector.front()),
      private_key_vector.size());

  std::string private_key;
  if (!ProducePEM(private_key_bytes, &private_key)) {
    // TODO(knorton): Error message.
    return NULL;
  }
  std::string pem_output;
  // TODO(knorton): Port this.
  if (!FormatPEMForFileOutput(private_key, &pem_output)) {
    // TODO(knorton): Error message.
    return NULL;
  }

  if (!output_private_key_path.empty()) {
    if (-1 == file_util::WriteFile(output_key_path, pem_output.c_str(),
        pem_output.size())) {
      // TODO(knorton): Error message.
      return NULL;
    }
  }

  return key_pair.release();
}

}  // namespace

int main(int argc, char* argv[]) {
  FilePath extension_dir("foo");
  FilePath private_key_path("bar");
  FilePath private_key_output_path("baz");
  
  ValidateInput(extension_dir, private_key_path, private_key_output_path);
  return 0;
}
