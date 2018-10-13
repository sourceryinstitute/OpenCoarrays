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

for version in ${GCC}; do
    mkdir "cmake-build-gcc${GCC}"
    export BLD_DIR="cmake-build-gcc${GCC}"
    export FC=gfortran-${version}
    export CC=gcc-${version}
    if [[ ${OSTYPE} == [Dd]arwin* ]]; then
	# We should use clang on macOS because that's what homebrew and everyone else does
	export CC=gcc-8
	brew unlink openmpi || true
	brew unlink mpich || true
	for mpi in "mpich" "open-mpi"; do
	    brew unlink "${mpi}" || true
	    brew ls --versions "${mpi}" >/dev/null || brew install "${mpi}"
	    brew outdated "${mpi}" || brew upgrade "${mpi}"
	    brew unlink "${mpi}" || true
	done
	brew link open-mpi
    fi
    ${FC} --version
    ${CC} --version
    mpif90 --version && mpif90 -show
    mpicc --version && mpicc -show

    # shellcheck disable=SC2153
    for BUILD_TYPE in ${BUILD_TYPES}; do
	# shellcheck disable=SC2015
	[[ -d "${BLD_DIR}" ]] && rm -rf "${BLD_DIR:?}"/* || true
	(
	    cd "${BLD_DIR}"
	    cmake -DCMAKE_INSTALL_PREFIX:PATH="${HOME}/OpenCoarrays" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" ..
	    make -j 4
	    CTEST_FLAGS=(--output-on-failure --schedule-random --repeat-until-fail "${NREPEAT:-5}" --timeout "${TEST_TIMEOUT:-200}")
	    if [[ "${BUILD_TYPE}" =~ Deb ]]; then
		ctest "${CTEST_FLAGS[@]}" > "${BUILD_TYPE}.log" || cat "${BUILD_TYPE}.log"
	    else
		ctest "${CTEST_FLAGS[@]}"
	    fi
	    make install
	    make uninstall
	)
    done
done
echo "Done."
