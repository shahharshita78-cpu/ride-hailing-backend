# ──────────────────────────────────────────────────────────────────────────────
# Stage 1: deps — build all third-party C++ libraries from source
# This layer is cached aggressively so rebuilds after source changes are fast.
# ──────────────────────────────────────────────────────────────────────────────
FROM ubuntu:22.04 AS deps

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    git \
    gcc \
    g++ \
    cmake \
    libjsoncpp-dev \
    uuid-dev \
    zlib1g-dev \
    openssl \
    libssl-dev \
    libpq-dev \
    postgresql-client \
    postgresql-server-dev-all \
    librdkafka-dev \
    libboost-dev \
    libboost-program-options-dev \
    libhiredis-dev \
    wget \
    curl \
    make \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build_deps

# ── Drogon ──────────────────────────────────────────────────────────────────
RUN git clone --depth=1 --recurse-submodules https://github.com/drogonframework/drogon.git && \
    cd drogon && \
    mkdir _build && cd _build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=OFF -DBUILD_CTL=OFF && \
    make -j$(nproc) && \
    make install && \
    cd ../.. && rm -rf drogon

# ── jwt-cpp (header-only, just install) ─────────────────────────────────────
RUN git clone --depth=1 https://github.com/Thalhammer/jwt-cpp.git && \
    cd jwt-cpp && \
    mkdir _build && cd _build && \
    cmake .. -DJWT_BUILD_EXAMPLES=OFF -DJWT_DISABLE_PICOJSON=ON && \
    make install && \
    cd ../.. && rm -rf jwt-cpp

# ── redis-plus-plus ─────────────────────────────────────────────────────────
RUN git clone --depth=1 https://github.com/sewenew/redis-plus-plus.git && \
    cd redis-plus-plus && \
    mkdir _build && cd _build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DREDIS_PLUS_PLUS_CXX_STANDARD=17 && \
    make -j$(nproc) && \
    make install && \
    cd ../.. && rm -rf redis-plus-plus

# ── cppkafka ────────────────────────────────────────────────────────────────
RUN git clone --depth=1 https://github.com/mfontanini/cppkafka.git && \
    cd cppkafka && \
    mkdir _build && cd _build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCPPKAFKA_BUILD_SHARED=ON \
        -DCPPKAFKA_DISABLE_TESTS=ON \
        -DCPPKAFKA_DISABLE_EXAMPLES=ON && \
    make -j$(nproc) && \
    make install && \
    cd ../.. && rm -rf cppkafka

RUN ldconfig

# ──────────────────────────────────────────────────────────────────────────────
# Stage 2: builder — compile the application
# ──────────────────────────────────────────────────────────────────────────────
FROM deps AS builder

WORKDIR /workspace

# Copy source first — layer is invalidated only when source changes
COPY CMakeLists.txt .
COPY src/ src/
COPY config/ config/

RUN mkdir -p _build && cd _build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

# ──────────────────────────────────────────────────────────────────────────────
# Stage 3: runtime — slim image with only what's needed to run
# ──────────────────────────────────────────────────────────────────────────────
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive
ENV LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

RUN apt-get update && apt-get install -y \
    libssl3 \
    libpq5 \
    libhiredis0.14 \
    librdkafka1 \
    libjsoncpp25 \
    libuuid1 \
    zlib1g \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Copy installed third-party libs from deps stage
COPY --from=deps /usr/local/lib /usr/local/lib
COPY --from=deps /usr/local/include /usr/local/include

# Copy compiled binary
COPY --from=builder /workspace/_build/ride_hailing_backend /app/ride_hailing_backend

# Copy config (will be overridden by env vars at runtime)
COPY --from=builder /workspace/config /app/config

RUN ldconfig

WORKDIR /app

EXPOSE 8080

CMD ["/app/ride_hailing_backend"]
