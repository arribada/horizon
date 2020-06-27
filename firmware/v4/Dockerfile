FROM ubuntu:18.04

RUN apt-get -qq update
RUN apt-get -qq install -y wget unzip make build-essential libpcre3 libpcre3-dev ruby python python-pip default-jre git

RUN mkdir tools

# Install gcc_arm_none_eabi_7_2018_q2_update
RUN wget -q https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/7-2018q2/gcc-arm-none-eabi-7-2018-q2-update-linux.tar.bz2
RUN tar -xjf gcc-arm-none-eabi-7-2018-q2-update-linux.tar.bz2 -C /tools
RUN rm gcc-arm-none-eabi-7-2018-q2-update-linux.tar.bz2
ENV PATH="/tools/gcc-arm-none-eabi-7-2018-q2-update/bin:$PATH"

# Install Clang 9.0.0
RUN wget -q https://releases.llvm.org/9.0.0/clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz
RUN tar -xJf clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz -C /tools
RUN rm clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz
ENV PATH="/tools/clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-18.04/bin:/tools/clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-18.04/libexec:$PATH"

# Install CMake 3.13.3
RUN wget -q https://github.com/Kitware/CMake/releases/download/v3.13.3/cmake-3.13.3-Linux-x86_64.tar.gz
RUN tar -zxf cmake-3.13.3-Linux-x86_64.tar.gz -C /tools
RUN rm cmake-3.13.3-Linux-x86_64.tar.gz
ENV PATH="/tools/cmake-3.13.3-Linux-x86_64/bin:$PATH"

# Install CppCheck 1.89
RUN wget -q https://github.com/danmar/cppcheck/archive/1.89.tar.gz
RUN tar -zxf 1.89.tar.gz -C /tools
RUN rm 1.89.tar.gz
WORKDIR tools/cppcheck-1.89
RUN make -j$(nproc) --quiet MATCHCOMPILER=yes PREFIX=. FILESDIR=/tools/cppcheck-1.89 HAVE_RULES=yes CXXFLAGS="-O2 -DNDEBUG -w"
WORKDIR /
ENV PATH="/tools/cppcheck-1.89:$PATH"

# Install PMD 6.18.0
RUN wget -q https://github.com/pmd/pmd/releases/download/pmd_releases%2F6.18.0/pmd-bin-6.18.0.zip
RUN unzip -q -n pmd-bin-6.18.0.zip -d /tools
RUN rm pmd-bin-6.18.0.zip
# Rename run.sh to pmd so we actually have a useful executable name to call
RUN mv /tools/pmd-bin-6.18.0/bin/run.sh /tools/pmd-bin-6.18.0/bin/pmd
ENV PATH="/tools/pmd-bin-6.18.0/bin:$PATH"

# Install nrfutil
RUN pip install nrfutil

# Install gcovr
RUN pip install gcovr