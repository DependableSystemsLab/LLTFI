# Base image
FROM ubuntu:20.04

RUN apt-get update
RUN apt-get -y upgrade

# Install tools needed
RUN distro=$(cat /etc/os-release|grep -Po '(?<=^ID=").*(?=")|(?<=^ID=)[^"].*[^"]') \
    && TZ="America/New_York" \
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
          make -j${NPROC} install && \
          cd ${WORK_DIR} && rm -rf ninja; \
       fi \
    && ln -sf /usr/bin/python3 /usr/bin/python
    
RUN pip install tensorflow
RUN pip install tf2onnx
    
WORKDIR /home/
RUN mkdir LLTFI

### LLVM
RUN git clone https://github.com/llvm/llvm-project.git && \
    cd llvm-project && git checkout 23dd750279c9 && cd .. && \
    mkdir llvm-project/build && cd llvm-project/build && \
    cmake -G Ninja ../llvm \
      -DLLVM_ENABLE_PROJECTS="clang;mlir;tools" \
      -DLLVM_BUILD_TESTS=ON \
      -DLLVM_TARGETS_TO_BUILD="host" \
      -DLLVM_ENABLE_ASSERTIONS=ON \
      -DLLVM_ENABLE_RTTI=ON && \
    cmake --build . --target clang check-mlir mlir-translate opt llc lli llvm-dis llvm-link -j 3&& \
    ninja install -j 1 && \
    cd ../.. 

### libprotoc
RUN curl -OL https://github.com/protocolbuffers/protobuf/releases/download/v3.17.2/protobuf-all-3.17.2.zip
RUN unzip protobuf-all-3.17.2.zip
WORKDIR /home/LLTFI/protobuf-3.17.2

RUN ./configure
RUN make
RUN make check
RUN make install
RUN ldconfig

WORKDIR /home/LLTFI/

### ONNX_MLIR
RUN git clone --recursive https://github.com/onnx/onnx-mlir.git && \
    cd onnx-mlir && \
    git checkout 221b8e1d2ad && \
    cd .. && \
    export MLIR_DIR=$(pwd)/llvm-project/build/lib/cmake/mlir && \
    mkdir onnx-mlir/build && cd onnx-mlir/build && \
    if [[ -z "$pythonLocation" ]]; then \
  	cmake -G Ninja \
        -DCMAKE_CXX_COMPILER=/usr/bin/c++ \
        -DMLIR_DIR=${MLIR_DIR} \
        .. \
    else \
  	cmake -G Ninja \
        -DCMAKE_CXX_COMPILER=/usr/bin/c++ \
        -DPython3_ROOT_DIR=$pythonLocation \
        -DMLIR_DIR=${MLIR_DIR} \
        .. \
    fi && \
    cmake --build . -j 2 && \
    export LIT_OPTS=-v && \
    cmake --build . --target check-onnx-lit && \
    ninja install

WORKDIR /home/LLTFI/

### LLTFI
RUN git clone https://github.com/DependableSystemsLab/LLTFI.git && \
    cd /home/LLTFI/LLTFI && \
    ./setup -LLFI_BUILD_ROOT $(pwd)/build -LLVM_SRC_ROOT $(pwd)/../llvm-project -LLVM_DST_ROOT $(pwd)/../llvm-project/build --no_gui && \
    export LLFI_BUILD_ROOT=$(pwd)/build