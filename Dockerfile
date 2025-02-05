FROM --platform=$TARGETPLATFORM alpine
RUN apk add build-base git python3 cmake ninja
ADD . /workspace/sokol-tools
RUN cd /workspace/sokol-tools && \
    ./fips build linux-ninja-release && \
    strip /workspace/fips-deploy/sokol-tools/linux-ninja-release/sokol-shdc