#! /bin/bash --posix

# The script is used to send timeline series sensor data from Arduino to Elasticsearch

# logfile to use on /dev/shm
LOGFILE=/dev/shm/report.json
# this is the location to insert your Arduino password
ARDUINO_PASS=3fb77e036133e2fd69f8510d35ca86342cd2b9872594bbf4645e378f8c4a2b3e27700c75a6ac228278193f129bef2cf47ba03f08a1e93656a939491ca26ece50
# this is the location to insert your base64 Elasticsearch password
ELASTIC_PASS=
# this is the location to insert your Elasticsearch hostname
ELASTIC_HOST=

# start screen session
sudo -u $USER screen  -dmSL serialReport  -Logfile $LOGFILE socat stdio /dev/ttyACM0,b57600,raw,crnl
# clear log now, before running commands
rm $LOGFILE
# recreate logfile
touch $LOGFILE
# runReport command, as user, in background
sudo -u $USER echo "{\"getReport\": \"$ARDUINO_PASS\"}" > /dev/ttyACM0 &
#sudo -u smc ./runReport.sh &
# sleep 10 <- an interval to allow time between runReport(s)   
sleep 10
# output screenlog.0 with the json we need.
cat $LOGFILE
# send json timeseries data to elasticsearch
sudo -u $USER curl --cacert ~/.certs/http_ca.crt -u  elastic:$ELASTIC_PASS -XPOST https://$ELASTIC_HOST:9200/my-data-stream/_doc -H "Accept: application/json" -H "Content-Type:application/json" --data $(cat $LOGFILE)
# close the screen session
screen -S serialReport -X quit
# remove the logfie
rm $LOGFILE
