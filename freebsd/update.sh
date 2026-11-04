#!/bin/bash
set -ex

# Generate diff to submit to FreeBSD.
# Run this on a machine that's set up to "ssh freebsd"
# or on a FreeBSD machine itself
# or use ../.github/workflows/freebsd-update.yml

Take_Upstream_Makefile=0 # set to 1 if FreeBSD maintainers add a patch we want to keep.  Probably should leave at 0 and address any periodic build failures by changing paths in ../pooler/Makefile as needed (e.g. /usr/man to /usr/share/man)

# Make sure Git is configured correctly, that the box has a
# ports tree and linters etc, and set OldV to FreeBSD's
# *current* (pre-update) version.
Setup_FreeBSD_Box () {
    git config --global user.name "Silas S. Brown"
    git config --global user.email ssb22$(echo @)cam.ac.uk
    git config --global pull.rebase false
    pkg info portlint || pkg install -y portlint
    grep DEVELOPER=yes /etc/make.conf 2>/dev/null || echo 'DEVELOPER=yes' >> /etc/make.conf
    if [ -f /usr/ports/Mk/bsd.port.mk ] ; then git -C /usr/ports pull; else mkdir -p /usr/ports; git clone --depth 1 https://github.com/freebsd/freebsd-ports /usr/ports/.; fi # if ports does not already exist, assume we're on a GitHub runner and use the mirror to save upstream bandwidth: we're not going to push from here.  If running on a local box, assume ports will already have been set to the most sensible mirror and isn't one of the quarterly branches if we need to catch latest upstream changes to Makefile (quarterly OK for a new port)
}
Check_Current_Version () {
    grep -m1 '^DISTVERSION=' /usr/ports/biology/pooler/Makefile | cut -wf2 # (-w is a FreeBSD extension, so we include this in the pipe that runs on the box itself)
}

Check_And_Commit () {
    OldDir="$(pwd)"
    cd /usr/ports/biology/pooler/
    rm -rf work distinfo
    make makesum
    rm -rf work
    portlint -A
    make deinstall || true
    make install
    rm -rf work
    git add .
    git commit -am "biology/pooler "$(grep -m 1 '^#.*Program_Version' ../pooler/version.h|cut -d ' ' -f5|tr -d '"')
    cd "$OldDir"
}

if [ "$1" = "--set-up-box" ]; then
    Setup_FreeBSD_Box ; Check_Current_Version ; exit
elif [ "$1" = "--check-and-commit" ]; then
    Check_And_Commit ; exit
elif [ "$(uname -s)" = "FreeBSD" ]; then
    Setup_FreeBSD_Box
    OldV=$(Check_Current_Version)
    # The following can be skipped if the FreeBSD box is different from the one that has PrimerPooler cloned:
    cd ..
    git config --global --add safe.directory "$(pwd)"
    for N in $(find . -type d); do git config --global --add safe.directory "$(pwd)/$N"; done
    cd freebsd
else
    # Assume we can ssh to the FreeBSD box as root
    scp "$0" freebsd:/tmp/update.sh
    OldV=$(ssh freebsd bash /tmp/update.sh --set-up-box | tail -1)
fi

# Work out the new version number, put it in NewV.
# This can be done whether or not we're on the FreeBSD box.
# Then update Makefile version.
Tags=$(
    (git describe --tags ||
         git fetch --unshallow >/dev/null &&
         git describe --tags
    ) | sed -e s/v//)
NewV=$(echo "$Tags"|sed -e 's/-.*//')

echo "updating Makefile to actual current version"
echo "PORTNAME=		pooler" > m
echo "DISTVERSIONPREFIX=	v" >> m
echo "DISTVERSION=		$NewV" >> m
if echo "$Tags"|grep '\-' >/dev/null; then echo "DISTVERSIONSUFFIX=	$(echo "$Tags"|sed 's/.*-/-/')" >> m; echo "GH_TAGNAME=		$(echo "$Tags"|sed -e 's/.*-g*//')" >> m; fi # else we're at a version point without extra commits
if [ "$Take_Upstream_Makefile" = 1 ]; then
  if [ "$(uname -s)" = "FreeBSD" ]; then
    cp /usr/ports/biology/pooler/Makefile .
  else
    scp freebsd:/usr/ports/biology/pooler/Makefile .
  fi
fi
grep -v ^DIST < Makefile | grep -v ^PORTNAME | grep -v ^GH_TAGNAME >> m
mv m Makefile
if [ "$(uname -s)" = "FreeBSD" ]; then
    [ "$(pkg version -t "$NewV" "$OldV")" != "<" ] || { set +x;echo;echo "ERROR: FreeBSD will interpret DISTVERSION $NewV as being before old version $OldV, try git tag v${NewV}0 and git push --tags?"; exit 1; }
    cp Makefile pkg-descr /usr/ports/biology/pooler/
    # (we don't worry about unconditionally overwriting pkg-descr: will notice any diffs in that before submitting.  but important to sync Makefile otherwise automatic check on will-an-update-work can fail and alert even before we need to actually submit one)
    Check_And_Commit
    git -C /usr/ports format-patch --stdout -1 > pooler.mbox
else
    [ "$(ssh freebsd pkg version -t "$NewV" "$OldV")" != "<" ] || { set +x;echo;echo "ERROR: FreeBSD will interpret DISTVERSION $NewV as being before old version $OldV, try git tag v${NewV}0 and git push --tags?"; exit 1; }
    scp Makefile pkg-descr freebsd:/usr/ports/biology/pooler/
    ssh freebsd bash /tmp/update.sh --check-and-commit
    ssh freebsd git -C /usr/ports format-patch --stdout -1 > pooler.mbox
    ssh freebsd rm /tmp/update.sh
fi
echo "pooler.mbox to https://bugs.freebsd.org/bugzilla/enter_bug.cgi (as attachment with Content Type set to Patch: use Choose File not copy-paste)"
echo "If the diff is wrong and we need to re-run update.sh after a change, first do: ssh freebsd 'cd /usr/ports;git reset --hard HEAD~1'"
