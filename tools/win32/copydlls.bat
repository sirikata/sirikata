set DEBDEST=build\cmake\Debug\
set RELDEST=build\cmake\RelWithDebInfo\

copy dependencies\boost_1_44_0\lib\*.dll %DEBDEST%
copy dependencies\boost_1_44_0\lib\*.dll %RELDEST%

copy dependencies\berkelium\bin\*.* %DEBDEST%
copy dependencies\berkelium\bin\*.* %RELDEST%
mkdir %DEBDEST%\locales
mkdir %RELDEST%\locales
copy dependencies\berkelium\bin\locales\*.* %DEBDEST%\locales\
copy dependencies\berkelium\bin\locales\*.* %RELDEST%\locales\

copy dependencies\libantlr3c-3.1.3\bin\*.* %DEBDEST%
copy dependencies\libantlr3c-3.1.3\bin\*.* %RELDEST%

copy dependencies\bullet-2.74\bin\*.* %DEBDEST%
copy dependencies\bullet-2.74\bin\*.* %RELDEST%
copy dependencies\ogre-1.7.2\bin\debug\*.* %DEBDEST%
copy dependencies\ogre-1.7.2\bin\release\*.* %RELDEST%

mkdir %DEBDEST%

copy dependencies\opencollada\Externals\expat\win32\bin\Debug\*.dll %DEBDEST%
copy dependencies\opencollada\Externals\expat\win32\bin\Release\*.dll %RELDEST%

copy dependencies\SDL-1.3.0\bin\*.dll %DEBDEST%
copy dependencies\SDL-1.3.0\bin\*.dll %RELDEST%

copy dependencies\installed-sqlite\lib\sqlite3_d.dll %DEBDEST%
copy dependencies\installed-sqlite\lib\sqlite3.dll %RELDEST%

copy dependencies\ois-1.0\bin\*.dll %DEBDEST%
copy dependencies\ois-1.0\bin\*.dll %RELDEST%
copy dependencies\protobufs\bin\*.dll %DEBDEST%
copy dependencies\protobufs\bin\*.dll %RELDEST%

copy dependencies\installed-gsl\bin\*.dll %DEBDEST%
copy dependencies\installed-gsl\bin\*.dll %RELDEST%

copy dependencies\FreeImage\*.dll %DEBDEST%
copy dependencies\FreeImage\*.dll %RELDEST%

copy dependencies\installed-ffmpeg\bin\*.dll %DEBDEST%
copy dependencies\installed-ffmpeg\bin\*.dll %RELDEST%

mkdir %DEBDEST%\chrome
mkdir %DEBDEST%\chrome\img
mkdir %DEBDEST%\chrome\css
mkdir %DEBDEST%\chrome\js
mkdir %RELDEST%\chrome
mkdir %RELDEST%\chrome\img
mkdir %RELDEST%\chrome\css
mkdir %RELDEST%\chrome\js
copy liboh\plugins\ogre\data\chrome\*.* %DEBDEST%\chrome
copy liboh\plugins\ogre\data\chrome\*.* %RELDEST%\chrome
copy liboh\plugins\ogre\data\chrome\ui\*.* %DEBDEST%\chrome\ui
copy liboh\plugins\ogre\data\chrome\ui\*.* %RELDEST%\chrome\ui
copy liboh\plugins\ogre\data\chrome\img\*.* %DEBDEST%\chrome\img
copy liboh\plugins\ogre\data\chrome\img\*.* %RELDEST%\chrome\img
copy liboh\plugins\ogre\data\chrome\js\*.* %DEBDEST%\chrome\js
copy liboh\plugins\ogre\data\chrome\js\*.* %RELDEST%\chrome\js
