라즈베리파이 시너지 설치

1. v1.8.8-stable.tar.gz 파일 다운로드하고 압축 풀기 
	https://sourceforge.net/projects/synergy-stable-builds/files/v1.8.8-stable/

2. cmake 하기 위한 준비사항 (시너지 위키에 해당 명령이 설명되어 있다.)
 	https://github.com/symless/synergy-core/wiki/Compiling-v1.3.5-to-v1.8-%28legacy%29#Debian_78

 2.1 필요한 패키지를 설치한다.
 	Debian 7/8
 	sudo apt-get install build-essential cmake libavahi-compat-libdnssd-dev libcurl4-openssl-dev libssl-dev lintian python qt4-dev-tools xorg-dev fakeroot

 	Ubuntu 10.04 to 15.10
 	sudo apt-get install git cmake make g++ xorg-dev libcurl4-openssl-dev libavahi-compat-libdnssd-dev libssl-dev libx11-dev 
	(libqt4-dev 설치 안해도 됨)
 
3. (1)에서 압축 푼 폴더에 CMakeList.txt 파일에서 아래 내용 변경한다.

	Find the following:
	set(CMAKE_INCLUDE_PATH "${CMAKE_INCLUDE_PATH}:/usr/local/include")

	Change it to:
	set(CMAKE_INCLUDE_PATH "${CMAKE_INCLUDE_PATH}:/usr/include")

4. src/CMakeLists.txt 주석처리 
	googletest 패키지를 설치하지 않기 때문에 아래 하위디렉토리를 빌드하지 못하도록 주석처리한다.

 	#if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "IRIX")
 	#	add_subdirectory(test)
 	#endif()

5. src/lib/syngergy/KeyMap.h 헤더 파일에 2군데 주석처리한다. 

 	5.1 //#include "gtest/gtest_prod.h"

 	5.2  private:
	      /*
		FRIEND_TEST(KeyMapTests,
					findBestKey_requiredDown_matchExactFirstItem);
		FRIEND_TEST(KeyMapTests,
			findBestKey_requiredAndExtraSensitiveDown_matchExactFirstItem);
		FRIEND_TEST(KeyMapTests,
			findBestKey_requiredAndExtraSensitiveDown_matchExactSecondItem);
		FRIEND_TEST(KeyMapTests,
			findBestKey_extraSensitiveDown_matchExactSecondItem);
		FRIEND_TEST(KeyMapTests,
					findBestKey_noRequiredDown_matchOneRequiredChangeItem);
		FRIEND_TEST(KeyMapTests,
					findBestKey_onlyOneRequiredDown_matchTwoRequiredChangesItem);
		FRIEND_TEST(KeyMapTests, findBestKey_noRequiredDown_cannotMatch);
	     */
	5.3 

6.(1)에서 압축 푼 폴더에서 cmake 실행한다.
	$sudo cmake .

7. make 실행하면 bin 폴더에 클라이언트와 서버파일이 만들어진다. 
 	$sudo make 를 실해하면 에러가 발생한다. 
	
	7.1 에러 #1
	
	from /home/rpi4b/Downloads/afzaalace-synergy-stable-builds-c30301e/src/lib/arch/Arch.cpp:19:
	/usr/include/c++/11/bits/functional_hash.h:265:12: error: redefinition of ‘struct std::hash<long int>’
  	265 |     struct hash<nullptr_t> : public __hash_base<size_t, nullptr_t>
      		|            ^~~~~~~~~~~~~~~
	/usr/include/c++/11/bits/functional_hash.h:157:3: note: previous definition of ‘struct std::hash<long int>’
  	157 |   _Cxx_hashtable_define_trivial_hash(long)
	
	<해결책> /usr/include/c++/11/bits/functional_hash.h 파일에 주석 처리 
	  /// Explicit specialization for long.
	  //_Cxx_hashtable_define_trivial_hash(long)
	  
	7.2 에러 #2
	/usr/include/c++/11/ostream:250:7: error: ‘std::basic_ostream<_CharT, _Traits>::__ostream_type& std::basic_ostream<_CharT, _Traits>::operator<<(std::nullptr_t)’ cannot be overloaded with ‘std::basic_ostream<_CharT, _Traits>::__ostream_type& std::basic_ostream<_CharT, _Traits>::operator<<(long int)’
  250 |       operator<<(nullptr_t)
      |       ^~~~~~~~
/usr/include/c++/11/ostream:166:7: note: previous declaration ‘std::basic_ostream<_CharT, _Traits>::__ostream_type& std::basic_ostream<_CharT, _Traits>::operator<<(long int)’
  166 |       operator<<(long __n)
      |       ^~~~~~~~
	==> 해결책 정의문 주석 처리 
		#if __cplusplus >= 201703L
		//      __ostream_type&
		//      operator<<(nullptr_t)
		//      { return *this << "nullptr"; }
	    #endif

	7.3 에러 #3
	/home/rpi4b/Downloads/afzaalace-synergy-stable-builds-c30301e/src/lib/net/SecureSocket.cpp: In member function ‘void SecureSocket::showSecureCipherInfo()’:
/home/rpi4b/Downloads/afzaalace-synergy-stable-builds-c30301e/src/lib/net/SecureSocket.cpp:810:53: error: invalid use of incomplete type ‘SSL’ {aka ‘struct ssl_st’}
  810 |         STACK_OF(SSL_CIPHER) * cStack = m_ssl->m_ssl->session->ciphers;
      |                                                     ^~
In file included from /usr/include/openssl/crypto.h:36,
                 from /usr/include/openssl/comp.h:22,
                 from /usr/include/openssl/ssl.h:28,
                 from /home/rpi4b/Downloads/afzaalace-synergy-stable-builds-c30301e/src/lib/net/SecureSocket.cpp:27:
/usr/include/openssl/types.h:184:16: note: forward declaration of ‘SSL’ {aka ‘struct ssl_st’}
  184 | typedef struct ssl_st SSL;
      |                ^~~~~~
      
      해결책 => m_ssl->m_ssl->session->ciphers; 변경한다. SSL_get_client_ciphers(m_ssl->m_ssl) 

8. 클라이언트는 아래 명령어로 실행
	$./synergyc -n <screen name> <server ip>
	$./synergyc -n RASPBERRY-PI 192.168.0.46

9. 우분투 22.04 버전에서 마우스 커서가 보이지 않을 때 
	Ubuntu 22.04 LTS
	vim /etc/gdm3/custom.conf
	find #WaylandEnable=false delete #
	:wq
	reboot

<참조>
http://raspberrypiblog.weebly.com/blog/synergy-on-raspberry-pi
https://github.com/symless/synergy-core/wiki/Compiling-v1.3.5-to-v1.8-%28legacy%29#Debian_78
https://sourceforge.net/projects/synergy-stable-builds/files/v1.8.8-stable/



