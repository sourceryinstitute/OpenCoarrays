PTPKG: ParaTools Packages
===============================================================================

README file

Created: 12 Jan 2012

Author: John C. Linford (jlinford@paratools.com)

License: BSD

Copyright:  ParaTools, Inc.

Introduction
-------------------------------------------------------------------------------

PTPKG is a hierarchical, cross-platform software distribution management system
written entirely in Bash shell script.  It is particularly good at creating
"shrink-wrapped" software distributions.  PTPKG provides enough flexibility
to work with most software, yet it is structured enough to be productive.
Its goal is to make software installation less of a yak shave and more of a
paint by numbers. PTPKG calls a few common external tools (e.g. grep, cut, sed),
but most of the work is done in Bash.  It should work on most POSIX systems.

PTPKG catalogues software to be installed a package library, which is simply a
collection of packages.  A package defines a single software product that can be
installed, tested, and uninstalled from the software distribution.  For example,
you could have a package named "Python-2.7.2" that installs the Python
interpreter version 2.7.2.

Packages can depend on other packages, exclude other packages, or provide
descriptive features.  This defines an abstract dependency tree that PTPKG
walks to determine which packages need to be installed to realize a desired
functionality.  Package installation and dependency information can be
customized for each target platform, so not only are packages portable, but they
concisely describe the steps required to install any given software on a target.

Requirements
-------------------------------------------------------------------------------

 * Bash shell version 3.0 or later.
 * Your typical Unix utilities: grep, awk, sed, cut, paste, tar, etc.
 * Compression: gzip, bzip2, unzip.

 AIX needs gtar as well as tar, and possibly other "freeware" utilities.

Anatomy of a package
------------------------------------------------------------------------------

Packages are defined by creating a specially-named directory in the "packages"
directory in `$PTPKG_HOME` (usually the directory directly above the one containing
this README file).  This specially-named directory is the top-level directory for
the package and can be referenced via `$PTPKG_PKG_DIR`.

Package names must follow certain rules:
  1. Packages names are of the format `<name>-<version>`.
  2. `<name>` must not contain spaces, but it may contain dashes and underscores.
  3. `<version>` must not contain spaces or dashes, but it may contain underscores.
  4. The use of non-alphanumeric characters is probably OK, but discouraged.

The package folder can contain any kind of file you like (archives, patches,
licenses, readme, etc.) and it must contain at least one target configuration
folder named `<os>-<distribution>-<compiler>-<architecture>`.  There is always one 
target configuration folder named `any-any-any-any` that contains the default actions
for the package when a more specific action cannot be found.

Target configuration directories in a package may contain these files:
  * `source`       : Describes package sources.
  * `dependencies` : Describes package dependencies.
  * `environment`  : Describes the package run-time and compile-time environment.
  * `install`      : A Bash shell script that installs the package.
  * `test`         : A Bash shell script that tests the package.
  * `uninstall`    : A Bash shell script that uninstalls the package.

There is always one target configuration folder named `any-any-any-any` that 
contains at least **source** and **dependencies** files.  Default definitions of
the other files (e.g. environment) can be placed here and will be used by
other target configurations when that file is not present.

Package script API
-------------------------------------------------------------------------------

PTPKG sets environment variables that can be referenced from the package
processing scripts (environment, install, test, and uninstall).

**You are encouraged to consider the following variables read-only:**

  * `PTPKG_BUILD_PREFIX`
    - Description: Full path to the directory where packages will be built
    - Example:     "/home/jlinford/software/ptoolsrte/build"


  * `PTPKG_INSTALL_PREFIX`
    - Description: Full path to the directory where packages will be installed
    - Example:     "/home/jlinford/software/ptoolsrte/packages"


  * `PTPKG_LOGFILE_PREFIX`
    - Description: Full path to the directory where logfiles will be saved
    - Example:     "/home/jlinford/software/ptoolsrte/logs"


  * `PTPKG_ETCFILE_PREFIX`
    - Description: Full path to the directory where configuration files will be saved
    - Example:     "/home/jlinford/software/ptoolsrte/etc"


  * `PTPKG_TARGET`
    - Description: Target configuration
    - Example:     linux-gnu-x86_64


  * `PTPKG_OS`
    - Description: Target operating system
    - Example:     "linux"


  * `PTPKG_COMPILER`
    - Description: Target compiler family
    - Example:     "gnu"


  * `PTPKG_ARCH`
    - Description: Target architecture
    - Example:     "x86_64"


  * `PTPKG_TARGET_DIR`
    - Description: Full directory path to the target configuration
    - Example:     "/home/jlinford/ptoolsrte/targets/linux-gnu-x86_64"


  * `PTPKG_TARGET_LIBRARY_PATH`
    - Description: The operating-specific environment variable specifying the dynamic library search path
    - Example:     "LD_LIBRARY_PATH"  or  "DYLD_LIBRARY_PATH"


  * `PTPKG_PKG`
    - Description: Full package name
    - Example:     "openmpi-1.4.4"


  * `PTPKG_PKG_NAME`
    - Description: Package name
    - Example:     "openmpi"


  * `PTPKG_PKG_VERSION`
    - Description: Package version
    - Example:     "1.4.4"


  * `PTPKG_PKG_DIR`
    - Description: Full directory path to the package
    - Example:     "/home/jlinford/ptoolsrte/packages/openmpi-1.4.4"


  * `PTPKG_PKG_BUILD_DIR`
    - Description: Full directory path where the package is built
    - Example:     "/home/jlinford/software/ptoolsrte/build/openmpi-1.4.4"


  * `PTPKG_PKG_INSTALL_DIR`
    - Description: Full directory path where the package will be installed
    - Example:     "/home/jlinford/software/ptoolsrte/build/openmpi-1.4.4"

**The following variables can (and *should*) be set in the package manifest file:**

  * `PTPKG_PKG_ARCHIVE`
    - Description: Path to package archive file, relative to PTPKG_PKG_DIR
    - Example:     "openmpi-1.4.4.tar.gz"
    - Details:     The package archive file should be some kind of compressed archive file (e.g. zip, tgz, or tar.bz2).  It will be expanded to create `PTPKG_PKG_BUILD_DIR`, so all files in the archive should be contained in a top-level directory.  Most non-evil people organize their archives this way by default.
                  
  * `PTPKG_PKG_PROVIDES`
    - Description: List of features the package provides.
    - Example:     "MPI"
    - Details:     `PTPKG_PKG` and `PTPKG_PKG_NAME` are automatically added to `PTPKG_PKG_PROVIDES`

  * `PTPKG_PKG_REQUIRES`
    - Description: List of features the package requires
    - Example:     "gfortran"
    - Details:     At least one package that provides each feature in `PTPKG_PKG_REQUIRES` must be installed before this package can be installed.
              
  * `PTPKG_PKG_EXCLUDES`
    - Description: List of features the package excludes
    - Example:     "mpich2"
    - Details:     Any package that provide the features listed in `PTPKG_PKG_EXCLUDES` cannot be installed if this package is installed.

PTPKG provides commands to simplify writing package processing scripts:

.... someday I'll actually finish this README file .... 


