#!/usr/bin/python
import subprocess
import signal
import sys
import os

# Record the path to reap.py before somebody chdirs us.
REAP_PATH = os.path.realpath(__file__)

# The parent PID we were spawned with.  Only valid in sub_reap().
PARENT_PID = None

# The child process we spawned from sub_reap().
CHILD_PROC = None


def Popen(args, **kwargs):
    '''
    Works similarly to Popen.  This will execute reap.py as a standalone script
    in a subprocess, and that sub-reap.py will then execute your desired
    command in another subprocess.  Sub-reap.py will periodically check for
    either a dead child or a new parent PID and either kill itself or kill the
    child as necessary.
    '''
    # Execute ourselves as a sub-process.
    cmd = ['/usr/bin/python', REAP_PATH, '%s' % os.getpid()] + args
    return subprocess.Popen(cmd, **kwargs)


def sigchld(signum, frame):
    '''
    A child died.  We only have one child.  wait() on it and then pass it's
    error code upwards.  os.wait() gives us a weird return status:

        (exit_status << 8) | (signal_number)

    If signal_number is 0, then the subprocesses exited normally with status
    exit_status.  If signal number is non-0 then it's the signal that killed
    us - but to pass that upwards we need to OR it with 0x80.
    '''
    pid, status = os.wait()
    assert pid == CHILD_PROC.pid

    if (status & 0xFF) == 0:
        os._exit(status >> 8)
    os._exit(0x80 | (status & 0xFF))


def sigalrm(signum, frame):
    '''
    Check if our parent PID no longer matches and kill the child if that's the
    case.  If we end up killing the child, we'll later get a SIGCHLD and clean
    ourselves up too.
    '''
    if os.getppid() != PARENT_PID:
        CHILD_PROC.kill()
    else:
        signal.alarm(1)


def sub_reap(args):
    '''
    usage: reap.py parent_pid cmd [args...]

    We install a SIGCHLD handler to watch for dead children, spawn the child
    then install a SIGALRM handler to periodically check for a new parent_pid.
    Using signals avoids race conditions.
    '''
    global PARENT_PID
    global CHILD_PROC

    PARENT_PID = int(args[1])
    signal.signal(signal.SIGCHLD, sigchld)
    signal.signal(signal.SIGALRM, sigalrm)
    CHILD_PROC = subprocess.Popen(args[2:])
    signal.alarm(1)

    while True:
        try:
            signal.pause()
        except:
            pass


if __name__ == '__main__':
    sub_reap(sys.argv)
