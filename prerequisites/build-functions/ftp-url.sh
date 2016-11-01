# Download a file from an anonymous ftp site
#
# Usage:
#    ftp-url  <ftp-mode>  ftp://<fully-qualified-domain>:/<path-to-file>/<file-name>
#
# Example:
#    ftp-url -n ftp://ftp.gnu.org:/gnu/gcc/gcc-6.1.0/gcc-6.1.0.tar.bz2
ftp-url()
{
  ftp_mode="${1}"
  url="${2}"

  if [[ "${ftp_mode}" != "-n" ]]; then
    emergency "Unexpected ftp mode received by ftp-url.sh: ${ftp_mode}"
  fi

  protocol="${url%%:*}" # grab text_before_first_colon
  if [[ "${protocol}" != "ftp" ]]; then
    emergency "URL with unexpected protocol received by ftp-url.sh: ${text_before_first_colon}"
  fi

  text_after_double_slash="${url##*//}"
  FTP_SERVER="${text_after_double_slash%:*}" # grab remaining text before colon
  
  text_after_final_colon="${url##*:}"
  FILE_NAME="${url##*/}" # grab text after final slash
  FILE_PATH="${text_after_final_colon%/*}" # grab remaining text before final slash

  USERNAME=anonymous
  PASSWORD=""
  info "starting anonymous download: ${protocol} ${ftp_mode} ${FTP_SERVER}... cd ${FILE_PATH}... get ${FILE_NAME}"

ftp "${ftp_mode}" "${FTP_SERVER}" <<Done-ftp
user "${USERNAME}" "${PASSWORD}"
cd "${FILE_PATH}"
passive
binary
get "${FILE_NAME}"
bye
Done-ftp

info "finished anonymous ftp"
}
