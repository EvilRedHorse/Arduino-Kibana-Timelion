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

#### **DNF/RPM Requirements:**

```
sudo dnf -y install screen socat curl crond java-11-openjdk snapd
sudo ln -s /var/lib/snapd/snap /snap
```

#### **Install Arduino IDE & CLI:**
```
sudo usermod -a -G dialout $USER
sudo snap install arduino
sudo snap install arduino-cli
```

#### **Install Elasticsearch & Kibana:**
https://www.elastic.co/guide/en/elasticsearch/reference/current/rpm.html
