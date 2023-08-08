[![Build Status][chkconfig-github-action-svg]][chkconfig-github-action]
[![Coverage Status][chkconfig-codecov-svg]][chkconfig-codecov]

[chkconfig-github]: https://github.com/nuovations/chkconfig
[chkconfig-github-action]: https://github.com/nuovations/chkconfig/actions?query=workflow%3Abuild+branch%3Amain+event%3Apush
[chkconfig-github-action-svg]: https://github.com/nuovations/chkconfig/actions/workflows/build.yml/badge.svg?branch=main&event=push
[chkconfig-codecov]: https://codecov.io/gh/Nuovations/chkconfig
[chkconfig-codecov-svg]: https://codecov.io/gh/Nuovations/chkconfig/branch/main/graph/badge.svg

chkconfig
=========

# Introduction

_chkconfig_ is an admnistrative command line interface utility that
may be used to get, check, or set the configuration state for one or
more flags used by system initialization or start-up scripts.

_chkconfig_ is inspired by and designed to work as a superset of the
functionality of the same name that originally appeared on Silicon
Graphics' (also known as, SGI) now-defunct IRIX operating system.

In the days of [_systemd_](https://github.com/systemd/systemd), the
need for a utility like _chkconfig_ might seem unclear. However, there
remain embedded system use cases, particularly when used with [BusyBox](https://busybox.net)
init, where things are sufficiently simple that _chkconfig_ fills an
administrative void on such a system, even 30+ years after its
predecessor first appeared on IRIX.

Please see the [_chkconfig_ manual reference
page](./doc/man/chkconfig.adoc) for more information.

# Getting Started with chkconfig

## Building chkconfig

If you are not using a prebuilt distribution of chkconfig,
building chkconfig should be a straightforward, two- or three-step
process. If you are building from the main branch, start with:

    % ./bootstrap

If you are building from the main branch, a release branch, or a
distribution package, continue (main branch) or start with:

    % ./configure
    % make

The first `bootstrap` step creates the `configure` script and
`Makefile.in` files from `configure.ac` and `Makefile.am` respectively
and only needs to be done once unless those input files have changed.

The second `configure` step generates `Makefile` files from
`Makefile.in` files and only needs to be done once unless those input
files have changed.

Although not strictly necessary, the additional step of sanity
checking the build results is recommended:

    % make check

### Dependencies

In addition to depending on the C Standard Library, chkconfig depends
on:

  * [nlassert](https://github.com/Nuovations/nlassert)
  * [nlunit-test](https://github.com/Nuovations/nlunit-test)

However, nlunit-test is only required when building and running the chkconfig
unit test suite.

If you want to modify or otherwise maintain the chkconfig build
system, see "Maintaining chkcofig" below for more information.

## Installing chkconfig

To install chkconfig for your use simply invoke:

    % make install

to install chkconfig in the location indicated by the --prefix
`configure` option (default "/usr/local"). If you intended an
arbitrarily relocatable chkconfig installation and passed
`--prefix=/` to `configure`, then you might use DESTDIR to, for
example install chkconfig in your user directory:

    % make DESTIDIR="${HOME}" install

Note, however, that there are several influential `configure` options
in terms of where chkconfig (and the associated library) will, by
default, look for flags and state:

  * `--prefix`
  * `--localstatedir`
  * `--sysconfdir`
  * `--with-chkconfig-defaultdir`
  * `--with-chkconfig-statedir`

The `--prefix` option, by default, influences both `--localstatedir`
and `--sysconfdir`. In turn, `--localstatedir`, by default, influences
`--with-chkconfig-statedir` and `--sysconfdir`, by default, influences
`--with-chkconfig-defaultdir`.

So, if you wanted to ensure that the default and state directories
ended up in _/etc/config_ and _/var/config_, respectively, then you
would want to invoke `configure` as follows:

  * `configure --prefix=/ --sysconfdir=/etc --localstatedir=/var`

However, if you prefer other values for those options, you could also invoke `configure` as follows for similar results:

  * `configure --with-chkconfig-defaultdir=/etc/config --with-chkconfig-statedir=/var/config`

## Maintaining chkconfig

If you want to maintain, enhance, extend, or otherwise modify
chkconfig, it is likely you will need to change its build system,
based on GNU autotools, in some circumstances.

After any change to the chkconfig build system, including any
*Makefile.am* files or the *configure.ac* file, you must run the
`bootstrap` or `bootstrap-configure` (which runs both `bootstrap` and
`configure` in one shot) script to update the build system.

### Dependencies

Due to its leverage of GNU autotools, if you want to modify or
otherwise maintain the chkconfig build system, the following
additional packages are required and are invoked by `bootstrap`:

  * autoconf
  * automake
  * libtool

#### Linux

On Debian-based Linux distributions such as Ubuntu, these dependencies
can be satisfied with the following:

    % sudo apt-get install autoconf automake libtool

#### Mac OS X

On Mac OS X, these dependencies can be installed and satisfied using
[Brew](https://brew.sh/):

    % brew install autoconf automake libtool

# Interact

There are numerous avenues for chkconfig support:

  * Bugs and feature requests — [submit to the Issue Tracker](https://github.com/Nuovations/chkconfig/issues)

# Versioning

chkconfig follows the [Semantic Versioning guidelines](http://semver.org/)
for release cycle transparency and to maintain backwards compatibility.

# License

chkconfig is released under the [Apache License, Version 2.0 license](https://opensource.org/licenses/Apache-2.0).
See the `LICENSE` file for more information.
