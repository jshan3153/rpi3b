import requests
import time

url = "http://protocol.cleancitynetworks.com/cfx/photo?iccId=89314404000847095834"

payload={}
files=[
  ('filename',('test.jpg',open('/home/pi/Pictures/test.jpg','rb'),'image/jpeg'))
]
headers = {}

#for i in range (0,3):
response = requests.request("POST", url, headers=headers, data=payload, files=files)
 # print(i)
  #time.sleep(5)

print(response.text)
