#!/bin/sh

set -e # exit on error

print_usage_info()
{
    echo "OpenCoarrays Installation Script"
    echo ""
    echo "USAGE:"
    echo "./install.sh [--help | [--prefix=PREFIX]"
    echo ""
    echo " --help             Display this help text"
    echo " --prefix=PREFIX    Install binary in 'PREFIX/bin'"
    echo " --prereqs          Display a list of prerequisite software."
    echo "                    Default prefix='\$HOME/.local/bin'"
    echo ""
    echo "For a non-interactive build with the 'yes' utility installed, execute"
    echo "yes | ./install.sh"
}

GCC_VERSION=11

list_prerequisites()
{
    echo "By default, OpenCoarrays and this installer use the following prerequisite package" 
    echo "versions, which also indicate the versions that the installer will install if"
    echo "missing and if granted permission:"
    echo ""
    echo "  GCC $GCC_VERSION"
    echo "  fpm 0.5.0"
    echo "  pkg-config 0.29.2"
    echo "  realpath 9.0 (Homebrew coreutils 9.0)"
    echo ""
    echo "Some earlier versions will also work."
}

while [ "$1" != "" ]; do
    PARAM=$(echo "$1" | awk -F= '{print $1}')
    VALUE=$(echo "$1" | awk -F= '{print $2}')
    case $PARAM in
        -h | --help)
            print_usage_info
            exit
            ;;
        --prereqs)
            list_prerequisites
            exit
            ;;
        --prefix)
            PREFIX=$VALUE
            ;;
        *)
            echo "ERROR: unknown parameter \"$PARAM\""
            usage
            exit 1
            ;;
    esac
    shift
done

set -u # error on use of undefined variable

if [ -z ${MPIFC+x} ] || [ -z ${MPICC+x} ] ; then
  if command -v mpifort > /dev/null 2>&1; then
    MPIFC=`which mpifort`
    echo "Setting MPIFC=$MPIFC"
  fi
  if command -v mpicc > /dev/null 2>&1; then
    MPICC=`which mpicc`
    echo "Setting MPICC=$MPICC"
  fi
fi

if command -v pkg-config > /dev/null 2>&1; then
  PKG_CONFIG=`which pkg-config`
fi
  
if command -v realpath > /dev/null 2>&1; then
  REALPATH=`which realpath`
fi

if command -v fpm > /dev/null 2>&1; then
  FPM=`which fpm`
fi

ask_permission_to_use_homebrew()
{
  echo ""
  echo "Either one or both of the environment variables MPIFC and MPICC are unset or"
  echo "one or more of the following packages are not in the PATH: pkg-config, realpath, and fpm."
  echo "If you grant permission to install prerequisites, you will be prompted before each installation." 
  echo ""
  echo "Press 'Enter' to choose the square-bracketed default answer:"
  printf "Is it ok to use Homebrew to install prerequisite packages? [yes] "
}

ask_permission_to_install_homebrew()
{
  echo ""
  echo "Homebrew not found. Installing Homebrew requires sudo privileges."
  echo "If you grant permission to install Homebrew, you may be prompted to enter your password." 
  echo ""
  echo "Press 'Enter' to choose the square-bracketed default answer:"
  printf "Is it ok to download and install Homebrew? [yes] "
}

ask_permission_to_install_homebrew_package()
{
  echo ""
  if [ ! -z ${2+x} ]; then
    echo "Homebrew installs $1 collectively in one package named '$2'."
    echo ""
  fi
  printf "Is it ok to use Homebrew to install $1? [yes] "
}

CI=${CI:-"false"} # GitHub Actions workflows set CI=true

exit_if_user_declines()
{
  if [ $CI = true ]; then 
    echo " 'yes' assumed (GitHub Actions workflow detected)"
    return
  fi
  read answer
  if [ -n "$answer" -a "$answer" != "y" -a "$answer" != "Y" -a "$answer" != "Yes" -a "$answer" != "YES" -a "$answer" != "yes" ]; then
    echo "Installation declined."
    case ${1:-} in  
      *MPI*) 
        echo "To use compilers other than Homebrew-installed mpifort and mpicc,"
        echo "please set the MPIFC and MPICC environment variables and rerun './install.sh'." ;;
      *) 
        echo "Please ensure that $1 is installed and in your PATH and then rerun './install.sh'." ;;
    esac
    echo "OpenCoarrays was not installed." 
    exit 1
  fi
}

DEPENDENCIES_DIR="build/dependencies"
if [ ! -d $DEPENDENCIES_DIR ]; then
  mkdir -p $DEPENDENCIES_DIR
fi

