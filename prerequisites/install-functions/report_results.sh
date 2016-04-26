report_results()
{
  # Report installation success or failure:
  if [[ -x "$install_path/bin/caf" && -x "$install_path/bin/cafrun" ]]; then

    # Installation succeeded
    echo "$this_script: Done."
    echo ""
    echo "*** The OpenCoarrays compiler wrapper (caf) and program  ***"
    echo "*** launcher (cafrun) are in the following directory:    ***"
    echo ""
    echo "$install_path/bin."
    echo ""
    if [[ -f setup.sh ]]; then
      ${SUDO:-} rm setup.sh
    fi
    # Prepend the OpenCoarrays license to the setup.sh script:
    while IFS='' read -r line || [[ -n "$line" ]]; do
        echo "# $line" >> setup.sh
    done < "${opencoarrays_src_dir}/LICENSE"
    echo "#                                                                      " >> setup.sh
    echo "# Execute this script via the following command:                       " >> setup.sh
    echo "# source $install_path/setup.sh                                        " >> setup.sh
    echo "                                                                       " >> setup.sh
    gcc_install_path=`"${build_script}" -P gcc`
    if [[ -x "$gcc_install_path/bin/gfortran" ]]; then
      echo "if [[ -z \"\$PATH\" ]]; then                                         " >> setup.sh
      echo "  export PATH=\"$gcc_install_path/bin\"                              " >> setup.sh
      echo "else                                                                 " >> setup.sh
      echo "  export PATH=\"$gcc_install_path/bin:\$PATH\"                       " >> setup.sh
      echo "fi                                                                   " >> setup.sh
    fi
    if [[ -d "$gcc_install_path/lib" || -d "$gcc_install_path/lib64" ]]; then
      gfortran_lib_paths="$gcc_install_path/lib64/:$gcc_install_path/lib"
      echo "if [[ -z \"\$LD_LIBRARY_PATH\" ]]; then                              " >> setup.sh
      echo "  export LD_LIBRARY_PATH=\"$gfortran_lib_paths\"                     " >> setup.sh
      echo "else                                                                 " >> setup.sh
      echo "  export LD_LIBRARY_PATH=\"$gfortran_lib_paths:\$LD_LIBRARY_PATH\"   " >> setup.sh
      echo "fi                                                                   " >> setup.sh
    fi
    echo "                                                                       " >> setup.sh
    mpich_install_path=`"${build_script}" -P mpich`
    if [[ -x "$mpich_install_path/bin/mpif90" ]]; then
      echo "if [[ -z \"\$PATH\" ]]; then                                         " >> setup.sh
      echo "  export PATH=\"$mpich_install_path/bin\"                            " >> setup.sh
      echo "else                                                                 " >> setup.sh
      echo "  export PATH=\"$mpich_install_path/bin\":\$PATH                     " >> setup.sh
      echo "fi                                                                   " >> setup.sh
    fi
    cmake_install_path=`"${build_script}" -P cmake`
    if [[ -x "$cmake_install_path/bin/cmake" ]]; then
      echo "if [[ -z \"\$PATH\" ]]; then                                         " >> setup.sh
      echo "  export PATH=\"$cmake_install_path/bin\"                            " >> setup.sh
      echo "else                                                                 " >> setup.sh
      echo "  export PATH=\"$cmake_install_path/bin\":\$PATH                     " >> setup.sh
      echo "fi                                                                   " >> setup.sh
    fi
    flex_install_path=`"${build_script}" -P flex`
    if [[ -x "$flex_install_path/bin/flex" ]]; then
      echo "if [[ -z \"\$PATH\" ]]; then                                         " >> setup.sh
      echo "  export PATH=\"$flex_install_path/bin\"                             " >> setup.sh
      echo "else                                                                 " >> setup.sh
      echo "  export PATH=\"$flex_install_path/bin\":\$PATH                      " >> setup.sh
      echo "fi                                                                   " >> setup.sh
    fi
    bison_install_path=`"${build_script}" -P bison`
    if [[ -x "$bison_install_path/bin/yacc" ]]; then
      echo "if [[ -z \"\$PATH\" ]]; then                                         " >> setup.sh
      echo "  export PATH=\"$bison_install_path/bin\"                            " >> setup.sh
      echo "else                                                                 " >> setup.sh
      echo "  export PATH=\"$bison_install_path/bin\":\$PATH                     " >> setup.sh
      echo "fi                                                                   " >> setup.sh
    fi
    m4_install_path=`"${build_script}" -P m4`
    if [[ -x "$m4_install_path/bin/m4" ]]; then
      echo "if [[ -z \"\$PATH\" ]]; then                                         " >> setup.sh
      echo "  export PATH=\"$m4_install_path/bin\"                               " >> setup.sh
      echo "else                                                                 " >> setup.sh
      echo "  export PATH=\"$m4_install_path/bin\":\$PATH                        " >> setup.sh
      echo "fi                                                                   " >> setup.sh
    fi
    opencoarrays_install_path="${install_path}"
    if [[ -x "$opencoarrays_install_path/bin/caf" ]]; then
      echo "if [[ -z \"\$PATH\" ]]; then                                         " >> setup.sh
      echo "  export PATH=\"$opencoarrays_install_path/bin\"                     " >> setup.sh
      echo "else                                                                 " >> setup.sh
      echo "  export PATH=\"$opencoarrays_install_path/bin\":\$PATH              " >> setup.sh
      echo "fi                                                                   " >> setup.sh
    fi
    if ${SUDO:-} mv setup.sh $opencoarrays_install_path; then
       setup_sh_location=$opencoarrays_install_path
    else
       setup_sh_location=${PWD}
    fi
    echo "*** Before using caf, cafrun, or build, please execute the following command ***"
    echo "*** or add it to your login script and launch a new shell (or the equivalent ***"
    echo "*** for your shell if you are not using a bash shell):                       ***"
    echo ""
    echo " source $setup_sh_location/setup.sh"
    echo ""
    echo "*** Installation complete.                                                   ***"

  else # Installation failed

    echo "Something went wrong. Either the user lacks executable permissions for the"
    echo "OpenCoarrays compiler wrapper (caf), program launcher (cafrun), or prerequisite"
    echo "package installer (build), or these programs are not in the following, expected"
    echo "location:"
    echo "$install_path/bin."
    echo "Please review the following file for more information:"
    echo "$install_path/$installation_record"
    echo "and submit an bug report at https://github.com/sourceryinstitute/opencoarrays/issues"
    echo "[exit 100]"
    exit 100

  fi # Ending check for caf, cafrun, build not in expected path
}
