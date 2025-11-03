#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MAX_LINE_LENGTH 4096
#define MAX_DATA_POINTS 1000000
#define DECIMAL_PLACES 3
#define TIME_PRECISION 6
#define ROLLING_WINDOW 10

typedef struct {
    int chunk_index;
    double bitrate;
    int frame_count;
    double chunk_size;
    double timestamp;
} DataPoint;

typedef struct {
    double max;
    double min;
    double avg;
    double median;
    double std;
    double variance;
    double q1;
    double q3;
    double q5;
    double q95;
    double iqr;
    double range;
    double cv;
    double skewness;
    double kurtosis;
    int num_chunks;
    double total_duration;
    int total_frames;
    double avg_chunk_size;
    double time_resolution;
    double stability_index;
    double peak_to_avg_ratio;
} Statistics;

typedef struct {
    DataPoint *data;
    int count;
    int capacity;
} Dataset;

/* Function prototypes */
Dataset* create_dataset(int initial_capacity);
void free_dataset(Dataset *ds);
int add_data_point(Dataset *ds, DataPoint point);
int load_csv(const char *filename, Dataset *ds);
void compute_statistics(Dataset *ds, Statistics *stats);
double calculate_percentile(double *sorted_data, int count, double percentile);
void sort_doubles(double *arr, int n);
void calculate_moments(double *data, int count, double mean, Statistics *stats);
void generate_text_report(Dataset *ds, Statistics *stats, const char *output_file, const char *input_file);
void print_summary(const char *csv_file, const char *output_file, Statistics *stats);
char* get_quality_assessment(Statistics *stats);
char* get_recommendations(Statistics *stats);
void log_message(const char *level, const char *message);
void print_separator(FILE *fp, int length, char ch);
void print_newlines(FILE *fp, int count);

/* Dataset management */
Dataset* create_dataset(int initial_capacity) {
    Dataset *ds = (Dataset*)malloc(sizeof(Dataset));
    if (!ds) return NULL;
    
    ds->data = (DataPoint*)malloc(sizeof(DataPoint) * initial_capacity);
    if (!ds->data) {
        free(ds);
        return NULL;
    }
    
    ds->count = 0;
    ds->capacity = initial_capacity;
    return ds;
}

void free_dataset(Dataset *ds) {
    if (ds) {
        if (ds->data) free(ds->data);
        free(ds);
    }
}

int add_data_point(Dataset *ds, DataPoint point) {
    if (ds->count >= ds->capacity) {
        int new_capacity = ds->capacity * 2;
        DataPoint *new_data = (DataPoint*)realloc(ds->data, sizeof(DataPoint) * new_capacity);
        if (!new_data) return 0;
        ds->data = new_data;
        ds->capacity = new_capacity;
    }
    
    ds->data[ds->count++] = point;
    return 1;
}

/* Logging */
void log_message(const char *level, const char *message) {
    time_t now;
    time(&now);
    char *time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0'; // Remove newline
    printf("[%s] %s - %s\n", time_str, level, message);
}

/* Utility functions for formatted output */
void print_separator(FILE *fp, int length, char ch) {
    for (int i = 0; i < length; i++) {
        fputc(ch, fp);
    }
    fputc('\n', fp);
}

void print_newlines(FILE *fp, int count) {
    for (int i = 0; i < count; i++) {
        fputc('\n', fp);
    }
}

