#!/bin/bash

# MacPorts version + sums update.
# Currently, this works only at an exact release point:
# can't do "version 1.89 plus 1 commit for man page typo fix".

set -e
v=$(grep -m 1 '^#.*Program_Version' ../pooler/version.h|cut -d ' ' -f5|tr -d '"')
wget "https://github.com/ssb22/PrimerPooler/archive/refs/tags/$v.tar.gz"
sed -e "s/ssb22 PrimerPooler .* v/ssb22 PrimerPooler $(echo "$v"|sed -e s/v//) v/" -e "s/sha256  *[^ ]* /sha256 $(openssl dgst -sha256 "$v.tar.gz"|sed -e 's/.*= *//') /" -e "s/rmd160  *[^ ]* /rmd160 $(openssl dgst -rmd160 "$v.tar.gz"|sed -e 's/.*= *//') /" -e "s/size.*/size $(wc -c < "$v.tar.gz")/" < science/pooler/Portfile > Portfile
rm "$v.tar.gz"
mv Portfile science/pooler/
echo
echo "Now do: git clone --depth 1 https://github.com/macports/macports-ports"
echo "and scp $(hostname -s):$(pwd)/science/pooler/Portfile macports-ports/science/pooler/"
echo "then change .git/config to a fork and make a pull request"
