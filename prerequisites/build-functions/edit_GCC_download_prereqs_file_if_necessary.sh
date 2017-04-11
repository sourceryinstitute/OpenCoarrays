
# shellcheck disable=SC2154

replace_wget()
{
  # Define a file extension for the download_prerequisites backup
  backup_extension=".original"
  backup_file="${download_prereqs_file}${backup_extension}"
  if [[ -f ${backup_file}  ]]; then
    # Prevent overwriting an existing backup:
    backup_extension=""
  fi

  # Modify download_prerequisites if wget is unavailable
  if type wget &> /dev/null; then
    info "wget available. Leaving wget invocations unmodified in the GCC script contrib/download_prerequisites."
  else 
    info "wget unavailable. Editing GCC contrib/download_prerequisites to replace it with ${gcc_prereqs_fetch}"
  
    if [[ -z "${wget_line:-}"  ]]; then

      # Check whether a backup file already exists
      if [[ ! -f "${backup_file}" ]]; then
        emergency "replace_wget: gcc contrib/download_prerequisites does not use wget"
      fi

    else # Download_prerequisites contains wget so we haven't modified it
      
      case "${gcc_prereqs_fetch}" in
        "ftp_url")
          # Insert a new line after line 2 to include ftp_url.sh as a download option
          sed -i${backup_extension} -e '2 a\'$'\n'". ${OPENCOARRAYS_SRC_DIR}/prerequisites/build-functions/ftp_url.sh"$'\n' "${download_prereqs_file}"
          wget_command='wget --no-verbose -O "${directory}\/${ar}"'
        ;;
        "curl")
          wget_command="${wget_line%%\"\$\{directory\}*}" # grab everything before "${base_url}
          wget_command="wget${wget_command#*wget}" # keep everything from wget forward
        ;;
        *)
          emergency "Unknown download program ${gcc_prereqs_fetch} in edit_GCC_download_prereqs_file_if_necessary.sh"
        ;;
      esac

      arg_string="${gcc_prereqs_fetch_args[@]:-} "

      info "Using the following command to replace wget in the GCC download_prerequisites file:"
      info "sed -i${backup_extension} s/\"${wget_command}\"/\"${gcc_prereqs_fetch} ${arg_string} \"/ \"${download_prereqs_file}\""
      if [[ "${__operating_system}" == "Linux" ]]; then
        sed -i"${backup_extension}" s/"${wget_command}"/"${gcc_prereqs_fetch} ${arg_string} "/ "${download_prereqs_file}"
      else
        sed -i "${backup_extension}" s/"${wget_command}"/"${gcc_prereqs_fetch} ${arg_string} "/ "${download_prereqs_file}"
      fi

    fi # end if [[ -z "${wget_line:-}"  ]]; then
  
  fi # end if ! type wget &> /dev/null; 

}

edit_GCC_download_prereqs_file_if_necessary()
{
  __operating_system=$(uname)
  download_prereqs_file="${PWD}/contrib/download_prerequisites"

  # Grab the line with the first occurence of 'wget'
  wget_line=`grep wget "${download_prereqs_file}" | head -1` || true

  # Check for wget format used before GCC 7
  if [[ "${wget_line}" == *"ftp"* ]]; then
    replace_wget
  fi

}
