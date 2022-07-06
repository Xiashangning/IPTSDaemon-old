#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"

# remove old IPTSDaemon
launchctl unload /Library/LaunchAgents/com.xavier.IPTSDaemon.plist 2>/dev/null
sudo rm /usr/local/bin/IPTSDaemon 2>/dev/null
sudo rm /Library/LaunchAgents/com.xavier.IPTSDaemon.plist 2>/dev/null
sudo rm -rf /usr/local/ipts_config 2>/dev/null

sudo mkdir -p /usr/local/bin/
sudo chmod -R 755 /usr/local/bin/
sudo cp $DIR/IPTSDaemon /usr/local/bin/
sudo chmod 755 /usr/local/bin/IPTSDaemon
sudo chown root:wheel /usr/local/bin/IPTSDaemon
sudo xattr -d com.apple.quarantine /usr/local/bin/IPTSDaemon 2>/dev/null

sudo cp -r $DIR/ipts_config /usr/local/

sudo cp $DIR/com.xavier.IPTSDaemon.plist /Library/LaunchAgents
sudo chmod 644 /Library/LaunchAgents/com.xavier.IPTSDaemon.plist
sudo chown root:wheel /Library/LaunchAgents/com.xavier.IPTSDaemon.plist
sudo xattr -d com.apple.quarantine /Library/LaunchAgents/com.xavier.IPTSDaemon.plist 2>/dev/null

launchctl load /Library/LaunchAgents/com.xavier.IPTSDaemon.plist
