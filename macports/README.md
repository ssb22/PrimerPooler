
This has been submitted to MacPorts via [pull request](https://github.com/macports/macports-ports/pull/18108).

To try on MacPorts locally:

1. `sudo` and edit `/opt/local/etc/macports/sources.conf` adding a `file:///` entry to point to `PrimerPooler/macports`
2. In `macports` directory, do `portindex && port lint pooler`
