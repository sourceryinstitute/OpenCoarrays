cmd=`basename $0`

usage()
{
    echo ""
    echo " $cmd - Coarray Fortran (CAF)"
    echo ""
    echo " Usage:"
    echo "    $cmd <caf-source-file>"
    echo "    or"
    echo "    $cmd [option] ..."
    echo ""
    echo " Options:"
    echo "   --help, -h               Show this help message"
    echo "   --version, -v, -V        Report version and copyright information"
    echo ""
    echo " Examples:"
    echo ""
    echo "   $cmd foo.f90"
    echo "   $cmd -v"
    echo "   $cmd --help"
    echo ""
    echo "Given a Fortran 2015 CAF source file, $cmd produces a semantically equivalent"
    echo "program that uses a subset of Fortran supported any recently released Fortran "
    echo "compilers.  The subset uses the opencoarrays module and is referred to as the" 
    echo "OpenCoarrays CAF (OCAF) dialect."
}

# Print usage information if caf is invoked without arguments
if [ $# == 0 ]; then
  usage | less
  exit 10
fi


if [[ $1 == '-v' || $1 == '-V' || $1 == '--version' ]]; then
    echo ""
    echo "OpenCoarrays Coarray Fortran Compiler Wrapper (caf version $caf_version)"
    echo "Copyright (C) 2015-2016 Sourcery, Inc."
    echo ""
    echo "OpenCoarrays comes with NO WARRANTY, to the extent permitted by law."
    echo "You may redistribute copies of OpenCoarrays under the terms of the"
    echo "BSD 3-Clause License.  For more information about these matters, see"
    echo "the file named COPYRIGHT-BSD3."
    echo ""
elif [[ $1 == '-w' || $1 == '--wrapping' || $1 == '--wraps' ]]; then
  echo "caf wraps CAFC=$CAFC"
elif [[ $1 == '-h' || $1 == '--help' ]]; then
  # Print usage information
  usage | less
  exit 20
else
  export SOURCE=$1
  export OFP_HOME=$ofp_dir
  make -f $transpiler_dir/Makefile check
fi
