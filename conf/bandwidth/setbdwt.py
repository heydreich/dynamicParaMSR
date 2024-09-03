# python usage
#   python setbdwt.py
#       1. cluster [lab|aliyun]
#       2. bdwt for Mbps [1000]

import sys
import subprocess
import os

ip=sys.argv[1]
nic=sys.argv[2]
upBDWTMbps=sys.argv[3]
downBDWTMbps=sys.argv[4]

up = int(upBDWTMbps) * 1024
down = int(downBDWTMbps) * 1024

# home dir
cmd = r'echo ~'
home_dir_str, stderr = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE).communicate()
home_dir = home_dir_str.decode().strip()

# proj dir
proj_dir="{}/dynamicParaMSR".format(home_dir)
script_dir = "{}/scripts".format(proj_dir)
config_dir = "{}/conf".format(proj_dir)

WONDERSHAPER="{}/wondershaper".format(home_dir)

# ssh to host and set the bandwidth



cmd = "ssh "+ip+" \"cd "+WONDERSHAPER+"; sudo ./wondershaper -a "+nic+" -u "+str(up)+" -d "+str(down)+"\""
print(cmd)
# os.system(cmd)
