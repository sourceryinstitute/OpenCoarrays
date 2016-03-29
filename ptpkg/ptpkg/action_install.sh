#!/bin/bash
#
# ParaTools Packages
#
# Copyright (c) ParaTools, Inc.
#
# File: action_install.sh
# Created: 15 Dec. 2011
# Author: John C. Linford (jlinford@paratools.com)
#
# Description:
#    Performs an installation
#

################################################################################
#
# Default configuration
#
################################################################################

PTPKG_PREFIX_FLAG="ask"
PTPKG_BUILD_PREFIX_FLAG="auto"
PTPKG_TARGET_FLAG="auto"
PTPKG_CUSTOMIZE_FLAG="no"
PTPKG_REINSTALL_FLAG="no"
PTPKG_MAKE_JOBS=1

################################################################################
#
# Function definitions
#
################################################################################

#
# Initializes installation action.
# Sets PTPKG_CONFIG_* flags according to the arguments.
# Arguments: Command line arguments
# Returns: 0 if action was initilized successfully, 1 otherwise
#
function initAction {
  local _OS
  local _DIST
  local _COMP
  local _ARCH
  local _TARG
  local _ARGVAL

  # Process arguments
  while [ $# -gt 0 ] ; do
    _ARGVAL="`echo $1 | cut -d= -f2-`"
    case "$1" in
      *help*)
        echo
        hline
        echo " ** Install command parameters **"
        echo "  The first option is the default when the parameter is omitted"
        echo "  The option in ALL-CAPS is the default when the parameter value is omitted"
        echo
        echo "  --prefix=(ask|<path>)         Installation root directory"
        echo "  --build-prefix=(auto|<path>)  Package build root directory"
        echo "  --target=(auto|ask|<target>)  Specify target configuration"
        echo "  --customize[=(no|YES|ask)]    Answer to \"Customize installation?\" prompt"
        echo "  --reinstall[=(no|YES|ask)]    Answer all \"Reinstall package?\" prompts"
        echo "  --jobs=N                      Set number of parallel make jobs"
        echo "                                The default is the number of CPUs detected"
        echo "  --help                        Show this help message"
        echo
        echo " ** Examples **"
        echo
        echo "  configure.sh install:"
        echo "     prefix=ask, target=auto, customize=no, reinstall=no, jobs=<ncpus>"
        echo
        echo "  configure.sh install --prefix=<path> --reinstall --jobs=12"
        echo "     prefix=<path>, target=auto, customize=no, reinstall=yes, jobs=12"
        echo
        echo "  configure.sh install --customize --reinstall --target=linux-suse11cray-gnu-x86_64"
        echo "     prefix=ask, target=linux-suse11cray-gnu-x86_64, customize=yes, reinstall=yes, jobs=<ncpus>"
        hline
        echo
        return 1
        ;;
      --prefix*)
        case "$_ARGVAL" in
          ask|ASK)
            PTPKG_PREFIX_FLAG="ask"
            ;;
          $1)
            echo "--prefix requires a value.  Try '--help' for help."
            return 1
            ;;
          *)
            if ! isAbsolutePath "$_ARGVAL" ; then
              echo "The value for --prefix must be an absolute path."
              return 1
            fi
            PTPKG_PREFIX_FLAG="$_ARGVAL"
            ;;
        esac
        ;;
      --build-prefix*)
        case "$_ARGVAL" in
          auto|AUTO)
            # Value is already set by ptptkg.sh
            ;;
          $1)
            echo "--build-prefix requires a value.  Try '--help' for help."
            return 1
            ;;
          *)
            if ! isAbsolutePath "$_ARGVAL" ; then
              echo "The value for --build-prefix must be an absolute path."
              return 1
            fi
            export PTPKG_BUILD_PREFIX="$_ARGVAL"
            ;;
        esac
        ;;
      --target*)
        case "$_ARGVAL" in
          auto|AUTO)
            PTPKG_TARGET_FLAG="auto"
            ;;
          ask|ASK)
            PTPKG_TARGET_FLAG="ask"
            ;;
          $1)
            echo "--target requires a value.  Try '--help' for help."
            return 1
            ;;
          *)
            PTPKG_TARGET_FLAG="$_ARGVAL"
            ;;
        esac
        ;;
      --customize*)
        case "$_ARGVAL" in
          no|NO)
            PTPKG_CUSTOMIZE_FLAG="no"
            ;;
          ask|ASK)
            PTPKG_CUSTOMIZE_FLAG="ask"
            ;;
          $1|yes|YES)
            PTPKG_CUSTOMIZE_FLAG="yes"
            ;;
          *)
            echo "Invalid value for --customize: $_ARGVAL.  Try '--help' for help."
            return 1
            ;;
        esac
        ;;
      --reinstall*)
        case "$_ARGVAL" in
          no|NO)
            PTPKG_REINSTALL_FLAG="no"
            ;;
          ask|ASK)
            PTPKG_REINSTALL_FLAG="ask"
            ;;
          $1|yes|YES)
            PTPKG_REINSTALL_FLAG="yes"
            ;;
          *)
            echo "Invalid value for --reinstall: $_ARGVAL.  Try '--help' for help."
            return 1
            ;;
        esac
        ;;
      --jobs*)
        if [ "$_ARGVAL" == "$1" ] ; then
          echo "--jobs requires a value.  Try '--help' for help."
          return 1
        elif isNumeric "$_ARGVAL" ; then
          PTPKG_MAKE_JOBS=$_ARGVAL
        else
          echo "Invalid number of jobs: $_ARGVAL.  Try '--help' for help."
          return 1
        fi
        ;;
      *)
        echo "Unknown argument: $1.  Try '--help' for help."
        return 1
        ;;
    esac
    shift
  done

  # Discover supported targets
  getTargets

  # Get host configuration
  resolveHost

  # Select a target configuration
  case "$PTPKG_TARGET_FLAG" in
    ask)
      banner "Please select a target configuration"
      select _TARG in ${PTPKG_ALL_TARGETS[@]} ; do
        selectTarget "$_TARG" && break
        exitWithError "Invalid target: $_TARG.  The targets database is corrupt."
      done
      ;;
    auto)
      if ! autoSelectTarget ; then
        echo "The target configuration could not be determined."
        echo "Please specify a target with the --target flag."
        showAllTargets
        exitWithError "Aborting..."
      fi
      ;;
    *)
      if ! selectTarget "$PTPKG_TARGET_FLAG" ; then
        echo "Invalid target specified on command line: $PTPKG_TARGET_FLAG"
        showAllTargets
        echo
        exitWithError "Aborting..."
      fi
      ;;
  esac
  echo "Selected target configuration: $PTPKG_TARGET"

  # Discover available packages
  getPackages

  # Get number of parallel make jobs
  if [ $PTPKG_MAKE_JOBS -le 1 ] ; then
    getProcessorCount
    PTPKG_MAKE_JOBS=$?
    if [ -z "${PTPKG_MAKE_JOBS}" ] || [ ${PTPKG_MAKE_JOBS} -eq 0 ] ; then
      PTPKG_MAKE_JOBS=1
    fi
  fi

  return 0
}

