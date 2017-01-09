# load encoders on left side

set -e

# rise left foot
./pj3cmd motion set,id=5,meg=10
./pj3cmd motion set,id=7,meg=-20
sleep 1

./pj3cmd motion set,id=5,meg=10
./pj3cmd motion set,id=7,meg=-20
sleep 1

# load 9,11 right foot outer/inner
./pj3cmd motion set,id=9,meg=-20
./pj3cmd motion set,id=11,meg=-20
sleep 1
./pj3cmd motion set,id=9,meg=-30
./pj3cmd motion set,id=11,meg=-30
sleep 1

# twist
./pj3cmd motion set,id=1,meg=-30
./pj3cmd motion set,id=3,meg=30
sleep 1
./pj3cmd motion set,id=1,meg=30
./pj3cmd motion set,id=3,meg=-30
sleep 1

# put down left foot
./pj3cmd motion set,id=5,meg=-30
sleep 1
./pj3cmd motion set,id=9,meg=5
./pj3cmd motion set,id=11,meg=5
sleep 1
./pj3cmd motion set,id=7,meg=10
./pj3cmd motion set,id=9,meg=20
sleep 1
./pj3cmd motion set,id=5,meg=-10
./pj3cmd motion set,id=7,meg=30
./pj3cmd motion set,id=11,meg=5
