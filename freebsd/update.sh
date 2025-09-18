#!/bin/bash
set -e

# Generate diff to submit to FreeBSD.
# Run this on a machine that's set up to "ssh freebsd"
# or on a FreeBSD machine itself
# or use ../.github/workflows/freebsd-update.yml

if [ "$(uname -s)" = "FreeBSD" ]; then
    echo "Running on FreeBSD: checking Git is set up appropriately"
    git config --global user.name "Silas S. Brown"
    git config --global user.email ssb22$(echo @)cam.ac.uk
    git config --global pull.rebase false
    cd ..
    git config --global --add safe.directory $(pwd)
    for N in $(find . -type d); do git config --global --add safe.directory $(pwd)/$N; done
    cd freebsd
    echo "Done global git setup"
fi

echo "updating Makefile to actual current version"
echo "PORTNAME=		pooler" > m
echo "DISTVERSIONPREFIX=	v" >> m
export Tags=$(
    (git describe --tags ||
         echo trying unshallow clone 1>&2 &&
         git fetch --unshallow >/dev/null &&
         git describe --tags
    ) | sed -e s/v//)
echo "DISTVERSION=		$(echo "$Tags"|sed -e 's/-[^-]*$//')" >> m
if echo "$Tags"|grep '\-' >/dev/null; then echo "DISTVERSIONSUFFIX=	$(echo "$Tags"|sed -e 's/.*\(-[^-]*\)$/\1/')" >> m; fi # else we're at a version point without extra commits
grep -v ^DIST < Makefile | grep -v ^PORTNAME >> m
mv m Makefile

echo "creating pooler.mbox"
if [ "$(uname -s)" = "FreeBSD" ] ; then
    # assume we're root
    pkg info portlint || pkg install -y portlint
    grep DEVELOPER=yes /etc/make.conf 2>/dev/null || echo 'DEVELOPER=yes' >> /etc/make.conf
    if ! [ -d /usr/ports ] ; then git clone --depth 1 https://github.com/freebsd/freebsd-ports /usr/ports; fi # use the mirror to save upstream bandwidth: we're not going to push from here
    mkdir -p /usr/ports/biology/pooler/
    cp Makefile pkg-descr /usr/ports/biology/pooler/
    OldDir=$(pwd)
    cd /usr/ports/biology/pooler/
    rm -rf work distinfo
    make makesum
    portlint -A
    make deinstall || true
    make install
    rm -rf work
    git add *
    git commit * -m "biology/pooler '"$(grep -m 1 '^#.*Program_Version' ../pooler/version.h|cut -d ' ' -f5|tr -d '"')
    cd /usr/ports
    git format-patch --stdout -1 > $OldDir/pooler.mbox
else
    # assume we can ssh to the FreeBSD box as root
    ssh freebsd "pkg info portlint || pkg install -y portlint"
    ssh freebsd "grep DEVELOPER=yes /etc/make.conf 2>/dev/null || echo 'DEVELOPER=yes' >> /etc/make.conf"
    ssh freebsd "if ! [ -e .gitconfig ]; then git config --global user.name 'Silas S. Brown'; git config --global user.email ssb22$(echo @)cam.ac.uk ; git config --global pull.rebase false; fi"
ssh freebsd mkdir -p /usr/ports/biology/pooler/
scp Makefile pkg-descr freebsd:/usr/ports/biology/pooler/
ssh freebsd 'cd /usr/ports/biology/pooler/ && rm -rf work distinfo && make makesum && portlint -A && (make deinstall || true) && make install && rm -rf work && git add * && git commit * -m "biology/pooler '"$(grep -m 1 '^#.*Program_Version' ../pooler/version.h|cut -d ' ' -f5|tr -d '"')"'"'
ssh freebsd 'cd /usr/ports && git format-patch --stdout -1' > pooler.mbox
fi
echo "pooler.mbox to https://bugs.freebsd.org/bugzilla/enter_bug.cgi (as attachment with Content Type set to Patch: use Choose File not copy-paste)"
echo "If the diff is wrong and we need to re-run update.sh after a change, first do: ssh freebsd 'cd /usr/ports;git reset --hard HEAD~1'"
