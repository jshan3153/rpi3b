#_*_ coding: utf-8 _x_
#_*_ codnign:euc-kr _x_

from flask import Flask, request, jsonify
import os, ntpath
import json
import subprocess

lat1 = 0.0
lon1 = 0.0

lat2 = 0.0
lon2 = 0.0

app = Flask(__name__)

@app.route('/pos1', methods = ['POST'])
def pos1():
    data = request.json
    global lat1
    global lon1
    lat1 = float(data['lat'])
    lon1 = float(data['lon'])

    print ("lat %f, lon %f" % (lat1, lon1))
    
    return 'SET lat&lon1'

@app.route('/pos2', methods = ['POST'])
def pos2():
    data = request.json
    global lat2
    global lon2
    lat2 = float(data['lat'])
    lon2 = float(data['lon'])

    print ("lat %f, lon %f" % (lat2, lon2))
    
    return 'SET lat&lon2'

def set_gps(pos):
    global gps_pos
    gps_pos = pos
    print(gps_pos, pos)
    return 'set_gps'
    
def get_gps():
    print('get_pos %f %f %f %f' %(lat1, lon1, lat2, lon2))
    #print("1:",round(lat1, 1))
    #print("1:",format(lat1,".1f"))
    return lat1, lon1, lat2, lon2
    
@app.route('/getpos', methods = ['GET', 'POST'])
def readSensors1():
    if request.method == 'GET':
        #return '@%f,%f,%f,%f,' %(get_gps())
        #return '@37.484148,126.899590,37.484188,126.899590,'
        return '@%0.8f,%.8f,%.8f,%.8f,' %(lat1, lon1, lat2, lon2)

@app.route('/obd', methods = ['POST'])
def obd(speed, rpm, acc):
    return 'obd %d %d %d' %(speed, rpm, acc)
    
if __name__=='__main__':
    app.run(host='0.0.0.0', debug=True)