/* CSV parsing */
int load_csv(const char *filename, Dataset *ds) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        log_message("ERROR", "Cannot open CSV file");
        return 0;
    }
    
    char line[MAX_LINE_LENGTH];
    int line_num = 0;
    int chunk_idx = -1, bitrate_idx = -1, frame_idx = -1, size_idx = -1, time_idx = -1;
    
    // Read header
    if (fgets(line, sizeof(line), file)) {
        line_num++;
        char *token = strtok(line, ",\n\r");
        int col = 0;
        
        while (token) {
            // Trim whitespace
            while (*token == ' ' || *token == '\t') token++;
            char *end = token + strlen(token) - 1;
            while (end > token && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
                *end = '\0';
                end--;
            }
            
            if (strcmp(token, "chunk_index") == 0) chunk_idx = col;
            else if (strcmp(token, "bitrate_per_chunk") == 0) bitrate_idx = col;
            else if (strcmp(token, "frame_count") == 0) frame_idx = col;
            else if (strcmp(token, "chunk_size") == 0) size_idx = col;
            else if (strcmp(token, "timestamp") == 0) time_idx = col;
            
            token = strtok(NULL, ",\n\r");
            col++;
        }
    }
    
    if (chunk_idx == -1 || bitrate_idx == -1) {
        log_message("ERROR", "Required columns not found in CSV");
        fclose(file);
        return 0;
    }
    
    log_message("INFO", "CSV header validated successfully");
    
    // Read data
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        DataPoint point = {0};
        char *token = strtok(line, ",\n\r");
        int col = 0;
        
        while (token) {
            if (col == chunk_idx) {
                point.chunk_index = atoi(token);
            } else if (col == bitrate_idx) {
                point.bitrate = atof(token);
            } else if (col == frame_idx && frame_idx != -1) {
                point.frame_count = atoi(token);
            } else if (col == size_idx && size_idx != -1) {
                point.chunk_size = atof(token);
            } else if (col == time_idx && time_idx != -1) {
                point.timestamp = atof(token);
            }
            
            token = strtok(NULL, ",\n\r");
            col++;
        }
        
        if (point.bitrate > 0) {
            if (!add_data_point(ds, point)) {
                log_message("ERROR", "Failed to add data point");
                fclose(file);
                return 0;
            }
        }
    }
    
    fclose(file);
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Loaded %d data points from CSV", ds->count);
    log_message("INFO", msg);
    
    return 1;
}

