import os
import re
import csv

def read_dat_files(folder_path):
    data = []
    # Regex to extract T and M values from filenames
    filename_regex = r"T(\d+)a_M(\d+)\.dat"

    for filename in os.listdir(folder_path):
        match = re.match(filename_regex, filename)
        if match:
            T, M = match.groups()
            with open(os.path.join(folder_path, filename), 'r') as file:
                lines = file.readlines()
                avg_time = sum(float(line.strip()) for line in lines) / len(lines)
                data.append((int(T), int(M), avg_time))

    data.sort(key=lambda x: (x[0], x[1]))
    return data

def write_to_csv(data, output_file):
    with open(output_file, 'w', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(['T', 'M', 'Time'])
        for row in data:
            writer.writerow(row)

folder_path = 'runs/'
output_file = 'lab4_ecetesla1.csv' 

data = read_dat_files(folder_path)
write_to_csv(data, output_file)