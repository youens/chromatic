#!/bin/sh
set -eu
repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)"
tool_dir="$repo_root/.tools/gbdk"
if [ -x "$tool_dir/bin/lcc" ]; then
    printf '%s\n' "GBDK is already available at $tool_dir"
    exit 0
fi
if [ "$(uname -s)" != Darwin ] || [ "$(uname -m)" != arm64 ]; then
    printf '%s\n' 'This installer targets Apple Silicon macOS.' 'Download GBDK 4.5.0 for your platform from https://github.com/gbdk-2020/gbdk-2020/releases/tag/4.5.0 and set GBDK_HOME.' >&2
    exit 1
fi
stage_dir="$(mktemp -d)"
trap 'rm -rf "$stage_dir"' EXIT HUP INT TERM
curl -fL --retry 3 'https://github.com/gbdk-2020/gbdk-2020/releases/download/4.5.0/gbdk-macos-arm64.tar.gz' -o "$stage_dir/gbdk.tar.gz"
printf '%s  %s\n' '289ee60e46c5a2785a21e35533f84a5131ed4a063b21b0dbdedc9a10af15bf78' "$stage_dir/gbdk.tar.gz" | shasum -a 256 -c -
tar -xzf "$stage_dir/gbdk.tar.gz" -C "$stage_dir"
mkdir -p "$repo_root/.tools"
mv "$stage_dir/gbdk" "$tool_dir"
printf '%s\n' "Installed GBDK 4.5.0 at $tool_dir"
