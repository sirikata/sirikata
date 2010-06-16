##!/usr/bin/python

# time.py - Utilities for times and durations

# Converts a string containing a time, of the form 123u (microseconds),
# 123m (milliseconds) or 123 (seconds) to an int # of microseconds
def string_to_microseconds(str_time):
    if len(str_time) == 0:
        return 0

    # If we don't have an s at the end our only chance is to assume its
    # raw seconds
    last = str_time[-1:]
    if last != 's':
        return int( float(str_time) * 1000000 )

    # Otherwise, strip the s and check second to last
    str_time = str_time[:-1]
    last = str_time[-1:]

    if last == 'u':
        return int( float(str_time[:-1]) )
    elif last == 'm':
        return int( float(str_time[:-1]) * 1000 )
    else:
        return int( float(str_time) * 1000000 )
