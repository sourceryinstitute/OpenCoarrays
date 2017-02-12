<!-- Please fill out the issue template included below, failure to do -->
<!-- so may result in immediate closure of your issue. -->

<!-- Fill out all portions of this template that apply. Please delete -->
<!-- any unnecessary sections. -->

 - [ ] I have surrounded any code, or log output in codeblock fences
       (` ``` `) if it occupies more than one line, or in single
       backticks if it is short `short code`

<!-- Long code goes in a fenced code block: -->
<!-- ``` -->
<!-- Line one of code output -->
<!-- Line two of output -->
<!-- etc. -->
<!-- ``` -->


# Question: #

<!-- delete this section if it does not apply -->
The title of the issue should start with `Question:` followed by a
succinct title. Add the text of your question here. Be specific. Search for
answers on https://google.com and https://stackoverflow.com
before asking a new question

# RFE: #

<!-- delete this section if it does not apply -->
The title of the issue should start with `RFE:` followed by a succinct
title. Add a description of your requested enhancement here. If you are
willing to help out please also read the [Helping Out] section of
[CONTRIBUTING.md]



# Defect/bug report #

<!-- delete this section if it does not apply -->
The title of the issue should start with `Defect:` followed by a
succinct title.

Please replace `[ ]` with `[X]` to indicate you have taken the requested action

 - [ ] I have searched the [issues page] and [mailing list] and did
       not find any issue matching the one I would like to report
 - [ ] I have included a succinct description of the problem,
       including the steps necessary to reproduce it
 - [ ] I have included which version of OpenCoarrays I am working
       with, or the output of `git describe` if working with a cloned
       repository rather than an [official release]
 - [ ] I have included the MPI library name and version I am using
       with OpenCoarrays
   - [ ] name: [MPICH] version:
   - [ ] name: [Open-MPI] version:
   - [ ] name: [MVAPICH] version:
   - [ ] name: [MPT] version:
   - [ ] name: [Intel MPI] version:
   - [ ] other:
   <!-- delete all but the matching MPI implementation -->
 - [ ] I have included the Fortran compiler name and version that I am
       using with OpenCoarrays
 - [ ] I have included the output of `uname -a`
 - [ ] I have included the Operating system name and version,
       including linux distro
 - [ ] I have included the machine name if I am allowed to do so and
       wish to seek help debugging on that particular system (Titan,
       Pleiades, Excalibur, Lightening, Mira, etc.)

## Installation/build problem ##

<!-- delete this section if it does not apply -->

 - [ ] I have attached a log of the attempted build/installation,
       changing the extension to `.txt` and uploaded it to this issue
 - [ ] I have indicated if previous versions of OpenCoarrays are or
       were installed on the machine in question
<!-- please select one from the top level-->
 - [ ] Build/install was attempted with [install.sh]
    - [ ] I have attached the `install-opencoarrays.log` file
          after re-running the script with the debug flags `-d` and
          `-v` turned on (after renaming the log file to have a `.txt`
          extension)
 - [ ] Build/install was attempted with system package manager like
       [homebrew] (which one?)
 - [ ] Build/install was attempted with manual/advanced
       [CMake install]
   - [ ] Name and version of the C compiler being used
     - [ ] Name and version of the Fortran compiler being used
     - [ ] Attached/included the output of the configure step with
           CMake. (Usually something like `cd build && cmake ..`)
     - [ ] Attached/included the output of `make VERBOSE=1`
     - [ ] Attached/included the output of `ctest --verbose`
 - [ ] Other build/install was attempted (please describe)

Don't forget the description of the problem. The more details you give
us, the faster, and more easily we can help you

## Compile time or run time error

<!-- delete this section if it does not apply -->

<!-- pick one of the first two -->
 - [ ] I am experiencing a compile time error (all gfortran ICEs
       should be reported to the [GFortran bug tracker]... You can post
       the ICE here too for tracking purposes, with a link to the
       GFortran problem report)
 - [ ] I am experiencing a runtime issue
 - [ ] I have included a minimally complete verifiable example (MCVE)
       that exhibits the problem and suitable for adding to regression
       tests (optional, but you receive infinite karma
       for including this)

Description with as many details as possible to reproduce the problem.


[links]:#
[GFortran bug tracker]: https://gcc.gnu.org/bugzilla/
[Intel MPI]: https://software.intel.com/en-us/intel-mpi-library
[MPT]: http://www.sgi.com/products/software/sps.html
[MVAPICH]: http://mvapich.cse.ohio-state.edu
[MPICH]: https://www.mpich.org
[Open-MPI]: https://www.open-mpi.org
[CONTRIBUTING.md]: https://github.com/sourceryinstitute/opencoarrays/blob/master/CONTRIBUTING.md
[Helping Out]: https://github.com/sourceryinstitute/opencoarrays/blob/master/CONTRIBUTING.md#helping-out
[official release]: https://github.com/sourceryinstitute/opencoarrays/releases
[CMake install]: https://github.com/sourceryinstitute/opencoarrays/blob/master/INSTALL.md#cmake-scripts
[homebrew]: http://brew.sh
[issues page]: https://github.com/sourceryinstitute/opencoarrays/issues
[mailing list]: https://groups.google.com/forum/#!forum/opencoarrays
[install.sh]: https://github.com/sourceryinstitute/opencoarrays/blob/master/install.sh
