import subprocess
import reap
import os

class Disk(object):
    def __init__(self, path):
        super(Disk, self).__init__()
        self.path   = path
        self.params = ['-drive', 'file=%s,format=raw' % self.path]
        if not os.path.isfile(self.path):
            raise Exception('Not a file: %s' % self.path)


class CPU(object):
    def __init__(self, model, features=[]):
        super(CPU, self).__init__()
        self.model    = model
        self.features = ['+' + f for f in features]
        self.params   = ['-cpu', ','.join([self.model] + self.features)]


NET_ID = 0
class VirtioNetDev(object):
    def __init__(self, tcp_fwd={}, pcap=None, tftp=None, bootfile=None):
        super(VirtioNetDev, self).__init__()
        global NET_ID
        self.net_id   = NET_ID
        self.tcp_fwd  = tcp_fwd
        self.pcap     = pcap
        self.tftp     = tftp
        self.bootfile = bootfile
        NET_ID      += 1

        self.params = ['-device', 'virtio-net-pci,netdev=net%u,'
                       'disable-modern=off,'
                       'vectors=4' % self.net_id,
                       '-netdev', 'user,id=net%u' % self.net_id]
        for k, v in self.tcp_fwd.iteritems():
            self.params[-1] += ',hostfwd=tcp::%u-:%u' % (k, v)
        if self.tftp:
            self.params[-1] += ',tftp=%s' % self.tftp
        if self.bootfile:
            self.params[-1] += ',bootfile=%s' % self.bootfile
            self.params += ['-boot', 'n']
        if self.pcap:
            self.params += ['-object', 'filter-dump,id=dump%u,netdev=net%u,'
                            'file=%s' % (self.net_id, self.net_id, self.pcap)]


class ISADebugExitDev(object):
    def __init__(self):
        super(ISADebugExitDev, self).__init__()
        self.params = ['-device', 'isa-debug-exit']


class Popen(object):
    '''
    Class for controlling lokios under qemu.
    '''
    def __init__(self, cpus=2, mem_megs=64, objects=[], stdout=None,
                 stderr=None, stdin=None, no_run=False, *args, **kwargs):
        super(Popen, self).__init__()
        self.cpus     = cpus
        self.mem_megs = mem_megs
        self.objects  = objects
        self.args     = ['/usr/bin/qemu-system-x86_64',
                         '-smp', '%u' % self.cpus,
                         '-m', '%u' % self.mem_megs,
                         '-nographic']
        for o in self.objects:
            self.args += o.params

        if not no_run:
            self.proc = reap.Popen(self.args, stdout=stdout, stderr=stderr,
                                   stdin=stdin, *args, **kwargs)
