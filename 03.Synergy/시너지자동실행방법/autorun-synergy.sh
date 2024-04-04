#!/bin/bash

killall synergyc
sleep 1
sudo chmod 777 /home/pi/Synergy/synergyc
/home/pi/Synergy/synergyc --name RASPBERRY-PI 192.168.0.46
exit 0
