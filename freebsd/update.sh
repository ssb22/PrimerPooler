#!/bin/bash
# ssh freebsd svn switch https://svn.FreeBSD.org/ports/head /usr/ports # TODO: how to migrate this to git? (2021q3 branch is OK for now)
# ssh freebsd pkg install portlint
# ssh freebsd "echo 'DEVELOPER=yes' >> /etc/make.conf"
scp Makefile distinfo pkg-descr freebsd:/usr/ports/biology/pooler/ || exit 1
ssh freebsd 'cd /usr/ports/biology/pooler/ && rm -f distinfo && make makesum && portlint -A' || exit 1
scp freebsd:/usr/ports/biology/pooler/distinfo . || exit 1
ssh freebsd 'cd /usr/ports && git commit biology/pooler/* -m "Update PrimerPooler to upstream" && git format-patch --stdout -1' > pooler.mbox
echo "pooler.mbox to https://bugs.freebsd.org/bugzilla/enter_bug.cgi (as plain-text attachment if possible)"
