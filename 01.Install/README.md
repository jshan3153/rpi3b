# raspberry-pi
raspberry-pi 3b+ 기준

## 1. 라즈베리파이 버전 확인 방법
$cat /proc/device-tree/model

## 2. 수동 업데이트 및 업그레이드 : apt(Advance Packging Tools)

	2.1 설치 가능한 패키지 리스트를 최신화
	$sudo apt-get update

	2.2 현재 패키지 리스트 불러오기
	$sudo apt list

	2.3 설치된 패키시 리스트 중에서 업그레이드가 필요한 목록을 불러오기
	$sudo apt list --upgrable

	2.4 실제 패키지 업데이트
	$sudo apt-get upgrade

	2.5 패키지 의존성 포함 삭제
	$sudo apt remove --purge [패키지이름]
	
	2.6 설치된 패키지 위치 
	$dpkg -L [패키지명]

## 3. 한글 폰트 및 키보드 설정 
	3.1  폰트 설치
	$sudo apt install fonts-nanum
    $sudo apt install fonts-unfonts-core  #확인일:24-04-03
	
	3.2 키보드 설치
	$sudo apt install fcitx fcitx-hangul
 	$sudo apt install ibus ibus-hangul

	3.3 관리자 권한으로 파일 매니저 실행
	$ sudo pcmanfm
	$ cd /etc/default/im-config
	IM_CONFIG_DEFAULT_MODE=AUTO -> fcitx 변경
	기본 설정 > 입력기 
 	$ cd /etc/default/im-config 파일이 없는 경우
  	$ sudo apt install im-config 설치
	
## 4. gcc 컴파일을 위한 설정
	4.1 wiringPI 설치
	$git clone https://github.com/WiringPi/WiringPi
	$cd WiringPi
	$./build
	$gpio -v
	
	4.2 libcur 설치
	$sudo apt-get install libcurl4-openssl-dev
	
