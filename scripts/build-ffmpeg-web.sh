pushd $(dirname "$0")/../video/ffmpeg
set -e
. ~/Public/emsdk_portable/emsdk_set_env.sh

if [ ! -d "FFmpeg/build-web" ]; then
mkdir FFmpeg/build-web
cd FFmpeg/build-web

emconfigure ../configure \
    --cc=emcc \
    --enable-cross-compile \
    --target-os=none \
    --arch=x86 \
    --disable-runtime-cpudetect \
    --disable-asm \
    --disable-fast-unaligned \
    --disable-pthreads \
    --disable-w32threads \
    --disable-os2threads \
    --disable-debug \
    --disable-stripping \
    \
    --disable-all \
    --enable-avcodec \
    --enable-avutil \
    --disable-network \
    --disable-d3d11va \
    --disable-dxva2 \
    --disable-vaapi \
    --disable-vdpau \
    --enable-decoder=flv,vp6a,vp6f,flashsv,flashsv2,h264 \
    --disable-bzlib \
    --disable-iconv \
    --disable-libxcb \
    --disable-lzma \
    --disable-sdl2 \
    --disable-securetransport \
    --disable-xlib \
    --disable-zlib

emmake make -j8
fi

popd