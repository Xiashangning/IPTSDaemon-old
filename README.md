# IPTSDaemon

This is the user space IPTS daemon for Surface touch screen process

It is supposed to be used with `BigSurface.kext` to enable touch screen & stylus support on macOS.

Raw touching data is sent by the kernel driver to be processed in user space then touch & stylus hid events are sent back to the kernel.

The code is ported from linux-surface/iptsd and quo/iptsd

**Warning, the processing algorithm is NOT optimised enough yet and it consumes around 10% cpu usage when fingers are detected, 4% or less when stylus is detected**

Use this under your own consideration! Touching process is very energy consuming.

To install the daemon, simply run the script `install_daemon.sh` and all files needed will be copied to desired locations. After installation IPTSDaemon will be running when macOS starts.

You will also need to install two `dylib`(`fmt` and `inih`) and a daemon(`sleepwatcher`) for it to run properly.

`Homebrew` is recommended to install them:

1. Install [Homebrew](https://brew.sh)

2. in `Terminal`, execute `brew install fmt inih sleepwatcher`

Then, in order to **resume the service after wakeup**, you need to configure `sleepwatcher`:

1. Create ~/.sleep and ~/.wakeup files.
2. In .sleep: `launchctl unload /Library/LaunchDaemons/com.xavier.IPTSDaemon.plist 2>/dev/null`
3. In .wakeup: `sleep 2 && launchctl load /Library/LaunchDaemons/com.xavier.IPTSDaemon.plist`

For example, you can:

```
cd ~
touch .sleep
touch .wakeup
echo "launchctl unload /Library/LaunchDaemons/com.xavier.IPTSDaemon.plist 2>/dev/null" >> .sleep
echo "sleep 2 && launchctl load /Library/LaunchDaemons/com.xavier.IPTSDaemon.plist" >> .wakeup
```

If you want to disable touch when palm is detected or stylus is detected or near the stylus, go to config folder and find your device's config, add this:

```
[Touch]
DisableOnPalm = true

[Stylus]
# disable touch when using stylus
DisableTouch = true
# disable touch near the stylus
Cone = true
```

### Enable on screen keyboard on login screen

To enable the on screen keyboard to show up on the login screen you need to change your Accessibility settings in the `System Preferences>Users & Groups>Login Options>Accessibility Options` put a checkbox on the `Accessibility Keyboard`.
