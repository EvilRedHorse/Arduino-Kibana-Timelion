# Arduino-Kibana-Timelion
Secured Arduino Sensor Data to Elasticsearch, Kibana &amp; Timelion.

### The purposes for this compilation:
1. To gather sensor data from a group of Arduino sensors.

2. To shape our data; so that in can be consumed as timeline data by Elasticsearch, Kibana & Timelion.

3. To add a layer of security to the microcontroller; with a password to secure reports and prevent over-irrigation.

4. To send data to Elastisearch automatically and at intervals; using available tools such as screen, socat, curl & crond.

5. To view our sensor data in a Kibana Timelion time-series based visualization.

6.  i)   Arduino - 1024-bit password
   
  ii)  Linux OS - PAM

  iii) Elasticsearch & Kibana - username:base64password

#### DNF/RPM Requirements:

```
sudo dnf -y install screen socat curl crond java-11-openjdk snapd
sudo ln -s /var/lib/snapd/snap /snap
```

#### Install Arduino IDE & CLI:

```
sudo usermod -a -G dialout $USER
sudo snap install arduino
sudo snap install arduino-cli
```

#### Install Elasticsearch & Kibana:

Elasticsearch Information: [Elasticsearch Reference](https://www.elastic.co/guide/en/elasticsearch/reference/current/rpm.html "Elasticsearch Reference").


#### Certs
Copy your existing Elasticsearch certs.

```
sudo cp /etc/elasticsearch/certs/http_ca.crt  ~/.certs/http_ca.crt
```

#### Mem
To avoid disk writes, allow user access to `/dev/shm` 

```
# allow read/write on dev/shm
sudo chmod u+rw /dev/shm
```

#### Get repo & add Crontab
Add bash script to crontab send data from Arduino to Elasticsearch at regular intervals.

```
cd Public
git clone https://github.com/EvilRedHorse/Arduino-Kibana-Timelion
crontab -e
```

Add this line to your crontab; a report will be run every 25 minutes.

```
*/25 * * * * sudo -u $USER /usr/bin/bash -c /home/$USER/Public/sendSerial.sh

```

#### Timelion
Our soil moisture sensor data ranges from 0-1023 and it is reversed. With a time lion expression we can bypass the Arduino map() function while normalising the data.

These are not full timelion expressions; this is to show us inverting the graph data, scaling the data down by 8x and normalising by subtracting our available range.

```
.subtract(term=1024)
.multiply(multiplier=-0.125)
```
Timelion Information:  [Kibana Timelion Functions](https://github.com/coralogix/kibana-timelion-functions/blob/master/README.md "Kibana Timelion Functions").
