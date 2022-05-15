OpenCoarrays Runtime Libraries
==============================

One original goal of the OpenCoarrays project was to provide a transparent application binary interface (ABI) supported by interchangeable parallel runtime libraries based on a range of communication libraries.  The following header files in this directory define that ABI:

* libcaf.h
* libcaf-gfortran-descriptor.h
* libcaf-version-def.h: 

where the above files are in dependency order: each file depends on the file(s) listed below it.  The [runtime-libraries](./runtime-libraries) subdirectory contains runtimes based on the following communication libraries:

* Message Passing Interface (MPI)
* OpenSHMEM
* GASNet

We currently maintain, build, and test only the MPI runtime.  The GASNet and OpenSHMEM runtime library source code serve only to provide detailed, open-source documentation of the research described by [Fanfarillo et al. (2014)] and [Rouson et al. (2017)].  For a Fortran 2018 parallel runtime library based on the more recent GASNet-EX exascale networking middleware described by [Bonachea and Hargrove (2018)], please see the [Caffeine] repository.

One exception regarding the transparent nature of the ABI is the [opencoarrays module], which provides a public function that returns the underlying MPI communicator. This capability can be useful for using coarray Fortran to drive an ensemble of simulations using pre-existing MPI as described in [Rouson, McCreight, and Fanfarillo (2017)].

[Fanfarillo et al. (2014)]: http://dx.doi.org/10.1145/2676870.2676876
[Rouson et al. (2017)]: https://doi.org/10.1145/3144779.3169104
[Bonachea and Hargrove (2018)]: https://doi.org/10.1007/978-3-030-34627-0_11
[Caffeine]: https://go.lbl.gov/caffeine
[opencoarrays module]: ./runtime-libraries/mpi/opencoarrays.F90
[Rouson, McCreight, and Fanfarillo (2017)]: https://doi.org/10.1145/3144779.3169110
