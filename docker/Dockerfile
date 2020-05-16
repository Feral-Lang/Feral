FROM archlinux/base:latest

RUN pacman -Sy base-devel vim valgrind git bc ccache cmake gmp --needed --noconfirm
RUN pacman -Sy libffi --needed --noconfirm
WORKDIR /feral
RUN echo -e "# build feral\n\
function bf {\n\
        CWD=\$(pwd)\n\
        cd /feral/Feral\n\
        rm -rf dockerbuild && mkdir -p dockerbuild && cd dockerbuild && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j\$(nproc) && make install\n\
        cd \$CWD\n\
}\n\
\n\
alias b='rm -rf dockerbuild && mkdir -p dockerbuild && cd dockerbuild && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j\$(nproc) && make install; cd ..'\n\
alias ccb='rm -rf dockerbuild && mkdir -p dockerbuild && cd dockerbuild && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && make -j\$(nproc) && make install; cd ..'\n\
alias db='rm -rf dockerbuild && mkdir -p dockerbuild && cd dockerbuild && cmake .. -DCMAKE_BUILD_TYPE=Debug && make -j\$(nproc) && make install; cd ..'\n\
\n\
export PATH=$HOME/.feral/bin:$PATH\n" >> ~/.bashrc
