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

test: pooler
	./pooler --dg=328 --print-bonds=-11 example.txt | grep "Bonding positions: 92 " >/dev/null
	@echo "Your pooler binary seems to be working."

CODE=64-128.h 64-only.h 64.h all-primers.h amplicons.c ansi.h bit-basics.h bit-choice.h bit-common.c debug.h deltaG-degen.h deltaG.h genome.c genome.h load-common.c memcheck.c memcheck.h numbers.h openmp.h pool-split.c random.h title.c triangle.h user.c version.h # listed so 'make pooler' will update it if any of this changed

pooler: 128.h 32.h 32-64.h $(CODE)
	$(PCC) $(CFLAGS) $(OpenMpFlag) *.c -o pooler -lm

win-crosscompile: pooler.exe pooler64.exe
.PHONY: win-crosscompile mingw publish
pooler.exe: 128.h 32.h 32-64.h mingw
	i686-w64-mingw32-gcc $(WinFlags) *.c -l comdlg32 -o pooler.exe -lm
	chmod -x pooler.exe
pooler64.exe: 128.h 32.h 32-64.h mingw
	x86_64-w64-mingw32-gcc $(WinFlags) *.c -l comdlg32 -o pooler64.exe -lm
	chmod -x pooler64.exe
mingw:
	@if ! which x86_64-w64-mingw32-gcc >/dev/null; then if which apt-get >/dev/null; then echo "Installing the cross-compiler with apt-get..."; sudo apt-get install gcc-mingw-w64; elif which yum >/dev/null; then echo "Installing the cross-compiler with yum..."; sudo yum install mingw32-gcc mingw64-gcc mingw32-libgomp mingw64-libgomp mingw32-winpthreads-static mingw64-winpthreads-static; else echo "Cannot find the cross-compiler or how to install it"; exit 1; fi; fi

# The following rule is for use with older MingW compilers
# - you get 32-bit only, no threads, no link-time opts...
# but might be useful for quick tests if you have an old
# MingW on your system and don't want to update it:
win-crosscompile-old: 128.h 32.h 32-64.h
	i386-mingw32-gcc -Wall -Os *.c -l comdlg32 -o pooler.exe -lm

CC ?= cc
# Some Macs have both cc and gcc but only the gcc
# supports OpenMP, so if we're on a Mac then prefer gcc
# if available and CC not overridden to something other
# than "cc".  (Do this ONLY on a Mac, not FreeBSD etc.)
# ('make' by itself can behave like 'make CC=cc', so we
# need to check for CC being equal to cc even if we didn't
# have the ?= line above)
PCC=$$(sh -c 'case "$$OSTYPE" in darwin*) [ "$(CC)" = cc ] && gcc --version 2>/dev/null >/dev/null ;; *) false ;; esac' && echo gcc || echo "$(CC)")

# Default flags for Unix.  We add -mtune=native to the
# default set if the compiler supports it (but NOT -march
# because that would render the binary non-distributable),
# but we allow a user CFLAGS to override all of this.
UnixFlags=-Wall -O3
# (some distros have a buggy -flto, so don't use that)
MTuneFlag=$$(echo 'int main(){}' > _test.c; if $(PCC) -mtune=native _test.c -o /dev/null 2>/dev/null; then echo -mtune=native; fi; rm _test.c)
CFLAGS ?= $(UnixFlags) $(MTuneFlag)

# But, we definitely want the -fopenmp flag if supported by the compiler.
# That's the only way we'll get the parallelisation to work.
# So we have -fopenmp even if CFLAGS is set without it, if CC supports it
# (i.e. we don't rely on the user to remember to include -fopenmp in CFLAGS,
# although they can still use a compiler without support)
OpenMpFlag=$$(echo 'int main(){}' > _test.c; if $(PCC) -fopenmp _test.c -o /dev/null 2>/dev/null; then echo -fopenmp; fi; rm _test.c)

# Flags for cross-compiling to Windows:
WinFlags=-Wall -Os -flto -static -fopenmp # TODO: comdlg32 etc doesn't have to be static, only OpenMP/pthread (but low-priority because comdlg32 itself is only 20k or so)

.PHONY: clean test test-other
clean:
	rm -f 128.h 32.h 32-64.h *~

