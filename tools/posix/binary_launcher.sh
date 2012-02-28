#!/bin/bash

# Sirikata launcher. This is a simple wrapper for Sirikata binaries that sets up
# the environment so libraries and plugin can be found, and you can run it with
# --debug to also get things running in a debugger.

# Path to this script, should be under prefix/bin/
SELF_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
# And from that, the path to prefix/
BASE_PATH=`dirname $SELF_PATH`
# From that we can determine the library path
LIBS_PATH=$BASE_PATH/lib
SIRIKATA_LIBS_PATH=$BASE_PATH/lib/sirikata
# And the path to use for the binary
BINARY_PATH=$SIRIKATA_LIBS_PATH/@BINARY_EXE@

GDB=/usr/bin/gdb

# Set up linker paths for libraries
if [ "x$@LD_LIBRARY_PATH_NAME@" != "x" ]; then
    @LD_LIBRARY_PATH_NAME@=@LD_LIBRARY_PATH_NAME@:$SIRIKATA_LIBS_PATH:$LIBS_PATH
else
    @LD_LIBRARY_PATH_NAME@=$SIRIKATA_LIBS_PATH:$LIBS_PATH
fi
export @LD_LIBRARY_PATH_NAME@

# Options
want_debug=0
while [ $# -gt 0 ]; do
  case "$1" in
    -h | --help | -help )
      usage
      exit 0 ;;
    -g | --debug )
      want_debug=1
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
    tmpfile=`mktemp /tmp/@BINARY_NAME@args.XXXXXX` || { echo "Cannot create temporary file" >&2; exit 1; }
    trap " [ -f \"$tmpfile\" ] && /bin/rm -f -- \"$tmpfile\"" 0 1 2 3 13 15
    echo "set args ${1+"$@"}" > $tmpfile
    echo "# Env:"
    echo "# @LD_LIBRARY_PATH_NAME@=$@LD_LIBRARY_PATH_NAME@"
    echo "#                PATH=$PATH"
    echo "$GDB $BINARY_PATH -x $tmpfile"
    $GDB "$BINARY_PATH" -x $tmpfile
  exit $?
else
    exec $BINARY_PATH "$@"
fi