#!/bin/sh
ECHO=/bin/echo
MKTEMP=/bin/mktemp
TAR=/bin/tar

REALPATH=/usr/bin/realpath
BASENAME=/usr/bin/basename
MAKE=/usr/bin/make
GIT=/usr/bin/git

REPTILIAN_DIR=/usr/rep/src/reptilian-kernel
PATCHNAME="p1.diff"
ARCHIVENAME="process_log.tar.gz"
SOURCE_DIR="process_log"
PATCHFILE=`$REALPATH $1`
ARCHIVEFILE=`$REALPATH $2`
ORIGIN=$PWD

$ECHO "****************************************************"
$ECHO "This script detects packaging problems but does not"
$ECHO "test functionality! Be vigilant."
if [ `basename $PATCHFILE` != $PATCHNAME ]; then
  $ECHO
  $ECHO "WARNING: Patch file has incorrect name!"
fi

if [ `basename $ARCHIVEFILE` != $ARCHIVENAME ]; then
  $ECHO
  $ECHO "WARNING: Archive file has incorrect name!"
fi
$ECHO "****************************************************"

cd $REPTILIAN_DIR
$ECHO
$ECHO
$ECHO "****************** APPLYING PATCH ******************"
$GIT apply $PATCHFILE
$ECHO "****************************************************"

TEMP_DIR=`mktemp -d`
cd $TEMP_DIR
$ECHO
$ECHO
$ECHO "******** DECOMPRESSING AND BUILDING ARCHIVE ********"
$TAR zxvf $ARCHIVEFILE
cd $SOURCE_DIR
$MAKE
$ECHO "****************************************************"
