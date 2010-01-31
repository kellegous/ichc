
#include <string>
#include <stdio.h>
#include <vector>

#include "base/at_exit.h"
#include "base/base64.h"
#include "base/command_line.h"
#include "base/crypto/rsa_private_key.h"
#include "base/crypto/signature_creator.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/scoped_nsautorelease_pool.h"
#include "base/scoped_handle.h"
#include "base/scoped_ptr.h"
#include "base/scoped_temp_dir.h"
#include "base/string_util.h"
#include "chrome/common/zip.h"


namespace {

const size_t kExtensionHeaderMagicSize = 4;
const char kExtensionHeaderMagic[] = "Cr24";
const uint32 kCurrentVersion = 2;

struct ExtensionHeader {
  char magic[kExtensionHeaderMagicSize];
  uint32 version;
  uint32 key_size;
  uint32 signature_size;
};

const int kRSAKeySize = 1024;
const int kPEMOutputColumns = 65;

// KEY MARKERS
const char kKeyBeginHeaderMarker[] = "-----BEGIN";
const char kKeyBeginFooterMarker[] = "-----END";
const char kKeyInfoEndMarker[] = "KEY-----";
const char kPublic[] = "PUBLIC";
const char kPrivate[] = "PRIVATE";

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

base::RSAPrivateKey* ReadInputKey(const FilePath& private_key_path,
    std::string* error_message) {
  if (!file_util::PathExists(private_key_path)) {
    error_message->assign("Input value for private key must exist.");
    return false;
  }

  std::string private_key_contents;
  if (!file_util::ReadFileToString(private_key_path, &private_key_contents)) {
    error_message->assign("Failed to read private key.");
    return false;
  }

  std::string private_key_bytes;
  if (!ParsePEMKeyBytes(private_key_contents, &private_key_bytes)) {
    error_message->assign("Invalid private key.");
    return false;
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
  output->append(is_public ? kPublic : kPrivate);
  output->append(" ");
  output->append(kKeyInfoEndMarker);
  output->append("\n");
  for (size_t i = 0; i < input.length();) {
    int slice = std::min<int>(input.length() - 1, kPEMOutputColumns);
    output->append(input.substr(i, slice));
    output->append("\n");
    i += slice;
  }
  output->append(kKeyBeginFooterMarker);
  output->append(" ");
  output->append(is_public ? kPublic : kPrivate);
  output->append(" ");
  output->append(kKeyInfoEndMarker);
  output->append("\n");

  return true;
}

base::RSAPrivateKey* GenerateKey(const FilePath& output_private_key_path,
    std::string* error_message) {
  scoped_ptr<base::RSAPrivateKey> key_pair(base::RSAPrivateKey::Create(
      kRSAKeySize));
  if (!key_pair.get()) {
    error_message->assign("Failed to generate RSA private key.");
    return NULL;
  }

  std::vector<uint8> private_key_vector;
  if (!key_pair->ExportPrivateKey(&private_key_vector)) {
    error_message->assign("Failed to export private key.");
    return NULL;
  }

  std::string private_key_bytes(
      reinterpret_cast<char*>(&private_key_vector.front()),
      private_key_vector.size());

  std::string private_key;
  if (!ProducePEM(private_key_bytes, &private_key)) {
    error_message->assign("Failed to output private key.");
    return NULL;
  }
  std::string pem_output;
  if (!FormatPEMForFileOutput(private_key, &pem_output, false)) {
    error_message->assign("Failed to output private key.");
    return NULL;
  }

  if (!output_private_key_path.empty()) {
    if (-1 == file_util::WriteFile(output_private_key_path, pem_output.c_str(),
        pem_output.size())) {
      error_message->assign("Failed to write private key.");
      return NULL;
    }
  }

  return key_pair.release();
}

bool CreateZip(const FilePath& extension_dir,
               const FilePath& temp_path,
               FilePath* zip_path,
               std::string* error_message) {
  *zip_path = temp_path.Append(FILE_PATH_LITERAL("extension.zip"));
  if (!Zip(extension_dir, *zip_path, false)) {
    error_message->assign("Failed to create temporary zip file during "
                           "packaging.");
    return false;
  }

  return true;
}

bool SignZip(const FilePath& zip_path,
             base::RSAPrivateKey* private_key,
             std::vector<uint8>* signature,
             std::string* error_message) {
  scoped_ptr<base::SignatureCreator> signature_creator(
      base::SignatureCreator::Create(private_key));
  ScopedStdioHandle zip_handle(file_util::OpenFile(zip_path, "rb"));
  size_t buffer_size = 1 << 16;
  scoped_array<uint8> buffer(new uint8[buffer_size]);
  int bytes_read = -1;
  while ((bytes_read = fread(buffer.get(), 1, buffer_size,
      zip_handle.get())) > 0) {
    if (!signature_creator->Update(buffer.get(), bytes_read)) {
      error_message->assign("Unable to sign extension.");
      return false;
    }
    zip_handle.Close();

    signature_creator->Final(signature);
    return true;
  }
}

bool WriteCRX(const FilePath& zip_path,
              base::RSAPrivateKey* private_key,
              const std::vector<uint8>& signature,
              const FilePath& crx_path,
              std::string* error_message) {
  if (file_util::PathExists(crx_path))
    file_util::Delete(crx_path, false);
  ScopedStdioHandle crx_handle(file_util::OpenFile(crx_path, "wb"));
  std::vector<uint8> public_key;
  if (!private_key->ExportPublicKey(&public_key)) {
    error_message->assign("Failed to export public key.");
    return false;
  }

  ExtensionHeader header;
  memcpy(&header.magic, kExtensionHeaderMagic, kExtensionHeaderMagicSize);
  header.version = kCurrentVersion;
  header.key_size = public_key.size();
  header.signature_size = signature.size();

  if (fwrite(&header, sizeof(ExtensionHeader), 1, crx_handle.get()) != 1) {
    error_message->assign("Unable to write CRX.");
    return false;
  }

  if (fwrite(&public_key.front(), sizeof(uint8), public_key.size(),
      crx_handle.get()) != public_key.size()) {
    error_message->assign("Unable to write CRX.");
    return false;
  }

  if (fwrite(&signature.front(), sizeof(uint8), signature.size(),
      crx_handle.get()) != signature.size()) {
    error_message->assign("Unable to write CRX.");
    return false;
  }

  size_t buffer_size = 1 << 16;
  scoped_array<uint8> buffer(new uint8[buffer_size]);
  size_t bytes_read = 0;
  ScopedStdioHandle zip_handle(file_util::OpenFile(zip_path, "rb"));
  while ((bytes_read = fread(buffer.get(), 1, buffer_size,
                             zip_handle.get())) > 0) {
    if (fwrite(buffer.get(), sizeof(char), bytes_read, crx_handle.get()) !=
        bytes_read) {
      error_message->assign("Unable to write CRX.");
      return false;
    }
  }

  return true;
}

bool ValidateInput(int argc,
                   char* argv[],
                   FilePath* ext_path,
                   FilePath* key_path,
                   FilePath* crx_path,
                   bool* generate_key,
                   bool* show_help,
                   std::string* error_message) {
  CommandLine command_line(argc, argv);

  if (command_line.HasSwitch("help")) {
    *show_help = true;
    return true;
  }

  *key_path = command_line.GetSwitchValuePath("key");
  *crx_path = command_line.GetSwitchValuePath("output");
  if (argc - command_line.GetSwitchCount() <= 1) {
    error_message->assign("No input directory specified.");
    return false;
  }

  *ext_path = FilePath(argv[argc - 1]);

  if (crx_path->empty())
    *crx_path = ext_path->ReplaceExtension(FILE_PATH_LITERAL(".crx"));

  if (key_path->empty())
    *key_path = ext_path->ReplaceExtension(FILE_PATH_LITERAL(".pem"));

  if (ext_path->empty() || !file_util::DirectoryExists(*ext_path)) {
    error_message->assign("Input directory must exist.");
    return false;
  }

  *generate_key = file_util::PathExists(*ext_path);
  return true;
}

}  // namespace

int main(int argc, char* argv[]) {
  base::ScopedNSAutoreleasePool autorelease_pool;
  base::AtExitManager at_exit;

  FilePath ext_path;
  FilePath crx_path;
  FilePath key_path;
  bool generate_key = false;
  bool show_help = false;

  std::string error_message;
  if (!ValidateInput(argc, argv, &ext_path, &key_path, &crx_path, &generate_key, &show_help, &error_message)) {
    fprintf(stderr, "ERROR: %s\n", error_message.c_str());
    return 1;
  }

  if (show_help) {
    fprintf(stderr, "ichc [options] extension_dir\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, " --key=path     path to the private key (if the path doesn't exist, a key will be generated.\n");
    fprintf(stderr, " --output=path  path to write the crx.\n");
    return 0;
  }

  scoped_ptr<base::RSAPrivateKey> key_pair(generate_key
      ? GenerateKey(key_path, &error_message)
      : ReadInputKey(key_path, &error_message));

  if (!key_pair.get()) {
    fprintf(stderr, "ERROR: %s\n", error_message.c_str());
    return 1;
  }

  ScopedTempDir tmp_path;
  if (!tmp_path.CreateUniqueTempDir()) {
    fprintf(stderr, "ERROR: Unable to create temporary directory.\n");
    return 1;
  }

  FilePath zip_path;
  std::vector<uint8> signature;
  if (!CreateZip(ext_path, tmp_path.path(), &zip_path, &error_message)) {
    fprintf(stderr, "ERROR: %s\n", error_message.c_str());
    return 1;
  }

  if (!SignZip(zip_path, key_pair.get(), &signature, &error_message)) {
    fprintf(stderr, "ERROR: %s\n", error_message.c_str());
    return 1;
  }

  if (!WriteCRX(zip_path, key_pair.get(), signature, crx_path, &error_message)) {
    fprintf(stderr, "ERROR: %s\n", error_message.c_str());
    return 1;
  }

  return 0;
}
