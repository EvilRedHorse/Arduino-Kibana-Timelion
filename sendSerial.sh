#! /bin/bash --posix

# The script is used to send sensor data from Arduino to Elasticsearch for Kibana/Timelion.

# allow read/write on dev/shm
# sudo chmod u+rw /dev/shm

# logfile timestamp
readonly TIME=$(date +%s%N)
# logfile to use on /dev/shm
LOGFILE=/dev/shm/report$TIME.json

# TTY connection/session info
TTY=/dev/ttyACM0
BAUD=2400
TTY_SESSION_SEC=65

# Arduino password
ARDUINO_PASS=""
# Elasticsearch password
ELASTIC_PASS=""
# Elasticsearch Host, Port & Cert directory.
ELASTIC_HOST=127.0.0.1
ELASTIC_PORT=9200
CERT_DIR=~/.certs/http_ca.crt

counter=0

tty () {
    exec 4<$TTY 5>$TTY
    stty -F $TTY $BAUD -echo
    sleep 1
    if [[ $counter -lt 1 ]]; then
        echo "{\"pass\": \"$ARDUINO_PASS\"}" > $TTY >&5 &
    fi
}

while [[ -z "${REPLY}" ]] && [[ $counter -lt 2 ]]; do
    tty
    read -t $TTY_SESSION_SEC -e REPLY <&4
    let "counter++"
    done

echo "$REPLY" > $LOGFILE
# show logfile
cat $LOGFILE

# send json timeseries data to elasticsearch
sudo -u $USER curl --cacert $CERT_DIR -u elastic:$ELASTIC_PASS  -XPOST https://$ELASTIC_HOST:$ELASTIC_PORT/my-data-stream/_doc -H "Accept: application/json" -H "Content-Type:application/json" --data $(cat $LOGFILE)

# remove the logfile
rm $LOGFILE
exit 0
