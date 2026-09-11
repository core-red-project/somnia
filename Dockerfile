# Multi-stage Dockerfile for Somnia CLI

# Stage 1: Build environment
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    clang \
    cmake \
    curl \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Install just
RUN curl --proto '=https' --tlsv1.2 -sSf https://just.systems/install.sh | bash -s -- --to /usr/local/bin

WORKDIR /app
COPY . .

RUN just build

# Stage 2: Minimal runtime image
FROM ubuntu:22.04 AS runtime

WORKDIR /app
COPY --from=builder /app/build/cli/somnia-cli /usr/local/bin/somnia

ENTRYPOINT ["somnia"]
CMD ["generate", "-n", "16", "--scale", "lydian"]
