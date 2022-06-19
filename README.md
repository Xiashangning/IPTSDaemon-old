# IPTSDaemon
This is the user space IPTS daemon for Surface touch screen process

It is supposed to be used with `BigSurface.kext` to enable touch screen & stylus support on macOS.

Raw touching data is sent by the kernel driver to be processed in user space then hid events are sent back to the kernel.

The code is ported from linux-surface/iptsd and quo/iptsd

**Warning, the processing algorithm is NOT optimised enough and it consumes around 10% cpu usage when fingers are detected, 4% or less when stylus is detected**

Use this under your own consideration! Touching process is very energy consuming.

To install the daemon, simply run the script `install_daemon.sh` and IPTSDaemon will be started when macOS starts.

Then, in order to resume the service after wakeup, you need to install `sleepwatcher` using `Homebrew`: `brew install sleepwatcher`

Finally, create ~/.sleep and ~/.wakeup files.

In .sleep: `launchctl unload /Library/LaunchAgents/com.xavier.IPTSDaemon.plist 2>/dev/null`

In .wakeup: `sleep 2 && launchctl load /Library/LaunchAgents/com.xavier.IPTSDaemon.plist`
