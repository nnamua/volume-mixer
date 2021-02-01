import os, re

class InvalidFile(Exception):
    pass

class XBMImage:
    def __init__(self, filepath):
        self.filepath = filepath
        if not os.path.isfile(filepath):
            raise FileNotFoundError()
        with open(filepath, "r") as file:
            self.string = file.read()

        self.width = 64
        self.height = 64
        self.bits = []
        self.check_file()

    def check_file(self):
        lines = self.string.split("\n")
        width_line = lines[0]
        width_pattern = re.compile("#define .*_width 64")
        if width_pattern.match(width_line) == None:
            raise InvalidFile("Invalid width statement.")
        height_line = lines[1]
        height_pattern = re.compile("#define .*_height 64")
        if height_pattern.match(height_line) == None:
            raise InvalidFile("Invalid height statement.")
        array_line = lines[2]
        array_pattern = re.compile(r"static char .*_bits\[\] = \{")
        if array_pattern.match(array_line) == None:
            raise InvalidFile("Invalid array declaration statement.")
        
        lines = lines[3:]
        
        array = "".join(lines)
        array = "".join(array.split())

        if not array.endswith(",};"):
            raise InvalidFile("Invalid bits array.")
        else:
            array = array[:-3]

        self.extract_values(array)

    def extract_values(self, array):
        array = array.split(",")
        byte_pattern = re.compile(r"0x[A-F0-9]{2}")
        for byte in array:
            if byte_pattern.match(byte) == None:
                raise InvalidFile(f"Encountered invalid byte: {byte}.")
            byte = byte.replace("0x","")
            self.bits.append(bytes.fromhex(byte))

        if len(self.bits) * 8 != self.width * self.height:
            raise InvalidFile(f"Array does not match specified width / height values (itemcount={len(self.bits) * 8}.")

    def send_to_arduino(self, arduino, flag=1):
        if not hasattr(arduino, "write_bytes") or not callable(arduino.write_bytes):
            raise RuntimeError("Arduino has no write method.")
        try:
            to_send = b"".join(self.bits)
            bytes_send = 0
            while bytes_send < len(to_send):
                chunk_start = bytes_send
                chunk_end = min(bytes_send + 62, len(to_send))
                chunk = to_send[chunk_start:chunk_end]
                #print(f"Sending {len(chunk)} bytes...")
                arduino.write_bytearray(chunk, flag=flag)
                #print("Chunk send! Waiting for handshake...")
                arduino.read()
                bytes_send = chunk_end
                #print("Handshake received!")

        except Exception as e:
            print(f"Failed to send. Cause: " + str(e))



if __name__ == "__main__":
    img = XBMImage("icons/test.xbm")
    print(img.bits)
