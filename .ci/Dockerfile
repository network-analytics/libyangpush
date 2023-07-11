FROM ubuntu:20.04

ENV GIT_SSL_NO_VERIFY=1

RUN apt update

RUN DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt install git wget libtool libpython3-dev libssh-dev libjansson-dev libyang-dev python3 pkgconf cmake -y

# Install m4@1.4.19
RUN wget --no-check-certificate https://ftp.gnu.org/gnu/m4/m4-1.4.19.tar.gz
RUN tar xvf m4-1.4.19.tar.gz
RUN cd m4-1.4.19 && ./configure && make install

# Install autoconf@2.71
RUN wget --no-check-certificate https://ftp.gnu.org/gnu/autoconf/autoconf-2.71.tar.gz
RUN tar xvf autoconf-2.71.tar.gz
RUN cd autoconf-2.71 && ./configure && make install

# Install automake@1.16.5
RUN wget --no-check-certificate https://ftp.gnu.org/gnu/automake/automake-1.16.5.tar.gz
RUN tar xvf automake-1.16.5.tar.gz
RUN cd automake-1.16.5 && ./configure && make install

# Install libtool@2.4.7
RUN wget --no-check-certificate https://ftp.gnu.org/gnu/libtool/libtool-2.4.7.tar.gz
RUN tar xvf libtool-2.4.7.tar.gz
RUN cd libtool-2.4.7 && ./configure && make install

# Install pkgconf@1.9.5
RUN git clone --depth 1 --branch pkgconf-1.9.5 https://github.com/pkgconf/pkgconf.git pkgconf
RUN cd pkgconf && ./autogen.sh && ./configure && make install

# Install meson@1.1.1
RUN git clone --depth 1 --branch 1.1.1 https://github.com/mesonbuild/meson meson
RUN cd meson && chmod +x meson.py && ln -s $PWD/meson.py /bin/meson

# Install cmocka@1.1.7
RUN git clone --depth 1 --branch cmocka-1.1.7 https://gitlab.com/cmocka/cmocka.git cmocka
RUN cd cmocka && mkdir build && cd build && cmake .. && make install

# Install pcre2@10.42
RUN git clone --depth 1 --branch pcre2-10.42 https://github.com/PCRE2Project/pcre2.git pcre2
RUN cd pcre2 && mkdir build && cd build && cmake .. && make install

# Install xz-utils@5.4.3 (libxml2 dep)
RUN git clone --depth 1 --branch v5.4.3 https://git.tukaani.org/xz.git xz-utils
RUN cd xz-utils && mkdir build && cd build && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON .. && make install

# Install libxml2@2.11.4
RUN wget --no-check-certificate https://github.com/GNOME/libxml2/archive/refs/tags/v2.11.4.tar.gz
RUN tar xvf v2.11.4.tar.gz && cd libxml2-2.11.4 && ./autogen.sh && CFLAGS='-O2 -fno-semantic-interposition' ./configure && make install
