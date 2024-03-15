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

3.After the device connects to the terminal through Bluetooth, the terminal provides a service interface that can modify the Bluetooth name and mac address of the terminal. However, some problems may occur during modification.
