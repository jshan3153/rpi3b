#!/bin/bash
killall synergyc
sleep 1
/home/pi/synergy/synergyc --name RASPBERRY-PI 192.168.0.41
exit 0