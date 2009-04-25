import sys
serverid=int(sys.argv[len(sys.argv)-1])
sys.argv[len(sys.argv)-3]+=" "+sys.argv[len(sys.argv)-2]
sys.argv=[sys.argv[0],"-"+sys.argv[1]]+[str(serverid)]+sys.argv[2:(len(sys.argv)-2)]

import runsim

