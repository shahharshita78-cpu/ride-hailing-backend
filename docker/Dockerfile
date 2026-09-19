FROM ubuntu:22.04

# Avoid tzdata prompts during installation
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies for Drogon, PostgreSQL, Redis, Kafka, and JWT
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
    postgresql-server-dev-all \
    librdkafka-dev \
    libboost-dev \
    libboost-program-options-dev \
    libhiredis-dev \
    wget \
    make \
    && rm -rf /var/lib/apt/lists/*

# Install drogon from source
WORKDIR /usr/src
RUN git clone https://github.com/drogonframework/drogon && \
    cd drogon && \
    git submodule update --init && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make -j$(nproc) && \
    make install && \
    cd ../.. && rm -rf drogon

# Install jwt-cpp
RUN git clone https://github.com/Thalhammer/jwt-cpp.git && \
    cd jwt-cpp && \
    mkdir build && cd build && \
    cmake .. && make install && \
    cd ../.. && rm -rf jwt-cpp

# Install redis-plus-plus (modern C++ redis client)
RUN git clone https://github.com/sewenew/redis-plus-plus.git && \
    cd redis-plus-plus && \
    mkdir build && cd build && \
    cmake .. && make -j$(nproc) && make install && \
    cd ../.. && rm -rf redis-plus-plus

# Install cppkafka
RUN git clone https://github.com/mfontanini/cppkafka.git && \
    cd cppkafka && \
    mkdir build && cd build && \
    cmake -DCPPKAFKA_BUILD_SHARED=ON -DCPPKAFKA_DISABLE_TESTS=ON -DCPPKAFKA_DISABLE_EXAMPLES=ON .. && \
    make -j$(nproc) && make install && \
    cd ../.. && rm -rf cppkafka

WORKDIR /workspace

# Set library path
ENV LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
