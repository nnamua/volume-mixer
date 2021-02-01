from Arduino import Arduino
import win32api, win32process
from traceback import print_exc
from pycaw.pycaw import AudioUtilities, ISimpleAudioVolume

knownProcesses = { "C:\Program Files\WindowsApps\Microsoft.ZuneMusic_10.20122.11121.0_x64__8wekyb3d8bbwe\Music.UI.exe" : "Groove Music",
                    "C:\\Program Files (x86)\\Steam\\steam.exe" : "Steam",
                    'C:\\Program Files\\Mozilla Firefox\\firefox.exe' : "Mozilla Firefox"
                    }

"""
    Returns the name of the program for a given executable.
    @param executable Full path of the .exe file
"""
def getProcessName(executable):
    if executable in knownProcesses:
        return knownProcesses[executable]
    try:
        language, codepage = win32api.GetFileVersionInfo(executable, "\\VarFileInfo\\Translation")[0]
        stringFileInfo = u"\\StringFileInfo\\%04X%04X\\%s" % (language, codepage, "FileDescription")
        description = win32api.GetFileVersionInfo(executable, stringFileInfo)
        if description == None:
            raise Exception("ProcessName not found.")
    except:
        description = executable
    return description

"""
    Trims the given name to a maximum of n characters.
    @param name Original name
    @param n Maximum number of chars
    @param suffix If specified, this suffix will be appended (and taken into calculation)
    @param dots Defaults to 2, controls the number of dots added to the string if the string is too short
    @throws Exception An Exception is thrown if there are not enough chars to display at least 1 character of 
                      the name, 'dots' amount of dots aswell as the suffix.
"""
def trimProcessName(name, n, suffix="", dots=2):
    name = name.strip()
    
    if n - (len(suffix) + dots) < 1:
        raise Exception("Suffix or number of dots to large.")
    
    totalLength = len(name) + len(suffix)
    if totalLength > n:
        substringEnd = len(name) - (totalLength - n) - dots
        name = name[0:substringEnd] + "." * dots

    result = ""
    result += name + suffix
    return result

"""
    Returns a list containing all current processes aswell as their current volume value
"""
def getProcessData():
    data = []
    sessions = AudioUtilities.GetAllSessions()
    for session in sessions:
        volume = session._ctl.QueryInterface(ISimpleAudioVolume)
        if session.Process == None:
            name = "Systemsounds"
        else:
            pid = session.Process.pid
            handle = win32api.OpenProcess(0x1F0FF, False, pid)
            executablePath = win32process.GetModuleFileNameEx(handle, 0)
            name = getProcessName(executablePath)
            name = trimProcessName(name, 16)
        masterVolume = volume.GetMasterVolume()
        data.append([name, round(masterVolume * 100)])
    return data

"""
    Creates a sendable string of the process data
"""
def prepareProcessData(processData):
    string = ""
    for name, value in processData:
        if "%" in name or "&" in name:
            raise Exception(f"Process name '{name}' cannot contain '%' or '&'. ")
        string += name + "%" + str(value) + "&"
    return string + "$"

"""
    Sets the volume for the given process
"""
def setProcessVolume(process, value):
    sessions = AudioUtilities.GetAllSessions()
    for session in sessions:
        volume = session._ctl.QueryInterface(ISimpleAudioVolume)
        if session.Process == None:
            name = "Systemsounds"
        else:
            pid = session.Process.pid
            handle = win32api.OpenProcess(0x1F0FF, False, pid)
            executablePath = win32process.GetModuleFileNameEx(handle, 0)
            name = getProcessName(executablePath)
            name = trimProcessName(name, 16)
        if name == process:
            masterVolume = volume.SetMasterVolume(value / 100, None)

"""
    Mutes / Unmutes the given process
"""
def setProcessMute(process, mute):
    sessions = AudioUtilities.GetAllSessions()
    for session in sessions:
        volume = session._ctl.QueryInterface(ISimpleAudioVolume)
        if session.Process == None:
            name = "Systemsounds"
        else:
            pid = session.Process.pid
            handle = win32api.OpenProcess(0x1F0FF, False, pid)
            executablePath = win32process.GetModuleFileNameEx(handle, 0)
            name = getProcessName(executablePath)
            name = trimProcessName(name, 16)
        if name == process:
            volume.SetMute(mute, None)

def main(arduino):
    arduino.write("H")

    while True:
        request = arduino.readline()
        request = request.decode("ascii").replace("\r\n", "")
        print(f"Received request: {request}")
        
        # 'GET_PV' triggers a response containing all current process names and their corresponding values.
        # The response is formatted as follows: PROCESS_NAME1%PROCESS_VALUE1&PROCESS_NAME2%PROCESS_VALUE2&$
        if request == "GET_PD":
            processData = getProcessData()
            processString = prepareProcessData(processData)
            arduino.write(processString)

        # 'SET_PV&PROCESS_NAME%PROCESS_VALUE' sets the volume value for a given process.
        elif request.startswith("SET_PV"):
            request = request.split("&")
            processName, processValue = request[1].split("%")
            processValue = int(processValue)
            setProcessVolume(processName, processValue)
            
        # 'MUTE_PROC&PROCESS_NAME%IS_MUTE' mutes/unmutes the given process
        elif request.startswith("MUTE_PROC"):
            request = request.split("&")
            processName, isMute = request[1].split("%")
            isMute = False if isMute == "0" else True
            setProcessMute(processName, isMute)

if __name__ == "__main__":
    arduino = None
    try:
        arduino = Arduino(port="COM6")
        main(arduino)
    except Exception as e:
        print_exc(e)
        del arduino
    