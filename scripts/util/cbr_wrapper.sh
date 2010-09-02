#!/bin/sh

# CBR Launcher
# Adapted from Chromium launcher

# Authors:
#  Ewen Cheslack-Postava
#  Fabien Tassin <fta@sofaraway.org>
# License: GPLv2 or later

DIR=`pwd`
APPOFFSET=build/cmake
APPNAME=space
GDB=/usr/bin/gdb

APPDIR=""
script_dir=`dirname $0`
# Search up to 3 directories higher for the app
for reldir in . .. ../.. ../../.. ; do
    if [ -f ${reldir}/${APPOFFSET}/${APPNAME} ] ; then
        APPDIR=${reldir}/${APPOFFSET}
        break
    fi
    if [ -f ${reldir}/${APPOFFSET}/${APPNAME}_d ] ; then
        APPDIR=${reldir}/${APPOFFSET}
        break
    fi

    if [ -f ${script_dir}/${reldir}/${APPOFFSET}/${APPNAME} ] ; then
        APPDIR=${script_dir}/${reldir}/${APPOFFSET}
        break
    fi
    if [ -f ${script_dir}/${reldir}/${APPOFFSET}/${APPNAME}_d ] ; then
        APPDIR=${script_dir}/${reldir}/${APPOFFSET}
        break
    fi
done
if [ -z "${APPDIR}" ] ; then
    echo "Couldn't find binary."
    echo "Trying to run ${0+"$@"}"
    exit 1
fi

BUILD_POST="_d"
build_config=`grep BUILD_TYPE ${APPDIR}/CMakeCache.txt | grep Debug`
if [ -z "${build_config}" ] ; then
    BUILD_POST=""
fi

usage () {
  echo "[space|simoh|genpack|cseg|pinto|analysis|bench] [-h|--help] [-g|--debug] [options]"
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
want_oprofile=0
while [ $# -gt 0 ]; do
  case "$1" in
    space | simoh | genpack | cseg | pinto | analysis | bench)
      APPNAME="${1}${BUILD_POST}"
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
    --oprofile )
      if [ `which opcontrol` ] ; then
          want_oprofile=1
      fi
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
  echo "set breakpoint pending on" >> $tmpfile
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
    exec valgrind --leak-check=full --error-limit=no --num-callers=12 $APPDIR/$APPNAME "$@"
  else
    # Note that oprofile support will only work properly if you both have
    # oprofile installed and have enabled sudo'ing for opcontrol without
    # a password via your /etc/sudoers file, e.g. by adding the line
    #  meru    ALL = NOPASSWD: /usr/bin/opcontrol
    # to allow the user meru to run opcontrol as root without a password
    if [ $want_oprofile -eq 1 ] ; then
      sudo opcontrol --no-vmlinux --start
      sudo opcontrol --reset
      sudo opcontrol --callgraph=6
    fi
    exec $APPDIR/$APPNAME "$@"
    if [ $want_oprofile -eq 1 ] ; then
      sudo opcontrol --stop
      sudo opcontrol --shutdown
    fi
  fi
fi
