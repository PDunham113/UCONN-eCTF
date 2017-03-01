import struct
def grabKeys():
    with open("secret_build_output.txt",'r') as keyFile:
        y = keyFile.readline()
        while len(y) > 5:
            y = y.split(" ")
            z = y[2]
            z=z[1:-2]
            keyValues = {}
            key = []
            for i in range(len(z)//4):
                    key.append(struct.pack(">B",int(z[4*i+2:4*i+4],16)))
            key = b''.join(key)
            keyValues[y[1]] = key
            y = keyFile.readline()
        return keyValues
    
