PrimerPooler binaries are now [on MacPorts](https://ports.macports.org/port/pooler/)
for i386, x86-64 and ARM64 architectures.

To install: `sudo port install pooler`

To remove: `sudo port uninstall pooler`

To compile locally via MacPorts:

1. `sudo` and edit `/opt/local/etc/macports/sources.conf` adding a `file:///` entry to point to `PrimerPooler/macports`
2. In `macports` directory, do `portindex` and check it does not say `failed` (it does not return an error to shell when that happens)
3. Do `port lint --nitpick pooler`
4. Do `sudo port install pooler`
5. Optionally restore `/opt/local/etc/macports/sources.conf` removing or commenting out the `file:///` entry

Here is the [MacPorts pull request](https://github.com/macports/macports-ports/pull/18108) for reference, plus the [pull request to enable OpenMP](https://github.com/macports/macports-ports/pull/18147),
and the [pull request to update from 1.86 to 1.88](https://github.com/macports/macports-ports/pull/20329)
and [1.88 to 1.89](https://github.com/macports/macports-ports/pull/27554).
