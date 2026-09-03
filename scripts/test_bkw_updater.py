#!/usr/bin/env python3
"""Compile and exercise the real BKW release selector without network writes."""
import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE = r'''
#include <engine/client/updater.cpp>
#include <cassert>
CConfig g_Config;
int main()
{
    using BkwUpdate::IsNewer;
    assert(IsNewer("r0.1", "b0.3", 1));
    assert(!IsNewer("r0.1", "b0.3", 0));
    assert(!IsNewer("b0.3", "b0.3", 0));
    assert(!IsNewer("b0.2", "b0.3", 0));
    assert(IsNewer("B0.10", "b0.9", 0));
    assert(!IsNewer("b0.3.0", "b0.3", 0));
    assert(!IsNewer("b999999999999999999999", "b0.3", 0));
    assert(!IsNewer("beta0.4", "b0.3", 0));
    assert(!IsNewer("r0.1junk", "b0.3", 1));
    const char *text = R"JSON([
      {"name":"b0.4","tag_name":"arbitrary","draft":false,"prerelease":true,"assets":[
        {"name":"bkw-b0.4-linux_x86_64.tar.xz","browser_download_url":"https://github.com/Kakkek53/test/releases/download/arbitrary/bkw-b0.4-linux_x86_64.tar.xz"}]},
      {"name":"b9.0","draft":true,"assets":[]},
      {"name":"b8.0","assets":[{"name":"bkw-b8-linux_x86_64.tar.xz","browser_download_url":"https://example.com/evil"}]},
      {"name":"b7.0","assets":[{"name":"bkw-win64.zip","browser_download_url":"https://github.com/Kakkek53/test/releases/download/b7/a.zip"}]},
      {"name":"b6.0","assets":[]},
      {"name":"r0.1","assets":[{"name":"bkw-r0.1-linux_x86_64.tar.xz","browser_download_url":"https://github.com/Kakkek53/test/releases/download/r0.1/bkw-r0.1-linux_x86_64.tar.xz"}]},
      {"name":"unrelated","tag_name":"b99","assets":[]}
    ])JSON";
    json_value *json = json_parse(text, str_length(text));
    assert(json);
    char version[64], name[128], url[2048];
    g_Config.m_BkwUpdateChannel = 0;
    assert(ParseLatestRelease(json, version, sizeof(version), name, sizeof(name), url, sizeof(url)));
    assert(std::string(version) == "b0.4");
    g_Config.m_BkwUpdateChannel = 1;
    assert(ParseLatestRelease(json, version, sizeof(version), name, sizeof(name), url, sizeof(url)));
    assert(std::string(version) == "r0.1");
    json_value_free(json);
}
'''

with tempfile.TemporaryDirectory() as directory:
    directory = pathlib.Path(directory)
    generated = directory / "generated"
    generated.mkdir()
    for name, command in [
        ("protocol.h", ["datasrc/compile.py", "network_header"]),
        ("protocol7.h", ["-m", "datasrc.seven.compile", "network_header"]),
    ]:
        with (generated / name).open("w") as output:
            subprocess.run(["python3", *command], cwd=ROOT, stdout=output, check=True)
    source = directory / "test.cpp"
    source.write_text(SOURCE)
    binary = directory / "test"
    subprocess.run([
        "g++", "-std=c++20", "-ffunction-sections", "-fdata-sections", "-Isrc",
        "-I" + str(directory), str(source), "src/base/str.cpp",
        "src/base/unicode/tolower.cpp", "src/engine/shared/json.cpp",
        "src/engine/external/json-parser/json.c", "-Wl,--gc-sections", "-o", str(binary),
    ], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
print("BKW updater: channel, version, asset and release selection tests passed")
