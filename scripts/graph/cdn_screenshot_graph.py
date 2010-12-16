import sys
import os
import pylab as plt

base_dir = sys.argv[1]
tests = os.listdir(base_dir)

plt.figure(1, figsize=(12,9), dpi=100)
plt.title('Perceptual Error Over Bytes Downloaded')
plt.xlabel('Megabytes Downloaded')
plt.ylabel('Delta E CIE2000 Sum')

labels = []

for test in tests:
    print test
    labels.append(test)
    
    images = os.listdir(os.path.join(base_dir, test, 'images'))
    images.sort()
    
    results_path = os.path.join(base_dir, test, 'results.txt')
    results_file = open(results_path, 'r')
    
    byte_counts = []
    scores = []
    total_bytes = 0
    
    i=0
    for line in results_file:
        tokens = images[i].split('_')
        bytes = int(tokens[1])
        score = float(line)
        total_bytes += bytes
        i+=1
        print i, float(total_bytes) / 1024 / 1024, score
        byte_counts.append(float(total_bytes) / 1024 / 1024)
        scores.append(score)
        
    plt.plot(byte_counts, scores, 'x-')

plt.legend(labels)
plt.savefig('perceptual_error_over_bytes_downloaded.pdf')
