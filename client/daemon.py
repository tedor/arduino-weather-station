from serial import Serial
import config
import psycopg2

def listenPort():
    ser = Serial(port=config.serialPort, baudrate=config.serialBaudrate, timeout=1)
    print("connected to: " + ser.portstr)
    while True:
        line = ser.readline()
        if line:
            parse(line)
    ser.close()

def parse(line):
    device, listParams = line.split("|")

    if device == '1':
        parseWeather(device, listParams)

def parseWeather(device, listParams):
    temperature = pressure = humidity = batteryVoltage = 0
    for param in listParams.split(","):
        paramName, paramValue = param.split(":")
        if paramName == 't':
            temperature = paramValue
        elif paramName == 'p':
            pressure = paramValue
        elif paramName == 'h':
            humidity = paramValue
        elif paramName == 'bv':
            batteryVoltage = paramValue

    saveWeather(device, temperature, pressure, humidity, batteryVoltage)

def saveWeather(device, temperature, pressure, humidity, batteryVoltage):
    try:
        conn = psycopg2.connect(config.databaseConnect)
    except ValueError:
        print "Unable to connect to the database."
        return False

    cur = conn.cursor()
    
    try:
        cur.execute("""INSERT INTO sensor(device_id, temperature, pressure, humidity, battery_voltage, created_at)
                        VALUES (%s, %s, %s, %s, %s, NOW());""",
                        (int(device), float(temperature), int(pressure), float(humidity), float(batteryVoltage)))
    except ValueError:
        print "Can not insert data."

    conn.commit()
    cur.close()
    conn.close()


if __name__ == '__main__':
#    listenPort()
    parse('1|t:26.3,p:741,h:27.7,bv:4.49')
