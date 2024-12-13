#!/usr/bin/env bash

set -exo pipefail

TARGET=${TARGET:=glad/files}

if type -P curl >/dev/null; then
	DOWNLOAD_COMMAND="curl -L -o"
fi
if type -P wget >/dev/null;then
	DOWNLOAD_COMMAND="wget --quiet --show-progress -O"
fi
if [[ -z "$DOWNLOAD_COMMAND" ]]; then
	echo "Couldn't find curl or wget, can't continue"
	exit 1
fi

download_file() {
	rm -f "$1"
	${DOWNLOAD_COMMAND} "$1" "$2"
}

download_gitiles() {
	rm -f "$1"
	${DOWNLOAD_COMMAND} - "$2" | base64 -d > "$1"
}

download_file "${TARGET}/egl.xml" https://raw.githubusercontent.com/KhronosGroup/EGL-Registry/main/api/egl.xml
download_gitiles "${TARGET}/egl_angle_ext.xml" "https://chromium.googlesource.com/angle/angle/+/refs/heads/main/scripts/egl_angle_ext.xml?format=TEXT"

download_file "${TARGET}/gl.xml" https://raw.githubusercontent.com/KhronosGroup/OpenGL-Registry/main/xml/gl.xml
download_gitiles "${TARGET}/gl_angle_ext.xml" "https://chromium.googlesource.com/angle/angle/+/refs/heads/main/scripts/gl_angle_ext.xml?format=TEXT"

# These extensions are GLSL-only, so they don't end up in the Khronos XML
download_file "${TARGET}/glsl_exts.xml" 'https://www.uplinklabs.net/glsl_exts.xml'

download_file "${TARGET}/glx.xml" https://raw.githubusercontent.com/KhronosGroup/OpenGL-Registry/main/xml/glx.xml

download_file "${TARGET}/wgl.xml" https://raw.githubusercontent.com/KhronosGroup/OpenGL-Registry/main/xml/wgl.xml

download_file "${TARGET}/vk.xml" https://raw.githubusercontent.com/KhronosGroup/Vulkan-Docs/main/xml/vk.xml

download_file "${TARGET}/khrplatform.h" https://raw.githubusercontent.com/KhronosGroup/EGL-Registry/main/api/KHR/khrplatform.h

download_file "${TARGET}/eglplatform.h" https://raw.githubusercontent.com/KhronosGroup/EGL-Registry/main/api/EGL/eglplatform.h

download_file "${TARGET}/vk_platform.h" https://raw.githubusercontent.com/KhronosGroup/Vulkan-Docs/main/include/vulkan/vk_platform.h

download_file "${TARGET}/vulkan_video_codecs_common.h" https://raw.githubusercontent.com/KhronosGroup/Vulkan-Headers/main/include/vk_video/vulkan_video_codecs_common.h

download_file "${TARGET}/vulkan_video_codec_h264std.h" https://raw.githubusercontent.com/KhronosGroup/Vulkan-Headers/main/include/vk_video/vulkan_video_codec_h264std.h
download_file "${TARGET}/vulkan_video_codec_h264std_decode.h" https://raw.githubusercontent.com/KhronosGroup/Vulkan-Headers/main/include/vk_video/vulkan_video_codec_h264std_decode.h
download_file "${TARGET}/vulkan_video_codec_h264std_encode.h" https://raw.githubusercontent.com/KhronosGroup/Vulkan-Headers/main/include/vk_video/vulkan_video_codec_h264std_encode.h

download_file "${TARGET}/vulkan_video_codec_h265std.h" https://raw.githubusercontent.com/KhronosGroup/Vulkan-Headers/main/include/vk_video/vulkan_video_codec_h265std.h
download_file "${TARGET}/vulkan_video_codec_h265std_decode.h" https://raw.githubusercontent.com/KhronosGroup/Vulkan-Headers/main/include/vk_video/vulkan_video_codec_h265std_decode.h
download_file "${TARGET}/vulkan_video_codec_h265std_encode.h" https://raw.githubusercontent.com/KhronosGroup/Vulkan-Headers/main/include/vk_video/vulkan_video_codec_h265std_encode.h

download_file "${TARGET}/vulkan_video_codec_av1std.h" https://raw.githubusercontent.com/KhronosGroup/Vulkan-Headers/main/include/vk_video/vulkan_video_codec_av1std.h
download_file "${TARGET}/vulkan_video_codec_av1std_decode.h" https://raw.githubusercontent.com/KhronosGroup/Vulkan-Headers/main/include/vk_video/vulkan_video_codec_av1std_decode.h
download_file "${TARGET}/vulkan_video_codec_av1std_encode.h" https://raw.githubusercontent.com/KhronosGroup/Vulkan-Headers/main/include/vk_video/vulkan_video_codec_av1std_encode.h
