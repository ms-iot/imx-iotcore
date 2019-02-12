#pip install pySerial
import serial
import sys
import codecs
import os
import struct

ser = serial.Serial('COM4', 115200)
crosscert = "c:\\temp\\cert.cer"
ekcertlog = "c:\\temp\\mfgek.txt"

def sendfile( filepath ):
    with open(filepath, "rb") as f:
        buf = f.read()
        print("file size: " + str(len(buf)))
        ser.write(struct.pack('I', len(buf)))
        ser.write(buf)
        sum = 0
        for i in range(len(buf)):
            sum = sum + buf[i]
        #print("checksum: " + str(sum))
        ser.write(struct.pack('I', sum))

def sendstring( string ):
        print("string length: " + str(len(string)))
        ser.write(struct.pack('I', len(string)))
        ser.write(string)
        sum = 0
        for i in range(len(string)):
            sum = sum + string[i]
        #print("checksum: " + str(sum))
        ser.write(struct.pack('I', sum))

def fatalerror( errormsg ):
    print(errormsg)
    print("REBOOTING SYSTEM")

while 1:
    try:
        line = ser.readline().decode('UTF-8')[0:-2]
    except:
        print("exception during decode")
        continue
    if line == "MFG:reqmac":
        # Write MAC0 (Placeholder)
        ser.write(b'0xDEADBEEF\n')
        # Write MAC1 (Placeholder)
        ser.write(b'0x0000BEEF\n')
    if line == "MFG:hostcheck":
        ser.write(bytes([0x48, 0x47, 0x46, 0x4D])) #HGFM
    if line == "MFG:devicecert":
        # Send a cross-signed certificate (Placeholder)
        sendfile(crosscert)
    if line == "MFG:smbiossystemserial":
        # Write Serial Number String (Placeholder)
        sendstring(b'RealSerialNumber123456789\n')
    if line == "MFG:ekcert":
        data = ser.read(4)
        length = data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24)

        ekbytes = ser.read(length)

        data = ser.read(4)
        devicesum = data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24)

        hostsum = 0
        for i in ekbytes:
            hostsum = hostsum + i
        if hostsum != devicesum:
            fatalerror("Invalid EK certificate recieved!")
            continue
        # ekcert should be passed through limpet to confirm an actual length
        # the properties of the ftpm may change the length.
        ekbase64 = codecs.encode(ekbytes[10:326], 'base64').decode()
        print("fTPM Endorsement Key Certificate:")
        print(ekbase64)
        with open(ekcertlog, "a") as ekfile:
            ekfile.write(ekbase64)
        continue
    if line == "MFG:success":
        print("Device provisioning successful. Power off device now!")
        continue
    if line == "MFGF:remotehost":
        fatalerror("Device failed to communicate with host!")
    if line == "MFGF:ekcert":
        fatalerror("Device failed to retrieve EK certificate!")
    if line == "MFGF:devicecert":
        fatalerror("Device failed to store device certificate!")
    if line == "MFGF:smbios":
        fatalerror("Device failed to store smbios values!")
    if line == "MFGF:provisionedS":
        fatalerror("Device failed to store provisioning status!")
    print(line)