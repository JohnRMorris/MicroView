import serial
import time
import sys

def io (cmd):
    global ser
    ser.write (cmd)
    return ser.readline().strip()

def connect ():
    global ser
    ser = serial.Serial ("/dev/ttyUSB0", 19200, timeout = 1)
    connected = False
    for i in range (100):
        if io("e") == '0':
            connected = True
            break;
        time.sleep (0.1)
    if not connected:
        print "Not connected"
        ser.close
        sys.exit (-1)
    print "Connected"
    return ser

def run_mode ():
    io("r")

def prog_mode ():
    if io("p") != '0':
        print "Failed entering prog mode"
        sys.exit (1)

def io4 (a, b = 0, c = 0, d = 0):
    global ser
    ser.write ("I%02X%02X%02X%02X" % (a, b, c, d))
    return int (ser.readline().strip(), 16)

def prog_read (baddr):
    waddr = baddr >> 1
    hi = io4 (0x28, (waddr >> 8) & 0xFF, waddr & 0xFF, 0);
    lo = io4 (0x20, (waddr >> 8) & 0xFF, waddr & 0xFF, 0);
    return hi * 256 + lo

def read_code ():
    code = []
    expected_addr = 0
    with open ("MicroView_combined_8-19-14.hex") as f:
        for ll in f:
            l = ll.strip()
            if l[0] != ':':
                print "Bad line", l
                sys.exit (1)
            bytes = [int (l[i:i+2],16) for i in xrange (1, len(l), 2)]            
            reclen = bytes[0]
            addr = bytes[1] * 256 + bytes[2]
            rectype = bytes[3]
            data = bytes[4:-1]
            csum = bytes[-1]
            calc_csum = (((sum(bytes[:-1]) & 0xFF) ^ 0xFF) + 1) & 0xFF
            # print bytes, reclen, addr, rectype, data, csum, calc_csum, l.strip()
            if csum != calc_csum:
                print "Checksum error!"
                sys.exit (-1)
            if len(data) != reclen:
                print "Length error!"
                sys.exit (-1)
            if rectype == 0:
                if addr != expected_addr:
                    print "Address glitch!"
                    sys.exit (-1)
                code.extend (data)
                expected_addr += len(data)
            elif rectype == 1:
                print "Returning %d code bytes" % len(code)
                return code
            else:
                print "Unknown record type!"
                sys.exit (-1)

def prog_page (code, page_no):
    print "Program page %d..." % page_no,
    sys.stdout.flush()
    bytes = code[page_no *128:(page_no + 1) * 128]
    for i in xrange (0, 128, 2):
        io4 (0x40, 0, i >> 1, bytes[i])
        io4 (0x48, 0, i >> 1, bytes[i + 1])
    addr = page_no * 64
    io4 (0x4C, addr >> 8, addr & 0xFF)
    while io4 (0xF0) & 1:
        pass
    errors = 0
    for i in xrange (0, 128, 2):
        addr = page_no * 64 + i // 2

        rb = io4 (0x20, (addr >> 8) & 0xFF, addr & 0xFF, 0)
        if rb != bytes[i]:
            print "Low verify error %04X - %02X / %02X" % (addr, rb, bytes[i])
            errors += 1

        rb = io4 (0x28, (addr >> 8) & 0xFF, addr & 0xFF, 0)
        if rb != bytes[i+1]:
            print "High verify error %04X - %02X / %02X" % (addr, rb, bytes[i+1])
            errors += 1
    if errors != 0:
        sys.exit (-1)
    print "Page %d verified ok" % page_no
                        
code = read_code ()
connect ()
prog_mode ()
print "Lock bits: 0x%02X" % io4 (0x58)
print "Signature Byte: 0x%02X 0x%02X 0x%02X" % (io4 (0x30, 0, 0), io4 (0x30, 0, 1), io4 (0x30, 0, 2))
print "Fuse bits: 0x%02X" % io4 (0x50)
print "Fuse bits high: 0x%02X" % io4 (0x58, 8)
print "Extended fuse bits: 0x%02X" % io4 (0x50, 8)
print "Calibration byte: 0x%02X" % io4 (0x38)
for addr in xrange (0, 4, 2):
    print "0x%04x: 0x%04X" % (addr, prog_read (addr))
print "Erase!"
io4 (0xac, 0x80)
while io4 (0xF0) & 1:
    pass
for addr in xrange (0, 4, 2):
    print "0x%04x: 0x%04X" % (addr, prog_read (addr))
for page in xrange (256):
    prog_page (code, page)

run_mode ()

