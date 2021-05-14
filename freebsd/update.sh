#!/bin/bash

echo "ERROR: update.sh needs changing"
echo "FreeBSD ports have migrated from svn to git"
exit 1

# ssh freebsd svn switch https://svn.FreeBSD.org/ports/head /usr/ports
# ssh freebsd pkg install portlint
# ssh freebsd "echo 'DEVELOPER=yes' >> /etc/make.conf"
scp Makefile distinfo pkg-descr freebsd:/usr/ports/biology/pooler/ || exit 1
ssh freebsd 'cd /usr/ports/biology/pooler/ && svn up && rm -f distinfo && make makesum && portlint -A' || exit 1
scp freebsd:/usr/ports/biology/pooler/distinfo . || exit 1
ssh freebsd 'cd /usr/ports/biology/pooler/ && svn diff' > pooler.diff
echo "pooler.diff to https://bugs.freebsd.org/bugzilla/enter_bug.cgi"
