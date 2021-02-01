from Arduino import Arduino

"""
    This script creates and destroys an Arduino object, thus resetting the COM port.
"""

arduino = Arduino(port="COM6", wait=False)
del arduino