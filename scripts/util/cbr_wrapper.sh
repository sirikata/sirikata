#!/bin/sh

# CBR Launcher
# Adapted from Chromium launcher

# Authors:
#  Ewen Cheslack-Postava
#  Fabien Tassin <fta@sofaraway.org>
# License: GPLv2 or later

DIR=`pwd`
APPDIR=../build/cmake
APPNAME=cbr
GDB=/usr/bin/gdb

usage () {
  echo "$APPNAME [cbr|oh|cseg|analysis] [-h|--help] [-g|--debug] [options]"
  echo
  echo "        -g or --debug           Start within $GDB"
  echo "        -h or --help            This help screen"
  echo
}

#if [ -f /etc/$APPNAME/default ] ; then
#  . /etc/$APPNAME/default
#fi

export CBR_WRAPPER=true
want_valgrind=0
want_debug=0
want_interactive=0
while [ $# -gt 0 ]; do
  case "$1" in
    cbr | oh | cseg | analysis )
      APPNAME="$1"
      shift;;
    -h | --help | -help )
      usage
      exit 0 ;;
    -g | --debug )
      want_debug=1
      shift ;;
    -i | --idebug )
      want_debug=1
      want_interactive=1
      shift ;;
    --valgrind )
      want_valgrind=1
      shift ;;
    -- ) # Stop option prcessing
      shift
      break ;;
    * )
      break ;;
  esac
done

if [ $want_debug -eq 1 ] ; then
  if [ ! -x $GDB ] ; then
    echo "Sorry, can't find usable $GDB. Please install it."
    exit 1
  fi
  tmpfile=`mktemp /tmp/cbrargs.XXXXXX` || { echo "Cannot create temporary file" >&2; exit 1; }
  trap " [ -f \"$tmpfile\" ] && /bin/rm -f -- \"$tmpfile\"" 0 1 2 3 13 15

  # chromium was doing this, but it doesn't preserve quotes properly:
  #echo "set args ${1+"$@"}" > $tmpfile
  # instead, we build ours manually:
  args=""
  while [ $# -gt 0 ]; do
      case "$1" in
          * ) # Stop option prcessing
              args="$args \"$1\""
              shift ;;
      esac
  done
  echo "set args $args" > $tmpfile
  echo "run" >> $tmpfile
  if [ $want_interactive -ne 1 ] ; then
    echo "bt" >> $tmpfile
    #echo "thread apply all bt full no registers" >> $tmpfile
    echo "quit" >> $tmpfile
  fi
  echo "$GDB $APPDIR/$APPNAME -x $tmpfile -quiet"
  $GDB "$APPDIR/$APPNAME" -x $tmpfile -quiet
  exit $?
else
  if [ $want_valgrind -eq 1 ] ; then
    export LD_LIBRARY_PATH=/home/meru/usr/lib:$LD_LIBRARY_PATH
    export PATH=/home/meru/usr/bin:$PATH
    exec valgrind --error-limit=no --num-callers=12 $APPDIR/$APPNAME "$@"
  else
    exec $APPDIR/$APPNAME "$@"
  fi
fi
