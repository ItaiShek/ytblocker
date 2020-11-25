## Info
ytblocker is a program that runs along with Pi-hole and add youtube ads to pi-hole's blacklist automatically.
* Disclaimer: This is still an alpha version and won't neccerily stop ads in real time, and in the meantime tested on raspberry pi zero with raspbian OS.

## Installation
Install ytblocker on the same device that Pi-hole is running on
```
$ sudo chmod +x install.sh
$ sudo ./install.sh
```

Check that the service is running
```
$ systemctl status ytblocker.service
```
