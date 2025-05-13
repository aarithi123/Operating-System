#!/usr/bin/env sh

if [ -z "$1" ]
then
    echo -e "Usage: ./manfile_checker.sh name_of_file\n"
    echo -e "Creates name_of_file_text.txt and name_of_file_errors.txt\n"
    echo -e "Prints number of errors and number of words in plaintext output\n"
    echo -e "View text output using cat or less to see formatting"
else
    manfile=$1
    if [ -e "$(readlink -f ${manfile})" ]
    then
        basename_txt="${manfile%.*}"
        txt_output="${basename_txt}_text.txt"
        err_output="${basename_txt}_errors.txt"
        errors=$(groff -man -Tascii -ww -Wbreak "${manfile}" 2>&1 > "./${txt_output}" | grep -v "grotty")
        echo "$errors" > "./${err_output}"
        echo "Number of errors: $(wc -l ${err_output})"
        echo "Number of words: $(wc -w ${txt_output})"
    else
        echo "${manfile} does not exist."
    fi
fi
