FROM alpine
ADD . /workspace/sokol-tools
RUN apk add build-base git python3 cmake ninja
RUN cd /workspace/sokol-tools && \
    ./fips build linux-ninja-release && \
    strip /workspace/fips-deploy/sokol-tools/linux-ninja-release/sokol-shdc