if [ -z ${MPIFC+x} ] || [ -z ${MPICC+x} ] || [ -z ${PKG_CONFIG+x} ] || [ -z ${REALPATH+x} ] || [ -z ${FPM+x} ] ; then

  ask_permission_to_use_homebrew 
  exit_if_user_declines "brew"

  BREW="brew"

  if ! command -v $BREW > /dev/null 2>&1; then

    ask_permission_to_install_homebrew
    exit_if_user_declines "brew"

    if ! command -v curl > /dev/null 2>&1; then
      echo "No download mechanism found. Please install curl and rerun ./install.sh"
      exit 1
    fi

    curl -L https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh -o $DEPENDENCIES_DIR/install-homebrew.sh --create-dirs
    chmod u+x $DEPENDENCIES_DIR/install-homebrew.sh

    if [ -p /dev/stdin ]; then 
       echo ""
       echo "Pipe detected.  Installing Homebrew requires sudo privileges, which most likely will"
       echo "not work if you are installing non-interactively, e.g., via 'yes | ./install.sh'."
       echo "To install OpenCoarrays non-interactiely, please rerun the OpenCoarrays installer after" 
       echo "executing the following command to install Homebrew:"
       echo "\"./$DEPENDENCIES_DIR/install-homebrew.sh\""
       exit 1
    else
      ./$DEPENDENCIES_DIR/install-homebrew.sh
      rm $DEPENDENCIES_DIR/install-homebrew.sh
    fi

    if [ $(uname) = "Linux" ]; then
      BREW=/home/linuxbrew/.linuxbrew/bin/brew
      eval "$($BREW shellenv)"
    fi
  fi

  if [ -z ${MPIFC+x} ] || [ -z ${MPICC+x} ]; then
    ask_permission_to_install_homebrew_package "'mpifort' and 'mpicc'" "open-mpi"
    exit_if_user_declines "mpifort and mpicc"
    "$BREW" install open-mpi
  fi
  MPICC=`which mpifort`
  MPIFC=`which mpicc`

  if [ -z ${PKG_CONFIG+x} ]; then
    ask_permission_to_install_homebrew_package "'pkg-config'"
    exit_if_user_declines "pkg-config"
    "$BREW" install pkg-config
  fi
  PKG_CONFIG=`which pkg-config`

  if [ -z ${REALPATH+x} ] ; then
    ask_permission_to_install_homebrew_package "'realpath'" "coreutils"
    exit_if_user_declines "realpath"
    "$BREW" install coreutils
  fi
  REALPATH=`which realpath`

  if [ -z ${FPM+x} ] ; then
    ask_permission_to_install_homebrew_package "'fpm'"
    exit_if_user_declines "fpm"
    "$BREW" tap awvwgk/fpm
    "$BREW" install fpm
  fi
  FPM=`which fpm`
fi

PREFIX=`$REALPATH ${PREFIX:-"${HOME}/.local"}`
echo "PREFIX=$PREFIX"

PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"
mkdir -p "$PKG_CONFIG_PATH"
echo "PKG_CONFIG_PATH=$PKG_CONFIG_PATH"

CAF_VERSION=$(sed -n '/[0-9]\{1,\}\(\.[0-9]\{1,\}\)\{1,\}/{s/^\([^.]*\)\([0-9]\{1,\}\(\.[0-9]\{1,\}\)\{1,\}\)\(.*\)/\2/p;q;}' .VERSION)
Fortran_COMPILER="$(which $($MPIFC --showme:command))"
CAF_MPI_Fortran_LINK_FLAGS="$($MPIFC --showme:link)"
CAF_MPI_Fortran_COMPILE_FLAGS=""
CAF_MPI_C_COMPILE_FLAGS="$(MPICC --showme:link)"
CAF_LIBS="lib/libopencoarrays.a"

MPIFC_LIBS="$($MPIFC --showme:libs)"
MPIFC_LIBS="$(echo -L${MPIFC_LIBS} | sed -n -e 's/\ / -l/g')"

MPIFC_LIB_DIRS="$($MPIFC --showme:libdirs)"
MPIFC_LIB_DIRS="$(echo -L${MPIFC_LIB_DIRS} | sed -n -e 's/\ / -L/g')"

OPENCOARRAYS_LIBS="-L$PREFIX/lib/ -lopencoarrays"

OPENCOARRAYS_PC="$PKG_CONFIG_PATH/opencoarrays.pc"
echo "OPENCOARRAYS_FC=$MPIFC"                                                    > $OPENCOARRAYS_PC
echo "OPENCOARRAYS_CC=$MPICC"                                                   >> $OPENCOARRAYS_PC
echo "OPENCOARRAYS_CFLAGS='-DPREFIX_NAME=_gfortran_caf_ -DGCC_GE_7 -DGCC_GE_8'" >> $OPENCOARRAYS_PC
echo "OPENCOARRAYS_LIBS=$OPENCOARRAYS_LIBS"                                     >> $OPENCOARRAYS_PC
echo ""                                                                         >> $OPENCOARRAYS_PC
echo "Name: OpenCoarrays"                                                       >> $OPENCOARRAYS_PC
echo "Description: Coarray Fortran parallel runtime library"                    >> $OPENCOARRAYS_PC
echo "URL: https://github.com/sourceryinstitute/opencoarrays"                   >> $OPENCOARRAYS_PC
echo "Version: $CAF_VERSION"                                                    >> $OPENCOARRAYS_PC
echo "Libs: ${OPENCOARRAYS_LIBS}"                                               >> $OPENCOARRAYS_PC

