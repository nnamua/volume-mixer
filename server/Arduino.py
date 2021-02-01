import serial, time

"""
    This class models the behavior of the arduino board.
    It wraps the serial communication, and offers more abstract method of sending/receiving messages.
    The serial communication is closed when this object is destroyed.
"""
class Arduino:
    def __init__(self, port="COM5", baud_rate=115200, wait=True):
        self.port = port
        self.baud_rate = baud_rate
        self.data = serial.Serial(port, baud_rate)
        if wait:
            time.sleep(2)

    def __dest__(self):
        self.data.close()

    """
        Sends a message to the board. If msg is not of type 'bytes', it will first be converted to a string and then to bytes.
    """
    def write(self, msg):
        if not isinstance(msg, bytes):
            if not isinstance(msg, str):
                msg = str(msg)
            msg = msg.encode("ascii")
        print(f"Sending: {msg} of type={type(msg)}")
        self.data.write(msg)

    """
        Reads a message from the board. If convert is True (default), the message will be decoded to an ASCII-String
    """
    def read(self, size=1, convert=True):
        msg = self.data.read(size)
        if convert:
            msg = msg.decode("ascii")
        return msg

    """
        Reads a line from the board. If convert is True (default), the message will be decoded to an ASCII-String, and \r\n will be removed.
    """
    def readline(self, convert=True):
        msg = self.data.readline()
        if convert:
            msg = msg.decode("ascii").replace("\r\n", "")
        return msg

    """
        This method is deprecated.
    """
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

    
