import cv2
import time #fps 계산을 위해

print(cv2.__version__)


#이미지에 텍스트를 출력하는 함수
def draw_text(img, text, x, y):
    font = cv2.FONT_HERSHEY_SIMPLEX
    font_scale = 1
    font_thickness = 2
    text_color=(255, 0, 0)
    text_color_bg=(0, 0, 0)

    text_size, _ = cv2.getTextSize(text, font, font_scale, font_thickness)
    text_w, text_h = text_size
    offset = 5

    cv2.rectangle(img, (x - offset, y - offset), (x + text_w + offset, y + text_h + offset), text_color_bg, -1)
    cv2.putText(img, text, (x, y + text_h + font_scale - 1), font, font_scale, text_color, font_thickness)

#0 = 내부캠
cap = cv2.VideoCapture(0)

#1 이상은 외부캠
#cap = cv2.VideoCapture(1)

print('width : %d, hegith : %d ' % (cap.get(3), cap.get(4)))

# 웹캠에서 fps 값 획득
fps = cap.get(cv2.CAP_PROP_FPS)

print('fps', fps)

if fps == 0.0:
    fps = 30.0

time_per_frame_video = 1/fps

last_time = time.perf_counter()

while(True):
    ret, frame = cap.read()    # Read 결과와 frame

    # fsp 계산
    time_per_frame = time.perf_counter() - last_time
    time_sleep_frame = max(0, time_per_frame_video - time_per_frame)
    time.sleep(time_sleep_frame)

    real_fps = 1/(time.perf_counter()-last_time)
    last_time = time.perf_counter()


    x = 30
    y = 50
    text = '%.2f fps' % real_fps
    #이미지의 (x,y)에 텍스트 출력
    draw_text(frame, text, x,y)

    if(ret) :
        gray = cv2.cvtColor(frame,  cv2.COLOR_BGR2GRAY)    # 입력 받은 화면 Gray로 변환

        cv2.imshow('frame_color', frame)    # 컬러 화면 출력
        #cv2.imshow('frame_gray', gray)    # Gray 화면 출력

        if cv2.waitKey(1) == ord('q'):
            break
    
    #print('%f', frame)

cap.release()

cv2.destroyAllWindows()