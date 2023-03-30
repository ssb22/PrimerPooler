PrimerPooler binaries are now [on MacPorts](https://ports.macports.org/port/pooler/)
for i386, x86-64 and ARM64 architectures.

To install: `sudo port install pooler`

To remove: `sudo port uninstall pooler`

Unfortunately you get only the single-core version at the moment
because MacPorts' BuildBot runs a `clang` compiler that doesn't
support `-fopenmp` (not tried with `gcc12` yet).
If multicore is important and you don't have a compiler that
supports `-fopenmp`, you might have to use the
[old Mac binary](http://ssb22.user.srcf.net/pooler/pooler.zip)
and let it run under Rosetta2 if you're on ARM.

To compile locally via MacPorts:

1. `sudo` and edit `/opt/local/etc/macports/sources.conf` adding a `file:///` entry to point to `PrimerPooler/macports`
2. In `macports` directory, do `portindex && port lint pooler`
3. `sudo port install pooler`
4. Optionally restore `/opt/local/etc/macports/sources.conf` removing the `file:///` entry

Here is the [MacPorts pull request](https://github.com/macports/macports-ports/pull/18108) for reference.
