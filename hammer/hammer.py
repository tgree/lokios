#!/usr/bin/python
import qemu
import loki

import argparse
import sys
import os


def spawn(image, port, command_line, pxe):
    # Spawn a qemu instance, letting it inherit all the file descriptors.
    if pxe:
        tftp     = os.path.dirname(image)
        bootfile = os.path.basename(image)
    else:
        tftp     = None
        bootfile = None

    objects = [
        qemu.CPU('qemu64',['popcnt']),
        qemu.ISADebugExitDev(),
        qemu.VirtioNetDev(tcp_fwd={12346:port}, pcap='net0dump.pcap',
                          tftp=tftp, bootfile=bootfile)]
    if not pxe:
        objects.append(qemu.Disk(image))

    p = qemu.Popen(cpus=2, objects=objects, no_run=command_line)

    # If we aren't supposed to spawn it, just print the command line and exit.
    if command_line:
        print ' '.join(p.args)
        sys.exit(0)

    # Close our copy of stdin so that any keyboard input will go to the qemu
    # subprocess instead of randomly being distributed between us and the child.
    sys.stdin.close()

    return p


def run_standalone(p, n):
    # We pass the Loki exit code out as our exit code.
    p.proc.wait()
    return p.proc.returncode


def run_hammer(n, timeout):
    # Try to connect to the qemu Loki instance.
    if not n.connect(timeout=timeout):
        print 'hammer: Failed to connect to %s, exiting.' % n
        sys.exit(2)
    print 'hammer: Connected to %s.' % n

    # Run tests here.

    # Stop loki and return the expected guest exit code.
    n.shutdown(11)
    return 11


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--standalone', action='store_true',
                        help="if set, start lokios but don't run tests")
    parser.add_argument('--command-line', action='store_true',
                        help="instead of starting hammer, just print the "
                             "command line that would be used to spawn qemu")
    parser.add_argument('--live', action='store_true',
                        help="connect to a live loki node instead of spawning "
                             "a new qemu instance")
    parser.add_argument('--pxe', action='store_true',
                        help="serve the image as the pxe bootfile instead of "
                             "as a block device")
    parser.add_argument('-i', '--ip', default='127.0.0.1')
    parser.add_argument('-p', '--port', default=12346, type=int)
    parser.add_argument('image', default='', nargs='?',
                        help='image to load, default is '
                             'bin/lokios.elf.mbr or bin/lokios.0 for --pxe')
    rv = parser.parse_args()

    if rv.image == '':
        if rv.pxe:
            rv.image = 'bin/lokios.0'
        else:
            rv.image = 'bin/lokios.elf.mbr'

    if rv.standalone and rv.live:
        print "Don't specify both --standalone and --live"
        sys.exit(1)

    if not rv.live:
        p = spawn(rv.image, rv.port, rv.command_line, rv.pxe)

    n = loki.Node(rv.ip, rv.port)

    if rv.standalone:
        exit_code = run_standalone(p, n)
    else:
        if rv.live:
            timeout = 2
        elif rv.pxe:
            timeout = 30
        else:
            timeout = 10
        expected_code = run_hammer(n, timeout)
        if not rv.live:
            p.proc.wait()
            assert p.proc.returncode == expected_code
        exit_code = 0

    sys.exit(exit_code)
