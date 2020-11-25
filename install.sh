#!/bin/bash

make ytblocker
mkdir /opt/ytblocker
cp ytblocker /opt/ytblocker/
cp ytblocker.service /etc/systemd/system/
systemctl enable ytblocker.service
systemctl start ytblocker.service
rm ytblocker
rm ytblocker.c
rm ytblocker.service
rm install.sh
