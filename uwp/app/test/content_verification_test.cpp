#include "../uwp_preparation.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void write_bytes(const std::filesystem::path& path, const std::initializer_list<unsigned char> bytes)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    for (const auto byte : bytes) output.put(static_cast<char>(byte));
}
}

int main()
{
    using namespace xps3::preparation;

    const auto temp = std::filesystem::temp_directory_path() / "xps3-content-verification-test";
    std::filesystem::remove_all(temp);
    std::filesystem::create_directories(temp);

    const auto elf = temp / "pad_test.elf";
    write_bytes(elf, {0x7f, 'E', 'L', 'F', 2, 2, 1, 0});
    const auto elf_result = verify_content(elf.string());
    require(elf_result.ok && elf_result.kind == content_kind::elf,
        "readable ELF magic must be accepted as homebrew/test content");
    require(elf_result.executable_candidates.size() == 1,
        "ELF verification must report its executable candidate");

    const auto game = temp / "ExtractedGame";
    const auto eboot = game / "PS3_GAME" / "USRDIR" / "EBOOT.BIN";
    write_bytes(eboot, {0x53, 0x43, 0x45, 0x00});
    const auto folder_result = verify_content(game.string());
    require(folder_result.ok && folder_result.kind == content_kind::extracted_game,
        "PS3_GAME/USRDIR/EBOOT.BIN must identify an extracted game");
    require(folder_result.executable_candidates.front().find("EBOOT.BIN") != std::string::npos,
        "extracted game must report EBOOT.BIN");

    const auto iso = temp / "legal-backup.iso";
    write_bytes(iso, {0x00, 0x01, 0x02, 0x03});
    const auto iso_result = verify_content(iso.string());
    require(iso_result.ok && iso_result.kind == content_kind::iso,
        "readable ISO input must reach the existing ISO reader during boot");

    const auto package = temp / "title.pkg";
    write_bytes(package, {0x7f, 0x50, 0x4b, 0x47});
    const auto package_result = verify_content(package.string());
    require(!package_result.ok && package_result.kind == content_kind::package,
        "PKG must be recognized but rejected as a direct boot source");
    require(package_result.error_code == "INSTALL_REQUIRED",
        "PKG direct boot must explain that installation is required");

    const auto missing_result = verify_content((temp / "missing.elf").string());
    require(!missing_result.ok && missing_result.error_code == "CONTENT_MISSING",
        "missing content must fail before Emu.BootGame");

    const auto unsupported = temp / "notes.txt";
    write_bytes(unsupported, {'n', 'o', 't', 'e'});
    const auto unsupported_result = verify_content(unsupported.string());
    require(!unsupported_result.ok && unsupported_result.kind == content_kind::unsupported,
        "unsupported files must be rejected explicitly");

    std::filesystem::remove_all(temp);
    std::cout << "PASS: content verification\n";
    return 0;
}
