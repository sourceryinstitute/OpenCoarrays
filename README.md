<a name="top"> </a>

[This document is formatted with GitHub-Flavored Markdown.              ]:#
[For better viewing, including hyperlinks, read it online at            ]:#
[https://github.com/sourceryinstitute/OpenCoarrays/blob/master/README.md]:#
<div align="center">

[![Sourcery Institute][sourcery-institute logo]][Sourcery Institute]

OpenCoarrays
============

[![CI Build Status][build img]](https://travis-ci.org/sourceryinstitute/OpenCoarrays)
[![Release Downloads][download img]][Releases]
[![Gitter](https://img.shields.io/gitter/room/sourceryinstitute/opencoarrays.svg?style=flat-square)](https://gitter.im/sourceryinstitute/opencoarrays)
[![GitHub license][license img]](./LICENSE)
[![GitHub release][release img]](https://github.com/sourceryinstitute/OpenCoarrays/releases/latest)
[![homebrew](https://img.shields.io/homebrew/v/opencoarrays.svg?style=flat-square)](https://formulae.brew.sh/formula/opencoarrays)
[![Download as PDF][pdf img]](https://md2pdf.herokuapp.com/sourceryinstitute/OpenCoarrays/blob/master/README.pdf)
[![Twitter URL][twitter img]][default tweet]

[Overview](#overview) | [Downloads](#downloads) |
[Compatibility](#compatibility) | [Prerequisites](#prerequisites) |
[Installation](#installation) | [Getting Started](#getting-started) |
[Contributing](#contributing) | [Status](#status)
[Support](#support) | [Acknowledgments](#acknowledgments) | [Donate](#donate)

</div>

Overview
--------

[OpenCoarrays] supports the GNU Compiler Collection ([GCC]) Fortran compiler
([`gfortran`]) by providing a parallel application binary interface (ABI) that
abstracts away the underlying communication library.  OpenCoarrays thus enables
`gfortran` to support Fortran's parallel programming features, often called
"Coarray Fortran," without making direct reference to the back-end communication
library: the Message Passing Interface (MPI).  This ensures that Fortran
programs and Fortran compilers may take advantage of other communication
libraries without costly refactoring.  Work is underway on the [Caffeine]
project to support alternative communication libraries and alternative compilers
by defining a compiler-independent parallel ABI atop the [GASNet-EX] exascale
networking middleware.

OpenCoarrays provides a compiler wrapper (`caf`), a parallel runtime library
(`libcaf_mpi`), and a program launcher (`cafrun`).  The wrapper and launcher
provide a uniform abstraction for compiling and executing Coarray Fortran
without direct reference to the underlying MPI layer.

Downloads
---------

Please see our [Releases] page.

Compatibility
-------------

The OpenCoarrays ABI was adopted by `gfortran` in release the GCC 5.1.0 release
and `gfortran` continues to work with OpenCoarrays as of this writing.  

Prerequisites
-------------

Building OpenCoarrays requires

* An MPI implementation,
* CMake, and
* The GCC C and Fortran compilers: `gcc` and `gfortran`.

If you use a package manager or the OpenCoarrays installer, any missing
prerequisites will be built for you.


Installation
------------

Please see the [INSTALL.md] file.

Or [try OpenCoarrays online] as a [Jupyter] [notebook kernel]
using [Binder] with no downloads, configuration or installation required.
The default [index.ipynb] notebook is read only, but you can
execute it, copy it to make changes, or create an entirely
new [CAF kernel][notebook kernel] notebook.

Packaged Version
----------------

If you would like to be able to install OpenCoarrays through your
favorite package manager, please ask them to add it, or contribute it
yourself. If you see your favorite package manager has an outdated
version, please ask them to update it, or contribute an update
yourself.

[![Packaging status][repology-badge]][OC-on-repology]

Getting Started
---------------

To start using OpenCoarrays, please see the [GETTING_STARTED.md] file.

Contributing
------------

Please see the [CONTRIBUTING.md] file.

Status
------

A list of open issues can be viewed on the
[issues page](https://github.com/sourceryinstitute/opencoarrays/issues).

Support
-------

Please submit bug reports and feature requests via our [Issues] page.

Acknowledgments
----------------

We gratefully acknowledge support from the following institutions:

* The U.S. Nuclear Regulatory Commission ([NRC]) for funding the work that led to support for
  - the Windows operating system, 
  - the `random_init` subroutine, and 
  - `co_broadcast` of derived-type objects with `allocatable` components.
* The National Aeronautics and Space Administration [NASA] for funding the work that led to
  support for the `co_broadcast` of derived-type objects.
* [Arm] for approving compiler engineer contributions of code.
* [National Center for Atmospheric Research] for access to the
  Yellowstone/Caldera supercomputers and for logistics support during
  the initial development of OpenCoarrays.
* [CINECA] for access to Eurora/PLX for the project HyPS- BLAS under
  the ISCRA grant program for 2014.
* [Google] for support of a related [Google Summer of Code] 2014
  project.
* The National Energy Research Scientific Computing Center ([NERSC]),
  which is supported by the Office of Science of the U.S. Department
  of Energy under Contract No. DE-AC02-05CH11231, for access to the
  Hopper and Edison supercomputers under the OpenCoarrays project
  start allocation.
* [Archaeologic Inc.] for financial support for the domain registration,
  web hosting, advanced development, and conference travel.

Donate
------

If you find this software useful, please consider donating
[your time](CONTRIBUTING.md) or
[your money](http://www.sourceryinstitute.org/store/p5/Donation.html)
to aid in development efforts.

---

<div align="center">

[![GitHub forks](https://img.shields.io/github/forks/sourceryinstitute/OpenCoarrays.svg?style=social&label=Fork)](https://github.com/sourceryinstitute/OpenCoarrays/fork)
[![GitHub stars](https://img.shields.io/github/stars/sourceryinstitute/OpenCoarrays.svg?style=social&label=Star)](https://github.com/sourceryinstitute/OpenCoarrays)
[![GitHub watchers](https://img.shields.io/github/watchers/sourceryinstitute/OpenCoarrays.svg?style=social&label=Watch)](https://github.com/sourceryinstitute/OpenCoarrays)
[![Twitter URL][twitter img]][default tweet]

</div>

[Hyperlinks]:#

[Overview]: #overview
[Downloads]: #downloads
[Compatibility]: #compatibility
[Prerequisites]: #prerequisites
[Installation]: #installation
[Contributing]: #contributing
[Acknowledgments]: #acknowledgments

[Arm]: https://www.arm.com

[OpenSHMEM]: http://www.openshmem.org/site/
[sourcery-institute logo]: http://www.sourceryinstitute.org/uploads/4/9/9/6/49967347/sourcery-logo-rgb-hi-rez-1.png
[OpenCoarrays]: http://www.opencoarrays.org
[ABI]: https://gcc.gnu.org/onlinedocs/gfortran/Coarray-Programming.html#Coarray-Programming
[MPI]: https://www.mpi-forum.org/
[GCC]: https://gcc.gnu.org
[`gfortran`]: https://gcc.gnu.org/wiki/GFortran
[MPICH]: https://www.mpich.org
[Sourcery Institute]: http://www.sourceryinstitute.org
[Google]: https://www.google.com
[CINECA]: https://www.cineca.it/en
[NERSC]: https://www.nersc.gov
[National Center for Atmospheric Research]: https://ncar.ucar.edu
[INSTALL.md]: ./INSTALL.md
[GASNet]: https://gasnet.lbl.gov
[CONTRIBUTING.md]: ./CONTRIBUTING.md
[GETTING_STARTED.md]: ./GETTING_STARTED.md
[Google Summer of Code]: https://www.google-melange.com/archive/gsoc/2014/orgs/gcc

[Issues]: https://github.com/sourceryinstitute/OpenCoarrays/issues
[Releases]: https://github.com/sourceryinstitute/OpenCoarrays/releases

[try OpenCoarrays online]: https://bit.ly/CAF-Binder
[notebook kernel]: https://github.com/sourceryinstitute/jupyter-CAF-kernel
[Binder]: https://mybinder.org
[Jupyter]: https://jupyter.org
[index.ipynb]: https://nbviewer.jupyter.org/github/sourceryinstitute/jupyter-CAF-kernel/blob/master/index.ipynb

[OC-on-repology]: https://repology.org/project/opencoarrays/versions
[repology-badge]: https://repology.org/badge/vertical-allrepos/opencoarrays.svg

[build img]: https://img.shields.io/travis/sourceryinstitute/OpenCoarrays.svg?style=flat-square "Build badge"
[CI Master Branch]: https://travis-ci.org/sourceryinstitute/OpenCoarrays?branch=master "View Travis-CI builds"
[download img]: https://img.shields.io/github/downloads/sourceryinstitute/OpenCoarrays/total.svg?style=flat-square "Download count badge"
[license img]: https://img.shields.io/badge/license-BSD--3-blue.svg?style=flat-square "BSD-3 License badge"
[release img]: https://img.shields.io/github/release/sourceryinstitute/OpenCoarrays.svg?style=flat-square "Latest release badge"
[pdf img]: https://img.shields.io/badge/PDF-README.md-6C2DC7.svg?style=flat-square "Download this readme as a PDF"
[twitter img]: https://img.shields.io/twitter/url/http/shields.io.svg?style=social
[NRC]: https://www.nrc.gov
[NASA]: https://www.nasa.gov
[Caffeine]: https://go.lbl.gov/caffeine
[Archaeologic Inc.]: https://www.archaeologic.codes
[GASNet-EX]: https://go.lbl.gov/gasnet

[default tweet]: https://twitter.com/intent/tweet?hashtags=HPC,Fortran,PGAS&related=zbeekman,gnutools,HPCwire,HPC_Guru,hpcprogrammer,SciNetHPC,DegenerateConic,jeffdotscience,travisci&text=Stop%20programming%20w%2F%20the%20%23MPI%20docs%20in%20your%20lap%2C%20try%20Coarray%20Fortran%20w%2F%20OpenCoarrays%20%26%20GFortran!&url=https%3A//github.com/sourceryinstitute/OpenCoarrays
