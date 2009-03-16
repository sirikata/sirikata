import sys
import os
import math
nservers=int(sys.argv[1]);
duration=float(sys.argv[2]);
flatness=float(sys.argv[3]);
if len(sys.argv)>4:
    waitadditional=sys.argv[4];
else:
    waitadditional="8"
if len(sys.argv)>5:
    shouldcapbandwidth=sys.argv[5];
else:
    shouldcapbandwidth="false"

xservers=nservers
yservers=1
if math.sqrt(nservers)==int(math.sqrt(nservers))*1.0:
    xservers=yservers=int(math.sqrt(nservers));
serveripfile="serverip-"+str(xservers)+'-'+str(yservers)+'.txt'
fp=open(serveripfile,'w')
port=6666
for i in range(0,xservers):
    for j in range(0,yservers):
        fp.write("localhost:"+str(port)+'\n')
        port+=1
fp.close()

sid=0

import time
datestr=time.strftime("%Y-%m-%d %H:%M:%S%z")
print datestr
for i in range(0,xservers):
    for j in range(0,yservers):
        sid+=1
        shouldwait=os.P_NOWAIT
        if i+1==xservers and j+1==yservers:
            shouldwait=os.P_WAIT
        #in the future you want to change this to ssh to the appropriate machine and run the command below as an argument to ssh
        os.spawnl(shouldwait,"./cbr","./cbr","--id="+str(sid),"--layout=<"+str(xservers)+','+str(yservers)+",1>","--duration="+str(duration)+'s',"--serverips="+serveripfile,"--flatness="+str(flatness),"--capexcessbandwidth="+shouldcapbandwidth,"--wait-until="+datestr+"","--wait-additional="+waitadditional);
    
