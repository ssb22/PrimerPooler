#!/bin/bash
# ssh freebsd pkg install portlint
# ssh freebsd "echo 'DEVELOPER=yes' >> /etc/make.conf"
ssh freebsd 'cd /usr/ports/biology/pooler/ && rm -f Makefile pkg-descr && wget https://cgit.freebsd.org/ports/plain/biology/pooler/Makefile https://cgit.freebsd.org/ports/plain/biology/pooler/pkg-descr' || exit 1
ssh freebsd 'cd /usr/ports/biology/pooler/ && git commit -am "sync pooler to FreeBSD-upstream for diff"' # no error if 'nothing to commit' (this step is needed in case pooler was changed on main branch and we have an old quarterly)
scp Makefile pkg-descr freebsd:/usr/ports/biology/pooler/ || exit 1
if ! ssh freebsd 'cd /usr/ports/biology/pooler/ && rm -f distinfo && make makesum && portlint -A'; then
    # This might mean forgot to push the new version first
    # Need to undo last commit + reinstate distinfo, ready
    # for next try
    ssh freebsd 'cd /usr/ports && git reset --hard HEAD~1'
    exit 1
fi
ssh freebsd 'cd /usr/ports/biology/pooler/ && git commit * -m "biology/pooler: update to '"$(grep -m 1 '^#.*Program_Version' ../pooler/version.h|cut -d ' ' -f5|tr -d '"')"'"' &&
ssh freebsd 'cd /usr/ports && git format-patch --stdout -1' > pooler.mbox &&
echo "pooler.mbox to https://bugs.freebsd.org/bugzilla/enter_bug.cgi (as plain-text attachment; on second page, set URL to http://ssb22.user.srcf.net/pooler/#changes )"
echo "If the diff is wrong and we need to re-run update.sh after a change, first do: ssh freebsd 'cd /usr/ports;git reset --hard HEAD~1'"
