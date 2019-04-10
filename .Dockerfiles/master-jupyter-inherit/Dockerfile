FROM jupyter/minimal-notebook:4cdbc9cdb7d1

USER root
ENV GCC_VERSION 8.2.0

RUN DEBIAN_FRONTEND=noninteractive transientBuildDeps="file" \
    && echo "NB_USER=${NB_USER}" \
    && set -v \
    && cat /etc/issue \
    && uname -a \
    && echo "${DEBIAN_FRONTEND}" \
    && apt-get update -y \
    && apt-get install -y --no-install-recommends \
    software-properties-common \
    && add-apt-repository ppa:ubuntu-toolchain-r/test -y \
    && apt-get update -y \
    && apt-get install -y --no-install-recommends \
    build-essential \
    gcc-8>=8.2.0 \
    gfortran-8>=8.2.0 \
    g++-8>=8.2.0 \
    mpich>=3.2 \
    libmpich-dev>=3.2 \
    ${transientBuildDeps} \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 60 --slave /usr/bin/gfortran gfortran /usr/bin/gfortran-8 \
    && update-alternatives --set gcc "/usr/bin/gcc-8" \
    && gcc --version \
    && gfortran --version \
    && mpiexec --version \
    && mpif90 --version \
    && mpicc --version \
    && apt-get clean \
    && apt-get purge -y --auto-remove ${transientBuildDeps} \
    && rm -rf /var/lib/apt/lists/* /var/log/* /tmp/*

# Build-time metadata as defined at http://label-schema.org
    ARG BUILD_DATE
    ARG VCS_REF
    ARG VCS_URL
    ARG VCS_VERSION=latest
    LABEL org.label-schema.schema-version="1.0" \
          org.label-schema.build-date="${BUILD_DATE}" \
          org.label-schema.name="" \
          org.label-schema.description="Coarray Fortran compiler with GFortran ${GCC_VERSION}, OpenCoarrays and MPICH backend" \
          org.label-schema.url="https://github.com/sourceryinstitute/OpenCoarrays#readme/" \
          org.label-schema.vcs-ref="${VCS_REF}" \
          org.label-schema.vcs-url="${VCS_URL}" \
          org.label-schema.version="${VCS_VERSION}" \
          org.label-schema.vendor="SourceryInstitute" \
          org.label-schema.license="BSD" \
          org.label-schema.docker.cmd="docker run -i -t -p 8888:8888 sourceryinstitute/opencoarrays_jupyter"

ARG RUN_TESTS=false

RUN DEBIAN_FRONTEND=noninteractive transientBuildDeps="cmake cmake-data git" \
    && set -v \
    && echo "${DEBIAN_FRONTEND}" \
    && apt-get update && apt-get install -y --no-install-recommends \
    ${transientBuildDeps} \
    && cmake --version \
    && gcc --version \
    && gfortran --version \
    && git clone --single-branch https://github.com/sourceryinstitute/OpenCoarrays \
    && mkdir OpenCoarrays/build \
    && cd OpenCoarrays/build \
    && FC="$(command -v gfortran)" CC="$(command -v gcc)" cmake -DCAF_ENABLE_FAILED_IMAGES=FALSE -DCMAKE_BUILD_TYPE=Release .. \
    && make -j "$(nproc)" install \
    && if "${RUN_TESTS}" ; then ctest --output-on-failure ; fi \
    && caf --version \
    && cafrun --version \
    && cd ../.. \
    && rm -rf ../../OpenCoarrays \
    && apt-get clean \
    && apt-get purge -y --auto-remove ${transientBuildDeps} \
    && rm -rf /var/lib/apt/lists/* /var/log/* /tmp/*

USER "${NB_USER}"
