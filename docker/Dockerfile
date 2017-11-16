# Build with: docker build --build-arg username=YOUR_NAME --build-arg name='FIRST_NAME LAST_NAME' --build-arg email=YOUR@EMAIL -t lokios-build .
# Run with  : docker run --tmpfs /tmp:rw,size=2G,mode=1777 -ti lokios-build
FROM ubuntu:latest

# Install the base OS packages.
RUN apt-get -y update && apt-get install -y \
    g++                                     \
    gdb                                     \
    git                                     \
    make                                    \
    man                                     \
    manpages                                \
    manpages-dev                            \
    python                                  \
    sudo                                    \
    tig                                     \
    tmux                                    \
    vim

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
