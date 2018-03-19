FROM jupyter/minimal-notebook:da2c5a4d00fa

MAINTAINER Izaak Beekman <izaak@izaakbeekman.com>

USER root
ENV GCC_VERSION 7.2.0

ENV NB_USER jovyan
ENV NB_UID 1000
ENV HOME /home/${NB_USER}

RUN DEBIAN_FRONTEND=noninteractive transientBuildDeps="file" \
    && set -v \
    && cat /etc/issue \
    && uname -a \
    && echo "${DEBIAN_FRONTEND}" \
    && apt-get update -y \
    && apt-get install -y \
    software-properties-common \
    && add-apt-repository ppa:ubuntu-toolchain-r/test -y \
    && apt-get update -y \
    && apt-get install -y \
    build-essential \
    gcc-7=${GCC_VERSION}-1ubuntu1~16.04 \
    gfortran-7=${GCC_VERSION}-1ubuntu1~16.04 \
    g++-7=${GCC_VERSION}-1ubuntu1~16.04 \
    mpich \
    ${transientBuildDeps} \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 60 --slave /usr/bin/gfortran gfortran /usr/bin/gfortran-7 \
    && update-alternatives --set gcc "/usr/bin/gcc-7" \
    && gcc --version \
    && gfortran --version \
    && mpiexec --version \
    && mpifort --version \
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
          org.label-schema.docker.cmd="docker run -i -t -p 8888:8888 sourceryinstitute/opencoarrays-notebook"

ARG RUN_TESTS=false

RUN DEBIAN_FRONTEND=noninteractive transientBuildDeps="cmake git" \
    && set -v \
    && echo "${DEBIAN_FRONTEND}" \
    && apt-get update && apt-get install -y \
    ${transientBuildDeps} \
    && cmake --version \
    && gcc --version \
    && gfortran --version \
    && git clone --single-branch https://github.com/sourceryinstitute/OpenCoarrays \
    && mkdir OpenCoarrays/build \
    && cd OpenCoarrays/build \
    && FC="$(which gfortran)" CC="$(which gcc)" cmake -DCMAKE_BUILD_TYPE=Release .. \
    && make -j "$(nproc)" install \
    && if [ "${RUN_TESTS}" = "true" ] ; then ctest --output-on-failure ; fi \
    && cd ../.. \
    && caf --version \
    && cafrun --version \
    && rm -rf OpenCoarrays \
    && apt-get clean \
    && apt-get purge -y --auto-remove ${transientBuildDeps} \
    && rm -rf /var/lib/apt/lists/* /var/log/* /tmp/*
