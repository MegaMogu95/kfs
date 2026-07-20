# ============================================================
#  Stage 1 — build the i686-elf cross-compiler from source
#  (slow: ~20-40 min, but it happens ONCE and is then cached)
# ============================================================
FROM debian:bookworm-slim AS toolchain

ARG BINUTILS_VERSION=2.45.1
ARG GCC_VERSION=16.1.0
ARG TARGET=i686-elf
ARG PREFIX=/opt/cross

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential bison flex texinfo wget xz-utils ca-certificates \
        libgmp-dev libmpc-dev libmpfr-dev libisl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

# ---- binutils: gives us i686-elf-as and i686-elf-ld ----
RUN wget -q "https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.xz" \
    && tar xf "binutils-${BINUTILS_VERSION}.tar.xz" \
    && mkdir build-binutils && cd build-binutils \
    && ../binutils-${BINUTILS_VERSION}/configure \
         --target=${TARGET} --prefix=${PREFIX} \
         --with-sysroot --disable-nls --disable-werror \
    && make -j"$(nproc)" && make install \
    && cd /src && rm -rf build-binutils binutils-*

# ---- gcc: only all-gcc + libgcc (no libc, we are freestanding) ----
RUN wget -q "https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.xz" \
    && tar xf "gcc-${GCC_VERSION}.tar.xz" \
    && mkdir build-gcc && cd build-gcc \
    && ../gcc-${GCC_VERSION}/configure \
         --target=${TARGET} --prefix=${PREFIX} \
         --disable-nls --enable-languages=c --without-headers \
    && make -j"$(nproc)" all-gcc all-target-libgcc \
    && make install-gcc install-target-libgcc \
    && cd /src && rm -rf build-gcc gcc-*

# ============================================================
#  Stage 2 — the small image you actually use day to day
# ============================================================
FROM debian:bookworm-slim

# runtime shared libs the cross-gcc links against, + ISO/GRUB tooling
RUN apt-get update && apt-get install -y --no-install-recommends \
        make \
        libgmp10 libmpfr6 libmpc3 libisl23 libzstd1 \
        grub-common grub-pc-bin xorriso mtools \
    && rm -rf /var/lib/apt/lists/*

# bring in the toolchain built above
COPY --from=toolchain /opt/cross /opt/cross
ENV PATH="/opt/cross/bin:${PATH}"

WORKDIR /kfs

# quick sanity check when the image is built
RUN i686-elf-gcc --version && i686-elf-as --version | head -1

CMD ["make"]