#! /bin/bash

export GIT_SSL_NO_VERIFY=1

apt update

apt install autoconf libtool automake libpython3-dev libssh-dev libjansson-dev libyang-dev -y

# Install cmocka 1.1.7
git clone --depth 1 --branch cmocka-1.1.7 https://gitlab.com/cmocka/cmocka.git cmocka
$(cd cmocka && mkdir build && cd build && cmake .. && make install)

# Install pcre2 10.42
git clone --depth 1 --branch pcre2-10.42 https://github.com/PCRE2Project/pcre2.git pcre2
$(cd pcre2 && mkdir build && cd build && cmake .. && make install)

# Install xz-utils 5.4.3 (libxml2 dep)
git clone --depth 1 --branch v5.4.3 https://git.tukaani.org/xz.git xz-utils
$(cd xz-utils && mkdir build && cd build && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON .. && make install)

# Install libxml2 2.11.4
git clone --depth 1 --branch v2.11.4 https://github.com/GNOME/libxml2.git libxml2
$(cd libxml2 && ./autogen.sh && CFLAGS='-O2 -fno-semantic-interposition' ./configure && make check && make install)
