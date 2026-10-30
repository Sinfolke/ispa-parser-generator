# Generator-compatible libraries

The built-in catalog downloads `libraries-<generator-version>-latest` from GitHub
(for example `libraries-v1.2.3-latest`). `ispa-pkg` reads the version from
`ISPA --version`, or `ispa --version` on PATH. Development (`dev`) builds use `main`.
Custom catalogs can use `{generator}` in a GitHub `ref` as well.
Explicit `install --ref`, local paths and archive sources keep their selected source.

Each installed `.ispa-package.json` records `generator_version`, the requested
source, and the resolved source. The generator version participates in the install
fingerprint so an unchanged source is rebuilt after a generator upgrade.

Run `ispa-pkg doctor` after switching generators. It reinstalls packages whose
generator version differs or is missing, repairs dependencies before building,
and refreshes saved exports. Matching packages are left in place. Install scripts
still require `--allow-scripts`. Run `ispa-pkg update` to fetch newer compatible
library revisions without changing generators.

The Generator library tags workflow creates the compatibility tag on a `v*`
release. For existing releases, or to publish compatible library fixes, manually
run it with the generator release tag and the library source commit/ref. Updates
to one generator's latest tag leave other generators' tags unchanged.
