#!/usr/bin/env bash
# Builds offline renderers for the Faust prototype in `sound design/`.
# Deliberately separate from the plugin's CMake build: this compiles the
# PROTOTYPE, not the product, and invariant #15 keeps throwaway targets out
# of the plugin build.
#
#   build.sh                 # builds both renderers
#   build.sh four_pads       # builds just one
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$here/../.." && pwd)"
out="$repo/build/faust"
mkdir -p "$out"

build_one() {
    local name="$1"
    faust -a "$here/render_arch.cpp" -I "$repo/sound design" \
          -o "$out/${name}_gen.cpp" "$repo/sound design/${name}.dsp"
    c++ -std=c++17 -O2 -I"$(brew --prefix faust)/include" \
        -o "$out/${name}_render" "$out/${name}_gen.cpp"
    echo "built: $out/${name}_render"
}

if [[ $# -gt 0 ]]; then
    build_one "$1"
else
    build_one four_pads
    build_one g5_layers
fi
