import sys
import os
import math
print sys.argv
nservers=int(sys.argv[1]);
if (nservers<0):
    nservers=-nservers;
    doid=1+int(sys.argv[2]);
else:
    doid=0
xservers=nservers
yservers=1
if math.sqrt(nservers)==int(math.sqrt(nservers))*1.0:
    xservers=yservers=int(math.sqrt(nservers));
serveripfile="serverip-"+str(xservers)+'-'+str(yservers)+'.txt'
fp=open(serveripfile,'w')
port=6666
servercount=0;
for i in range(0,xservers):
    for j in range(0,yservers):
        if (id==0):
            fp.write("localhost:"+str(port)+'\n')
        else:
            if (servercount<5):
                fp.write("meru0"+str(servercount)+":"+str(port)+'\n')
            else:
                fp.write("ipatch:"+str(port)+'\n')
        port+=1
        servercount+=1
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
        if (doid!=0):
            shouldwait=os.P_WAIT
        if (doid!=0 and doid!=sid):
            continue;
        #print ["./cbr","--id="+str(sid),"--layout=<"+str(xservers)+','+str(yservers)+",1>","--serverips="+serveripfile,"--wait-until="+datestr]+sys.argv[2:]
        #in the future you want to change this to ssh to the appropriate machine and run the command below as an argument to ssh
        args=["./cbr","--id="+str(sid),"--layout=<"+str(xservers)+','+str(yservers)+",1>","--serverips="+serveripfile];
        if (doid==0):
            args.append("--wait-until="+datestr);
        if (doid!=0):
            args=args+sys.argv[3:]#+[sys.argv[len(sys.argv)-3]+sys.argv[len(sys.argv)-2]]
        else:
            args=args+sys.argv[2:]
        
        print os.spawnl(shouldwait,"./cbr",*args);
