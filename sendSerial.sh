#! /bin/bash --posix

# The script is used to send sensor data from Arduino to Elasticsearch for Kibana/Timelion.

# allow read/write on dev/shm
# sudo chmod u+rw /dev/shm

# logfile timestamp
TIME=$(date +%s%N)
# logfile to use on /dev/shm
LOGFILE=/dev/shm/report$TIME.json

# TTY connection/session info 
TTY=/dev/ttyACM0
BAUD=2400
TTY_SESSION_SEC=60
LOGFILE=/dev/shm/report.json
# Arduino password
ARDUINO_PASS=
# Elasticsearch password
ELASTIC_PASS=
# Elasticsearch Host, Port & Cert directory.
ELASTIC_HOST=127.0.0.1
ELASTIC_PORT=9200
CERT_DIR=~/.certs/http_ca.crt

tty () {
exec 4<$TTY 5>$TTY
stty -F $TTY $BAUD -echo
echo "{\"pass\": \"$ARDUINO_PASS\"}" > $TTY >&5 &
read -t $TTY_SESSION_SEC -e REPLY <&4
echo "$REPLY" > $LOGFILE
}

while [[ -z "${REPLY}" ]]; do
    tty
    sleep 3
done

# show logfile
cat $LOGFILE

# send json timeseries data to elasticsearch
sudo -u $USER curl --cacert $CERT_DIR -u elastic:$ELASTIC_PASS  -XPOST https://$ELASTIC_HOST:$ELASTIC_PORT/my-data-stream/_doc -H "Accept: application/json" -H "Content-Type:application/json" --data $(cat $LOGFILE)

# remove the logfile
rm $LOGFILE
exit 0
