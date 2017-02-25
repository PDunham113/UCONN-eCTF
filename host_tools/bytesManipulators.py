# functions for managing raw data files
def writeBytesToFile(data,targetName):
    with open(targetName,'wb') as target:
        target.write(data)
def appendBytesToFile(data,targetName):
    with open(targetName,'ab') as target:
        target.write(data)

# converts bytes to c string with everything encoded \xNN
def bytesToCString(data):
    return "".join(bytesToHexList(data))
def bytesToHexList(data):
    return ["\\x{:x}".format(b) for b in data]

