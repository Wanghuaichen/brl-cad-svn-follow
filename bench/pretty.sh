#!/bin/sh
# WARNING THIS IS A REAL CPU HOG
../rt/rt -p90 -f1024 -H3 -M $*\
 -o cube.pix\
 ../db/cube.g\
 'all.g' \
 2> cube.log\
 <<EOF
6.847580140e+03
3.699276190e+03 3.032924070e+03 3.658674860e+03
-5.735762590e-01 8.191521640e-01 0.000000000e+00 0.000000000e+00 
-3.461885920e-01 -2.424037690e-01 9.063078880e-01 0.000000000e+00 
7.424040310e-01 5.198366640e-01 4.226181980e-01 0.000000000e+00 
0.000000000e+00 0.000000000e+00 0.000000000e+00 1.000000000e+00 
EOF
