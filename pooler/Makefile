# This file is part of Primer Pooler (c) Silas S. Brown.  For Wen.
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

pooler: 128.h 32.h 32-64.h
	$(CC) $(CFLAGS) $(OpenMpFlag) *.c -o pooler -lm

win-crosscompile: pooler.exe pooler64.exe
.PHONY: mac-portable win-crosscompile mingw publish
pooler.exe: 128.h 32.h 32-64.h mingw
	i686-w64-mingw32-gcc $(WinFlags) *.c -l comdlg32 -o pooler.exe -lm
pooler64.exe: 128.h 32.h 32-64.h mingw
	x86_64-w64-mingw32-gcc $(WinFlags) *.c -l comdlg32 -o pooler64.exe -lm
mingw:
	@if ! which x86_64-w64-mingw32-gcc >/dev/null; then if which apt-get >/dev/null; then echo "Installing the cross-compiler with apt-get..."; sudo apt-get install gcc-mingw-w64; elif which yum >/dev/null; then echo "Installing the cross-compiler with yum..."; sudo yum install mingw32-gcc mingw64-gcc mingw32-libgomp mingw64-libgomp mingw32-winpthreads-static mingw64-winpthreads-static; else echo "Cannot find the cross-compiler or how to install it"; exit 1; fi; fi

# The following rule is for use with older MingW compilers
# - you get 32-bit only, no threads, no link-time opts...
# but might be useful for quick tests if you have an old
# MingW on your system and don't want to update it:
win-crosscompile-old: 128.h 32.h 32-64.h
	i386-mingw32-gcc -Wall -Os *.c -l comdlg32 -o pooler.exe -lm

# Default compiler for Unix.  Some Macs have both cc and
# gcc but only the gcc supports OpenMP, so prefer gcc.
CC ?= $(shell if gcc --version >/dev/null 2>/dev/null; then echo gcc; else echo cc; fi)

# Default flags for Unix.  We add -mtune=native to the
# default set if the compiler supports it (but NOT -march
# because that would render the binary non-distributable),
# but we allow a user CFLAGS to override all of this.
UnixFlags=-Wall -O3
# (some distros have a buggy -flto, so don't use that)
MTuneFlag=$(shell echo 'int main(){}' > _test.c; if $(CC) -mtune=native _test.c -o /dev/null 2>/dev/null; then echo -mtune=native; fi; rm _test.c)
CFLAGS ?= $(UnixFlags) $(MTuneFlag)

# But, we definitely want the -fopenmp flag if supported by the compiler.
# That's the only way we'll get the parallelisation to work.
# So we have -fopenmp even if CFLAGS is set without it, if CC supports it
# (i.e. we don't rely on the user to remember to include -fopenmp in CFLAGS,
# although they can still use a compiler without support)
OpenMpFlag=$(shell echo 'int main(){}' > _test.c; if $(CC) -fopenmp _test.c -o /dev/null 2>/dev/null; then echo -fopenmp; fi; rm _test.c)

# Flags for cross-compiling to Windows:
WinFlags=-Wall -Os -flto -static -fopenmp # TODO: comdlg32 etc doesn't have to be static, only OpenMP/pthread (but low-priority because comdlg32 itself is only 20k or so)

.PHONY: clean test-other
clean:
	rm -f 128.h 32.h 32-64.h *~

# For the generated 128.h etc, we keep them read-only as
# an extra reminder that they're auto-generated (as well
# as the explanatory comment at the top)
128.h: 64.h
	chmod +w 128.h 2>/dev/null || true
	sed -e s/64/128/g < 64.h > 128.h
	chmod -w 128.h
32.h: 64.h
	chmod +w 32.h 2>/dev/null || true
	sed -e s/64/32/g < 64.h > 32.h
	chmod -w 32.h
32-64.h: 64-128.h
	chmod +w 32-64.h 2>/dev/null || true
	sed -e s/64/32/g -e s/128/64/g < 64-128.h > 32-64.h
	chmod -w 32-64.h

# -----------------------------------------------------
# Rules you won't need unless releasing new versions:
pooler.tgz: clean
	! grep ^#define debug.h # error if any debug.h option is still enabled
	rm -rf release
	mkdir -p release/pooler
	cp *.c *.h Makefile release/pooler/
	cd release && tar -cvf pooler.tar pooler && cd ..
	gzip -9 release/pooler.tar
	mv release/pooler.tar.gz pooler.tgz
	rm -rf release
# check things still at least compile:
test-other: 128.h 32.h 32-64.h
	$(CC) $(CFLAGS) $(OpenMpFlag) -m32 *.c -o /dev/null # 32-64.h
	$(CC) $(CFLAGS) $(OpenMpFlag) -D__ARM_ARCH_ISA_A64 *.c -o /dev/null # 64-only.h
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
# Use: make publish M="commit message goes here"
# (or if only comments etc changed & no recompile,
# make publish-src M="commit message goes here" )
publish: test-other
	rm -f pooler pooler.zip
	make pooler CFLAGS="$(UnixFlags) -mmacosx-version-min=10.7" # (lower is rarely possible nowadays)
	vagrant up # x86-64 MingW not installed locally:
	vagrant ssh -c 'cd /vagrant && advzip -4 --i 1000 -a pooler.zip pooler && rm pooler && make && make win-crosscompile' # pooler.zip is Mac version (advzip used to save an extra little bit of space over zip); GNU/Linux 'make' included in command just to make sure
	make upx pooler.tgz
	rm -f pooler.tar
	vagrant ssh -c 'cd /vagrant && gzip -d pooler.tgz && zopfli --i1000 pooler.tar && mv pooler.tar.gz pooler.tgz && rm -f pooler.tar' # save another 2k or so (but still use tgz in case someone doesn't have xz)
	vagrant halt
	scp pooler.zip pooler.exe pooler64.exe pooler.tgz ds:homepage/pooler/
	ssh ds 'CommitMsg_PrimerPooler="$(M)" homepage/update'
publish-src: test-other
	make pooler.tgz
	scp pooler.tgz ds:homepage/pooler/
	ssh ds 'CommitMsg_PrimerPooler="$(M)" homepage/update'

# Mac OS 10.9's gcc lacks OpenMP (although 10.7's has it);
# TODO: check for a Homebrew-installed gcc-6 & use that?
# TODO: what happens when ARM Macs come out?  Rosetta 1 (PowerPC to IA32) performance was 60%-80% of native; don't yet know about Rosetta 2
# Might at least be able to display a warning message?
# #include <sys/sysctl.h>
# #include <errno.h>
# int ret = 0; size_t size = sizeof(ret); if (sysctlbyname("sysctl.proc_translated", &ret, &size, NULL, 0) != -1 && ret) puts("WARNING: Rosetta is in use, this might slow things down");
# (but need to check to what extent it does JIT first)

# -----------------------------------------------------

# compatibility with bsd.port.mk:
all: pooler
install: all
	$(MKDIR) $(DESTDIR)/$(PREFIX)/bin
	$(MKDIR) $(DESTDIR)/$(PREFIX)/man/man1
	$(INSTALL_PROGRAM) pooler $(DESTDIR)/$(PREFIX)/bin/
	$(INSTALL_MAN) pooler.1 $(DESTDIR)/$(PREFIX)/man/man1/
install-strip: install
	$(STRIP) $(DESTDIR)/$(PREFIX)/bin/pooler
PREFIX          ?= usr/local
INSTALL_PROGRAM ?= cp
INSTALL_MAN     ?= cp
STRIP           ?= strip
MKDIR           ?= mkdir -p
