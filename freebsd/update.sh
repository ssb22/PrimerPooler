#!/bin/bash
# ssh freebsd svn switch https://svn.FreeBSD.org/ports/head /usr/ports # TODO: how to migrate this to git? (2021q3 branch is OK for now)
# ssh freebsd pkg install portlint
# ssh freebsd "echo 'DEVELOPER=yes' >> /etc/make.conf"
scp Makefile pkg-descr freebsd:/usr/ports/biology/pooler/ &&
ssh freebsd 'cd /usr/ports/biology/pooler/ && rm -f distinfo && make makesum && portlint -A && git commit * -m "biology/pooler: update to '"$(grep -m 1 '^#.*Program_Version' ../pooler/version.h|cut -d ' ' -f5|tr -d '"')"'"' &&
ssh freebsd 'cd /usr/ports && git format-patch --stdout -1' > pooler.mbox &&
echo "pooler.mbox to https://bugs.freebsd.org/bugzilla/enter_bug.cgi (as plain-text attachment if possible)"
