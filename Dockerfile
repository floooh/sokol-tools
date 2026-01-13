FROM --platform=$TARGETPLATFORM denoland/deno:alpine
RUN apk add build-base git cmake ninja
ADD . /workspace/sokol-tools
RUN cd /workspace/sokol-tools && \
    ./fibs config linux-ninja-release && \
    ./fibs build && \
    strip /workspace/sokol-tools/.fibs/dist/linux-ninja-release/sokol-shdc
