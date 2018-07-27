To build lokios:

1. Copy your ssh key into .ssh/id_rsa:

scp user@docker.for.mac.localhost:.ssh/id_rsa* ~/.ssh/

2. Clone lokios:

git clone git@github.com:tgree/lokios

3. Enter the lokios directory and build:

cd lokios
make

4. Have fun!
