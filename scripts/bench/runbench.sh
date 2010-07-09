cd scripts
cd ..
cd scripts
export HEREPLACE="$PWD"
git fetch origin
git checkout origin/master
git branch -D master
git checkout -b master origin/master
cd ..
make depends
cd $HEREPLACE/../build/cmake&&cmake .&&make&&cd ../../scripts
cluster/build.py reset_origin update update_dependencies
cluster/build.py deploy Release

export OBJECTPROGRESSIONLIST="125 250 500 750 1000 1250 1500 1750 2000 2500 3000 3500 4000 4500 5000 6000 7000 8000 9000 10000 11000 12000 13000 14000 15000 16000 17000 18000 19000"
for i in $OBJECTPROGRESSIONLIST; do curl -O http://graphics.stanford.edu/~danielrh/4x4traces/messagetrace.$i; done
curl -O http://graphics.stanford.edu/~danielrh/4x4traces/sl.trace.4x4
curl -O http://graphics.stanford.edu/~danielrh/4x4traces/1a_objects.pack

bench/1a_fairness.py 6014 10012


bench/4_latencybreakdown.py 100

export DAT=`date +%Y-%m-%d-%H.%M.%S`
mkdir $DAT
mv flow_fairness.log.100012 $DAT
mv flow_fairness.log.6014 $DAT
for i in $OBJECTPROGRESSIONLIST; do mv endtoend.$i $DAT/; done
bench/4_latencybreakdown.py 6060
mkdir $DAT/highrate
for i in $OBJECTPROGRESSIONLIST; do mv endtoend.$i $DAT/highrate/; done

