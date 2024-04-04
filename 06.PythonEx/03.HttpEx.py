#this module defines classes that implement the client side of the HTTP and HTTPS protocols
import http.client

conn = http.client.HTTPSConnection("protocol.cleancitynetworks.com")
payload = "8931440400065254618626990010010002000005000120000#1403400000+372913165+126536019902508031192+0001+0001+0001+1310085599900002410#"
headers = {
  'Content-Type': 'text/plain'
}
conn.request("POST", "/ttk", payload, headers)
res = conn.getresponse()
data = res.read()
print(data.decode("utf-8"))


'''
#Requests is a HTTP library for the Python
import requests

url = "http://protocol.cleancitynetworks.com/ttk"

payload = "8931440400065254618626990010010002000005000120000#1403400000+372913165+126536019902508031192+0001+0001+0001+1310085599900002410#"
headers = {
  'Content-Type': 'text/plain'
}

response = requests.request("POST", url, headers=headers, data=payload)

print(response.text)
'''