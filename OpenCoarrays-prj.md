---
project: OpenCoarrays
summary: OpenCoarrays is a Coarray Fortran (CAF) implementation supporting Fortran 2008 and 2015 features
project_dir: ./src
output_dir: ./doc
media_dir: ./media
md_extensions: markdown.extensions.toc(anchorlink=True)
               markdown.extensions.smarty(smart_quotes=False)
               markdown_checklist.extension
exclude_dir: armci
             gasnet
             single
extra_filetypes: c  //
                 h  //
                 sh #
preprocess: true
macro: PREFIX_NAME=_gfortran_caf_
       NDEBUG
include: /usr/local/opt/mpich/include
display: public
         protected
         private
source: true
graph: true
sort: alpha
extra_mods: iso_fortran_env:https://gcc.gnu.org/onlinedocs/gfortran/ISO_005fFORTRAN_005fENV.html
print_creation_date: true
project_github: https://github.com/sourceryinstitute/opencoarrays
favicon: media/.favicons/favicon.ico
project_download: https://github.com/sourceryinstitute/opencoarrays/releases/latest
project_website: http://www.opencoarrays.org
gitter_sidecar: sourceryinstitute/opencoarrays
license: bsd
author: Sourcery Institute
email: damian@sourceryinstitute.org
author_description: Sourcery Institute is a California public-benefit nonprofit corporation engaged in research, education, and consulting in computational science, engineering, and mathematics (CSEM).  We are a network of independent CSEM professionals who research and develop advanced software engineering methods, tools, and libraries for CSEM.  We teach related short courses and university courses.   We also lead and contribute to open-source software and open language standards used in CSEM fields.
author_pic: media/SI-Hat.jpg
github: https://github.com/sourceryinstitute
website: http://www.sourceryinstitute.org
---


[This document is a FORD project file, formatted with Pythonic Markdown                                      ]:#
[See https://github.com/cmacmackin/ford/wiki/Project-File-Options for more info on writing FORD project files]:#

--------------------

[TOC]

Brief description
-----------------

@warning
The OpenCoarrays API documentation is currently underconstruction. As
such, information may be missing orincomplete on this website. Please
check back frequently for updated content.

OpenCoarrays is an open-source software project that produces an
application binary interface (ABI) supporting coarray Fortran (CAF)
compilers, an application programming interface (API) that supports
users of non-CAF compilers, and an associated compiler wrapper and
program launcher.

OpenCoarrays supports the coarray parallel programming feature set
specified in the Fortran 2008 standard.  We also support several
features proposed for Fortran 2015 in the draft Technical
Specification [TS18508] Additional Parallel Features in Fortran.

### ToDo

 - [X] Add brief description text
 - [ ] Macros should be passed on command line to FORD by build system
 - [ ] MPI include should be passed on the command line (ideally,
       although presently not supported) or specified by configuring this
       file with CMake...
 - [ ] Deal with `include compiler_capabilities.txt`
 - [ ] Add documentation to [[libcaf.h]]
 - [ ] Add documentation to [[libcaf-gfortran-descriptor.h]]
 - [ ] Add documentation to [[mpi_caf.c]]
 - [ ] Add documentation to [[caf_auxiliary.c]]
 - [ ] Add documentation to [[opencoarrays(module)]]
 - [ ] Add documentation to [[single.c]] (or remove it...)
 - [ ] Add documentation for unit tests
 - [ ] Add documentation for integration tests

Compilers
---------

The GNU Compiler Collection (GCC) Fortran front-end (GFortran) v. 5.1 and later employ OpenCoarrays to support parallel execution.


Citations
---------

Please acknowledge the use of OpenCoarrays by citing the following publication:

> Fanfarillo, A., Burnus, T., Cardellini, V., Filippone, S., Nagle, D., & Rouson, D. (2014, October). OpenCoarrays: open-source transport layers supporting coarray Fortran compilers. In *Proceedings of the 8th International Conference on Partitioned Global Address Space Programming Models* (p. 4). ACM.﻿


[These show up as help tool-tips]:#
*[API]: Application Programming Interface--a set of routines, protocols, and tools for building software applications
*[ABI]: Application Binary Interface--the interface between two program modules, one of which is often a library or operating system, at the level of machine code
*[JSON]: JavaScript Object Notation--A human friendly syntax for storing and exchanging data
*[CAF]: Coarray Fortran--an extension of Fortran 95/2003 for parallel processing created by Robert Numrich and John Reid in the 1990s using a Partitioned Global Address Space (PGAS) programming model.
*[PGAS]: Partitioned Global Address Space--a parallel programming model assuming a global memory address space that is logically partitioned and a portion of it is local to each process or thread

[Links]:#
[TS18508]: http://isotc.iso.org/livelink/livelink?func=ll&objId=17288706&objAction=Open " draft Technical Specification TS18508 Additional Parallel Features in Fortran"
