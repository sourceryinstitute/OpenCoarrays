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
  local error_code
  error_code=${?}
  echo "Error in ${__file} in function ${1} on line ${2}." >&2
  exit ${error_code}
}
# Always provide an error backtrace
trap '__caf_err_report "${FUNCNAME:-.}" ${LINENO}' ERR

echo "Performing Travis-CI script phase for the OpenCoarrays direct cmake build..."

mkdir cmake-build || echo "Cannot mkdir cmake-build"
for BUILD_TYPE in ${BUILD_TYPES}; do
    rm -rf cmake-build/* || true
    (
	cd cmake-build || echo 'cannot cd to cmake-build/'
	cmake -DCMAKE_INSTALL_PREFIX:PATH="${HOME}/OpenCoarrays" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" .. || echo "running cmake failed"
	make -j 4 || echo "running make failed"
	ctest --output-on-failure --schedule-random --repeat-until-fail "${NREPEAT:-5}" --timeout "${TEST_TIMEOUT:-200}"
	make install
	make uninstall
    )
done

echo "Done."
