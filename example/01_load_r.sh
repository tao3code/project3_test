# load encoders on right side
# check is robot is stand

set -e

# rise right foot
./pj3cmd motion set,id=4,meg=10
./pj3cmd motion set,id=6,meg=-20
sleep 1

./pj3cmd motion set,id=4,meg=10
./pj3cmd motion set,id=6,meg=-20
sleep 1

# load 8,10 right foot outer/inner
./pj3cmd motion set,id=8,meg=-20
./pj3cmd motion set,id=10,meg=-20
sleep 1
./pj3cmd motion set,id=8,meg=-30
./pj3cmd motion set,id=10,meg=-30
sleep 1

# twist
./pj3cmd motion set,id=0,meg=-30
./pj3cmd motion set,id=2,meg=30
sleep 1
./pj3cmd motion set,id=0,meg=30
./pj3cmd motion set,id=2,meg=-30
sleep 1

# put down right foot
./pj3cmd motion set,id=4,meg=-30
sleep 1
./pj3cmd motion set,id=8,meg=5
./pj3cmd motion set,id=10,meg=5
sleep 1
./pj3cmd motion set,id=6,meg=10
./pj3cmd motion set,id=8,meg=20
sleep 1
./pj3cmd motion set,id=4,meg=-10
./pj3cmd motion set,id=6,meg=30
./pj3cmd motion set,id=10,meg=5
