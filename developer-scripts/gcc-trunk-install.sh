#!/usr/bin/env bash

# Exit on error or use of an unset variable:
set -o errexit
set -o nounset

# Return the highest exit code in a chain of pipes:
set -o pipefail

# Print usage information and exit if this script was invoked with no arugments or with -h or --help
# as the first argument or in a location other than the top-level OpenCoarrays source directory
function usage()
{
  echo "Usage:"
  echo "  cd <opencoarrays-source-directory>"
  echo "  ./developer_scripts/patched-trunk-install.sh [patch-file]"
  echo ""
  echo "  where omitting the patch file builds the unpatched GCC trunk."
  exit 1
}
[[ "${1:-}" == "-h" || "${1:-}" == "--help"  || ! -f src/libcaf.h ]] && usage

patch_file="${1:-}"

function set_absolute_path()
{
  : "${1?'set_absolute_path: no argument provided'}"

  arg=${1}
  first_character=$(echo "${arg}" | cut -c1-1)
  if [[ "${first_character}" == "/" ]]; then
    absolute_path="${arg}"
  else
    absolute_path="${PWD%%/}/${arg}"
  fi
}
if [[ ! -z "${1:-}" ]]; then
  set_absolute_path "${patch_file}"
fi


### Define functions
function choose_package_manager()
{
  OS=$(uname)
  case "${OS}" in

    "Darwin" )
      if type brew >& /dev/null; then
        package_manager="brew"
      elif type port >& /dev/null; then
        package_manager="port"
      fi
      ;;

    "Linux" )
      if [[ ! -f /etc/lsb-release ]]; then
        echo "I don't recognize this Linux distribution. Possibly it's too old to have the expected /etc/lsb-release file."
        exit 1
      fi
      . /etc/lsb-release

      case "${DISTRIB_ID:-}" in
        "Ubuntu" )
           package_manager="apt-get"
        ;;
        "Debian" )
           package_manager="dnf"
        ;;
        *)
          echo "I don't recognize the Linux distribution ${DISTRIB_ID:-}"
          exit 1
        ;;
      esac
      ;;

    *)
      echo "I don't recognize the operating system \${OS}"
      exit 1
      ;;
  esac
}
choose_package_manager

### Install any missing prerequisite packages and executable programs ###

### First install the packages for which package management suffices:
function install_if_missing()
{
  # Exit with error message if no first argument:
  : "${1?'\n ERROR: install_if_missing function requires a package name as the first argument.'}"

  package="${1}"          # Interpret the 1st argument as the package name
  executable="${2:-${1}}" # Interpret the 2nd argument as the prerequisite executable program (Default = 1st argument)

  printf "Checking whether ${executable} is in path...  "

  if type ${executable} >& /dev/null; then

    printf "yes.\n"

  else # install package

    printf "no\n"
    echo "sudo ${package_manager} install ${package}"
    sudo "${package_manager}" install ${package}

  fi
}

# Install subversion to check out the GCC trunk if not already in the PATH:
install_if_missing subversion svn

# Install released versions of the GNU compilers if not already in the PATH:
install_if_missing gfortran
install_if_missing g++

# Install build software:
install_if_missing cmake
install_if_missing make
install_if_missing flex

### Install the prerequisites that must be built from source ###

# Download and build the GCC trunk:
echo "Downloading the GCC trunk."
./install.sh --only-download --package gcc --install-branch trunk

if [[ ! -z "${absolute_path:-}" ]]; then
  # Patch the GCC trunk and rebuild
  echo "Patching the GCC source using ${absolute_path}."
  pushd prerequisites/downloads/trunk
    patch -p0 < "${absolute_path}"
  popd
fi

# Build the patched GCC trunk
echo "Rebuilding the patched GCC source."
./install.sh --package gcc --install-branch trunk --yes-to-all

# Verify that GCC installed in the expected path
patched_GCC_install_path=${PWD}/prerequisites/installations/gcc/trunk
if ! type "${patched_GCC_install_path}"/bin/gfortran >& /dev/null; then
  echo "gfortran is not installed in the expected location ${patched_GCC_install_path}."
  exit 1
fi

# Ensure that the just-installed GCC libraries are used when linking
echo "Setting and exporting LD_LIBRARY_PATH"

function prepend_to_LD_LIBRARY_PATH() {
  : ${1?'set_LD_LIBRARY_PATH: missing path'}
  new_path="${1}"
  if [[ -z "${LD_LIBRARY_PATH:-}" ]]; then
    export LD_LIBRARY_PATH="${new_path}"
  else
    export LD_LIBRARY_PATH="${new_path}:${LD_LIBRARY_PATH}"
  fi
}

old_path="${LD_LIBRARY_PATH:-}"

if [[ -d "${PWD}/prerequisites/installations/gcc/trunk/lib"   ]]; then
  prepend_to_LD_LIBRARY_PATH "${patched_GCC_install_path}/lib/"
fi

if [[ -d "${PWD}/prerequisites/installations/gcc/trunk/lib64" ]]; then
  prepend_to_LD_LIBRARY_PATH "${patched_GCC_install_path}/lib64/"
fi

echo "\${LD_LIBRARY_PATH}=${LD_LIBRARY_PATH:=}"

if [[ "${LD_LIBRARY_PATH}" == "${old_path}" ]]; then
  echo "gfortran libraries did not install where expected: ${patched_GCC_install_path}/lib64 or ${patched_GCC_install_path}/lib"
  exit 1
fi

# Build MPICH with the patched compilers.
echo "Building MPICH with the patched compilers."
./install.sh \
  --package mpich \
  --num-threads 4 \
  --yes-to-all \
  --with-fortran "${patched_GCC_install_path}/bin/gfortran" \
  --with-c "${patched_GCC_install_path}/bin/gcc" \
  --with-cxx "${patched_GCC_install_path}/bin/g++"

# Verify that MPICH installed where expected
mpich_install_path=$(./install.sh -P mpich)
if ! type "${mpich_install_path}"/bin/mpifort; then
  echo "MPICH is not installed in the expected location ${mpich_install_path}."
  exit 1
fi

# Build OpenCoarrays with the patched compilers and the just-built MPICH
echo "Building OpenCoarrays with the patched compilers"
./install.sh \
  --package opencoarrays \
  --disable-bootstrap \
  --num-threads 4 \
  --yes-to-all \
  --with-fortran "${patched_GCC_install_path}/bin/gfortran" \
  --with-c "${patched_GCC_install_path}/bin/gcc" \
  --with-cxx "${patched_GCC_install_path}/bin/g++" \
  --with-mpi "${mpich_install_path}"
