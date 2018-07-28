To build lokios:

1. Copy your ssh key into .ssh/id_rsa:

scp user@docker.for.mac.localhost:.ssh/id_rsa* ~/.ssh/

This is required for github access, under the assumption that you have set up
your github account to use your desktop's ssh identity.

2. Clone lokios:

git clone git@github.com:tgree/lokios

3. Enter the lokios directory and build:

cd lokios
make -j

4. Have fun!
