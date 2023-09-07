FreeBSD port
============

FreeBSD users should be able to install this via `pkg install pooler`

It was [committed to FreeBSD's ports tree](https://svnweb.freebsd.org/ports?view=revision&revision=555418)
under `/usr/ports/biology/pooler`, porting discussion was on [issue 251065](https://bugs.freebsd.org/bugzilla/show_bug.cgi?id=251065),
v1.77 update was [472c25a](https://cgit.freebsd.org/ports/commit/?id=472c25aa5d8dc3268fc5476b53100872d1900467)
discussed at [257975](https://bugs.freebsd.org/bugzilla/show_bug.cgi?id=257975),
v1.78 update was [541454f](https://cgit.FreeBSD.org/ports/commit/?id=541454f9939d3836bc1bcf642f8c748a1d04bb80)
discussed at [258120](https://bugs.freebsd.org/bugzilla/show_bug.cgi?id=258120),
v1.82 update was [8d34513](https://cgit.freebsd.org/ports/commit/?id=8d34513c3f7ac851134d8452109c28497f0442cb)
discussed at [261741](https://bugs.freebsd.org/bugzilla/show_bug.cgi?id=261741),
v1.84 update was [3b861fa](https://cgit.freebsd.org/ports/commit/?id=3b861fa65fe8386404132158eb153378007483bf)
discussed at [262498](https://bugs.freebsd.org/bugzilla/show_bug.cgi?id=262498),
1.85 update was [bcc206f](https://cgit.freebsd.org/ports/commit/?id=bcc206f2be0cca24bf8688ab6d81c351ccbc98d3)
discussed at [265163](https://bugs.freebsd.org/bugzilla/show_bug.cgi?id=265163),
1.86 update was [a75fab4](https://cgit.freebsd.org/ports/commit/?id=a75fab4072ee1d5d0c5f8240986c016f123a97fd)
discussed at [268445](https://bugs.freebsd.org/bugzilla/show_bug.cgi?id=268445)
(this [core-dumped on a quarterly build 8 months later](https://pkg-status.freebsd.org/beefy4/data/124i386-quarterly/f3d8c4bf5c40/logs/pooler-1.86.log) along with [60+ other packages](https://pkg-status.freebsd.org/beefy4/build.html?mastername=124i386-quarterly&build=f3d8c4bf5c40) possibly due to an infrastructure problem, but `pkg install` still works at least on 64-bit),
and
1.88 update is discussed at [273604](https://bugs.freebsd.org/bugzilla/show_bug.cgi?id=273604).
