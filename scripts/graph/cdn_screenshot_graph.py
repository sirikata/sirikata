import sys
import pylab as plt

logfile = sys.argv[1]
f = open(logfile, 'r')
total_bytes = 0

byte_counts = []
scores = []

for line in f:
    tokens = line.split(' ')
    index = int(tokens[0])
    byte = int(tokens[1])
    score = float(tokens[2])
    total_bytes += byte
    byte_counts.append(total_bytes / 1024 / 1024)
    scores.append(score)
    
plt.figure(1, figsize=(12,9), dpi=100)
plt.title('Perceptual Error Over Bytes Downloaded')
plt.xlabel('Megabytes Downloaded')
plt.ylabel('Delta E CIE2000 Sum')
plt.plot(byte_counts, scores, 'x-')
plt.savefig('perceptual_error_over_bytes_downloaded.pdf')