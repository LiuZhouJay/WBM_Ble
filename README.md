| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- |

## How to Use Example

Before project configuration and build, be sure to set the correct chip target using:

```
idf.py set-target <chip_name>
```

### Build and Flash

`idf.py -p PORT flash monitor`

``Ctrl-]``

[Getting Started Guide](https://idf.espressif.com/)

## currently

1.This version is capable of receiving messages from the gateway and then sending them over Bluetooth in the form of notifications.

2.When the device is connected to Bluetooth, it can send data to the gateway via Bluetooth

3.Added ota upgrade code file.

4.The file contains a Hex array file corresponding to.bin, which can be upgraded ota over Bluetooth.

## 3.24
5.The terminal can communicate with the gateway and send notifications to the applet

6.The data format of the terminal and the applet is complete, and the applet can change the terminal name and mac address.
7.Two service.There are two and four characteristics.

8.NVS is completed,can be used to store the terminal name and mac address.


## problem

1.Currently only hex array files of existing firmware can be upgraded

2.Changing the terminal mac address can be a bit of a problem


## version --matters need attention

This version only adds a feature to the service and modifies README.md compared to the previous version (version --6)