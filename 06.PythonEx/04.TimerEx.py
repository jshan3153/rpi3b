import threading
import time

def start_time(count):
    count+=1
    print(count)
    timer=threading.Timer(1,start_time, args=[count])
    timer.start()
    
    if count==5:
        print('stop')
        timer.cancel()
        
if __name__ == '__main__':
    start_time(0)
    
    while 1:
        time.sleep(1)    