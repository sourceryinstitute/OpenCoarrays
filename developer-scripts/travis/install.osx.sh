#!/usr/bin/env bash

set -o errexit
set -o verbose
set -o pipefail
set -o nounset
set -o errtrace

__file=developer-scripts/travis/install.osx.sh

# Error tracing
# requires `set -o errtrace`
__caf_err_report() {
  local error_code
  error_code=${?}
  echo "Error (code=${error_code}) in ${__file} in function ${1} on line ${2}." >&2
  false
  return ${error_code}
}
# Always provide an error backtrace
trap '__caf_err_report "${FUNCNAME:-.}" ${LINENO}' ERR

echo "Performing Travis-CI installation phase on macOS..."

# Update and install via Homebrew on macOS
brew update > /dev/null

brew ls --versions shellcheck >/dev/null || brew install --force-bottle shellcheck
brew outdated shellcheck || brew upgrade --force-bottle shellcheck
for pkg in ${OSX_PACKAGES}; do
    brew ls --versions "${pkg}" >/dev/null || brew install "${pkg}" || brew link --overwrite "${pkg}"
    brew outdated "${pkg}" || brew upgrade "${pkg}"
done
if [[ "${BUILD_TYPE:-}" == InstallScript ]]; then # uninstall some stuff if present
    brew uninstall --force --ignore-dependencies cmake || true
    brew uninstall --force --ignore-dependencies mpich || true
    brew uninstall --force --ignore-dependencies openmpi || true
else
    wget "${!MPICH_BOT_URL_HEAD}${MPICH_BOT_URL_TAIL}"
    brew install --force-bottle "${MPICH_BOT_URL_TAIL}"
    brew ls --versions mpich >/dev/null || brew install --force-bottle mpich
fi
mpif90 --version || mpif90 -show || true
mpicc --version || mpicc -show || true
cmake --version || true

echo "Done."
