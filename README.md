# WhaleOS
WhaleOS 是个暂时的名字，在将仓库移到 gitlab 之前会改
<br><br>
关于 loongarch 分支的一个和说明：
现在的 loongarch 还在开发中，临时的 loongarch 内核的 Makefile 位于 /kernel/arch/loongarch 下。需要调试的话还请先 cd 至此目录下
<br><br>
## Get Start
### riscv-gnu-toolchains
按照[官方文档](https://github.com/riscv-collab/riscv-gnu-toolchain)安装即可

```bash
sudo apt-get install autoconf automake autotools-dev curl python3 python3-pip python3-tomli libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev libexpat-dev ninja-build git cmake libglib2.0-dev libslirp-dev

git clone --recursive https://github.com/riscv-collab/riscv-gnu-toolchain.git \
&& cd riscv-gnu-toolchain \
&& ./configure --prefix=/opt/riscv \
&& make -j8 \
&& sudo make install
```
<br>
亦可以选择从[中科院软件所 ISCAS 提供的镜像](https://mirror.iscas.ac.cn/riscv-toolchains/release/riscv-collab/riscv-gnu-toolchain/LatestRelease/)获取 riscv-gnu-toolchain，选择 `[riscv64-elf-ubuntu-[version]-gcc-nightly-[date]-nightly.tar.xz` 一项下载然后将其 `tar -Jxvf` 到 `/opt` 下，并添加到 PATH
<br><br>

### qemu-system-*
```bash
cd ~/Downloads
wget wget https://download.qemu.org/qemu-9.2.1.tar.xz -O qemu-9.2.1.tar.xz \
    && tar xf qemu-9.2.1.tar.xz \
    && cd qemu-9.2.1 \
    && ./configure \
        --target-list=loongarch64-softmmu,riscv64-softmmu,aarch64-softmmu,x86_64-softmmu \
        --enable-gcov --enable-debug --enable-slirp \
    && make -j8 \
    && sudo make install
```
<br>

### loongarch-unknown-linux-gnu
赛方提供了[环境](https://gitlab.educg.net/wangmingjian/os-contest-2024-image/)
```bash
cd ~/Downloads
wget https://github.com/loongson/build-tools/releases/download/2022.05.29/loongarch64-clfs-5.0-cross-tools-gcc-full.tar.xz \
&& sudo tar -vxf loongarch64-clfs-5.0-cross-tools-gcc-full.tar.xz -C /opt \
&& echo "\n\n# loongarch cross-tools" >> $RC \
&& echo "CC_PREFIX=/opt/cross-tools" >> $RC \
&& echo 'export PATH=$PATH:$CC_PREFIX/bin' >> $RC \
&& echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$CC_PREFIX/lib' >> $RC \
&& echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$CC_PREFIX/loongarch64-unknown-linux-gnu/lib/' >> $RC \
source $RC
```

### loongarch-unknown-linux-gnu-gdb
```bash
cd ~/Downloads
git clone https://github.com/foxsen/binutils-gdb \
&& cd binutils-gdb \
&& git checkout loongarch-v2022-03-10 \
&& mkdir build \
&& cd build \
&& ../configure --target=loongarch64-unknown-linux-gnu --prefix=/opt/gdb --disable-werror --without-python --disable-doc \
&& make -j8 \
&& sudo make install
```
<br>


## Run & Debug
```bash
cd /path/to/WhaleOS
make run
```
使用 `Ctrl + a` 再按下 `x` 可以退出 qemu
<br><br>
若要调试，可以在项目目录下运行 `make gdb`，然后打开另一个终端输入：
```bash
cd /path/to/WhaleOS
riscv64-unknown-elf-gdb -x .gdbinit
```
建议在 `.*rc` 里面设置 `alias` 或 `function` 方便调试
<br><br>
注意，如果你的 riscv-gnu-toolchain 使用的是编译镜像，这里可能需要使用 `gdb-multiarch`
