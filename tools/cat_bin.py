#!/usr/bin/python
import sys
import struct

def cat_bin(lokios_1_bin, lokios_1_sym, lokios_1):
    with open(lokios_1, 'w+b') as l1f:
        # Copy lokios_1_bin into the target file.
        with open(lokios_1_bin, 'rb') as l1bf:
            l1bf_contents = l1bf.read()
            assert (len(l1bf_contents) % 4096) == 0
            assert l1bf_contents[:4] == 'LOKI'
            nsectors = struct.unpack('i', l1bf_contents[4:8])[0]
            assert nsectors == len(l1bf_contents)/512
        l1f.write(l1bf_contents)

        # Concatenate lokios_1_sym into the target file, padding up to a
        # multiple of 4K.
        with open(lokios_1_sym, 'rb') as l1sf:
            l1sf_contents = l1sf.read();
            if (len(l1sf_contents) % 4096) != 0:
                l1sf_contents += '\xCC'*(4096 - (len(l1sf_contents) % 4096))
        l1f.write(l1sf_contents)

        # Fix the num_sectors field.
        nsectors = l1f.tell()/512
        l1f.seek(4)
        l1f.write(struct.pack('i',nsectors))

if __name__ == '__main__':
    if len(sys.argv) != 4:
        print 'usage: cat_bin.py lokios.1.bin lokios.1.sym lokios.1'
        sys.exit(-1)

    cat_bin(sys.argv[1], sys.argv[2], sys.argv[3])
