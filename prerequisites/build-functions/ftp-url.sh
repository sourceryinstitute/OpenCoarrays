# Download a file from an anonymous ftp site 
#
# Usage:
#    ftp-url  <ftp-mode>  <ftp-site-address>:/<path-to-file>/<file-name>
#
# Example:  
#    ftp-url -n ftp.gnu.org:/gnu/m4/m4-1.4.17.tar.bz2
ftp-url()
{
  ftp_mode="${1}"
  url="${2}"

  text_before_colon="${url%%:*}"
  FTP_SERVER="${text_before_colon}"

  text_after_colon="${url##*:}"
  text_after_final_slash="${text_after_colon##*/}"
  FILE_NAME="${text_after_final_slash}"

  text_before_final_slash="${text_after_colon%/*}"
  FILE_PATH="${text_before_final_slash}"

  USERNAME=anonymous
  PASSWORD=""
  info "starting anonymous download: ftp ${ftp_mode} ${FTP_SERVER}... cd ${FILE_PATH}... get ${FILE_NAME}"

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
