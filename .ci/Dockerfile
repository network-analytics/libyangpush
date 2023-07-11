FROM ubuntu

ENV GIT_SSL_NO_VERIFY=1

RUN apt update

RUN apt install git autoconf libtool automake libpython3-dev libssh-dev libjansson-dev libyang-dev python3 pkgconf cmake -y
RUN ln -snf /bin/bash /bin/sh

# Install meson
RUN git clone --depth 1 --branch 1.1.1 https://github.com/mesonbuild/meson meson
RUN cd meson && chmod +x meson.py && ln -s $PWD/meson.py /bin/meson

RUN meson --version

# Install cmocka 1.1.7
RUN git clone --depth 1 --branch cmocka-1.1.7 https://gitlab.com/cmocka/cmocka.git cmocka
RUN cd cmocka && mkdir build && cd build && cmake .. && make install

# Install pcre2 10.42
RUN git clone --depth 1 --branch pcre2-10.42 https://github.com/PCRE2Project/pcre2.git pcre2
RUN cd pcre2 && mkdir build && cd build && cmake .. && make install

# Install xz-utils 5.4.3 (libxml2 dep)
RUN git clone --depth 1 --branch v5.4.3 https://git.tukaani.org/xz.git xz-utils
RUN cd xz-utils && mkdir build && cd build && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON .. && make install

# Install libxml2 2.11.4
RUN git clone --depth 1 --branch v2.11.4 https://github.com/GNOME/libxml2.git libxml2
RUN cd libxml2 && ./autogen.sh && CFLAGS='-O2 -fno-semantic-interposition' ./configure && make check && make install
