#!/usr/bin/env bash
#
# 下载并校验 DXC 官方 Linux 预编译包
#
# 版本与 SHA-256 在此钉死，对应 learning/adr/ADR-0002-shader-language.md。
# 安装到构建目录 build/tools/dxc（不进仓库）；rm -rf build 之后重跑本脚本即可。
#
# 用法：./tools/bootstrap_dxc.sh
# 产物：build/tools/dxc/bin/dxc

set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

DXC_VERSION="v1.9.2609"
DXC_ASSET="linux_dxc_2026_09_28.x86_x64.tar.gz"
DXC_URL="https://github.com/microsoft/DirectXShaderCompiler/releases/download/${DXC_VERSION}/${DXC_ASSET}"
DXC_SHA256="96faadc7f5c282d2ffda49804beb4c3ee38127bc252b723234e3c5cdf7aa39a1"

DEST="$REPO_DIR/build/tools/dxc"

if [[ -x "$DEST/bin/dxc" ]]; then
    echo "DXC 已存在：$DEST/bin/dxc"
    "$DEST/bin/dxc" --version
    exit 0
fi

tmpdir="$(mktemp -d)"
trap 'rm -rf "$tmpdir"' EXIT

echo "下载 DXC ${DXC_VERSION} ..."
curl -fsSL -o "$tmpdir/$DXC_ASSET" "$DXC_URL"

echo "校验 SHA-256 ..."
echo "${DXC_SHA256}  $tmpdir/$DXC_ASSET" | sha256sum -c -

mkdir -p "$DEST"
tar -xzf "$tmpdir/$DXC_ASSET" -C "$DEST"

echo "安装完成：$DEST"
"$DEST/bin/dxc" --version