# For the generated 128.h etc, we keep them read-only as
# an extra reminder that they're auto-generated (as well
# as the explanatory comment at the top)
128.h: 64.h
	chmod +w 128.h 2>/dev/null || true
	sed -e s/64/128/g < 64.h > 128.h
	chmod -w 128.h 2>/dev/null || true
32.h: 64.h
	chmod +w 32.h 2>/dev/null || true
	sed -e s/64/32/g < 64.h > 32.h
	chmod -w 32.h 2>/dev/null || true
32-64.h: 64-128.h
	chmod +w 32-64.h 2>/dev/null || true
	sed -e s/64/32/g -e s/128/64/g < 64-128.h > 32-64.h
	chmod -w 32-64.h 2>/dev/null || true

# -----------------------------------------------------
# Rules you won't need unless releasing new versions:
pooler.tgz: clean
	! grep ^#define debug.h # error if any debug.h option is still enabled
	rm -rf release
	mkdir -p release/pooler
	cp *.c *.h *.desktop Makefile example.txt release/pooler/
	cd release && tar -cvf pooler.tar pooler && cd ..
	gzip -9 release/pooler.tar
	mv release/pooler.tar.gz pooler.tgz
	rm -rf release
# check things still at least compile:
test-other: 128.h 32.h 32-64.h
	$(PCC) $(CFLAGS) $(OpenMpFlag) -m32 *.c -o /dev/null # 32-64.h
	$(PCC) $(CFLAGS) $(OpenMpFlag) -D__ARM_ARCH_ISA_A64 *.c -o /dev/null # 64-only.h
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
	vagrant halt
	gzip -d pooler.tgz && zopfli --i1000 pooler.tar && mv pooler.tar.gz pooler.tgz && rm -f pooler.tar # save another 2k or so (but still use tgz in case someone doesn't have xz)
	scp pooler.zip pooler.exe pooler64.exe pooler.tgz oci:homepage/pooler/
	ssh oci 'CommitMsg_PrimerPooler="$(M)" homepage/update'
	@rm pooler
publish-src: test-other
	make pooler.tgz
	scp pooler.tgz oci:homepage/pooler/
	ssh oci 'CommitMsg_PrimerPooler="$(M)" homepage/update'
GSED=$$(which gsed >/dev/null || which sed)
update-manual:
	(echo '## Primer Pooler';awk 'BEGIN{p=0} /^From http/{p=1} /^Compiling from source/{p=0} /^Usage$$/{p=1} /Typescript player/{p=0} //{if(p)print}' < $$(if grep PrimerPooler ../README.md 2>/dev/null >/dev/null; then echo ..; else echo ~/PrimerPooler; fi)/README.md|grep -v http://ssb22|$(GSED) -Ee "s/(‘)|(’)/'/g" -e 's/(“)|(”)/"/g' -e 's/ / /g' -e 's/—/---/g' -e 's/ or `pooler64`//' | grep -v '^    ' | grep -v '^IUPAC/IUBMB degenerate-base codes:' | grep -v 'designed to obtain an amplicon from position' | $(GSED) -e 's/,* such as:$$/./' -e 's/ Consider the following toy example:$$//' -e 's/^and you/You/' -e 's/^.this example does not represent real primers.. //' ) > pooler.1.md # (can't have the examples because ronn can't do preformatted)
	if which ronn >/dev/null; then ronn -r --organization="Silas S. Brown" pooler.1.md; else vagrant up && vagrant ssh -c 'ronn -r --organization="Silas S. Brown" /vagrant/pooler.1.md' && vagrant halt && mv pooler.1 ~/PrimerPooler/pooler/; fi
	@rm pooler.1.md

# Mac OS 10.9's gcc lacks OpenMP (although 10.7's has it);
# TODO: check for a Homebrew-installed gcc-6 & use that?
# TODO: can someone test on an ARM Mac?  Rosetta 1 (PowerPC to IA32) performance was 60%-80% of native; don't yet know about Rosetta 2
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
	$(INSTALL_DATA) pooler.desktop $(DESTDIR)/$(PREFIX)/share/applications
	$(INSTALL_MAN) pooler.1 $(DESTDIR)/$(PREFIX)/man/man1/
install-strip: install
	$(STRIP) $(DESTDIR)/$(PREFIX)/bin/pooler
PREFIX          ?= usr/local
INSTALL_PROGRAM ?= cp
INSTALL_DATA    ?= cp
INSTALL_MAN     ?= cp
STRIP           ?= strip
MKDIR           ?= mkdir -p