exit_if_pkg_config_pc_file_missing()
{
  export PKG_CONFIG_PATH
  if ! $PKG_CONFIG $1 ; then
    echo "$1.pc pkg-config file not found in $PKG_CONFIG_PATH"
    exit 1
  fi
}
exit_if_pkg_config_pc_file_missing "opencoarrays"

build_script_dir="build/script-templates"
mkdir -p $build_script_dir

CAF_MPI_LIBS="$MPIFC_LIB_DIRS"

cp src/script-templates/caf.in $build_script_dir/caf.in
sed -i '' -e "s/@CAF_VERSION@/$CAF_VERSION/g"                                     $build_script_dir/caf.in
sed -i '' -e "s:@Fortran_COMPILER@:$Fortran_COMPILER:g"                           $build_script_dir/caf.in
sed -i '' -e "s:@CAF_MPI_Fortran_LINK_FLAGS@:$CAF_MPI_Fortran_LINK_FLAGS:g"       $build_script_dir/caf.in
sed -i '' -e "s:@CAF_MPI_Fortran_COMPILE_FLAGS@:$CAF_MPI_Fortran_COMPILE_FLAGS:g" $build_script_dir/caf.in
sed -i '' -e "s:@CAF_LIBS@:$CAF_LIBS:g"                                           $build_script_dir/caf.in
sed -i '' -e "s:@CAF_MPI_LIBS@:$CAF_MPI_LIBS:g"                                   $build_script_dir/caf.in
cp build/script-templates/caf.in "$PREFIX"/bin/caf

MPIEXEC="$(which mpiexec)"
HAVE_FAILED_IMG=false
MPIEXEC_NUMPROC_FLAG="-n"
MPIEXEC_PREFLAGS=""
MPIEXEC_POSTFLAGS=""

cp src/script-templates/cafrun.in $build_script_dir/cafrun.in
sed -i '' -e "s/@CAF_VERSION@/$CAF_VERSION/g"                   $build_script_dir/cafrun.in
sed -i '' -e "s:@MPIEXEC@:$MPIEXEC:g"                           $build_script_dir/cafrun.in
sed -i '' -e "s/@MPIEXEC_NUMPROC_FLAG@/$MPIEXEC_NUMPROC_FLAG/g" $build_script_dir/cafrun.in
sed -i '' -e "s/@MPIEXEC_PREFLAGS@/$MPIEXEC_PREFLAGS/g"         $build_script_dir/cafrun.in
sed -i '' -e "s/@MPIEXEC_POSTFLAGS@/$MPIEXEC_POSTFLAGS/g"       $build_script_dir/cafrun.in
sed -i '' -e "s/@HAVE_FAILED_IMG@/$HAVE_FAILED_IMG/g"           $build_script_dir/cafrun.in
cp $build_script_dir/cafrun.in "$PREFIX"/bin/cafrun

if [ ! -x "$PREFIX"/bin/caf ]; then
  echo "caf not installed in '$PREFIX'/bin"
  exit 1
fi

if [ ! -x "$PREFIX"/bin/cafrun ]; then
  echo "cafrun not installed in '$PREFIX'/bin"
  exit 1
fi

cp src/script-templates/install.rsp-template $build_script_dir
sed -i '' -e "s:@CAF@:'$PREFIX'/bin/caf:g" $build_script_dir/install.rsp-template
sed -i '' -e "s:@MPICC@:'$MPICC':g"        $build_script_dir/install.rsp-template
mv $build_script_dir/install.rsp-template build/install.rsp
fpm @build/install

if [ ! -f "$PREFIX"/lib/libopencoarrays.a ]; then
  echo "libopencoarrays.a not installed in '$PREFIX'/lib"
  exit 1
fi

echo ""
echo "________________ OpenCoarrays has been installed ________________"
echo ""
echo "Compile and launch parallel Fortran programs using the installed"
echo "caf and cafrun scripts, respectively.  For example,"
echo ""
echo "$PREFIX/bin/caf example/hello.f90"
echo "$PREFIX/bin/cafrun -n 4 ./a.out"
echo ""
echo "The following command should work now to reinstall, OpenCoarrays:"
echo ""
echo "fpm @build/install"
