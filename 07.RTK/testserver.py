#_*_ coding: utf-8 _x_
#_*_ codnign:euc-kr _x_

from flask import Flask, request, jsonify
import os, ntpath
import json
import subprocess

gps_pos = 0.0

app = Flask(__name__)

# 실제로는 GPIO library를 사용해 읽은 스위치 값을 리턴해줘야 함

def readSwitch(id):
  print ("# readSwitch is called for switch %d" % id)
  return 1

# 실제로는 온도센서 값을 읽어 리턴해 줘야 함
def readTemp():
    return str(25.3)

# 실제로는 습도센서 값을 읽어 리턴해 줘야 함
def readHumid():
    return str(73)

# 실제로는 압력센서 값을 읽어 리턴해 줘야 함
def readPressure():
    return str(1025)

# 실제로는 조도센서 값을 읽어 리턴해 줘야 함
def readLight():
    return str(419)

def readSensor(s):
    return sensorFunc[s]()

sensorFunc = {'temperature': readTemp, 'humidity': readHumid, 'pressure': readPressure, 'light': readLight}

@app.route('/api/led', methods = ['POST'])
def ledSvc():
    data = request.json
    #ledId = data['id']
    #onOff = data['val']
    ledId = int(data['id'])
    onOff = int(data['val'])
    
    if onOff == 0:
      newLedState = 'OFF'
      # 여기에 해당 LED(LED번호는 변수 ledId에 들어 있음)를 끄는 코드를 집어 넣어 줌
    else:
      newLedState = 'ON'
      # 여기에 해당 LED(LED번호는 변수 ledId에 들어 있음)를 켜는 코드를 집어 넣어 줌
    print ("Turn LED %d to %s" % (ledId, newLedState))
    return newLedState

@app.route('/api/sw', methods = ['POST'])
def swStatus():
    data = request.json
    ledId = int(data['id'])
    swState = readSwitch(ledId)  # readSwitch() 함수에서 스위치(ledId) 값을 읽어 리턴해 줌
    print ("Switch %d state : %d" % (ledId,swState))
    return str(swState)

@app.route('/api/sensor/<string:sensorName>', methods = ['GET'])
def readSensor(sensorName):
    try:
        ret = sensorFunc[sensorName]()
        return ret
    except:
        return 'Wrong Sensor'

@app.route('/api/sensors', methods = ['GET', 'POST'])
def readSensors():
    if request.method == 'GET':
        result = {}
        for s in sensorFunc:
            result[s] = sensorFunc[s]()
    elif request.method == 'POST':
        result = {}
        sl = request.json['sensorList']
        print (sl)
        sl = sl.split(',')
        for s in sl:
            print (s)
            result[s] = readSensor(s)

    return json.dumps(result)
    
def set_gps(pos):
    #global gps_pos
    gps_pos = pos
    print(gps_pos, pos)
    return 'set_gps'
    
def get_gps():
    print('get_pos %f' %gps_pos)
    return gps_pos
    
@app.route('/api/pos', methods = ['POST'])
def ledSvc1():
    data = request.json
    #ledId = data['id']
    #onOff = data['val']
    ledId = int(data['id'])
    onOff = int(data['val'])
    onOff1 = float(data['pos'])
    
    set_gps(onOff1)
    
    if onOff == 0:
      newLedState = 'OFF'
      # 여기에 해당 LED(LED번호는 변수 ledId에 들어 있음)를 끄는 코드를 집어 넣어 줌
    else:
      newLedState = 'ON'
      # 여기에 해당 LED(LED번호는 변수 ledId에 들어 있음)를 켜는 코드를 집어 넣어 줌
    print ("Turn LED %d to %s %f" % (ledId, newLedState, onOff1))
    
    return newLedState

@app.route('/api/position', methods = ['GET', 'POST'])
def readSensors1():
    if request.method == 'GET':
        #return get_gps()
        return 'GET %f' %(get_gps())
        '''
        result = {}
        for s in sensorFunc:
            result[s] = sensorFunc[s]()
    elif request.method == 'POST':
        result = {}
        sl = request.json['sensorList']
        print (sl)
        sl = sl.split(',')
        for s in sl:
            print (s)
            result[s] = readSensor(s)
        
        return onOff1 #str(position) #json.dumps(result)
        '''
    
@app.route('/api/exec', methods = ['POST'])
def externalPgm():
    data = request.json
    cmds = data['cmd']
    print (cmds)
    cmdl = cmds.split(' ')
    result = subprocess.check_output(cmdl)
    return result

if __name__=='__main__':
    app.run(host='0.0.0.0', debug=True)
