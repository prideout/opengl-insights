from platform import uname as platform_name
import os

def platform_open(str):
    """Open specified file using your window manager's preferred application"""
    name = platform_name()[0]
    if name == "Linux":
        os.system("gnome-open " + str)
    elif name == "Windows":
        os.system("start " + str)
    else:
        os.system("open " + str)
