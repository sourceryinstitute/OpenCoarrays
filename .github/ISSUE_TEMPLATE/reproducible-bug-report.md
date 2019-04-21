---
name: Reproducible Bug Report
about: File a bug report so we can fix it

---

The title of the issue should start with `Defect:` followed by a
succinct title.

Please make sure to put any logs, terminal output, or code in
[fenced code blocks]. Please also read the [contributing guidelines][CONTRIBUTING.md]
before submitting a new issue.

**Please note we will close your issue without comment if you delete, do not read or do not fill out the issue checklist below and provide ALL the requested information.**

- [ ] I am reporting a bug others will be able to reproduce and not asking a question or requesting a new feature.

## System information including:
  - OpenCoarrays Version: <!-- `caf --version` or `./install.sh --version` -->
  - Fortran Compiler: <!-- vendor & version number-->
  - C compiler used for building lib: <!-- vendor & version -->
  - Installation method: <!-- `install.sh`, or package manager etc. -->
  - All flags & options passed to the installer
  - Output of `uname -a`:
  - MPI library being used: <!-- e.g., MPICH 3.2 -->
  - Machine architecture and number of physical cores:
  - Version of CMake: <!-- if preinstalled & installing yourself -->

## To help us debug your issue please explain:

### What you were trying to do (and why)


### What happened (include command output, screenshots, logs, etc.)


### What you expected to happen


### Step-by-step reproduction instructions to reproduce the error/bug



[links]:#
[fenced code blocks]: https://help.github.com/articles/creating-and-highlighting-code-blocks/
[CONTRIBUTING.md]: https://github.com/sourceryinstitute/OpenCoarrays/blob/master/CONTRIBUTING.md
