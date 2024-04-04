import socket
import os

def internetCheck():
    '''
    if not connect internet
    return 127.0.0.1
    '''
    #ipAddress = socket.gethostbyname(socket.gethostname())
    hostname = 'www.naver.com'
    
    try:
        ipAddress = socket.gethostbyname(hostname)
        print("IP ADDRESS {} is : {}".format(hostname, ipAddress))
        print("IP ADDRESS " + hostname + " is : " + ipAddress)
    except socket.gaierror:
        print("ignoreing failed address lookup")
        ipAddress = '127.0.0.1'
        
    print(ipAddress)

    return ipAddress

def ping_check():
    response = os.system("ping -c 1 " + "www.google.com")

    if response == 0:
        print("active")
    else:
        print("not")
    

ipCheck = internetCheck()

ping_check()