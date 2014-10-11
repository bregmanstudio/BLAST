#!/bin/bash
sudo ifconfig eth2 192.168.0.200 netmask 255.255.255.0
adb kill-server
ADBHOST=192.168.0.202 adb devices
adb shell

