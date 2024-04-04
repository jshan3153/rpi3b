import requests

url = "http://protocol.cleancitynetworks.com/cfx/photo?iccId=89314404000847095834"

payload={}
files=[
  ('filename',('test.jpg',open('/home/pi/Pictures/test.jpg','rb'),'image/jpeg'))
]
headers = {}

response = requests.request("POST", url, headers=headers, data=payload, files=files)

print(response.text)
