#!/bin/bash
# Regenerate the embedded SPIR-V headers from the GLSL sources.
# Needs glslc (no i686 MSYS2 package exists; the Windows Vulkan SDK ships it -
# SPIR-V is architecture-independent, so any glslc works).  The .spv.h files
# are committed; run this only after editing a shader.
set -e
cd "$(dirname "$0")"

GLSLC="${GLSLC:-C:/VulkanSDK/1.4.350.0/Bin/glslc.exe}"

for s in vert frag; do
    "$GLSLC" "present.$s" -o "present.$s.spv"
    {
        echo "/* SPIR-V for present.$s - regenerate with port/psyq/vk/shaders/gen_spv.sh */"
        echo "static const unsigned int g_present_${s}_spv[] = {"
        od -An -tx4 -v "present.$s.spv" | awk '{for(i=1;i<=NF;i++) printf "0x%s,", $i; print ""}'
        echo "};"
    } > "present.$s.spv.h"
    echo "wrote present.$s.spv.h"
done
