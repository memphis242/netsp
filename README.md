# :memo: `netsp` - Network Socket Programming
I'll use this as a network socket programming playground for various moments of inspiration or curiosity pursuits.

## Tips for Myself
### Sending a Test Msg to a Server
Quick test of a server awaiting reception of msg:

```bash
echo "<msg>" | nc --udp <ip_address> <port>
```

### Core Dumps Quick Ref
See [commit 781f232](https://github.com/memphis242/netsp/commit/781f232d513844a99f795326a019f0726ea38aa3).   

Brief points:
- On many Linux distros, core dumps are disabled by default (security)
```bash
ulimit -c # check if coredumps are enabled
ulimit -c unlimited # enable unlimited number of coredump files
```

- On Fedora, which is a `systemd`-based system (Debian and Ubuntu as well), core dumps are captured and managed by the `systemd-coredump` service
```bash
coredumpctl list # view list of captured coredumps
coredumpctl dump <pid> > /path/to/dir/core.<name> # extract a coredump file to a desired location
```

- To inspect a coredump /w gdb,
```bash
gdb <program> <coredump_file>
> bt full # to see all stack frames
> frame X # to view a specific stack frame and check out that context,
          # so you can `info locals`, `print <var>`, etc.
```

- Some libraries/programs will actually override the core dump size limitations that would have been inherited from `ulimit`. This can be done using calls to `setrlimit()`/`prlimit()` on `RLIMIT_CORE`. This was the case, for examlpe, for my libsodium playground app. You can find out what the _actual_ core limits placed on your app's process _while it's running_ by doing:
```bash
cat /proc/<pid>/limits
```
and you can investigate whether something is making the `setrlimit()`/`prlimit()` system calls by doing an `strace`:
```bash
strace -f -e prlimit64,prlimit,setrlimit,prctl -o <desired output spot like /tmp/rlimit.trace> ./<your_program>
```
and lastly, you can see the behavior at coredump time if you're not seeing your coredumps as expected by viewing system logs. One way is:
```bash
journalctl -k -b | egrep 'core|dump' # check the journald logs from the kernel from the current boot session
# Example output:
# Jan 03 14:27:43 fedora kernel: coredump: 136455(libsodium_playg): RLIMIT_CORE is set to 1, aborting core
```
You can try to override some third party library from setting `RLIMIT_CORE` to lower than you want by yourself calling `setrlimit()`/`prlimit()` (ideally, _only for debug builds_). For example:
```c
#ifndef NDEBUG
#include <sys/resource.h> // to setrlimit() on RLIMIT_CORE
#endif

#ifndef NDEBUG
struct rlimit core_rlim = { .rlim_cur = RLIM_INFINITY, .rlim_max = RLIM_INFINITY };
rc = setrlimit( RLIMIT_CORE, &core_rlim );
if ( rc != 0 )
{
  (void)fprintf( stderr,
           "Warning: setrlimit() failed, so coredumps might not work.\n"
           "         Returned: %d, errno: %s (%d): %s\n",
           rc, strerrorname_np(errno), errno, strerror(errno) );
}
#endif
```
