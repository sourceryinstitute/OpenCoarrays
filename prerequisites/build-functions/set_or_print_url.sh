# If -p, -D, -P, or -V specifies a package, set package_url
# If -U specifies a package, print the package_url and exit with normal status
# shellcheck disable=SC2154
set_or_print_url()
{
  # Verify requirements
  [ ! -z "${arg_U}" ] && [ ! -z "${arg_D:-${arg_p:-${arg_P:-${arg_V:-${arg_B}}}}}" ] &&
    emergency "Please pass only one of {-B, -D, -p, -P, -U, -V} or a longer equivalent (multiple detected)."

  # Get package name from argument passed with  -p, -D, -P, -V, or -U
  package_to_build="${arg_p:-${arg_D:-${arg_P:-${arg_U:-${arg_V:-${arg_B}}}}}}"

  if [[ "${package_to_build}" == 'cmake' ]]; then
    major_minor="${version_to_build%.*}"
  elif [[ "${package_to_build}" == "gcc" ]]; then
    if [[ -z "${arg_b:-${arg_B}}" ]]; then
      gcc_url_head="ftp://ftp.gnu.org:/gnu/gcc/gcc-${version_to_build}/"
    else
      gcc_url_head="svn://gcc.gnu.org/svn/gcc/"
    fi
  fi
  package_url_head=(
    "gcc;${gcc_url_head-}"
    "wget;ftp://ftp.gnu.org:/gnu/wget/"
    "m4;ftp://ftp.gnu.org:/gnu/m4/"
    "pkg-config;http://pkgconfig.freedesktop.org/releases/"
    "mpich;http://www.mpich.org/static/downloads/${version_to_build-}/"
    "flex;http://sourceforge.net/projects/flex/files/"
    "make;ftp://ftp.gnu.org/gnu/make/"
    "bison;ftp://ftp.gnu.org:/gnu/bison/"
    "cmake;http://www.cmake.org/files/v${major_minor-}/"
    "subversion;http://www.eu.apache.org/dist/subversion/"
  )
  for package in "${package_url_head[@]}" ; do
     KEY="${package%%;*}"
     VALUE="${package##*;}"
     info "KEY=${KEY}  VALUE=${VALUE}"
     
     if [[ "${package_to_build}" == "${KEY}" ]]; then
       # We recognize the package name so we set the URL head:
       url_head="${VALUE}"
       break
     fi
  done

  # Set differing tails for GCC release downloads versus development branch checkouts
  if [[ "${package_to_build}" == 'gcc' ]]; then
    if [[ "${fetch}" == 'svn' ]]; then
      gcc_tail=${version_to_build-branches}
    else
      gcc_tail="gcc-${version_to_build}.tar.bz2"
    fi
  fi
  package_url_tail=(
    "gcc;${gcc_tail-}"
    "wget;wget-${version_to_build-}.tar.gz"
    "m4;m4-${version_to_build-}.tar.bz2"
    "pkg-config;pkg-config-${version_to_build-}.tar.gz"
    "mpich;mpich-${version_to_build-}.tar.gz"
    "flex;flex-${version_to_build-}.tar.bz2"
    "bison;bison-${version_to_build-}.tar.gz"
    "make;make-${version_to_build-}.tar.bz2"
    "cmake;cmake-${version_to_build-}.tar.gz"
    "subversion;subversion-${version_to_build-}.tar.gz"
  )
  for package in "${package_url_tail[@]}" ; do
     KEY="${package%%;*}"
     VALUE="${package##*;}"
     if [[ "${package_to_build}" == "${KEY}" ]]; then
       # We recognize the package name so we set the URL tail:
       url_tail="${VALUE}"
       break
     fi
  done

  if [[ -z "${url_head:-}" || -z "${url_tail}" ]]; then
    emergency "Package ${package_name:-} not recognized.  Use --l or --list-packages to list the allowable names."
  fi

  package_url="${url_head}""${url_tail}"

  # If a printout of the package URL was requested, then print it and exit with normal status
  if [[ ! -z "${arg_U:-}" ]]; then
    printf "%s\n" "${package_url}"
    exit 0
  fi
}
