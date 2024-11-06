# usage
#   python createconf.py
#       1. cluster [lab]
#       2. block_source [standalone|hdfs]
#       3. blockMiB [1]
#       4. pktKB [64]
#       5. code [Clay]
#       6. ecn [4]
#       7. eck [2]
#       8. ecw [4]
#       9. batch [3]
#       10. repairsize [1]

import os
import sys
import subprocess

def usage():
    print('''
    # usage
    #   python createconf.py
    #       1. cluster [lab]
    #       2. block_source [standalone|hdfs]
    #       3. blockMiB [1]
    #       4. pktKB [64]
    #       5. code [Clay]
    #       6. ecn [4]
    #       7. eck [2]
    #       8. ecw [4]
    #       9. batch [3]
    #       10. repairsize [1]
    ''')


if len(sys.argv) < 2:
    usage()
    exit()

CLUSTER=sys.argv[1]

# home dir
cmd = r'echo ~'
home_dir_str, stderr = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE).communicate()
home_dir = home_dir_str.decode().strip()

# proj dir
proj_dir="{}/dynamicParaMSR".format(home_dir)
script_dir = "{}/scripts".format(proj_dir)
config_dir = "{}/conf".format(proj_dir)

gen_conf_dir = "{}/conf".format(script_dir)
cluster_dir = "{}/cluster/{}".format(script_dir, CLUSTER)

config_filename = "sysSetting.xml"

tradeoffPoint_dir = "{}/offline".format(proj_dir)

# not varing stripe num

clusternodes=[]
controller=""
datanodes=[]
repairnodes=[]

# get controller
count=0
f=open(cluster_dir+"/controller","r")
for line in f:
    controller=line[:-1]
    clusternodes.append(controller)
f.close()

# get datanodes
f=open(cluster_dir+"/agents","r")
for line in f:
    agent=line[:-1]
    clusternodes.append(agent)
    datanodes.append(agent)
f.close()

# get clients
f=open(cluster_dir+"/newnodes","r")
for line in f:
    node=line[:-1]
    clusternodes.append(node)
    repairnodes.append(node)
    count = count + 1
f.close()

print(controller)
print(datanodes)
print(repairnodes)
# print(clusternodes)

# threads
controller_threads = 4
agent_threads = 4
cmddist_threads = 4
if CLUSTER == "aliyunhdd" or CLUSTER == "lab":
    controller_threads = 20
    agent_threads = 20
    cmddist_threads = 10

for node in clusternodes:

    cmd="ssh "+node+" \" git clone https://github.com/heydreich/dynamicParaMSR.git \""
    print(cmd)
    os.system(cmd)

    cmd="ssh "+node+" \" cd /home/pararc/tools; git clone https://github.com/magnific0/wondershaper.git; cd wondershaper; sudo make install \""
    print(cmd)
    os.system(cmd)
