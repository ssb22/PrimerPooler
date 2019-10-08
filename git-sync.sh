#!/bin/bash
git pull --no-edit
wget -N http://ssb22.user.srcf.net/pooler/pooler.tgz
wget -N http://ssb22.user.srcf.net/pooler/script2canvas.py
wget -N http://ssb22.user.srcf.net/pooler/example.txt
tar -zxvf pooler.tgz
git add pooler/*
git commit -am update && git push
