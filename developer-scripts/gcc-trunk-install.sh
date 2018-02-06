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
  echo ""
  echo "  cd <opencoarrays-source-directory>"
  echo "  ./developer_scripts/gcc-trunk-install.sh [--patch-file <patch-file-name>] [--install-prefix <installation-path>]"
  echo "or"
  echo "  ./developer_scripts/gcc-trunk-install.sh [-p <patch-file-name>] [-i <installation-path>]"
  echo ""
  echo " Square brackets surround optional arguments."
  exit 0
}
[[ "${1:-}" == "-h" || "${1:-}" == "--help"  || ! -f src/libcaf.h ]] && usage

if [[ "${1:-}" == "-i" || "${1:-}" == "--install-prefix" ]]; then
  export install_prefix="${2}"
  if [[ "${3:-}" == "-i" || "${3:-}" == "--install-prefix" ]]; then
    export patch_file="${4}"
  fi
elif [[ "${1:-}" == "-p" || "${1:-}" == "--patch-file" ]]; then
  export patch_file="${2:-}"
  if [[ "${3:-}" == "-i" || "${3:-}" == "--install-prefix" ]]; then
    export install_prefix="${4}"
  fi
fi
export default_prefix="${HOME}/opt"
export install_prefix="${install_prefix:-${default_prefix}}"

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
if [[ ! -z "${patch_file:-}" ]]; then
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
./install.sh --only-download --package gcc --install-branch trunk --yes-to-all

if [[ ! -z "${absolute_path:-}" ]]; then
  # Patch the GCC trunk and rebuild
  echo "Patching the GCC source using ${absolute_path}."
  pushd prerequisites/downloads/trunk
    patch -p0 < "${absolute_path}"
  popd
fi

export GCC_install_prefix=${install_prefix}/gnu/trunk
# Build the patched GCC trunk
echo "Rebuilding the patched GCC source."
./install.sh \
  --package gcc \
  --install-branch trunk \
  --yes-to-all \
  --num-threads 4 \
  --disable-bootstrap \
  --install-prefix "${GCC_install_prefix}"

# Verify that GCC installed in the expected path
if ! type "${GCC_install_prefix}"/bin/gfortran >& /dev/null; then
  echo "gfortran is not installed in the expected location ${GCC_install_prefix}."
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

if [[ -d "${GCC_install_prefix}/lib64" ]]; then
  prepend_to_LD_LIBRARY_PATH "${GCC_install_prefix}/lib64/"
fi

echo "\${LD_LIBRARY_PATH}=${LD_LIBRARY_PATH:=}"

if [[ "${LD_LIBRARY_PATH}" == "${old_path}" ]]; then
  echo "gfortran libraries did not install where expected: ${GCC_install_prefix}/lib64 or ${GCC_install_prefix}/lib"
  exit 1
fi

# Build MPICH with the patched compilers.
echo "Building MPICH with the patched compilers."
export mpich_install_prefix="${install_prefix}/mpich/3.2/gnu/trunk"
./install.sh \
  --package mpich \
  --num-threads 4 \
  --yes-to-all \
  --with-fortran "${GCC_install_prefix}/bin/gfortran" \
  --with-c "${GCC_install_prefix}/bin/gcc" \
  --with-cxx "${GCC_install_prefix}/bin/g++" \
  --install-prefix "${mpich_install_prefix}"

# Verify that MPICH installed where expected
if ! type "${mpich_install_prefix}"/bin/mpifort; then
  echo "MPICH is not installed in the expected location ${mpich_install_prefix}."
  exit 1
fi

# Build OpenCoarrays with the patched compilers and the just-built MPICH
echo "Building OpenCoarrays."
export opencoarrays_version=$(./install.sh --version)
./install.sh \
  --package opencoarrays \
  --disable-bootstrap \
  --num-threads 4 \
  --yes-to-all \
  --with-fortran "${GCC_install_prefix}/bin/gfortran" \
  --with-c "${GCC_install_prefix}/bin/gcc" \
  --with-cxx "${GCC_install_prefix}/bin/g++" \
  --with-mpi "${mpich_install_prefix}" \
  --install-prefix "${install_prefix}/opencoarrays/${opencoarrays_version}/gnu/trunk"
