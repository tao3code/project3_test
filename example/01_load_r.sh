# load encoders on right side
# check is robot is stand

set -e

# rise right foot

./pj3cmd motion set,id=4,meg=20
./pj3cmd motion set,id=6,meg=-30
sleep 1
./pj3cmd motion set,id=4,meg=20
./pj3cmd motion set,id=6,meg=-30
sleep 1

