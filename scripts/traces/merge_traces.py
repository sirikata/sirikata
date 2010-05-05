#!/usr/bin/python

# Converts a bunch of processedTrace files from Second Life into a
# single trace file, where all coordinates have been converted
# properly.  This assumes that the traces are laid out in a square
# grid, that each server is 256m x 256m.


if __name__ == "__main__":
    trace_file_fmt = '4x4/processedTrace_%d'
    servers_per_side = 4
    server_width = 256
    combined_file = 'sl.trace.4x4.dat'

    objects = set()
    max_rad = 0
    min_rad = 10000
    fout = file(combined_file, 'w')
    for sy in range(servers_per_side):
        for sx in range(servers_per_side):
            idx = sy * servers_per_side + sx
            trace_file = trace_file_fmt % (idx)

            for line in file(trace_file):
                (uuid, colon, info) = line.partition(':')
                parts = info.split(',')
                x = float(parts[0].strip()) + (sx * 256.0)
                y = float(parts[1].strip()) + (sy * 256.0)
                z = float(parts[2].strip())
                t = int(parts[3].strip()) # milliseconds
                rad = float(parts[4].strip())

                objects.add(uuid)
                max_rad = max(max_rad, rad)
                min_rad = min(min_rad, rad)
                print >>fout, uuid, x, y, z, t, rad

    fout.close()

    print "Unique objects:", len(objects)
    print "Radii:", max_rad, "(max)", min_rad, "(min)"
