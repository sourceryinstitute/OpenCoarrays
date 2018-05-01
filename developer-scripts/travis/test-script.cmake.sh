#!/usr/bin/env bash

set -o errexit
set -o verbose
set -o pipefail
set -o nounset
set -o errtrace

__file=developer-scripts/travis/test-script.cmake.sh

# Error tracing
# requires `set -o errtrace`
__caf_err_report() {
    error_code=${?}
    echo "Error (code=${error_code}) in ${__file} in function ${1} on line ${2}." >&2
    false
    return ${error_code}
}
# Always provide an error backtrace
trap '__caf_err_report "${FUNCNAME:-.}" ${LINENO}' ERR

echo "Performing Travis-CI script phase for the OpenCoarrays direct cmake build..."

mkdir cmake-build

for version in ${GCC}; do
    export FC=gfortran-${version}
    export CC=gcc-${version}
    ${FC} --version
    ${CC} --version
    if [[ ${OSTYPE} == [Dd]arwin* ]]; then
	# Ideally this stuff would be in the `install:` section
	# but puting it here simplifies the Travis code a lot
	MPICH_BOT_URL_HEAD=MPICH_GCC${version}_BOT_URL_HEAD
	brew uninstall --force --ignore-dependencies mpich || true
	echo "Downloading Custom MPICH bottle ${!MPICH_BOT_URL_HEAD}${MPICH_BOT_URL_TAIL} ..."
	wget "${!MPICH_BOT_URL_HEAD}${MPICH_BOT_URL_TAIL}" > wget_mpichbottle.log 2>&1 || cat wget_mpichbottle.log
	brew install --force-bottle "${MPICH_BOT_URL_TAIL}"
	brew ls --versions mpich >/dev/null || brew install --force-bottle mpich
	rm "${MPICH_BOT_URL_TAIL}"
    fi
    mpif90 --version && mpif90 -show
    mpicc --version && mpicc -show

    # shellcheck disable=SC2153
    for BUILD_TYPE in ${BUILD_TYPES}; do
	rm -rf cmake-build/* || true
	(
	    cd cmake-build
	    cmake -DCMAKE_INSTALL_PREFIX:PATH="${HOME}/OpenCoarrays" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" ..
	    make -j 4
	    ctest --output-on-failure --schedule-random --repeat-until-fail "${NREPEAT:-5}" --timeout "${TEST_TIMEOUT:-200}"
	    make install
	    make uninstall
	)
    done
done
echo "Done."
