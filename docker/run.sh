#!/bin/sh

mkdir -p Build

cd Build
cmake ..
make -j

cd ../bin

echo "{
    \"mysql.master.host\" : \"${DB_HOST}\",
    \"mysql.master.port\" : ${DB_PORT},
    \"mysql.master.username\" : \"${DB_USER}\",
    \"mysql.master.password\" : \"\",
    \"mysql.master.database\" : \"osu\"
}" > config.json

./osu-performance all -t ${THREADS}
