# Build with: docker build --build-arg username=YOUR_NAME --build-arg name='FIRST_NAME LAST_NAME' --build-arg email=YOUR@EMAIL -t lokios-build .
# Run with  : docker run --cap-add=SYS_PTRACE --security-opt seccomp=unconfined --tmpfs /tmp:rw,size=2G,mode=1777 -ti lokios-build
#
# Note that the special privileges are only required for use with gdb if you
# want to debug unittests.  If you are concerned about the privilege level and
# only want to do builds then you can run without the extra privileges.

# We require Ubuntu 16.04.  Later versions of Ubuntu seem to have all sorts of
# problems building with PIC/PIE and mcmode=kernel even though we are providing
# all our own tools.
FROM ubuntu:16.04

# Install the base OS packages.
RUN apt-get update -y                                   \
 && apt-get install -y                                  \
    ctags                                               \
    gdb                                                 \
    git                                                 \
    libc6-dev-i386                                      \
    make                                                \
    man                                                 \
    manpages                                            \
    manpages-dev                                        \
    python                                              \
    python-requests                                     \
    qemu                                                \
    software-properties-common                          \
    sudo                                                \
    systemtap-sdt-dev                                   \
    telnet                                              \
    tig                                                 \
    tmux                                                \
    vim                                                 \
 && add-apt-repository ppa:ubuntu-toolchain-r/test -y   \
 && apt-get update -y                                   \
 && apt-get install -y                                  \
    g++-7                                               \
    gcc-7                                               \
    gcc-7-source                                        \
    lib32gcc-7-dev                                      \
 && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 60 --slave /usr/bin/g++ g++ /usr/bin/g++-7 \
 && update-alternatives --config gcc

# Configure symlink to latest gcc source.
RUN cd /usr/src/gcc-7       \
 && tar xf gcc-7.?.?.tar.xz \
 && ln -s gcc-7.?.? latest

# Copy in the filesystem.
COPY fs/ /

# Set up the user.
ARG username
COPY fs/home/userhome /home/$username
RUN useradd -M -s /bin/bash $username  \
 && echo $username:password | chpasswd \
 && adduser $username sudo             \
 && chown -R $username:$username /home/$username

# Switch to the new user and set docker to execute all further commands out of the
# new user's home directory.
USER $username
WORKDIR /home/$username

# Do user-specific configuration here.
ARG name
ARG email
RUN git config --global user.email "$email" \
 && git config --global user.name "$name"   \
 && git config --global push.default matching
