# This file is part of Primer Pooler v1.61 (c) 2016-19 Silas S. Brown.  For Wen.
# 
# This program is free software; you can redistribute and
# modify it under the terms of the General Public License
# as published by the Free Software Foundation; either
# version 3 of the License, or any later version.
#
# This program is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY.  See the GNU General
# Public License for more details.
unix: 128.h 32.h 32-64.h copyright.h
	$(CC) $(UnixFlags) *.c -o pooler -lm

mac-portable: 128.h 32.h 32-64.h copyright.h
	$(CC) -mmacosx-version-min=10.7 $(UnixFlags) *.c -o pooler -lm
# (this is for compiling for distribution on a Mac that is
# higher than 10.7, to make sure the result will still run
# on 10.7 (lower is rarely possible nowadays).  But 10.9's
# gcc lacks OpenMP (although 10.7's has it); 10.9 will
# therefore compile only the single-core version.
# TODO: check for a Homebrew-installed gcc-6 & use that?)

win-crosscompile: pooler.exe pooler64.exe
.PHONY: unix mac-portable win-crosscompile mingw publish
pooler.exe: 128.h 32.h 32-64.h copyright.h mingw
	i686-w64-mingw32-gcc $(WinFlags) *.c -l comdlg32 -o pooler.exe -lm
pooler64.exe: 128.h 32.h 32-64.h copyright.h mingw
	x86_64-w64-mingw32-gcc $(WinFlags) *.c -l comdlg32 -o pooler64.exe -lm
mingw:
	@if ! which x86_64-w64-mingw32-gcc >/dev/null; then if which apt-get >/dev/null; then echo "Installing the cross-compiler with apt-get..."; sudo apt-get install gcc-mingw-w64; elif which yum >/dev/null; then echo "Installing the cross-compiler with yum..."; sudo yum install mingw32-gcc mingw64-gcc mingw32-libgomp mingw64-libgomp mingw32-winpthreads-static mingw64-winpthreads-static; else echo "Cannot find the cross-compiler or how to install it"; exit 1; fi; fi

# The following rule is for use with older MingW compilers
# - you get 32-bit only, no threads, no link-time opts...
# but might be useful for quick tests if you have an old
# MingW on your system and don't want to update it:
win-crosscompile-old: 128.h 32.h 32-64.h copyright.h
	i386-mingw32-gcc -Wall -Os *.c -l comdlg32 -o pooler.exe -lm

UnixFlags=-Wall -O3 # some distros seem to have a buggy -flto, so don't use that even if have checked it's available
WinFlags=-Wall -Os -flto -static -fopenmp # TODO: comdlg32 etc doesn't have to be static, only OpenMP/pthread (but low-priority because comdlg32 itself is only 20k or so)

CC0=$(shell if gcc --version >/dev/null 2>/dev/null; then echo gcc; else echo cc; fi) # some Macs have both but only the gcc supports OpenMP, so prefer that
CC1=$(shell echo 'int main(){}' > _test.c; if $(CC0) -mtune=native _test.c -o /dev/null 2>/dev/null; then echo -mtune=native; fi; if $(CC0) -fopenmp _test.c -o /dev/null 2>/dev/null; then echo -fopenmp; fi; rm _test.c) # -march=native inadvisable if distributing the binary (and on some setups -mtune=native is available but -march=native isn't)
CC=$(CC0) $(CC1)

.PHONY: clean test-other
clean:
	rm -f 128.h 32.h 32-64.h copyright.h *~

128.h 32.h 32-64.h: 64.h 64-128.h
	chmod a+w 128.h 32.h 32-64.h 2>/dev/null || true
	sed -e s/64/128/g < 64.h > 128.h
	sed -e s/64/32/g < 64.h > 32.h
	sed -e s/64/32/g -e s/128/64/g < 64-128.h > 32-64.h
	chmod a-w 128.h 32.h 32-64.h

copyright.h: COPYING
	chmod a+w copyright.h 2>/dev/null || true
	( echo '/* automatically generated from COPYING */ static char copyright[]='; sed -e 's/This file is part of /"/' -e 's/$$/\\n/' -e 's/^# *//' < COPYING | tr -d $$'\n' ; echo '";') > copyright.h
	chmod a-w copyright.h

pooler.tgz: clean
	! grep ^#define debug.h # error if any debug.h option is still enabled
	rm -rf release
	mkdir -p release/pooler
	for N in *.h *.c; do if test $$(echo "$$N"|sed -e 's/..$$//') = tinymt32; then cp $$N release/pooler/; else (echo '/*';cat COPYING;echo '*/';cat $$N)>release/pooler/$$N; fi; done
	for N in Makefile; do (cat COPYING; cat $$N)>release/pooler/$$N; done
	cp COPYING table.py release/pooler
	cd release && tar -cvf pooler.tar pooler && cd ..
	gzip -9 release/pooler.tar
	mv release/pooler.tar.gz pooler.tgz
	rm -rf release

# check things still at least compile:
test-other: 128.h 32.h 32-64.h copyright.h
	$(CC) $(UnixFlags) -m32 *.c -o /dev/null # 32-64.h
	$(CC) $(UnixFlags) -D__ARM_ARCH_ISA_A64 *.c -o /dev/null # 64-only.h

# Use of UPX to reduce .exe size and save server space.
# This won't give as good a compression as .zip or .7z,
# but Windows unfortunately gives users the impression
# that they can "run a program directly from a zip" and
# they'll get confused about which directory it's in.
# Making it a standalone .exe means they can place it in
# whichever folder their data files are, and know where it
# is.  (Similarly, 7zip + Setup would make it harder for
# them to know what's going on, and self-extract in place
# might leave them not knowing where the extracted files
# are, so we'll use UPX as a compromise.)
# UPX 64-bit is experimental!  but seems to work for this.
upx:
	upx --best *.exe

# make publish M="commit message goes here"
publish: mac-portable test-other
	rm -f pooler.zip
	vagrant up # x86-64 MingW not installed locally:
	vagrant ssh -c 'cd /vagrant && advzip -4 --i 1000 -a pooler.zip pooler && rm pooler && make && make win-crosscompile' # pooler.zip is Mac version (advzip used to save an extra little bit of space over zip); GNU/Linux 'make' included in command just to make sure
	make upx pooler.tgz
	rm -f pooler.tar
	vagrant ssh -c 'cd /vagrant && gzip -d pooler.tgz && zopfli --i1000 pooler.tar && mv pooler.tar.gz pooler.tgz && rm -f pooler.tar' # save another 2k or so (but still use tgz in case someone doesn't have xz)
	vagrant halt
	scp pooler.zip pooler.exe pooler64.exe pooler.tgz ds:homepage/pooler/
	ssh ds 'CommitMsg_PrimerPooler="$(M)" homepage/update'
