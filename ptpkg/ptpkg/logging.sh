#
# ParaTools Packages
#
# Copyright (c) ParaTools, Inc.
#
# File: logging.sh
# Created: 8 Jan 2011
# Author: John C. Linford (jlinford@paratools.com)
#
# Description:
#    Logfile management functions
# 

#
# Initializes a logfile
# Arguments: $1: Name of logfile to select, $2: optional logfile banner
# Returns: none
#
function initLogfile {
  local _USER=`whoami`
  local _DATESTAMP=`date +%Y%m%d%H%M%S`
  local _PKG="$1"
  
  PTPKG_LOGFILE="${1}-${_DATESTAMP}"
  
  hline                     > $PTPKG_LOGFILE
  [ -z "$2" ] || echo "$2" >> $PTPKG_LOGFILE
  echo "Created: `date`"   >> $PTPKG_LOGFILE
  echo "User: $_USER"      >> $PTPKG_LOGFILE
  hline                    >> $PTPKG_LOGFILE
}

#
# Initializes a logfile for the selected package
# Arguments: $1: Name of logfile to select, $2: optional logfile banner
# Returns: none
#
function initPackageLogfile {
  local _USER=`whoami`
  local _DATESTAMP=`date +%Y%m%d%H%M%S`
  local _PKG="$1"
  local _PREFIX="$PTPKG_PREFIX/packages/$PTPKG_PKG/$PTPKG_TARGET"
  
  mkdir -p $_PREFIX
  PTPKG_LOGFILE="$_PREFIX/${1}-$_DATESTAMP"
  
  hline                     > $PTPKG_LOGFILE
  [ -z "$2" ] || echo "$2" >> $PTPKG_LOGFILE
  echo "Created: `date`"   >> $PTPKG_LOGFILE
  echo "User: $_USER"      >> $PTPKG_LOGFILE
  hline                    >> $PTPKG_LOGFILE
}


# EOF
