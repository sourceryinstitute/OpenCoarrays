#!/usr/bin/env bash

if [[ ! -f src/libcaf.h ]]; then
  echo "Run this script at the top level of the OpenCoarrays source tree."
  exit 1
fi

: "${1?'Missing absolute path to patch file'}"

# Absolute path to patch file
patch_file="${1}"

# Exit on error: 
set -o errexit

# Exit on use of unset variable: 
set -o nounset

# Return the highest exit code in a chain of pipes:
set -o pipefail

# Install the GCC trunk to default location: prerequisites/installations/gcc/trunk (takes several hours):
./install.sh --package gcc --install-branch trunk --yes-to-all 

# Patch the GCC trunk and rebuild (should be much faster than the initial build):
echo "Patching the GCC source."
pushd prerequistes/downloads/trunk 
  patch -p0 < "${patch_file}"
popd

echo "Rebuilding the patched GCC source."
./install.sh --package gcc --install-branch trunk --yes-to-all 

# Ensure the just-installed libraries are used when compiling
export LD_LIBRARY_PATH="${PWD}/prerequisites/installations/gcc/trunk/lib64":"${LD_LIBRARY_PATH}"

# Build MPICH with the patched compiler and install MPICH in the default location: prerequisites/installations/mpich/3.1.4:
./install.sh --package mpich --yes-to-all \
  --with-fortran "${PWD}/prerequisites/installations/gcc/trunk/bin/gfortran" \
  --with-c "${PWD}/prerequisites/installations/gcc/trunk/bin/gcc" \
  --with-C "${PWD}/prerequisites/installations/gcc/trunk/bin/g++" 

# Build OpenCoarrays with the patched compiler and install OpenCoarrays in the default location: prerequisites/installations:
./install.sh ---yes-to-all \
  --with-fortran "${PWD}/prerequisites/installations/gcc/trunk/bin/gfortran" \
  --with-c "${PWD}/prerequisites/installations/gcc/trunk/bin/gcc" \
  --with-C "${PWD}/prerequisites/installations/gcc/trunk/bin/g++" 
  --with-mpi "${PWD}/prerequisites/installations/mpich/3.1.4/" 
