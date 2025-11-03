# FFmpeg Microscope

**Ultra-Precise FFmpeg Bitrate Analysis Tool in Pure C**

FFmpeg Microscope is a lightweight video bitrate analyzer written in pure C. It processes FFprobe CSV output and generates detailed statistical reports. The project has no dependencies outside of the standard C library and math library.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_%28programming_language%29)
[![Standard: C99](https://img.shields.io/badge/Standard-C99-green.svg)](https://en.wikipedia.org/wiki/C99)


## Table of Contents

* [Features](#features)
* [Installation](#installation)

  * [Gentoo Linux](#gentoo-linux)
  * [Arch-based Distros](#arch-based-distros)
  * [Ubuntu/Debian](#ubuntudebian)
* [Generating CSV Data with FFprobe](#generating-csv-data-with-ffprobe)
* [Usage](#usage)
* [Output Example](#output-example)
* [Understanding the Statistics](#understanding-the-statistics)
* [Building from Source](#building-from-source)
* [CSV File Format](#csv-file-format)
* [Contributing](#contributing)
* [License](#license)


## Features

* Comprehensive statistics: mean, median, standard deviation, percentiles, skewness, kurtosis
* Automatic bitrate stability analysis and recommendations
* Fast and memory-efficient, using pure C
* Generates detailed, human-readable reports
* Cross-platform: Linux, macOS, BSD, and other POSIX systems


## Installation

### Prerequisites

* FFmpeg (includes FFprobe)
* C compiler (GCC recommended)

### Gentoo Linux

```bash
sudo emerge --ask media-video/ffmpeg
sudo emerge --ask sys-devel/gcc

git clone https://github.com/bipknit/ffmpeg_microscope.git
cd ffmpeg_microscope
make
sudo make install
```

### Arch-based Distros

```bash
sudo pacman -S ffmpeg base-devel

git clone https://github.com/bipknit/ffmpeg_microscope.git
cd ffmpeg_microscope
make
sudo make install
```

### Ubuntu/Debian

```bash
sudo apt update
sudo apt install ffmpeg build-essential

git clone https://github.com/bipknit/ffmpeg_microscope.git
cd ffmpeg_microscope
make
sudo make install
```

## Generating CSV Data with FFprobe

FFmpeg Microscope requires CSV files in a specific format:

**Required columns:**
`chunk_index,bitrate_per_chunk,frame_count,chunk_size`

### Packet-Level Analysis (Recommended)

```bash
{ echo "chunk_index,bitrate_per_chunk,frame_count,chunk_size"; \
  ffprobe -v error -select_streams v:0 \
    -show_entries packet=pts_time,size \
    -of csv=p=0 input.mp4 | \
  awk -F',' 'NF==2 {bitrate = ($2 * 8) / 1000; print NR-1","bitrate",1,"$2}'; \
} > bitrate_data.csv
```

### Frame-Level Analysis

```bash
{ echo "chunk_index,bitrate_per_chunk,frame_count,chunk_size"; \
  ffprobe -v error -select_streams v:0 \
    -show_entries frame=pkt_pts_time,pkt_size \
    -of csv=p=0 input.mp4 | \
  awk -F',' 'NF==2 {bitrate = ($2 * 8) / 1000; print NR-1","bitrate",1,"$2}'; \
} > bitrate_data.csv
```

### Time-Based Chunks (1-second intervals)

```bash
{ echo "chunk_index,bitrate_per_chunk,frame_count,chunk_size"; \
  ffprobe -v error -select_streams v:0 \
    -show_entries packet=pts_time,size \
    -of csv=p=0 input.mp4 | \
  awk -F',' 'NF==2 {
      second = int($1);
      size[second] += $2;
      count[second]++;
  }
  END {
      for (s in size) {
          bitrate = (size[s] * 8) / 1000;
          print s","bitrate","count[s]","size[s];
      }
  }' | sort -t',' -k1 -n; \
} > bitrate_data.csv
```

Check the first few lines:

```bash
head -5 bitrate_data.csv
```

Expected output:

```
chunk_index,bitrate_per_chunk,frame_count,chunk_size
0,5234.567,1,196293
1,5189.234,1,194595
2,5312.891,1,199230
3,5401.345,1,202549
```

## Usage

```bash
# Analyze a CSV file
bitrate_scope bitrate_data.csv output_report.txt

# View the report
cat output_report.txt
```

### Processing Multiple Videos

```bash
for video in *.mp4; do
    echo "Processing: $video"
    { echo "chunk_index,bitrate_per_chunk,frame_count,chunk_size"; \
      ffprobe -v error -select_streams v:0 \
        -show_entries frame=pkt_pts_time,pkt_size \
        -of csv=p=0 "$video" | \
      awk -F',' '{bitrate = ($2 * 8) / 1000; print NR-1","bitrate",1,"$2}'; \
    } > "${video%.mp4}_bitrate.csv"
    
    bitrate_scope "${video%.mp4}_bitrate.csv" "${video%.mp4}_report.txt"
    echo "Report saved: ${video%.mp4}_report.txt"
done
```

## Output Example

```
================================================================================
         VIDEO BITRATE ANALYSIS REPORT
================================================================================

Input File:  my_video_bitrate.csv
Output File: analysis_report.txt

BITRATE STATISTICS
Maximum: 8234.567 kbps
Minimum: 2134.890 kbps
Average: 5234.123 kbps
Median: 5198.456 kbps
Std Deviation: 890.234 kbps

PERCENTILES
5th percentile: 3456.789 kbps
25th (Q1): 4567.890 kbps
75th (Q3): 5890.123 kbps
95th percentile: 7123.456 kbps
IQR: 1322.233 kbps
Range: 6099.677 kbps

ADVANCED METRICS
Coefficient of Variation: 17.012 %
Skewness: 0.234
Kurtosis: -0.456
Stability Index: 0.830
Peak/Avg Ratio: 1.573

QUALITY ASSESSMENT
Overall Quality: GOOD
Stability: 0.830
Variability: 17.0 %
```


## Understanding the Statistics

* **Maximum/Minimum**: Highest and lowest bitrate
* **Average/Median**: Mean and median
* **Standard Deviation / Variance**: Measure of spread
* **Percentiles / IQR**: Helps identify outliers
* **Coefficient of Variation (CV)**: Relative variability
* **Skewness**: Asymmetry of data
* **Kurtosis**: “Tailedness” of distribution
* **Stability Index**: Custom metric (closer to 1 = more stable)
* **Peak/Avg Ratio**: Measures spikes


## Building from Source

### Manual Build

```bash
git clone https://github.com/bipknit/ffmpeg_microscope.git
cd ffmpeg_microscope

gcc -o bitrate_scope bitrate_analyzer.c -lm -O2 -Wall -Wextra
./bitrate_scope input.csv output.txt
```

### Using Makefile

```bash
make
sudo make install
make clean
make test
make valgrind
```


## CSV File Format

```
chunk_index,bitrate_per_chunk,frame_count,chunk_size
0,5234.5,30,196293
1,5189.2,30,194595
2,5312.8,30,199230
```

Required columns: `chunk_index`, `bitrate_per_chunk`
Optional: `frame_count`, `chunk_size`, `timestamp`


## Contributing

Fork the repository, make changes, and submit a pull request.

Development setup:

```bash
git clone https://github.com/bipknit/ffmpeg_microscope.git
cd ffmpeg_microscope
make debug
make test
make valgrind
make format
```


## License

MIT License - see [LICENSE](https://github.com/bipknit/ffmpeg_microscope/blob/main/LICENSE) for details


## Support

* **Issues**: [GitHub Issues](https://github.com/bipknit/ffmpeg_microscope/issues)
* **Discussions**: [GitHub Discussions](https://github.com/bipknit/ffmpeg_microscope/discussions)


## Roadmap

* JSON output format
* Multiple video stream support
* Graphical output with gnuplot
* Real-time analysis mode
* Audio bitrate analysis
* Windows native build support


**Made in C, focused on clarity and speed**
