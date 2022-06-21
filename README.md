# IPTSDaemon
This is the user space IPTS daemon for Surface touch screen process

It is supposed to be used with `BigSurface.kext` to enable touch screen & stylus support on macOS.

Raw touching data is sent by the kernel driver to be processed in user space then touch & stylus hid events are sent back to the kernel.

The code is ported from linux-surface/iptsd and quo/iptsd

**Warning, the processing algorithm is NOT optimised enough yet and it consumes around 10% cpu usage when fingers are detected, 4% or less when stylus is detected**

Use this under your own consideration! Touching process is very energy consuming.

To install the daemon, simply run the script `install_daemon.sh` and all files needed will be copied to desired locations. After installation IPTSDaemon will be running when macOS starts.

You will also need to install two `dylib` for it, in Terminal `brew install fmt inih`

Then, in order to **resume the service after wakeup**, you need to install `sleepwatcher` using `Homebrew`: `brew install sleepwatcher`

Finally, create ~/.sleep and ~/.wakeup files.

In .sleep: `launchctl unload /Library/LaunchAgents/com.xavier.IPTSDaemon.plist 2>/dev/null`

In .wakeup: `sleep 2 && launchctl load /Library/LaunchAgents/com.xavier.IPTSDaemon.plist`

If you want to disable touch when palm is detected or stylus is in using or near the stylus, go to config folder and find your device's config, add this:

```
[Touch]
DisableOnPalm = true

[Stylus]
# disable touch when using stylus
DisableTouch = true
# disable touch near the stylus
Cone = true
```
