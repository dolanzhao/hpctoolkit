# Main Containerfile fragment for Alma Linux 8 (proxy for RHEL 8)

# Install all the packages required:
#  - Base system pieces required by Spack
#  - Compilers
#  - Extra utilities
RUN yum install -y epel-release shadow-utils \
  && yum update -y \
  && yum install -y \
    bzip2 \
    ccache \
    clang \
    diffutils \
    eatmydata \
    file \
    gcc gcc-c++ gcc-gfortran \
    git \
    git-lfs \
    make \
    mercurial \
    patch \
    patchelf \
    perl-open \
    python3 \
    subversion \
    tar \
    unzip \
    xz \
    zstd \
  && yum clean all && rm -rf /var/yum/* /var/dnf/*

# Install the permanent Spack configuration
COPY config.yaml /etc/spack/
COPY compilers.yaml /etc/spack/linux/
RUN \
  case $TARGETARCH in \
  amd64) target=x86_64; ;; \
  arm64) target=aarch64; ;; \
  ppc64le) target=ppc64le; ;; \
  *) echo "Invalid TARGETARCH: $TARGETARCH"; exit 1; ;; \
  esac \
  && sed -i "s/@TARGET@/$target/g" /etc/spack/linux/compilers.yaml

INCLUDE ../spack.inc

# Install a Python 3.10 with Spack to overwrite the older system one
ADD https://bootstrap.pypa.io/get-pip.py /tmp/get-pip.py
RUN --mount=type=secret,id=buildcache,target=/root/.spack/mirrors.yaml \
    --mount=type=secret,id=buildcache_config,target=/root/.spack/config.yaml \
    --mount=type=cache,from=buildcache_cache,target=/cache/spack-bincache-shared \
    --mount=type=cache,from=buildcache_cache_nonprot,target=/cache/spack-bincache-nonprotected \
  $SPACK_ROOT/bin/spack install --fail-fast --no-check-signature python@3.10 \
  && cp -s $($SPACK_ROOT/bin/spack find --format '{prefix}' python@3.10 | head -n1)/bin/* /usr/local/bin/ \
  && yum remove -y python3 && yum clean all && rm -rf /var/yum/* /var/dnf/* \
  && python3 /tmp/get-pip.py && rm /tmp/get-pip.py

INCLUDE ../spack-bootstrap.inc