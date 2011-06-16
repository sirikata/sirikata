import csv
import sys
import os.path
import math

def median(lst):
    return (lst[len(lst)/2] + lst[int(float(len(lst))/2+0.5)]) / 2.0

def mean(lst):
    return float(sum(lst)) / len(lst)

def humanize_bytes(bytes, precision=1):
    abbrevs = (
        (1<<50L, 'PB'),
        (1<<40L, 'TB'),
        (1<<30L, 'GB'),
        (1<<20L, 'MB'),
        (1<<10L, 'KB'),
        (1, 'bytes')
    )
    if bytes == 1:
        return '1 byte'
    for factor, suffix in abbrevs:
        if math.fabs(bytes) >= factor:
            break
    return '%.*f %s' % (precision, bytes / factor, suffix)

def make_plus(s):
    s = str(s)
    if s[0] != '-':
        s = '+' + s
    return s

def format_percent(p):
    val = make_plus("%.1f" % (p * 100))
    return '(%s%%)' % val

def compare_two(name, orig, opt, op=str):
    print name
    print '=' * len(name)
    
    diff = [a-b for a,b in zip(opt,orig)]
    percents = [float(a)/b if b!=0 else 0 for a,b in zip(diff,orig)]
    percents = sorted(percents)
    diff = sorted(diff)
    
    print 'MEAN:', make_plus(op(mean(diff))), format_percent(mean(percents))
    print 'MEDIAN:', make_plus(op(median(diff))), format_percent(median(percents))
    print 'MIN:', make_plus(op(min(diff))), format_percent(min(percents))
    print 'MAX:', make_plus(op(max(diff))), format_percent(max(percents))
    print

def main():
    if len(sys.argv) != 2 or not os.path.isfile(sys.argv[1]):
        print 'usage: python meshtool_analyze.py results.csv'
        sys.exit(1)
        
    csvreader = csv.DictReader(open(sys.argv[1], 'r'), delimiter='\t')
    
    fields = {}
    for field in csvreader.fieldnames:
        fields[field] = []
    
    for row in csvreader:
        for field, value in row.iteritems():
            fields[field].append(value)
            
    for field in ['orig_size',
                  'orig_num_triangles',
                  'orig_num_draw_raw',
                  'orig_num_draw_with_instances',
                  'orig_num_draw_with_batching',
                  'orig_texture_ram',
                  'opt_size',
                  'opt_num_triangles',
                  'opt_num_draw_raw',
                  'opt_num_draw_with_instances',
                  'opt_num_draw_with_batching',
                  'opt_texture_ram']:
        fields[field] = map(int, fields[field])
            
    print 'Number of files compared:', len(fields['orig_size'])
    print
    compare_two('Zipped File Size', fields['orig_size'], fields['opt_size'], op=humanize_bytes)
    compare_two('Number of Triangles', fields['orig_num_triangles'], fields['opt_num_triangles'], op=int)
    compare_two('Raw Number of Draw Calls', fields['orig_num_draw_raw'], fields['opt_num_draw_raw'], op=int)
    compare_two('Number of Draw Calls with Instance Batching', fields['orig_num_draw_with_instances'], fields['opt_num_draw_with_instances'], op=int)
    compare_two('Number of Draw Calls with Instance and Material Batching', fields['orig_num_draw_with_batching'], fields['opt_num_draw_with_batching'], op=int)
    compare_two('Texture RAM', fields['orig_texture_ram'], fields['opt_texture_ram'], op=humanize_bytes)

if __name__ == '__main__':
    main()