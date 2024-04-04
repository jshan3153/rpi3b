1. Create a training dataset
 1.1 collecting and labeling
 1.2 LableImg 설치방법 ( 윈도우 10 기준, 참고 : https://uhhyunjoo.tistory.com/6)
    	1.2.1 https://github.com/heartexlabs/labelImg 에서 소스 다운로드
	1.2.2 Python 설치 
	1.2.3 PyQt5 설치
  	1.2.4 lxml 설치
   		1.2.4.1  c:>pip instll lxml
  	1.2.5 pyrcc5 사용을 위해 OSGeo4W 설치 
   		1.2.5.1 https://trac.osgeo.org/osgeo4w/ 접속
   		1.2.5.2 Quick Start for OSGeo4W Users 순서대로 설치
           		패키지는 모두 설치하고, 설치 후에는 OSGeo4W Shell 실행해서, labelImg 위치로 이동
			아래 명령어를 실행하면 특별한 메시지가 출력되지 않고 다음 프롬프트로 이동
			c:>pyrcc5 -o libs/resources.py resources.qrc
  
       	1.2.6 labelImg 실행
  	      OSGeo4W 쉘 또는 윈도우 쉘 모두 실행 가능


2. Train a custom model

3. Deploy the model to your app
