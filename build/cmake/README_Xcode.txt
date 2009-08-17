Regenerating Xcode project files using cmake v2.6
=================================================

Steps are:

   1. cd build/cmake
   2. ln -s ../../dependencies/Frameworks Frameworks
         1. to fix brittle paths in Xcode project.
   3. cmake -G Xcode .
   4. launch Xcode
   5. open Sirikata.xcodeproj
   6. edit project settings
         1. general tab
         2. choose project root
         3. select build/cmake directory. click ok. value should just be <Project File Directory>.
               1. This fixes problem that prevents debugger from working.
   7. project menu
         1. select active target ALL_BUILD
         2. select active project executable cppoh
         3. select active build configuration Debug
   8. build it
   9. Debugger
         1. Confirm that you can set a break point.
         2. Run debugger
         3. Confirm that it stops at a break point.
         4. Confirm that it displays source code.
         5. (trouble shoot back to step 6)

