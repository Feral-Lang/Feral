FROM archlinux/base:latest

RUN pacman -Sy libffi base-devel vim valgrind git bc ccache cmake gmp --needed --noconfirm
WORKDIR /feral
RUN echo -e "export PREFIX_DIR='/usr'\n\
# build feral\n\
function bf {\n\
        CWD=\$(pwd)\n\
        cd /feral/Feral\n\
        rm -rf dockerbuild && mkdir -p dockerbuild && cd dockerbuild && cmake .. && make -j\$(nproc) && make install\n\
        cd \$CWD\n\
}\n\
\n\
# build feral stdlib\n\
function bfs {\n\
        CWD=\$(pwd)\n\
        cd /feral/Feral-Std\n\
        rm -rf dockerbuild && mkdir -p dockerbuild && cd dockerbuild && cmake .. && make -j\$(nproc) && make install\n\
        cd \$CWD\n\
}\n\
\n\
alias b='rm -rf dockerbuild && mkdir -p dockerbuild && cd dockerbuild && cmake .. && make -j\$(nproc) && make install; cd ..'\n\
alias ccb='rm -rf dockerbuild && mkdir -p dockerbuild && cd dockerbuild && cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && make -j\$(nproc) && make install; cd ..'\n\
alias db='rm -rf dockerbuild && mkdir -p dockerbuild && cd dockerbuild && DEBUG=true cmake .. && make -j\$(nproc) && make install; cd ..'\n" >> ~/.bashrc