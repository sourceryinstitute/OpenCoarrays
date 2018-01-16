#!/usr/bin/env bash

set -o errexit
set -o verbose
set -o pipefail
set -o nounset
set -o errtrace

__file=developer-scripts/travis/install.linux.sh

# Error tracing
# requires `set -o errtrace`
__caf_err_report() {
  local error_code
  error_code=${?}
  echo "Error in ${__file} in function ${1} on line ${2}." >&2
  exit ${error_code}
}
# Always provide an error backtrace
trap '__caf_err_report "${FUNCNAME:-.}" ${LINENO}' ERR

echo "Performing Travis-CI installation phase on Linux..."

if [[ ${BUILD_TYPE} != InstallScript ]]; then # Ubuntu on Travis-CI, NOT testing install.sh
    if ! [[ -x "${HOME}/.local/bin/mpif90" && -x "${HOME}/.local/bin/mpicc" ]]; then
        # mpich install not cached
        # could use prerequisites/build instead...
        wget "${MPICH_URL_HEAD}/${MPICH_URL_TAIL}"
        mv "${MPICH_URL_TAIL}" "${MPICH_DIR}/.."
        (
	    cd "${MPICH_DIR}/.."
            tar -xzvf "${MPICH_URL_TAIL}"
            cd "${MPICH_URL_TAIL%.tar.gz}"
            ./configure --prefix="${MPICH_DIR}"
            make -j 4
            make install
        )
        for f in "${MPICH_DIR}/bin/"*; do
            if [[ -x "${f}" ]]; then
		ln -fs "${f}" "${HOME}/.local/bin/${f##*/}"
            fi
        done
    fi
    mpif90 --version
    mpicc --version
    cmake --version
fi

echo "Done."
