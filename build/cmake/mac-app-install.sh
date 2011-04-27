#!/bin/bash
# Based on Berkelium's mac-install.sh. See https://github.com/sirikata/berkelium/blob/chromium8/util/mac-install.sh.

binname="$1"
outdir="$2"
debug="$3"
[ x"$CHROME_ROOT" = x ] && CHROME_ROOT="../../externals/berkelium/chromium/chromium"

if [[ x = x"$outdir" || x = x"$binname" ]]; then
    echo "Usage: $0 bin-name output-directory [debug]"
    echo
    echo "Copies just the necessary files for a mac app into a single directory."
    echo
    echo "Warnings:"
    echo "- Run from the sirikata/build/cmake dir. Assumes Release chromium build in ./$CHROME_ROOT."
    echo "  (Change the location of berkelium by exporting CHROME_ROOT)"
    echo "- Do not run as root."
    echo "- Requires that the directory you compiled in is at least 23 characters long:"
    echo "  (e.g. /Users/alfred/berkelium but not /Users/al/berkelium)"
    echo "  This is because we replace strings in the executable files."
    exit 1
fi

chromiumappdir="$CHROME_ROOT/src/xcodebuild/Release/Chromium.app"
if [ \! -d "$chromiumappdir" ]; then
    echo "You need to build chromium first!"
    echo "If done, $chromiumappdir should exist."
    exit 1
fi

lib="liblibberkelium.dylib"
if [ xdebug = x"$debug" ]; then
  lib="liblibberkelium_d.dylib"
  binname="$binname"_d
fi

binoutdir="$outdir/bin"
liboutdir="$outdir/lib"
appname="$binname.app"
appoutdir="$binoutdir/$appname"
echo "Installing into $binoutdir"

mkdir "$outdir" || echo "Note: installing into unclean directory! You may end up with unused files."
mkdir -p "$binoutdir"
mkdir -p "$liboutdir/Chromium Framework.framework"
for versionsrc in "$CHROME_ROOT/src/xcodebuild/Release/Chromium.app/Contents/Versions"/*; do
    versionnum=$(basename "$versionsrc")
done
mkdir -p "$liboutdir/Chromium Framework.framework/Libraries"
cp "$versionsrc/Chromium Framework.framework/Libraries/libffmpegsumo.dylib" "$liboutdir/Chromium Framework.framework/Libraries/"
mkdir -p "$liboutdir/Chromium Framework.framework/Resources/en.lproj"
for copyfile in chrome.pak common.sb nacl_loader.sb renderer.sb utility.sb worker.sb en.lproj/locale.pak; do
    cp "$versionsrc/Chromium Framework.framework/Resources/$copyfile" "$liboutdir/Chromium Framework.framework/Resources/$copyfile"
done

# Link these in place to make the other loops simple (all files in the same dir)
for exe in berkelium "$lib" bin_replace; do
  cp "chrome/$exe" "$exe"
done

for exe in berkelium libplugin_carbon_interpose.dylib "$binname"; do
    echo "Installing executable $exe"
    ./bin_replace "$PWD/" '@executable_path/../lib/' < "$exe" > "$binoutdir/$exe"
    chmod +x "$binoutdir/$exe"
done

echo "Installing shared library $lib ..."
./bin_replace "$PWD/$lib" '@executable_path/../lib/'"$lib" < "$lib" | \
    ./bin_replace "$PWD/" 'Berkelium/' > "$liboutdir/$lib"
chmod +x "$liboutdir/$lib"

echo "Installing $appname"
mkdir -p "$appoutdir/Contents/MacOS"
ln -fs '../../../lib' "$appoutdir/Contents/lib"
ln -fs '../../../Frameworks' "$appoutdir/Contents/Frameworks"
for exe in berkelium libplugin_carbon_interpose.dylib "$binname"; do
    cp "$binoutdir/$exe" "$appoutdir/Contents/MacOS/$exe"
done
sh ../../externals/berkelium/util/make-info-plist.sh "$CHROME_ROOT" "$appoutdir" "$binname" "org.berkelium.$binname" &&
chmod -R a+rX "$outdir"

echo "Done!"
