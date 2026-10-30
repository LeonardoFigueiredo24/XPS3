#include "../uwp_cover_store.h"

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

void touch(const std::filesystem::path& path, std::uintmax_t size = 8)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (size)
    {
        output.seekp(static_cast<std::streamoff>(size - 1));
        output.put('\0');
    }
}
}

int main()
{
    using namespace xps3::covers;

    const auto temp = std::filesystem::temp_directory_path() / "xps3-cover-store-test";
    const auto central = temp / "COVERS";
    const auto game = temp / "GAMES" / "demo.iso";
    const auto embedded = temp / "embedded" / "ICON0.PNG";
    std::filesystem::remove_all(temp);
    touch(game);
    touch(embedded);
    touch(central / "BLES12345.jpg");
    touch(central / "BLES12345.png");

    auto found = resolve(central.string(), "BLES-12345", game.string(), embedded.string());
    require(found.source == source_kind::central_png,
        "central TITLE_ID PNG must have the highest priority");

    std::filesystem::remove(central / "BLES12345.png");
    found = resolve(central.string(), "BLES-12345", game.string(), embedded.string());
    require(found.source == source_kind::central_jpg,
        "central TITLE_ID JPG must be used when PNG is absent");

    std::filesystem::remove(central / "BLES12345.jpg");
    found = resolve(central.string(), "BLES-12345", game.string(), embedded.string());
    require(found.source == source_kind::embedded_icon,
        "embedded ICON0 must precede files beside the game");

    std::filesystem::remove(embedded);
    touch(game.parent_path() / "demo.png");
    touch(game.parent_path() / "cover.png");
    found = resolve(central.string(), "BLES-12345", game.string(), embedded.string());
    require(found.source == source_kind::sibling_named,
        "same-name PNG must precede generic cover.png");

    std::filesystem::remove(game.parent_path() / "demo.png");
    found = resolve(central.string(), "BLES-12345", game.string(), embedded.string());
    require(found.source == source_kind::sibling_cover,
        "cover.png must be accepted beside a game");

    std::filesystem::remove(game.parent_path() / "cover.png");
    touch(central / "BLES12345.png", max_encoded_bytes + 1);
    found = resolve(central.string(), "BLES-12345", game.string(), embedded.string());
    require(found.source == source_kind::placeholder,
        "oversized cover must not be decoded");
    require(!found.warning.empty(), "oversized cover must explain why it was rejected");

    std::filesystem::remove_all(temp);
    std::cout << "PASS: manual cover store\n";
    return 0;
}
