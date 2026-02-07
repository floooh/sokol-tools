FROM --platform=$TARGETPLATFORM denoland/deno:alpine
RUN LD_LIBRARY_PATH=/usr/lib:$LD_LIBRARY_PATH apk add build-base git cmake ninja
COPY . /workspace/sokol-tools
# LD_LIBRARY_PATH hack see: https://github.com/denoland/deno_docker/issues/373
RUN cd /workspace/sokol-tools && \
    LD_LIBRARY_PATH=/usr/lib:$LD_LIBRARY_PATH ./fibs config linux-ninja-release --verbose && \
    LD_LIBRARY_PATH=/usr/lib:$LD_LIBRARY_PATH ./fibs build --verbose && \
    LD_LIBRARY_PATH=/usr/lib:$LD_LIBRARY_PATH strip /workspace/sokol-tools/.fibs/dist/linux-ninja-release/sokol-shdc