################################################################################
#
# BEGIN installation action script
#
################################################################################

# Import common functionality
source `cd ${0%/*} && pwd -P`/ptpkg.sh

# Initialize action
initAction $@ || exit 1

# Allow the user to customize the installation
while true ; do
  echoAll "The following packages and their dependencies will be installed" ${PTPKG_DEFAULT_PACKAGES[@]}
  case "$PTPKG_CUSTOMIZE_FLAG" in
    yes)
      getPackageSelections
      ;;
    no)
      PTPKG_PACKAGE_SELECTION=( ${PTPKG_DEFAULT_PACKAGES[@]} )
      ;;
    *)
      if promptYesNo "Would you like to customize the installation?" ; then
        getPackageSelections
      else
        PTPKG_PACKAGE_SELECTION=( ${PTPKG_DEFAULT_PACKAGES[@]} )
      fi
      ;;
  esac
  buildInstallation && break
  echo "The set of packages you have selected cannot be installed."
  promptYesNo "Would you like to select a different set of packages?" || \
      exitWithError "Installation cannot continue.  Exiting..."
done

# Show target configuration
if [ ${#PTPKG_TARGET_PROVIDES[@]} -ne 0 ] ; then
  echoAll "Packages provided by $PTPKG_TARGET" ${PTPKG_TARGET_PROVIDES[@]}
fi
if [ ${#PTPKG_TARGET_REQUIRES[@]} -ne 0 ] ; then
  echoAll "Packages required by $PTPKG_TARGET" ${PTPKG_TARGET_REQUIRES[@]}
fi
if [ ${#PTPKG_TARGET_EXCLUDES[@]} -ne 0 ] ; then
  echoAll "Packages excluded by $PTPKG_TARGET" ${PTPKG_TARGET_EXCLUDES[@]}
fi

# Show required dependencies
if [ ${#PTPKG_DEPENDENCIES[@]} -gt 0 ] ; then
  echoAll "Required dependencies" ${PTPKG_DEPENDENCIES[@]}
fi

# Show installation list
echoAll "Packages to be installed, in order of installation" ${PTPKG_INSTALLATION[@]}
echo

# Get installation destination from user
if [ "$PTPKG_PREFIX_FLAG" == "ask" ] ; then
  while true ; do
    promptForDir "Please enter the installation destination directory" \
                "${PTPKG_HOME}/${PTPKG_TARGET}" "allow_nonexistant"
    setPrefix "$PTPKG_PROMPTFORDIR_RESULT" && break
  done
else
  setPrefix "$PTPKG_PREFIX_FLAG" || \
      exitWithError "Invalid path specified on command line: $PTPKG_PREFIX_FLAG"
fi

# Show work to be done
echo
hline
echo "  Host:   $PTPKG_HOST"
echo "  Target: $PTPKG_TARGET"
echo
echo "  Build directory:   $PTPKG_BUILD_PREFIX"
echo "  Install directory: $PTPKG_INSTALL_PREFIX"
echo
echo "  $PTPKG_MAKE_JOBS $PTPKG_MAKE_CMD jobs will be used"
hline
echo
promptYesNo "Do you wish to continue?" || exit 0

# Initialize installation destination
initPrefix || exitWithError "Error: Cannot initilize $PTPKG_PREFIX"

# Start proceedure timer
PTPKG_START_TIME=`date +%s`

# Create receipt baseline
echo "Scanning installation destination: $PTPKG_INSTALL_PREFIX"
find "$PTPKG_INSTALL_PREFIX" | sort > "$PTPKG_HOME/.receipt0"

# Install each package in the installation
for pkg in ${PTPKG_INSTALLATION[@]} ; do
  selectPackage "${pkg}" || exitWithError "${pkg}: Invalid package"
  if ! execPackageInstall ; then
    promptYesNo "Installation failed. Continue?" || \
            exitWithError "Aborted due to installation failure"
  fi
done

# Write environment scripts
banner "Writing bashrc environment file"
writeBashrc
echo

banner "Writing cshrc environment file"
writeCshrc
echo

banner "Writing $PTPKG_RELEASE_NAME module file"
writeModule
echo

# Install release files
if [ -d "$PTPKG_HOME/release" ] ; then
  cp -R "$PTPKG_HOME/release" "$PTPKG_PREFIX"
fi

# Install target configuration files
mkdir -p "$PTPKG_PREFIX/targets"
rm -rf "$PTPKG_PREFIX/targets/$PTPKG_TARGET"
cp -R "$PTPKG_TARGET_DIR" "$PTPKG_PREFIX/targets/$PTPKG_TARGET"

# Install PTPKG and configure.sh
rm -rf "$PTPKG_PREFIX/ptpkg" "$PTPKG_PREFIX/configure.sh"
cp -R "$PTPKG_SRC_DIR" "$PTPKG_PREFIX"
cp "$PTPKG_HOME/configure.sh" "$PTPKG_PREFIX"

# Disable some actions
pushd "$PTPKG_PREFIX/ptpkg"
mkdir .disabled
mv action_install.sh action_extract_target.sh .disabled
popd

# Compile Python modules
banner "Compiling all Python sources under $PTPKG_PREFIX"
bootstrapPackageDeps "Python"
python -m compileall "$PTPKG_PREFIX"

# Set permissions
banner "Setting file permissions"
find "$PTPKG_PREFIX" -type d -exec chmod a+rx {} \;
find "$PTPKG_PREFIX" -type f -exec chmod a+r {} \;

# Stop proceedure timer
PTPKG_STOP_TIME=`date +%s`
((PTPKG_SEC=$PTPKG_STOP_TIME - $PTPKG_START_TIME))

# Show package status lists
if [ ${#PTPKG_INSTALLED_PACKAGES[@]} -gt 0 ] ; then
  echoAll "Packages installed" ${PTPKG_INSTALLED_PACKAGES[@]}
fi
if [ ${#PTPKG_SKIPPED_PACKAGES[@]} -gt 0 ] ; then
  echoAll "Packages skipped" ${PTPKG_SKIPPED_PACKAGES[@]}
fi
if [ ${#PTPKG_ABORTED_PACKAGES[@]} -gt 0 ] ; then
  echoAll "Packages aborted" ${PTPKG_ABORTED_PACKAGES[@]}
fi

# Show installation size
banner "Installation size"
du -hs "$PTPKG_PREFIX"

echo "Installation completed in ${PTPKG_SEC} seconds"
echo

################################################################################
#
# END installation action script
#
################################################################################

# EOF
