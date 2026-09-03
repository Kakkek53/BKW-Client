#!/usr/bin/env python3
"""Run lightweight regressions against the real shop naming and material code."""
import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE = r"""
#include <game/client/components/skinshop.h>
#include <game/client/ui_rect.h>
#include <cassert>
#include <string>
int main()
{
    SSkinShopItem item;
    item.m_Id = "teedata:012345678901234567890123456789";
    item.m_Name = std::string(180, 'a') + ".png";
    auto name = CSkinShop::AssetName(item);
    assert(name.size() <= 49);
    assert(name == CSkinShop::AssetName(item));
    item.m_Name = "../../texture; bad \" name.png";
    name = CSkinShop::AssetName(item);
    assert(name.find('/') == std::string::npos);
    assert(name.find('"') == std::string::npos);
    assert(name.find(';') == std::string::npos);
    CSkinShop shop;
    assert(shop.InstallPath(item) == "assets/game/" + name + ".png");
    CUIRect::SetGlassOpacity(0.35f);
    {
        CUIRect::CScopedGlass standard(-1.0f);
        assert(CUIRect::GlassOpacity() == -1.0f);
        {
            CUIRect::CScopedGlass glass(0.5f);
            assert(CUIRect::GlassOpacity() == 0.5f);
        }
        assert(CUIRect::GlassOpacity() == -1.0f);
    }
    assert(CUIRect::GlassOpacity() == 0.35f);
}
"""

with tempfile.TemporaryDirectory() as directory:
    source = pathlib.Path(directory) / "shop.cpp"
    binary = pathlib.Path(directory) / "shop-test"
    source.write_text(SOURCE)
    subprocess.run([
        "g++", "-std=c++20", "-ffunction-sections", "-fdata-sections", "-Isrc",
        str(source), "src/game/client/components/skinshop.cpp",
        "src/game/client/ui_rect.cpp", "src/base/str.cpp",
        "src/base/unicode/tolower.cpp", "-Wl,--gc-sections", "-o", str(binary),
    ], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
print("Shop naming and scoped menu styles: passed")
