# Base image
FROM ubuntu:20.04

ARG NPROC=2

RUN apt-get update
RUN apt-get -y upgrade

# Install tools needed
RUN distro=$(cat /etc/os-release|grep -Po '(?<=^ID=").*(?=")|(?<=^ID=)[^"].*[^"]') \
    && TZ="America/Vancouver" \
    && if [ "${distro}" = "debian" ] || [ "${distro}" = "ubuntu" ]; then \
          DEBIAN_FRONTEND=noninteractive && \
          apt-get update -qq && \
          apt-get install -qq -y --no-install-recommends tzdata && \
          ln -sf /usr/share/zoneinfo/${TZ} /etc/localtime && \
          dpkg-reconfigure -f noninteractive tzdata && \
          apt-get install -qq -y --no-install-recommends \
                  autoconf automake ca-certificates cmake curl \
                  default-jdk-headless gcc g++ git libncurses-dev \
                  libtool make maven ninja-build openjdk-11-jdk-headless \
                  python3 python3-dev python3-distutils python3-numpy \
                  python3-pip python3-pytest-xdist python3-setuptools \
                  python3-typing-extensions zlib1g-dev && \
          rm -rf /var/lib/apt/lists/* && \
          ln -sf /usr/bin/pytest-3 /usr/bin/pytest; \
       elif [ "${distro}" = "rhel" ] || [ "${distro}" = "fedora" ]; then \
          ln -sf /usr/share/zoneinfo/${TZ} /etc/localtime && \
	  ([ -x /usr/bin/microdnf ] && microdnf install -y yum) && \
          yum install -q -y \
              https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm && \
          yum update -q -y && \
          yum install -q -y \
              autoconf automake ca-certificates cmake diffutils \
              file java-11-openjdk-devel java-11-openjdk-headless \
              gcc gcc-c++ git libtool make ncurses-devel \
              python39 python39-devel python39-numpy python39-pip \
              python39-setuptools python39-wheel zlib-devel && \
          # Use same versions as those in ubuntu:focal
          pip3 install -q \
               Cython pytest==4.6.9 pytest-forked==1.1.3 \
               pytest-xdist==1.31.0 typing-extensions==3.7.4.1 && \
          rm -rf /var/cache/dnf/* && \
          # Install ninja
          git clone -b v1.10.2 https://github.com/ninja-build/ninja.git && \
          cd ninja && mkdir -p build && cd build && \
          cmake .. && \
          make -j${NPROC} install; \
       fi \
    && ln -sf /usr/bin/python3 /usr/bin/python
    
RUN pip install tensorflow
RUN pip install tf2onnx
RUN pip3 install pyyaml===5.4.1
   
WORKDIR /home/

### LLVM
RUN git clone https://github.com/llvm/llvm-project.git && \
    cd llvm-project && git checkout 9778ec057cf4 && cd .. && \
    mkdir llvm-project/build && cd llvm-project/build && \
    cmake -G Ninja ../llvm \
      -DLLVM_ENABLE_PROJECTS="clang;mlir" \
      -DLLVM_TARGETS_TO_BUILD="host" \
      -DLLVM_ENABLE_ASSERTIONS=ON \
      -DCMAKE_BUILD_TYPE=Release \
      -DLLVM_ENABLE_RTTI=ON && \
    cmake --build . --target clang check-mlir mlir-translate opt llc lli llvm-dis llvm-link -j${NPROC} && \
    ninja install -j${NPROC} && \
    cd ../.. 

RUN apt-get update
RUN apt-get install unzip
RUN apt-get install -y wget

### libprotoc
RUN curl -OL https://github.com/protocolbuffers/protobuf/releases/download/v3.17.2/protobuf-all-3.17.2.zip
RUN unzip protobuf-all-3.17.2.zip
WORKDIR /home/protobuf-3.17.2

RUN ./configure
RUN make -j${NPROC}
RUN make check -j${NPROC}
RUN make install
RUN ldconfig

WORKDIR /home/
RUN git clone https://github.com/DependableSystemsLab/LLTFI.git


### ONNX_MLIR
RUN git clone --recursive https://github.com/DependableSystemsLab/onnx-mlir-lltfi.git && \
    export MLIR_DIR=$(pwd)/llvm-project/build/lib/cmake/mlir && \
    mv onnx-mlir-lltfi onnx-mlir && \
    cd onnx-mlir && \
    git checkout LLTFI && \
    cd .. && \
    mkdir onnx-mlir/build && cd onnx-mlir/build && \
    cmake -G Ninja \
        -DCMAKE_CXX_COMPILER=/usr/bin/c++ \
        -DMLIR_DIR=${MLIR_DIR} \
        .. && \
    cmake --build . -j${NPROC} && \
    ninja install

WORKDIR /home/LLTFI

### LLTFI
RUN ./setup -LLFI_BUILD_ROOT $(pwd)/build -LLVM_SRC_ROOT $(pwd)/../llvm-project -LLVM_DST_ROOT $(pwd)/../llvm-project/build && \
    export LLFI_BUILD_ROOT=$(pwd)/build && \
    cd /home/LLTFI/tools && \
    sh json-c-setup.sh

### Setting up the env variables
ENV LLFI_BUILD_ROOT=/home/LLTFI/build
ENV ONNX_MLIR_BUILD=/home/onnx-mlir/build
ENV ONNX_MLIR_SRC=/home/onnx-mlir
ENV LLVM_DST_ROOT=/home/llvm-project/build  

### Setting environment variables for custom include and library paths for json-c library
ENV C_INCLUDE_PATH=/root/local/include:$C_INCLUDE_PATH
ENV CPLUS_INCLUDE_PATH=/root/local/include:$CPLUS_INCLUDE_PATH
ENV LIBRARY_PATH=/root/local/lib:$LIBRARY_PATH
ENV LD_LIBRARY_PATH=/root/local/lib:$LD_LIBRARY_PATH

WORKDIR /home/LLTFI