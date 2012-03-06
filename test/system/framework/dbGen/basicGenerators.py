#!/usr/bin/python

from csvConstructorInfo import * 



KillAfterTenSecondsEntity = CSVConstructorInfo(script_type="js",
                                               script_contents="system.import('basicPresence.em');");


DefaultAndKillAfterTenSecondsEntity = CSVConstructorInfo(script_type="js",
                                      script_contents="system.import('std/default.em'); system.import('basicPresence.em');");


DefaultEntity = CSVConstructorInfo();
