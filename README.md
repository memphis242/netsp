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
