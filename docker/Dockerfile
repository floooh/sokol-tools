FROM alpine:edge
RUN apk add build-base git vim python cmake
RUN mkdir /workspace && cd /workspace && \
    git clone --recursive https://github.com/floooh/sokol-tools && \
    cd sokol-tools && \
    ./fips build linux-make-release && \
    strip /workspace/fips-deploy/sokol-tools/linux-make-release/sokol-shdc