void sort_doubles(double *arr, int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                double temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

double calculate_percentile(double *sorted_data, int count, double percentile) {
    double index = (percentile / 100.0) * (count - 1);
    int lower = (int)floor(index);
    int upper = (int)ceil(index);
    
    if (lower == upper) {
        return sorted_data[lower];
    }
    
    double weight = index - lower;
    return sorted_data[lower] * (1 - weight) + sorted_data[upper] * weight;
}

void calculate_moments(double *data, int count, double mean, Statistics *stats) {
    double sum_sq = 0, sum_cube = 0, sum_quad = 0;
    
    for (int i = 0; i < count; i++) {
        double diff = data[i] - mean;
        double diff_sq = diff * diff;
        sum_sq += diff_sq;
        sum_cube += diff_sq * diff;
        sum_quad += diff_sq * diff_sq;
    }
    
    double variance = sum_sq / (count - 1);
    double std = sqrt(variance);
    
    stats->variance = variance;
    stats->std = std;
    
    if (std > 0) {
        stats->skewness = (sum_cube / count) / pow(std, 3);
        stats->kurtosis = (sum_quad / count) / pow(variance, 2) - 3.0;
    } else {
        stats->skewness = 0;
        stats->kurtosis = 0;
    }
}

void compute_statistics(Dataset *ds, Statistics *stats) {
    if (ds->count == 0) return;
    
    // Allocate array for bitrates
    double *bitrates = (double*)malloc(sizeof(double) * ds->count);
    if (!bitrates) {
        log_message("ERROR", "Memory allocation failed");
        return;
    }
    
    // Extract bitrates and calculate basic stats
    double sum = 0, min = ds->data[0].bitrate, max = ds->data[0].bitrate;
    int total_frames = 0;
    double total_chunk_size = 0;
    
    for (int i = 0; i < ds->count; i++) {
        double br = ds->data[i].bitrate;
        bitrates[i] = br;
        sum += br;
        if (br < min) min = br;
        if (br > max) max = br;
        total_frames += ds->data[i].frame_count;
        total_chunk_size += ds->data[i].chunk_size;
    }
    
    double avg = sum / ds->count;
    
    sort_doubles(bitrates, ds->count);
    
    stats->max = max;
    stats->min = min;
    stats->avg = avg;
    stats->median = calculate_percentile(bitrates, ds->count, 50);
    stats->q1 = calculate_percentile(bitrates, ds->count, 25);
    stats->q3 = calculate_percentile(bitrates, ds->count, 75);
    stats->q5 = calculate_percentile(bitrates, ds->count, 5);
    stats->q95 = calculate_percentile(bitrates, ds->count, 95);
    stats->iqr = stats->q3 - stats->q1;
    stats->range = max - min;
    
    calculate_moments(bitrates, ds->count, avg, stats);
    
    stats->cv = (stats->std / avg) * 100.0;
    
    stats->num_chunks = ds->count;
    stats->total_frames = total_frames;
    stats->avg_chunk_size = total_chunk_size / ds->count;
    
    // Calculate duration and time resolution
    double min_time = ds->data[0].chunk_index;
    double max_time = ds->data[0].chunk_index;
    double time_diff_sum = 0;
    int time_diff_count = 0;
    
    for (int i = 0; i < ds->count; i++) {
        double t = (double)ds->data[i].chunk_index;
        if (t < min_time) min_time = t;
        if (t > max_time) max_time = t;
        
        if (i > 0) {
            time_diff_sum += fabs(t - (double)ds->data[i-1].chunk_index);
            time_diff_count++;
        }
    }
    
    stats->total_duration = max_time - min_time + 1.0;
    stats->time_resolution = time_diff_count > 0 ? time_diff_sum / time_diff_count : 1.0;
    
    stats->stability_index = 1.0 - (stats->std / stats->avg);
    stats->peak_to_avg_ratio = stats->max / stats->avg;
    
    free(bitrates);
    
    log_message("INFO", "Statistics computed successfully");
}

char* get_quality_assessment(Statistics *stats) {
    static char assessment[256];
    
    if (stats->stability_index > 0.8 && stats->cv < 10) {
        snprintf(assessment, sizeof(assessment), "EXCELLENT - Very stable bitrate");
    } else if (stats->stability_index > 0.6 && stats->cv < 20) {
        snprintf(assessment, sizeof(assessment), "GOOD - Reasonably stable");
    } else if (stats->stability_index > 0.4 && stats->cv < 40) {
        snprintf(assessment, sizeof(assessment), "MODERATE - Some fluctuation");
    } else {
        snprintf(assessment, sizeof(assessment), "POOR - Highly variable");
    }
    
    return assessment;
}

char* get_recommendations(Statistics *stats) {
    static char recs[512];
    recs[0] = '\0';
    
    if (stats->cv > 30) {
        strcat(recs, "• Consider CBR encoding\n");
    }
    if (stats->peak_to_avg_ratio > 3) {
        strcat(recs, "• Review rate control settings\n");
    }
    if (stats->std > stats->avg * 0.5) {
        strcat(recs, "• Check for scene complexity\n");
    }
    
    if (strlen(recs) == 0) {
        strcpy(recs, "• Current settings appear optimal\n");
    }
    
    return recs;
}

void generate_text_report(Dataset *ds, Statistics *stats, const char *output_file, const char *input_file) {
    FILE *out = fopen(output_file, "w");
    if (!out) {
        log_message("ERROR", "Cannot create output file");
        return;
    }
    
    print_separator(out, 80, '=');
    fprintf(out, "           ULTRA-PRECISE VIDEO BITRATE ANALYSIS REPORT\n");
    print_separator(out, 80, '=');
    print_newlines(out, 1);
    
    fprintf(out, "Input File:  %s\n", input_file);
    fprintf(out, "Output File: %s\n", output_file);
    fprintf(out, "Generated:   %s\n", ctime(&(time_t){time(NULL)}));
    print_newlines(out, 1);
    
    fprintf(out, "BITRATE STATISTICS\n");
    print_separator(out, 80, '-');
    fprintf(out, "Maximum:        %10.*f kbps\n", DECIMAL_PLACES, stats->max);
    fprintf(out, "Minimum:        %10.*f kbps\n", DECIMAL_PLACES, stats->min);
    fprintf(out, "Average:        %10.*f kbps\n", DECIMAL_PLACES, stats->avg);
    fprintf(out, "Median:         %10.*f kbps\n", DECIMAL_PLACES, stats->median);
    fprintf(out, "Std Deviation:  %10.*f kbps\n", DECIMAL_PLACES, stats->std);
    fprintf(out, "Variance:       %10.*f kbps²\n", DECIMAL_PLACES, stats->variance);
    print_newlines(out, 1);
    
    fprintf(out, "PERCENTILES\n");
    print_separator(out, 80, '-');
    fprintf(out, "5th percentile: %10.*f kbps\n", DECIMAL_PLACES, stats->q5);
    fprintf(out, "25th (Q1):      %10.*f kbps\n", DECIMAL_PLACES, stats->q1);
    fprintf(out, "75th (Q3):      %10.*f kbps\n", DECIMAL_PLACES, stats->q3);
    fprintf(out, "95th percentile:%10.*f kbps\n", DECIMAL_PLACES, stats->q95);
    fprintf(out, "IQR:            %10.*f kbps\n", DECIMAL_PLACES, stats->iqr);
    fprintf(out, "Range:          %10.*f kbps\n", DECIMAL_PLACES, stats->range);
    print_newlines(out, 1);
    
    fprintf(out, "ADVANCED METRICS\n");
    print_separator(out, 80, '-');
    fprintf(out, "Coeff. of Var:  %10.*f %%\n", DECIMAL_PLACES, stats->cv);
    fprintf(out, "Skewness:       %10.*f\n", DECIMAL_PLACES, stats->skewness);
    fprintf(out, "Kurtosis:       %10.*f\n", DECIMAL_PLACES, stats->kurtosis);
    fprintf(out, "Stability Index:%10.*f\n", DECIMAL_PLACES, stats->stability_index);
    fprintf(out, "Peak/Avg Ratio: %10.*f\n", DECIMAL_PLACES, stats->peak_to_avg_ratio);
    print_newlines(out, 1);
    
    fprintf(out, "DATA INFORMATION\n");
    print_separator(out, 80, '-');
    fprintf(out, "Total Chunks:   %10d\n", stats->num_chunks);
    fprintf(out, "Duration:       %10.*f s\n", TIME_PRECISION, stats->total_duration);
    fprintf(out, "Total Frames:   %10d\n", stats->total_frames);
    fprintf(out, "Time Resolution:%10.*f s\n", TIME_PRECISION, stats->time_resolution);
    fprintf(out, "Avg Chunk Size: %10.*f bytes\n", DECIMAL_PLACES, stats->avg_chunk_size);
    print_newlines(out, 1);
    
    fprintf(out, "QUALITY ASSESSMENT\n");
    print_separator(out, 80, '-');
    fprintf(out, "Overall Quality: %s\n", get_quality_assessment(stats));
    fprintf(out, "Stability:       %.3f\n", stats->stability_index);
    fprintf(out, "Variability:     %.1f%%\n", stats->cv);
    print_newlines(out, 1);
    
    fprintf(out, "RECOMMENDATIONS\n");
    print_separator(out, 80, '-');
    fprintf(out, "%s", get_recommendations(stats));
    print_newlines(out, 1);
    
    print_separator(out, 80, '=');
    fprintf(out, "                           END OF REPORT\n");
    print_separator(out, 80, '=');
    
    fclose(out);
    
    log_message("INFO", "Text report generated successfully");
}

void print_summary(const char *csv_file, const char *output_file, Statistics *stats) {
    print_newlines(stdout, 1);
    print_separator(stdout, 60, '=');
    printf("      ULTRA-PRECISE BITRATE ANALYSIS SUMMARY\n");
    print_separator(stdout, 60, '=');
    printf("File: %s\n", csv_file);
    printf("Output: %s\n", output_file);
    printf("Data Points: %d\n", stats->num_chunks);
    printf("Duration: %.*f seconds\n", TIME_PRECISION, stats->total_duration);
    printf("Average Bitrate: %.*f kbps\n", DECIMAL_PLACES, stats->avg);
    printf("Stability Index: %.*f\n", DECIMAL_PLACES, stats->stability_index);
    printf("Coefficient of Variation: %.*f%%\n", DECIMAL_PLACES, stats->cv);
    print_separator(stdout, 60, '=');
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Ultra-Precise FFmpeg Bitrate Analysis Tool\n");
        printf("Usage: %s <input_csv> <output_txt>\n", argv[0]);
        printf("Example: %s bitrate_stats.csv analysis.txt\n", argv[0]);
        return 1;
    }
    
    const char *csv_file = argv[1];
    const char *output_file = argv[2];
    
    log_message("INFO", "Starting bitrate analysis...");
    
    Dataset *ds = create_dataset(10000);
    if (!ds) {
        log_message("ERROR", "Failed to create dataset");
        return 1;
    }
    
    if (!load_csv(csv_file, ds)) {
        free_dataset(ds);
        return 1;
    }
    
    if (ds->count == 0) {
        log_message("ERROR", "No valid data points loaded");
        free_dataset(ds);
        return 1;
    }
    
    log_message("INFO", "Computing statistics...");
    Statistics stats = {0};
    compute_statistics(ds, &stats);
    
    log_message("INFO", "Generating report...");
    generate_text_report(ds, &stats, output_file, csv_file);
    
    print_summary(csv_file, output_file, &stats);
    
    log_message("INFO", "Analysis complete!");
    
    free_dataset(ds);
    
    return 0;
}
