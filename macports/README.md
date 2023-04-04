PrimerPooler binaries are now [on MacPorts](https://ports.macports.org/port/pooler/)
for i386, x86-64 and ARM64 architectures.

To install: `sudo port install pooler`

To remove: `sudo port uninstall pooler`

Unfortunately Macports server has only the single-core version at present.
Multi-core support is coming (see second pull request below).

To compile locally via MacPorts:

1. `sudo` and edit `/opt/local/etc/macports/sources.conf` adding a `file:///` entry to point to `PrimerPooler/macports`
2. In `macports` directory, do `portindex` and check it does not say `failed` (it does not return an error to shell when that happens)
3. Do `port lint --nitpick pooler`
4. Do `sudo port install pooler`
5. Optionally restore `/opt/local/etc/macports/sources.conf` removing or commenting out the `file:///` entry

Here is the [MacPorts pull request](https://github.com/macports/macports-ports/pull/18108) for reference, plus the [pull request to enable OpenMP](https://github.com/macports/macports-ports/pull/18147).
