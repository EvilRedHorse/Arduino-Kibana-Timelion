#! /bin/bash --posix

# The script is used to send sensor data from Arduino to Elasticsearch for Kibana/Timelion.

# allow read/write on dev/shm
# sudo chmod u+rw /dev/shm

# logfile timestamp
readonly TIME=$(date +%s+%N)

# logfile to use on /dev/shm
LOGFILE=/dev/shm/report$TIME.json

# TTY connection/session info
TTY=/dev/ttyACM0
BAUD=2400
TTY_SESSION_SEC=60

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

    if [[ $counter -lt 1 ]]; then
        #echo "direction setup"
        exec 4<$TTY 5>$TTY
        #echo "stty setup"
        stty -F $TTY $BAUD -echo
        #eleep briefly after setup.    
        sleep 3    
        #echo "sending command to Arduino"
        echo "{\"pass\": \"$ARDUINO_PASS\"}" > $TTY >&5 &
    fi

    #echo "reading"
    read -t $TTY_SESSION_SEC -e REPLY <&4   

}

while [[ -z "${REPLY}" ]] && [[ $counter -lt 3 ]]; do
    tty
    let "counter++"
done

echo "$REPLY" > $LOGFILE
# show logfile
cat $LOGFILE

# send json timeseries data to elasticsearch
sudo -u $USER curl -s --cacert $CERT_DIR -u elastic:$ELASTIC_PASS  -XPOST https://$ELASTIC_HOST:$ELASTIC_PORT/my-data-stream/_doc -H "Accept: application/json" -H "Content-Type:application/json" --data $(cat $LOGFILE)

# remove the logfile
rm $LOGFILE
exit 0
