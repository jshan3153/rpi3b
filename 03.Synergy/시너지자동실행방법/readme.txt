#RPI 부팅 후 시너지 자동으로 실행되는 방법

1. /home/pi/.config/폴더 밑에 자동 실행 폴더(autostart)가 없으면 만든다.
 : /home/pi/.config/autostart/

2. 1)폴더에 바로가기 파일(*.desktop) 만든다. 파일명은 무관하고, 확장자가 *.desktop 이여야 한다.
   바로가기 파일은 시너지를 실행하는 쉘 스크립트 호출한다.
 : /home/pi/.config/autostart/autorun-synergy.desktop 

  [Desktop Entry]
   Exec=sudo bash /home/pi/Synergy/autorun-synergy.sh

3. 쉘 스크립트에서 시너지를 실행하고 서버에 연결한다.
 : /home/pi/synergy/autorun-synergy.sh

  #!/bin/bash
  killall synergyc
  sleep 1
  /home/pi/Synergy/synergyc --name RASPBERRY-PI 192.168.0.46
  exit 0
