# Building On Unix

To build SassC, you must either have a local copy of the LibSass sources or
LibSass must already be installed in your system as a shared library with
development headers available (See [LibSass build instruction][1] for further
reference).

# Using the CI bootstrap script

This will need a git client available to fetch the dependencies.

1. Clone the SassC repo
    ```bash
    git clone https://github.com/sass/sassc.git
    ```

1. Run CI bootstrap script and import env-variables
    ```bash
    source sassc/script/bootstrap
    # or . sassc/script/bootstrap
    ```

1. Execute 'make' to compile all sources
   ```bash
   make -C sassc -j4
   ```

1. The binary should be created in the `bin` folder

    ```bash
    ./sassc/bin/sassc [input file] > output.css
    ```

1. Optionally install the resulting binary
   ```bash
   PREFIX="/usr" make -C sassc install
   ```

# Manualy building from git sources via github

```bash
SASS_LIBSASS_PATH=`pwd`/libsass
git clone https://github.com/sass/sassc.git
git clone https://github.com/sass/libsass.git
make -j4 -C sassc
```

# Manualy building from tar sources via github

Note: this is not really recommended as the build
will show "na" as compiled version, since this is
directly derived from the git version (until you
pass the information manually to the build call)!

```bash
SASSC_VERSION="3.4.5"
LIBSASS_VERSION="3.4.5"
curl -L https://github.com/sass/sassc/archive/${SASSC_VERSION}.tar.gz | tar -xz;
curl -L https://github.com/sass/libsass/archive/${LIBSASS_VERSION}.tar.gz | tar -xz;
SASS_LIBSASS_PATH=`pwd`/libsass-${LIBSASS_VERSION}
echo $SASSC_VERSION > sassc-${SASSC_VERSION}/VERSION
echo $LIBSASS_VERSION > libsass-${LIBSASS_VERSION}/VERSION
make -j4 -C sassc-${SASSC_VERSION}
sassc-${SASSC_VERSION}/bin/sassc --version
```


[1]: https://github.com/sass/libsass/blob/master/docs/build.md
