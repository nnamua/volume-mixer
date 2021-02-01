import serial, time
from queue import Queue

class Arduino:
    def __init__(self, port="COM5", baud_rate=115200, wait=True):
        self.port = port
        self.baud_rate = baud_rate
        self.data = serial.Serial(port, baud_rate)
        if wait:
            time.sleep(2)
        self.queue = Queue()

    def __dest__(self):
        self.data.close()

    def write(self, msg):
        if not isinstance(msg, bytes):
            if not isinstance(msg, str):
                msg = str(msg)
            msg = msg.encode("ascii")
        print(f"Sending: {msg} of type={type(msg)}")
        self.data.write(msg)

    def read(self, size=1):
        return self.data.read(size)

    def readline(self):
        return self.data.readline()

    def exchange(self, c, flag=0, read_size=1):
        self.write(c, flag)
        return self.read(read_size)



if __name__ == "__main__":
    arduino = Arduino()
    try:
        while True:
            x = input("Enter byte integer to send:  ")
            f = input("Enter flag to send:  ")
            print("Sending...")
            if f == "":
                arduino.write(int(x))
            else:
                arduino.write(int(x), flag=int(f))
    except Exception as e:
        print("Ended communication. Cause: " + str(e))
        del arduino

    
