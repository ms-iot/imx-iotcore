# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# pip install pySerial
import serial
import sys
import codecs
import os
import struct

ser = serial.Serial('COM4', 115200)
ekcert = "c:\\temp\\cert.cer"
ekpublog = "c:\\temp\\mfgek.txt"
mac0 = 0xDEADBEEF
mac1 = 0x0000BAD0

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
    print("FAILURE: DEVICE NEEDS ATTENTION")

while 1:
    try:
        line = ser.readline().decode('UTF-8')[0:-2]
    except:
        # Silence exceptions from non-printable WinDBG output.
        #print("exception during decode")
        continue
    if line == "MFG:reqmac":
        # Write MAC0 and MAC1 (Placeholder)
        ser.write((mac0).to_bytes(4, byteorder='little'))
        ser.write((mac1).to_bytes(4, byteorder='little'))
        # Send a simple checksum truncated to 32-bytes
        ser.write(((mac0 + mac1)&0xFFFFFFFF).to_bytes(4, byteorder='little'))
    if line == "MFG:hostcheck":
        ser.write((0x4D464748).to_bytes(4, byteorder='little')) # MFGH in Ascii
    if line == "MFG:ekcertificate":
        # Send a signed Endorsement Key Certificate (Placeholder)
        sendfile(ekcert)
    if line == "MFG:smbiossystemserial":
        # Write Serial Number String (Placeholder)
        sendstring(b'RealSerialNumber123456789')
    if line == "MFG:ekpublic":
        data = ser.read(4)
        length = struct.unpack_from("i", data)[0]

        ekbytes = ser.read(length)

        data = ser.read(4)
        devicesum = struct.unpack_from("i", data)[0]

        hostsum = 0
        for i in ekbytes:
            hostsum = hostsum + i
        if hostsum != devicesum:
            fatalerror("Invalid Endorsement Key Public received!")
            continue
        # Render EK Public into Base64 
        ekbase64 = codecs.encode(ekbytes, 'base64').decode()
        print("fTPM Endorsement Key Public:")
        print(ekbase64)
        with open(ekpublog, "a") as ekfile:
            ekfile.write(ekbase64)
        continue
    if line == "MFG:success":
        print("Device provisioning successful. Power off device now!")
        continue
    if line == "MFGF:mac":
        fatalerror("Device failed to receive MAC address!")
    if line == "MFGF:remotehost":
        fatalerror("Device failed to communicate with host!")
    if line == "MFGF:ekpublic":
        fatalerror("Device failed to retrieve EK certificate!")
    if line == "MFGF:ekcertificate":
        fatalerror("Device failed to store device certificate!")
    if line == "MFGF:smbios":
        fatalerror("Device failed to store smbios values!")
    if line == "MFGF:provisionedS":
        fatalerror("Device failed to store provisioning status!")
    print(line)
