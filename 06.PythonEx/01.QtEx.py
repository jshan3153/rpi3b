"""
>Qt 설치 
$pip3 install PyQt5

"""
from PyQt5.QtWidgets import QLabel, QHBoxLayout, QVBoxLayout, QApplication, QWidget

app = QApplication([])
window = QWidget()
layout_h = QHBoxLayout()
layout_v = QVBoxLayout()
image_label = QLabel()

#메인 함수 실행 
if __name__ == "__main__":
    image_label.setFixedSize(320, 240)
    window.setWindowTitle("Demo")
    layout_h.addWidget(image_label)    
    layout_v.addLayout(layout_h,20)
    window.setLayout(layout_v)
    window.resize(320, 240)

    #work.start()
    
    window.show()
    app.exec()
    #work.quit()
    #picam2.close()