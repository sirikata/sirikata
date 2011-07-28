#!/usr/bin/python

from csvConstructorInfo import * 



KillAfterTenSecondsEntity = CSVConstructorInfo(script_type=stringWrap("js"),
                                               script_contents=stringWrap("system.import('unitTests/emTests/basicPresence.em');"));


DefaultAndKillAfterTenSecondsEntity = CSVConstructorInfo(script_type=stringWrap("js"),
                                      script_contents=stringWrap("system.import('std/default.em'); system.import('unitTests/emTests/basicPresence.em');"));


DefaultEntity = CSVConstructorInfo();



