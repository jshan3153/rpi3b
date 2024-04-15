import threading
import time

count = 0

def start_time(count):
    
    count+=1
    print(count)
    
    if count==5:
        print('stop')
        timer.cancel()
    
    if timer.is_alive():
        print ('b')
        
if __name__ == '__main__':
    
    
    timer = threading.Timer(1, start_time, args=[count]).start()
    
    
    
    if timer.isAlive():
        print ('a')
    
    while 1:
        time.sleep(1) 