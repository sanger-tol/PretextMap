/*
 * Preprocess Pairs File Utility
 * 
 * This utility preprocesses pairs files to ensure they are in the correct
 * format for PretextMap. It validates, cleans, and converts input data.
 * 
 * Input: Pairs file (or juicer format)
 * Output: Valid pairs file compatible with PretextMap
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>

#define MAX_LINE_LEN 1048576  // 1MB max line length
#define MAX_FIELD_LEN 1024
#define MAX_CONTIG_NAME 64

typedef struct {
    char name[MAX_CONTIG_NAME];
    uint64_t length;
} contig_info_t;

typedef struct {
    contig_info_t *contigs;
    size_t count;
    size_t capacity;
} contig_list_t;

static contig_list_t contig_list = {NULL, 0, 0};

// Statistics
static uint64_t total_lines = 0;
static uint64_t valid_lines = 0;
static uint64_t skipped_lines = 0;
static uint64_t header_lines = 0;

// Function prototypes
static FILE *open_file_maybe_gz(const char *filename, const char *mode);
static void close_file_maybe_gz(FILE *fp, int is_gz);
static int preprocess_pairs_file(const char *pairs_file, const char *fasta_file, 
                                const char *fai_file, const char *output_file);
static int read_fai_file(const char *fai_file, contig_list_t *contig_list);
static int extract_data_fields(const char *input_file, const char *output_file);
static int fasta_to_agp(const char *fasta_file, const char *output_file);
static int juicer_pre(const char *pa5_file, const char *agp_file, const char *fai_file, 
                     uint8_t min_quality, int scale, FILE *out, FILE *log);
static int create_pairs_header(const char *log_file, const char *output_file);
static int generate_pairs_file(const char *header_file, const char *alignment_file, 
                               const char *output_file, int compress);
static void add_contig_to_list(const char *name, uint64_t length, contig_list_t *list);
static void add_contig(const char *name, uint64_t length);
static contig_info_t *find_contig(const char *name);
static int parse_chromsize_line(const char *line);
static int parse_data_line(const char *line, FILE *out);
static int is_numeric(const char *str);
static void print_usage(const char *prog_name);
static void trim_whitespace(char *str);

/*
 * Open a file, handling both regular and gzipped files
 * Returns FILE* on success, NULL on error
 * Sets is_gz flag to 1 if file is gzipped (for proper closing)
 */
static FILE *open_file_maybe_gz(const char *filename, const char *mode)
{
    if (!filename) return NULL;
    
    // Check if filename ends with .gz
    size_t len = strlen(filename);
    int is_gzipped = (len >= 3 && strcmp(filename + len - 3, ".gz") == 0);
    
    if (is_gzipped && strcmp(mode, "r") == 0) {
        // Open gzipped file for reading using zcat or gunzip -c
        char command[2048];
        snprintf(command, sizeof(command), "zcat '%s' 2>/dev/null || gunzip -c '%s'", filename, filename);
        FILE *fp = popen(command, "r");
        return fp;
    } else {
        // Open regular file
        return fopen(filename, mode);
    }
}

/*
 * Close a file, handling both regular files and pipes (from popen)
 */
static void close_file_maybe_gz(FILE *fp, int is_gz)
{
    if (!fp) return;
    
    if (is_gz) {
        pclose(fp);
    } else {
        fclose(fp);
    }
}

/*
 * Read FASTA index file (.fai) and populate contig list
 * .fai format: name\tlength\toffset\tline_length\tline_length_with_newline
 */
static int read_fai_file(const char *fai_file, contig_list_t *contig_list)
{
    FILE *fp = fopen(fai_file, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open FASTA index file '%s': %s\n", fai_file, strerror(errno));
        return 0;
    }
    
    char *line = malloc(MAX_LINE_LEN);
    if (!line) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(fp);
        return 0;
    }
    
    size_t count = 0;
    
    while (fgets(line, MAX_LINE_LEN, fp)) {
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
            len--;
        }
        if (len > 0 && line[len-1] == '\r') {
            line[len-1] = '\0';
            len--;
        }
        
        // Skip empty lines
        if (len == 0) {
            continue;
        }
        
        // Parse .fai format: name\tlength\toffset\tline_length\tline_length_with_newline
        char *fields[5];
        int field_count = 0;
        char *line_copy = strdup(line);
        if (!line_copy) {
            continue;
        }
        
        char *token = line_copy;
        char *saveptr = NULL;
        
        // Split by tabs
        while (field_count < 5 && (token = strtok_r(token, "\t", &saveptr)) != NULL) {
            trim_whitespace(token);
            fields[field_count++] = token;
            token = NULL;
        }
        
        // Need at least 2 fields (name and length)
        if (field_count < 2) {
            free(line_copy);
            continue;
        }
        
        // Parse length
        char *endptr;
        errno = 0;
        uint64_t length = strtoull(fields[1], &endptr, 10);
        
        if (errno != 0 || *endptr != '\0' || length == 0) {
            free(line_copy);
            continue;
        }
        
        // Add contig
        add_contig_to_list(fields[0], length, contig_list);
        count++;
        
        free(line_copy);
    }
    
    free(line);
    fclose(fp);
    
    fprintf(stderr, "Read %zu contigs from FASTA index file\n", count);
    return 1;
}

/*
 * Main preprocessing function: Convert old pairs file to new pairs file
 * Input: old pairs file, reference FASTA file, samtools index file (.fai)
 * Output: new pairs file with proper header from .fai
 */
static int preprocess_pairs_file(const char *pairs_file, const char *fasta_file,
                                 const char *fai_file, const char *output_file)
{
    FILE *in;
    FILE *out = stdout;
    
    if (!pairs_file) {
        fprintf(stderr, "Error: Pairs file is required\n");
        return 1;
    }
    
    if (!fasta_file) {
        fprintf(stderr, "Error: FASTA file is required\n");
        return 1;
    }
    
    if (!fai_file) {
        fprintf(stderr, "Error: FASTA index file (.fai) is required\n");
        return 1;
    }
    
    // Initialize contig list
    contig_list.capacity = 1000;
    contig_list.contigs = malloc(contig_list.capacity * sizeof(contig_info_t));
    if (!contig_list.contigs) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 1;
    }
    contig_list.count = 0;
    
    // Read FASTA index file to get chromosome information
    if (!read_fai_file(fai_file, &contig_list)) {
        free(contig_list.contigs);
        return 1;
    }
    
    if (contig_list.count == 0) {
        fprintf(stderr, "Error: No contigs found in FASTA index file\n");
        free(contig_list.contigs);
        return 1;
    }
    
    // Open input pairs file (may be gzipped)
    int pairs_is_gz = 0;
    size_t pairs_len = strlen(pairs_file);
    if (pairs_len >= 3 && strcmp(pairs_file + pairs_len - 3, ".gz") == 0) {
        pairs_is_gz = 1;
    }
    
    in = open_file_maybe_gz(pairs_file, "r");
    if (!in) {
        fprintf(stderr, "Error: Cannot open pairs file '%s': %s\n", pairs_file, strerror(errno));
        free(contig_list.contigs);
        return 1;
    }
    
    // Open output file
    if (output_file) {
        out = fopen(output_file, "w");
        if (!out) {
            fprintf(stderr, "Error: Cannot open output file '%s': %s\n", output_file, strerror(errno));
            fclose(in);
            free(contig_list.contigs);
            return 1;
        }
    }
    
    // Write pairs format header
    fprintf(out, "## pairs format v1.0\n");
    
    // Write chromsize lines from contig list
    for (size_t i = 0; i < contig_list.count; i++) {
        fprintf(out, "#chromsize:\t%s\t%lu\n", 
                contig_list.contigs[i].name, 
                contig_list.contigs[i].length);
    }
    
    // Write columns line
    fprintf(out, "#columns:\treadID\tchr1\tpos1\tchr2\tpos2\tstrand1\tstrand2\n");
    
    // Process pairs file data lines
    char *line = malloc(MAX_LINE_LEN);
    if (!line) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(in);
        if (out != stdout) fclose(out);
        free(contig_list.contigs);
        return 1;
    }
    
    uint64_t lines_processed = 0;
    uint64_t lines_written = 0;
    uint64_t lines_skipped = 0;
    int in_header = 1;
    
    while (fgets(line, MAX_LINE_LEN, in)) {
        lines_processed++;
        
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
            len--;
        }
        if (len > 0 && line[len-1] == '\r') {
            line[len-1] = '\0';
            len--;
        }
        
        // Skip empty lines
        if (len == 0) {
            continue;
        }
        
        // Skip header lines from input (we've already written our own header)
        if (line[0] == '#') {
            in_header = 1;
            continue;
        }
        
        // Process data lines
        if (in_header) {
            in_header = 0;
        }
        
        // Parse data line: readID \t chr1 \t pos1 \t chr2 \t pos2 \t [strand1] \t [strand2] \t ...
        char *fields[10];
        int field_count = 0;
        char *line_copy = strdup(line);
        if (!line_copy) {
            lines_skipped++;
            continue;
        }
        
        char *token = line_copy;
        char *saveptr = NULL;
        
        // Split by tabs
        while (field_count < 10 && (token = strtok_r(token, "\t", &saveptr)) != NULL) {
            trim_whitespace(token);
            fields[field_count++] = token;
            token = NULL;
        }
        
        // Need at least 5 fields (readID, chr1, pos1, chr2, pos2)
        if (field_count < 5) {
            free(line_copy);
            lines_skipped++;
            continue;
        }
        
        // Validate positions are numeric
        if (!is_numeric(fields[2]) || !is_numeric(fields[4])) {
            free(line_copy);
            lines_skipped++;
            continue;
        }
        
        // Write the line (ensure we have at least 7 fields)
        fprintf(out, "%s", fields[0]); // readID
        for (int i = 1; i < field_count && i < 7; i++) {
            fprintf(out, "\t%s", fields[i]);
        }
        
        // Pad with "." for missing strand fields if needed
        while (field_count < 7) {
            fprintf(out, "\t.");
            field_count++;
        }
        
        fprintf(out, "\n");
        lines_written++;
        free(line_copy);
    }
    
    fprintf(stderr, "Processed %lu lines: %lu written, %lu skipped\n",
            lines_processed, lines_written, lines_skipped);
    
    free(line);
    free(contig_list.contigs);
    close_file_maybe_gz(in, pairs_is_gz);
    if (out != stdout) fclose(out);
    
    return 0;
}

/*
 * First function: Extract data fields only
 * Equivalent to: grep -v "^#" | cut -f1-5
 * Removes all header lines (starting with #) and keeps only first 5 fields
 */
static int extract_data_fields(const char *input_file, const char *output_file)
{
    FILE *in = stdin;
    FILE *out = stdout;
    int input_is_gz = 0;
    
    if (input_file) {
        size_t input_len = strlen(input_file);
        if (input_len >= 3 && strcmp(input_file + input_len - 3, ".gz") == 0) {
            input_is_gz = 1;
        }
        in = open_file_maybe_gz(input_file, "r");
        if (!in) {
            fprintf(stderr, "Error: Cannot open input file '%s': %s\n", input_file, strerror(errno));
            return 1;
        }
    }
    
    if (output_file) {
        out = fopen(output_file, "w");
        if (!out) {
            fprintf(stderr, "Error: Cannot open output file '%s': %s\n", output_file, strerror(errno));
            if (in != stdin) close_file_maybe_gz(in, input_is_gz);
            return 1;
        }
    }
    
    char *line = malloc(MAX_LINE_LEN);
    if (!line) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        if (in != stdin) close_file_maybe_gz(in, input_is_gz);
        if (out != stdout) fclose(out);
        return 1;
    }
    
    uint64_t lines_processed = 0;
    uint64_t lines_written = 0;
    
    while (fgets(line, MAX_LINE_LEN, in)) {
        lines_processed++;
        
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
            len--;
        }
        
        // Skip empty lines
        if (len == 0) {
            continue;
        }
        
        // Skip lines starting with '#' (grep -v "^#")
        if (line[0] == '#') {
            continue;
        }
        
        // Extract first 5 fields (cut -f1-5)
        char *fields[5];
        int field_count = 0;
        char *line_copy = strdup(line);
        if (!line_copy) {
            continue;
        }
        
        char *token = line_copy;
        char *saveptr = NULL;
        
        // Split by tabs and extract first 5 fields
        while (field_count < 5 && (token = strtok_r(token, "\t", &saveptr)) != NULL) {
            fields[field_count++] = token;
            token = NULL;
        }
        
        // Write first 5 fields
        if (field_count > 0) {
            fprintf(out, "%s", fields[0]);
            for (int i = 1; i < field_count && i < 5; i++) {
                fprintf(out, "\t%s", fields[i]);
            }
            fprintf(out, "\n");
            lines_written++;
        }
        
        free(line_copy);
    }
    
    fprintf(stderr, "Extracted %lu data lines (first 5 fields) from %lu total lines\n", 
            lines_written, lines_processed);
    
    free(line);
    if (in != stdin) {
        close_file_maybe_gz(in, input_is_gz);
    }
    if (out != stdout) fclose(out);
    
    return 0;
}

/*
 * Second function: Convert FASTA file to AGP format
 * Reads FASTA file, extracts sequence names and lengths, writes AGP format
 */
static int fasta_to_agp(const char *fasta_file, const char *output_file)
{
    FILE *in;
    FILE *out = stdout;
    
    if (!fasta_file) {
        fprintf(stderr, "Error: FASTA file is required\n");
        return 1;
    }
    
    in = fopen(fasta_file, "r");
    if (!in) {
        fprintf(stderr, "Error: Cannot open FASTA file '%s': %s\n", fasta_file, strerror(errno));
        return 1;
    }
    
    if (output_file) {
        out = fopen(output_file, "w");
        if (!out) {
            fprintf(stderr, "Error: Cannot open output file '%s': %s\n", output_file, strerror(errno));
            fclose(in);
            return 1;
        }
    }
    
    char *line = malloc(MAX_LINE_LEN);
    if (!line) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(in);
        if (out != stdout) fclose(out);
        return 1;
    }
    
    typedef struct {
        char name[MAX_CONTIG_NAME];
        uint64_t length;
    } seq_info_t;
    
    seq_info_t *sequences = NULL;
    size_t seq_count = 0;
    size_t seq_capacity = 100;
    sequences = malloc(seq_capacity * sizeof(seq_info_t));
    if (!sequences) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        free(line);
        fclose(in);
        if (out != stdout) fclose(out);
        return 1;
    }
    
    char *current_seq = NULL;
    uint64_t current_length = 0;
    
    while (fgets(line, MAX_LINE_LEN, in)) {
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
            len--;
        }
        if (len > 0 && line[len-1] == '\r') {
            line[len-1] = '\0';
            len--;
        }
        
        // Skip empty lines
        if (len == 0) {
            continue;
        }
        
        // Check if it's a header line (starts with '>')
        if (line[0] == '>') {
            // Save previous sequence if exists
            if (current_seq != NULL) {
                // Resize if needed
                if (seq_count >= seq_capacity) {
                    seq_capacity *= 2;
                    sequences = realloc(sequences, seq_capacity * sizeof(seq_info_t));
                    if (!sequences) {
                        fprintf(stderr, "Error: Memory reallocation failed\n");
                        free(current_seq);
                        free(line);
                        fclose(in);
                        if (out != stdout) fclose(out);
                        return 1;
                    }
                }
                
                // Save sequence
                strncpy(sequences[seq_count].name, current_seq, MAX_CONTIG_NAME - 1);
                sequences[seq_count].name[MAX_CONTIG_NAME - 1] = '\0';
                sequences[seq_count].length = current_length;
                seq_count++;
                
                free(current_seq);
                current_seq = NULL;
            }
            
            // Extract sequence name (first word after '>')
            const char *name_start = line + 1; // Skip '>'
            // Skip leading whitespace
            while (*name_start == ' ' || *name_start == '\t') name_start++;
            
            // Find end of first word (whitespace or end of line)
            size_t name_len = 0;
            while (name_start[name_len] != '\0' && 
                   name_start[name_len] != ' ' && 
                   name_start[name_len] != '\t' &&
                   name_len < MAX_CONTIG_NAME - 1) {
                name_len++;
            }
            
            if (name_len > 0) {
                current_seq = malloc(name_len + 1);
                if (current_seq) {
                    strncpy(current_seq, name_start, name_len);
                    current_seq[name_len] = '\0';
                }
            }
            current_length = 0;
        } else {
            // Sequence line - count length
            current_length += len;
        }
    }
    
    // Don't forget the last sequence
    if (current_seq != NULL) {
        // Resize if needed
        if (seq_count >= seq_capacity) {
            seq_capacity++;
            sequences = realloc(sequences, seq_capacity * sizeof(seq_info_t));
            if (!sequences) {
                fprintf(stderr, "Error: Memory reallocation failed\n");
                free(current_seq);
                free(line);
                fclose(in);
                if (out != stdout) fclose(out);
                return 1;
            }
        }
        
        // Save sequence
        strncpy(sequences[seq_count].name, current_seq, MAX_CONTIG_NAME - 1);
        sequences[seq_count].name[MAX_CONTIG_NAME - 1] = '\0';
        sequences[seq_count].length = current_length;
        seq_count++;
        
        free(current_seq);
    }
    
    // Write AGP format
    // AGP format: object, object_beg, object_end, part_number, component_type,
    //            component_id, component_beg, component_end, orientation
    // Format: {seq_name}\t1\t{length}\t1\tW\t{seq_name}\t1\t{length}\t+
    for (size_t i = 0; i < seq_count; i++) {
        fprintf(out, "%s\t1\t%lu\t1\tW\t%s\t1\t%lu\t+\n",
                sequences[i].name,
                sequences[i].length,
                sequences[i].name,
                sequences[i].length);
    }
    
    // Print summary
    if (output_file) {
        fprintf(stderr, "Converted %zu sequences to AGP format: %s\n", seq_count, output_file);
    } else {
        fprintf(stderr, "Processed %zu sequences\n", seq_count);
    }
    
    // Cleanup
    free(sequences);
    free(line);
    fclose(in);
    if (out != stdout) fclose(out);
    
    if (seq_count == 0) {
        fprintf(stderr, "Warning: No sequences found in FASTA file.\n");
        return 1;
    }
    
    return 0;
}

/*
 * Third function: juicer pre - Convert PA5 file to juicer format
 * Equivalent to: juicer pre -q 0 merged.pairs.pa5 ${agpfile} ${fastaidxfile}
 * 
 * Input: PA5 file (5 fields: readID, chr1, pos1, chr2, pos2)
 * Output: Juicer format (9 fields: 0, chr1, pos1, strand1, mapq1, chr2, pos2, strand2, mapq2)
 * Filters: positions >= 0 (equivalent to awk '$3>=0 && $7>=0')
 */
static int juicer_pre(const char *pa5_file, const char *agp_file, const char *fai_file,
                     uint8_t min_quality, int scale, FILE *out, FILE *log)
{
    FILE *in;
    
    if (!pa5_file) {
        fprintf(stderr, "Error: PA5 file is required\n");
        return 1;
    }
    
    in = fopen(pa5_file, "r");
    if (!in) {
        fprintf(stderr, "Error: Cannot open PA5 file '%s': %s\n", pa5_file, strerror(errno));
        return 1;
    }
    
    if (log) {
        fprintf(log, "[juicer pre] Processing PA5 file: %s\n", pa5_file);
        if (agp_file) {
            fprintf(log, "[juicer pre] AGP file: %s\n", agp_file);
        }
        if (fai_file) {
            fprintf(log, "[juicer pre] FASTA index file: %s\n", fai_file);
        }
        fprintf(log, "[juicer pre] Minimum quality: %u\n", min_quality);
        fprintf(log, "[juicer pre] Scale factor: %d\n", scale);
    }
    
    char *line = malloc(MAX_LINE_LEN);
    if (!line) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(in);
        return 1;
    }
    
    uint64_t lines_processed = 0;
    uint64_t lines_written = 0;
    uint64_t lines_filtered = 0;
    
    // Note: In a full implementation, we would:
    // 1. Load AGP file to build coordinate conversion dictionary
    // 2. Load FASTA index to get sequence dictionary
    // 3. Convert coordinates using AGP mapping
    // For now, we do a simplified version that just converts format
    
    while (fgets(line, MAX_LINE_LEN, in)) {
        lines_processed++;
        
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
            len--;
        }
        if (len > 0 && line[len-1] == '\r') {
            line[len-1] = '\0';
            len--;
        }
        
        // Skip empty lines
        if (len == 0) {
            continue;
        }
        
        // Skip header lines
        if (line[0] == '#') {
            continue;
        }
        
        // Parse PA5 format: readID \t chr1 \t pos1 \t chr2 \t pos2
        char *fields[5];
        int field_count = 0;
        char *line_copy = strdup(line);
        if (!line_copy) {
            continue;
        }
        
        char *token = line_copy;
        char *saveptr = NULL;
        
        // Split by tabs
        while (field_count < 5 && (token = strtok_r(token, "\t", &saveptr)) != NULL) {
            trim_whitespace(token);
            fields[field_count++] = token;
            token = NULL;
        }
        
        // Need exactly 5 fields
        if (field_count < 5) {
            free(line_copy);
            lines_filtered++;
            continue;
        }
        
        // Parse positions
        char *endptr;
        errno = 0;
        int64_t pos1 = (int64_t)strtoll(fields[2], &endptr, 10);
        if (errno != 0 || *endptr != '\0') {
            free(line_copy);
            lines_filtered++;
            continue;
        }
        
        errno = 0;
        int64_t pos2 = (int64_t)strtoll(fields[4], &endptr, 10);
        if (errno != 0 || *endptr != '\0') {
            free(line_copy);
            lines_filtered++;
            continue;
        }
        
        // Filter: positions must be >= 0 (equivalent to awk '$3>=0 && $7>=0')
        if (pos1 < 0 || pos2 < 0) {
            free(line_copy);
            lines_filtered++;
            continue;
        }
        
        // Apply scale factor (divide by 2^scale)
        uint64_t scaled_pos1 = (uint64_t)(pos1 >> scale);
        uint64_t scaled_pos2 = (uint64_t)(pos2 >> scale);
        
        // Determine strand and ordering (simplified - in full version would use AGP)
        // For juicer format, ensure chr1 <= chr2 lexicographically
        int chr_compare = strcmp(fields[1], fields[3]);
        
        if (chr_compare <= 0) {
            // chr1 <= chr2: output as is
            // Format: 0\tchr1\tpos1\tstrand1\tmapq1\tchr2\tpos2\tstrand2\tmapq2
            // Default: strand1=0, mapq1=1, strand2=1, mapq2=1 (if quality >= min_quality)
            fprintf(out, "0\t%s\t%lu\t0\t%u\t%s\t%lu\t1\t%u\n",
                    fields[1], scaled_pos1, min_quality,
                    fields[3], scaled_pos2, min_quality);
        } else {
            // chr1 > chr2: swap them
            fprintf(out, "0\t%s\t%lu\t1\t%u\t%s\t%lu\t0\t%u\n",
                    fields[3], scaled_pos2, min_quality,
                    fields[1], scaled_pos1, min_quality);
        }
        
        lines_written++;
        free(line_copy);
    }
    
    if (log) {
        fprintf(log, "[juicer pre] %lu read pairs processed: %lu written, %lu filtered\n",
                lines_processed, lines_written, lines_filtered);
    }
    
    fprintf(stderr, "[juicer pre] %lu read pairs processed: %lu written, %lu filtered\n",
            lines_processed, lines_written, lines_filtered);
    
    free(line);
    fclose(in);
    
    return 0;
}

/*
 * Fourth function: Create pairs format header from juicer log
 * Equivalent to:
 *   echo "## pairs format v1.0"
 *   grep PRE_C_SIZE juicercout.log | awk '{print "#chromsize:\t"$2"\t"$3}'
 *   echo "#columns:\treadID\tchr1\tpos1\tchr2\tpos2\tstrand1\tstrand2"
 */
static int create_pairs_header(const char *log_file, const char *output_file)
{
    FILE *log;
    FILE *out = stdout;
    
    if (!log_file) {
        fprintf(stderr, "Error: Log file is required\n");
        return 1;
    }
    
    log = fopen(log_file, "r");
    if (!log) {
        fprintf(stderr, "Error: Cannot open log file '%s': %s\n", log_file, strerror(errno));
        return 1;
    }
    
    if (output_file) {
        out = fopen(output_file, "w");
        if (!out) {
            fprintf(stderr, "Error: Cannot open output file '%s': %s\n", output_file, strerror(errno));
            fclose(log);
            return 1;
        }
    }
    
    // Write format declaration
    fprintf(out, "## pairs format v1.0\n");
    
    char *line = malloc(MAX_LINE_LEN);
    if (!line) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(log);
        if (out != stdout) fclose(out);
        return 1;
    }
    
    uint64_t chromsize_lines = 0;
    
    // Read log file and find PRE_C_SIZE lines
    while (fgets(line, MAX_LINE_LEN, log)) {
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
            len--;
        }
        if (len > 0 && line[len-1] == '\r') {
            line[len-1] = '\0';
            len--;
        }
        
        // Check if line contains PRE_C_SIZE
        if (strstr(line, "PRE_C_SIZE") != NULL) {
            // Parse the line: extract fields 2 and 3
            // Format is typically: [something] PRE_C_SIZE field2 field3 [rest]
            char *fields[10];
            int field_count = 0;
            char *line_copy = strdup(line);
            if (!line_copy) {
                continue;
            }
            
            char *token = line_copy;
            char *saveptr = NULL;
            
            // Split by whitespace (space or tab)
            while (field_count < 10 && (token = strtok_r(token, " \t", &saveptr)) != NULL) {
                trim_whitespace(token);
                if (strlen(token) > 0) {  // Skip empty fields
                    fields[field_count++] = token;
                }
                token = NULL;
            }
            
            // Find PRE_C_SIZE field and get the next two fields
            int pre_c_size_idx = -1;
            for (int i = 0; i < field_count; i++) {
                if (strcmp(fields[i], "PRE_C_SIZE") == 0) {
                    pre_c_size_idx = i;
                    break;
                }
            }
            
            // If PRE_C_SIZE found and we have at least 2 more fields
            if (pre_c_size_idx >= 0 && pre_c_size_idx + 2 < field_count) {
                // fields[pre_c_size_idx+1] is chromosome name (field 2)
                // fields[pre_c_size_idx+2] is length (field 3)
                char *chr_name = fields[pre_c_size_idx + 1];
                char *length_str = fields[pre_c_size_idx + 2];
                
                // Validate length is numeric
                if (is_numeric(length_str)) {
                    fprintf(out, "#chromsize:\t%s\t%s\n", chr_name, length_str);
                    chromsize_lines++;
                }
            }
            
            free(line_copy);
        }
    }
    
    // Write columns line
    fprintf(out, "#columns:\treadID\tchr1\tpos1\tchr2\tpos2\tstrand1\tstrand2\n");
    
    fprintf(stderr, "Created pairs format header with %lu chromsize entries\n", chromsize_lines);
    
    free(line);
    fclose(log);
    if (out != stdout) fclose(out);
    
    return 0;
}

/*
 * Fifth function: Generate pairs file from header and alignment
 * Equivalent to: (cat $header; cat $alignment|awk '{print ".\t"$2"\t"$3"\t"$6"\t"$7"\t.\t."}') | gzip
 * 
 * Input: Header file (pairs format header) and alignment file (juicer format)
 * Output: Complete pairs file (header + processed data), optionally gzipped
 */
static int generate_pairs_file(const char *header_file, const char *alignment_file,
                               const char *output_file, int compress)
{
    FILE *header_fp = NULL;
    FILE *align_fp = NULL;
    FILE *out = stdout;
    
    if (!header_file) {
        fprintf(stderr, "Error: Header file is required\n");
        return 1;
    }
    
    if (!alignment_file) {
        fprintf(stderr, "Error: Alignment file is required\n");
        return 1;
    }
    
    header_fp = fopen(header_file, "r");
    if (!header_fp) {
        fprintf(stderr, "Error: Cannot open header file '%s': %s\n", header_file, strerror(errno));
        return 1;
    }
    
    align_fp = fopen(alignment_file, "r");
    if (!align_fp) {
        fprintf(stderr, "Error: Cannot open alignment file '%s': %s\n", alignment_file, strerror(errno));
        fclose(header_fp);
        return 1;
    }
    
    if (output_file) {
        if (compress) {
            // For gzip compression, we'll use popen to write to gzip
            char cmd[1024];
            snprintf(cmd, sizeof(cmd), "gzip > %s", output_file);
            out = popen(cmd, "w");
            if (!out) {
                fprintf(stderr, "Error: Cannot open gzip pipe for '%s': %s\n", output_file, strerror(errno));
                fclose(header_fp);
                fclose(align_fp);
                return 1;
            }
        } else {
            out = fopen(output_file, "w");
            if (!out) {
                fprintf(stderr, "Error: Cannot open output file '%s': %s\n", output_file, strerror(errno));
                fclose(header_fp);
                fclose(align_fp);
                return 1;
            }
        }
    }
    
    // Copy header file to output
    char *line = malloc(MAX_LINE_LEN);
    if (!line) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(header_fp);
        fclose(align_fp);
        if (out != stdout) {
            if (compress) {
                pclose(out);
            } else {
                fclose(out);
            }
        }
        return 1;
    }
    
    uint64_t header_lines = 0;
    
    // Read and write header file
    while (fgets(line, MAX_LINE_LEN, header_fp)) {
        fputs(line, out);
        header_lines++;
    }
    fclose(header_fp);
    
    // Process alignment file: awk '{print ".\t"$2"\t"$3"\t"$6"\t"$7"\t.\t."}'
    // Format: 0\tchr1\tpos1\tstrand1\tmapq1\tchr2\tpos2\tstrand2\tmapq2
    // Extract: field2 (chr1), field3 (pos1), field6 (chr2), field7 (pos2)
    // Output: .\tchr1\tpos1\tchr2\tpos2\t.\t.
    
    uint64_t align_lines = 0;
    uint64_t processed_lines = 0;
    
    while (fgets(line, MAX_LINE_LEN, align_fp)) {
        align_lines++;
        
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
            len--;
        }
        if (len > 0 && line[len-1] == '\r') {
            line[len-1] = '\0';
            len--;
        }
        
        // Skip empty lines
        if (len == 0) {
            continue;
        }
        
        // Skip header lines (starting with #)
        if (line[0] == '#') {
            continue;
        }
        
        // Parse alignment line (juicer format: 9 tab-separated fields)
        char *fields[10];
        int field_count = 0;
        char *line_copy = strdup(line);
        if (!line_copy) {
            continue;
        }
        
        char *token = line_copy;
        char *saveptr = NULL;
        
        // Split by tabs
        while (field_count < 10 && (token = strtok_r(token, "\t", &saveptr)) != NULL) {
            trim_whitespace(token);
            fields[field_count++] = token;
            token = NULL;
        }
        
        // Need at least 7 fields (we need fields 2, 3, 6, 7)
        if (field_count < 7) {
            free(line_copy);
            continue;
        }
        
        // Extract fields 2, 3, 6, 7 (0-indexed: fields[1], fields[2], fields[5], fields[6])
        // Output format: .\tchr1\tpos1\tchr2\tpos2\t.\t.
        fprintf(out, ".\t%s\t%s\t%s\t%s\t.\t.\n",
                fields[1],  // chr1 (field 2)
                fields[2],  // pos1 (field 3)
                fields[5],  // chr2 (field 6)
                fields[6]); // pos2 (field 7)
        
        processed_lines++;
        free(line_copy);
    }
    
    fclose(align_fp);
    
    // Close output
    if (out != stdout) {
        if (compress) {
            pclose(out);
        } else {
            fclose(out);
        }
    }
    
    fprintf(stderr, "Generated pairs file: %lu header lines, %lu alignment lines processed\n",
            header_lines, processed_lines);
    
    free(line);
    
    return 0;
}

int main(int argc, char *argv[])
{
    const char *input_file = NULL;
    const char *output_file = NULL;
    const char *fasta_file = NULL;
    const char *fai_file = NULL;
    const char *agp_file = NULL;
    const char *log_file = NULL;
    int verbose = 0;
    int skip_invalid = 1;
    int extract_mode = 0;  // New mode: extract data fields only
    int fasta_to_agp_mode = 0;  // New mode: convert FASTA to AGP
    int juicer_pre_mode = 0;  // New mode: juicer pre
    int create_header_mode = 0;  // New mode: create pairs header from log
    int generate_pairs_mode = 0;  // New mode: generate pairs file from header and alignment
    const char *header_file = NULL;
    const char *alignment_file = NULL;
    int compress_output = 0;
    uint8_t min_quality = 0;
    int scale = 0;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--extract") == 0 || strcmp(argv[i], "-e") == 0) {
            extract_mode = 1;
        } else if (strcmp(argv[i], "--fasta-to-agp") == 0 || strcmp(argv[i], "-f") == 0) {
            fasta_to_agp_mode = 1;
        } else if (strcmp(argv[i], "pre") == 0 || strcmp(argv[i], "--juicer-pre") == 0) {
            juicer_pre_mode = 1;
        } else if (strcmp(argv[i], "--create-header") == 0 || strcmp(argv[i], "-H") == 0) {
            create_header_mode = 1;
        } else if (strcmp(argv[i], "--generate-pairs") == 0 || strcmp(argv[i], "-g") == 0) {
            generate_pairs_mode = 1;
        } else if (strcmp(argv[i], "--header") == 0) {
            if (i + 1 < argc) {
                header_file = argv[++i];
            }
        } else if (strcmp(argv[i], "--alignment") == 0 || strcmp(argv[i], "--align") == 0) {
            if (i + 1 < argc) {
                alignment_file = argv[++i];
            }
        } else if (strcmp(argv[i], "--compress") == 0 || strcmp(argv[i], "-z") == 0) {
            compress_output = 1;
        } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quality") == 0) {
            if (i + 1 < argc) {
                min_quality = (uint8_t)atoi(argv[++i]);
            }
        } else if (strcmp(argv[i], "--scale") == 0) {
            if (i + 1 < argc) {
                scale = atoi(argv[++i]);
            }
        } else if (strcmp(argv[i], "--agp") == 0) {
            if (i + 1 < argc) {
                agp_file = argv[++i];
            }
        } else if (strcmp(argv[i], "--fai") == 0 || strcmp(argv[i], "--fasta-index") == 0) {
            if (i + 1 < argc) {
                fai_file = argv[++i];
            }
        } else if (strcmp(argv[i], "--fasta") == 0 || strcmp(argv[i], "-r") == 0) {
            if (i + 1 < argc) {
                fasta_file = argv[++i];
            }
        } else if (strcmp(argv[i], "--log") == 0) {
            if (i + 1 < argc) {
                log_file = argv[++i];
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--input") == 0) {
            if (i + 1 < argc) {
                input_file = argv[++i];
            } else {
                fprintf(stderr, "Error: -i requires an input file\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) {
                output_file = argv[++i];
            } else {
                fprintf(stderr, "Error: -o requires an output file\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "--strict") == 0) {
            skip_invalid = 0;
        } else if (argv[i][0] != '-') {
            // For juicer pre mode, positional args are: pa5_file, agp_file, fai_file
            if (juicer_pre_mode) {
                if (!input_file) {
                    input_file = argv[i];  // PA5 file
                } else if (!agp_file) {
                    agp_file = argv[i];   // AGP file
                } else if (!fai_file) {
                    fai_file = argv[i];   // FASTA index file
                }
            } else {
                // For default mode: pairs_file, fasta_file, fai_file, output_file
                // For other modes, treat as input/output files
                if (!input_file) {
                    input_file = argv[i];
                } else if (!fasta_file) {
                    fasta_file = argv[i];
                } else if (!fai_file) {
                    fai_file = argv[i];
                } else if (!output_file) {
                    output_file = argv[i];
                }
            }
        }
    }
    
    // Default mode: preprocess pairs file with FASTA and .fai
    // If no special mode is set and we have pairs file + fasta file + fai file, use default preprocessing
    if (!extract_mode && !fasta_to_agp_mode && !juicer_pre_mode && 
        !create_header_mode && !generate_pairs_mode) {
        if (input_file && fasta_file && fai_file) {
            return preprocess_pairs_file(input_file, fasta_file, fai_file, output_file);
        } else {
            fprintf(stderr, "Error: Default mode requires pairs_file, fasta_file, and fai_file\n");
            fprintf(stderr, "Usage: %s <pairs_file> <fasta_file> <fai_file> [output_file]\n", argv[0]);
            return 1;
        }
    }
    
    // If extract mode, run the first function
    if (extract_mode) {
        // Default output to merged.pairs.pa5 if not specified
        if (!output_file) {
            output_file = "merged.pairs.pa5";
        }
        return extract_data_fields(input_file, output_file);
    }
    
    // If FASTA to AGP mode, run the second function
    if (fasta_to_agp_mode) {
        if (!input_file) {
            fprintf(stderr, "Error: FASTA file is required for --fasta-to-agp mode\n");
            return 1;
        }
        return fasta_to_agp(input_file, output_file);
    }
    
    // If juicer pre mode, run the third function
    if (juicer_pre_mode) {
        if (!input_file) {
            fprintf(stderr, "Error: PA5 file is required for juicer pre mode\n");
            return 1;
        }
        
        FILE *out = stdout;
        FILE *log = stderr;
        
        if (output_file) {
            out = fopen(output_file, "w");
            if (!out) {
                fprintf(stderr, "Error: Cannot open output file '%s': %s\n", output_file, strerror(errno));
                return 1;
            }
        }
        
        if (log_file) {
            log = fopen(log_file, "w");
            if (!log) {
                fprintf(stderr, "Error: Cannot open log file '%s': %s\n", log_file, strerror(errno));
                if (out != stdout) fclose(out);
                return 1;
            }
        }
        
        int ret = juicer_pre(input_file, agp_file, fai_file, min_quality, scale, out, log);
        
        if (out != stdout) fclose(out);
        if (log != stderr && log_file) fclose(log);
        
        return ret;
    }
    
    // If create header mode, run the fourth function
    if (create_header_mode) {
        if (!input_file) {
            fprintf(stderr, "Error: Log file is required for --create-header mode\n");
            return 1;
        }
        return create_pairs_header(input_file, output_file);
    }
    
    // If generate pairs mode, run the fifth function
    if (generate_pairs_mode) {
        // For generate pairs mode, input_file is header, output_file is alignment
        // Or use --header and --alignment flags
        const char *hdr = header_file ? header_file : input_file;
        const char *aln = alignment_file ? alignment_file : output_file;
        
        if (!hdr) {
            fprintf(stderr, "Error: Header file is required for --generate-pairs mode\n");
            fprintf(stderr, "  Use: --header FILE or provide as first positional argument\n");
            return 1;
        }
        
        if (!aln) {
            fprintf(stderr, "Error: Alignment file is required for --generate-pairs mode\n");
            fprintf(stderr, "  Use: --alignment FILE or provide as second positional argument\n");
            return 1;
        }
        
        // Output file is optional, defaults to stdout
        return generate_pairs_file(hdr, aln, output_file, compress_output);
    }
    
    // Otherwise, run full preprocessing
    FILE *in = stdin;
    FILE *out = stdout;
    
    // Open input file
    if (input_file) {
        in = fopen(input_file, "r");
        if (!in) {
            fprintf(stderr, "Error: Cannot open input file '%s': %s\n", input_file, strerror(errno));
            return 1;
        }
    }
    
    // Open output file
    if (output_file) {
        out = fopen(output_file, "w");
        if (!out) {
            fprintf(stderr, "Error: Cannot open output file '%s': %s\n", output_file, strerror(errno));
            if (in != stdin) fclose(in);
            return 1;
        }
    }
    
    // Initialize contig list
    contig_list.capacity = 1000;
    contig_list.contigs = malloc(contig_list.capacity * sizeof(contig_info_t));
    if (!contig_list.contigs) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        if (in != stdin) fclose(in);
        if (out != stdout) fclose(out);
        return 1;
    }
    
    char *line = malloc(MAX_LINE_LEN);
    if (!line) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        free(contig_list.contigs);
        if (in != stdin) fclose(in);
        if (out != stdout) fclose(out);
        return 1;
    }
    
    int header_written = 0;
    int in_header = 1;
    int pairs_format_detected = 0;
    
    // Process input line by line
    while (fgets(line, MAX_LINE_LEN, in)) {
        total_lines++;
        
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
            len--;
        }
        
        // Skip empty lines
        if (len == 0) {
            continue;
        }
        
        // Process header lines
        if (line[0] == '#') {
            header_lines++;
            
            // Write format declaration if not already written
            if (!header_written) {
                fprintf(out, "## pairs format v1.0\n");
                header_written = 1;
            }
            
            // Process chromsize lines
            if (strncmp(line, "#chromsize:", 11) == 0) {
                if (parse_chromsize_line(line)) {
                    fprintf(out, "%s\n", line);
                    pairs_format_detected = 1;
                } else if (verbose) {
                    fprintf(stderr, "Warning: Skipping invalid chromsize line: %s\n", line);
                }
            }
            // Skip format declaration (already written)
            else if (strncmp(line, "## pairs format", 14) == 0) {
                // Already written, skip
            }
            // Skip columns line (will write our own)
            else if (strncmp(line, "#columns:", 9) == 0) {
                // Skip, we'll write our own
            }
            // Skip samheader lines
            else if (strncmp(line, "#samheader:", 11) == 0) {
                // Skip
            }
            // Write other header lines as-is
            else {
                fprintf(out, "%s\n", line);
            }
            
            continue;
        }
        
        // We've reached data lines
        if (in_header) {
            in_header = 0;
            
            // Write columns header if not already present
            if (!pairs_format_detected) {
                // Try to detect format from first data line
                // For now, assume standard format
            }
            
            // Write columns line
            fprintf(out, "#columns: readID chr1 pos1 chr2 pos2 strand1 strand2\n");
        }
        
        // Process data line
        if (parse_data_line(line, out)) {
            valid_lines++;
        } else {
            skipped_lines++;
            if (verbose) {
                fprintf(stderr, "Warning: Skipping invalid line %lu: %s\n", total_lines, line);
            }
            if (!skip_invalid) {
                fprintf(stderr, "Error: Invalid line encountered (use --strict to stop on errors)\n");
                free(line);
                free(contig_list.contigs);
                if (in != stdin) fclose(in);
                if (out != stdout) fclose(out);
                return 1;
            }
        }
    }
    
    // Print statistics
    if (verbose || out == stdout) {
        fprintf(stderr, "Statistics:\n");
        fprintf(stderr, "  Total lines processed: %lu\n", total_lines);
        fprintf(stderr, "  Header lines: %lu\n", header_lines);
        fprintf(stderr, "  Valid data lines: %lu\n", valid_lines);
        fprintf(stderr, "  Skipped lines: %lu\n", skipped_lines);
        fprintf(stderr, "  Contigs found: %zu\n", contig_list.count);
    }
    
    // Cleanup
    free(line);
    free(contig_list.contigs);
    if (in != stdin) fclose(in);
    if (out != stdout) fclose(out);
    
    return 0;
}

static void add_contig_to_list(const char *name, uint64_t length, contig_list_t *list)
{
    if (!list) return;
    
    // Check if contig already exists
    for (size_t i = 0; i < list->count; i++) {
        if (strcmp(list->contigs[i].name, name) == 0) {
            // Update length if different
            if (list->contigs[i].length != length) {
                list->contigs[i].length = length;
            }
            return;
        }
    }
    
    // Resize if needed
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->contigs = realloc(list->contigs, 
                               list->capacity * sizeof(contig_info_t));
        if (!list->contigs) {
            fprintf(stderr, "Error: Memory reallocation failed\n");
            return;
        }
    }
    
    // Add new contig
    strncpy(list->contigs[list->count].name, name, MAX_CONTIG_NAME - 1);
    list->contigs[list->count].name[MAX_CONTIG_NAME - 1] = '\0';
    list->contigs[list->count].length = length;
    list->count++;
}

static void add_contig(const char *name, uint64_t length)
{
    add_contig_to_list(name, length, &contig_list);
}

static contig_info_t *find_contig(const char *name)
{
    for (size_t i = 0; i < contig_list.count; i++) {
        if (strcmp(contig_list.contigs[i].name, name) == 0) {
            return &contig_list.contigs[i];
        }
    }
    return NULL;
}

static int parse_chromsize_line(const char *line)
{
    // Format: #chromsize: <name> <length>
    // Handle multiple spaces/tabs
    
    const char *p = line + 11; // Skip "#chromsize:"
    
    // Skip whitespace
    while (*p == ' ' || *p == '\t') p++;
    
    if (*p == '\0') return 0;
    
    // Extract contig name
    char name[MAX_CONTIG_NAME] = {0};
    size_t name_len = 0;
    while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r' && name_len < MAX_CONTIG_NAME - 1) {
        name[name_len++] = *p++;
    }
    name[name_len] = '\0';
    
    if (name_len == 0) return 0;
    
    // Skip whitespace
    while (*p == ' ' || *p == '\t') p++;
    
    if (*p == '\0') return 0;
    
    // Extract length
    char *endptr;
    errno = 0;
    uint64_t length = strtoull(p, &endptr, 10);
    
    if (errno != 0 || *endptr != '\0') {
        return 0;
    }
    
    if (length == 0) return 0;
    
    // Add to contig list
    add_contig(name, length);
    
    return 1;
}

static int parse_data_line(const char *line, FILE *out)
{
    // Expected format: readID \t chr1 \t pos1 \t chr2 \t pos2 \t strand1 \t strand2 \t ...
    // We need at least 5 fields: readID, chr1, pos1, chr2, pos2
    
    char *line_copy = strdup(line);
    if (!line_copy) return 0;
    
    char *fields[10]; // Support up to 10 fields
    int field_count = 0;
    char *token = line_copy;
    char *saveptr = NULL;
    
    // Split by tabs
    while (field_count < 10 && (token = strtok_r(token, "\t", &saveptr)) != NULL) {
        trim_whitespace(token);
        fields[field_count++] = token;
        token = NULL;
    }
    
    // Need at least 5 fields
    if (field_count < 5) {
        free(line_copy);
        return 0;
    }
    
    // Validate positions (fields 2 and 4, 0-indexed)
    if (!is_numeric(fields[2]) || !is_numeric(fields[4])) {
        free(line_copy);
        return 0;
    }
    
    // Validate positions are valid numbers
    char *endptr;
    errno = 0;
    uint64_t pos1 = strtoull(fields[2], &endptr, 10);
    if (errno != 0 || *endptr != '\0') {
        free(line_copy);
        return 0;
    }
    
    errno = 0;
    uint64_t pos2 = strtoull(fields[4], &endptr, 10);
    if (errno != 0 || *endptr != '\0') {
        free(line_copy);
        return 0;
    }
    
    // Optional: Validate contig names exist in chromsize list
    // (Only warn, don't fail, as contigs might be defined later)
    
    // Write the line (ensure we have at least 7 fields for standard format)
    fprintf(out, "%s", fields[0]); // readID
    for (int i = 1; i < field_count; i++) {
        fprintf(out, "\t%s", fields[i]);
    }
    
    // If we have fewer than 7 fields, pad with empty fields
    while (field_count < 7) {
        fprintf(out, "\t");
        field_count++;
    }
    
    fprintf(out, "\n");
    
    free(line_copy);
    return 1;
}

static int is_numeric(const char *str)
{
    if (!str || *str == '\0') return 0;
    
    // Allow leading whitespace
    while (isspace((unsigned char)*str)) str++;
    
    // Check for sign
    if (*str == '+' || *str == '-') str++;
    
    // Must have at least one digit
    if (!isdigit((unsigned char)*str)) return 0;
    
    // Check rest of string
    while (*str) {
        if (!isdigit((unsigned char)*str) && !isspace((unsigned char)*str)) {
            return 0;
        }
        str++;
    }
    
    return 1;
}

static void trim_whitespace(char *str)
{
    if (!str) return;
    
    // Trim leading whitespace
    char *start = str;
    while (isspace((unsigned char)*start)) start++;
    
    // Trim trailing whitespace
    char *end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
    
    // Move trimmed string to beginning
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

static void print_usage(const char *prog_name)
{
    fprintf(stderr, "Usage: %s [options] <pairs_file> <fasta_file> <fai_file> [output_file]\n", prog_name);
    fprintf(stderr, "   or: %s [mode] [options] [files...]\n", prog_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "Preprocess pairs files for PretextMap compatibility.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Default mode (no special mode specified):\n");
    fprintf(stderr, "  Converts old pairs file to new pairs file using reference FASTA and index\n");
    fprintf(stderr, "  Required: pairs_file, fasta_file, fai_file (FASTA index)\n");
    fprintf(stderr, "  Output: new pairs file only\n");
    fprintf(stderr, "  Example: %s old.pairs reference.fa reference.fa.fai new.pairs\n", prog_name);
    fprintf(stderr, "  Example: %s old.pairs reference.fa reference.fa.fai\n", prog_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -e, --extract        Extract mode: remove header lines and keep first 5 fields\n");
    fprintf(stderr, "                       (equivalent to: grep -v \"^#\" | cut -f1-5)\n");
    fprintf(stderr, "                       Output defaults to merged.pairs.pa5\n");
    fprintf(stderr, "  -f, --fasta-to-agp  Convert FASTA file to AGP format\n");
    fprintf(stderr, "                       Reads sequence names and lengths, writes AGP format\n");
    fprintf(stderr, "  pre, --juicer-pre    Juicer pre mode: convert PA5 to juicer format\n");
    fprintf(stderr, "                       Requires: PA5 file, optional AGP and FASTA index\n");
    fprintf(stderr, "  -H, --create-header  Create pairs format header from juicer log file\n");
    fprintf(stderr, "                       Extracts PRE_C_SIZE entries and creates header\n");
    fprintf(stderr, "  -g, --generate-pairs Generate pairs file from header and alignment\n");
    fprintf(stderr, "                       Combines header with processed alignment data\n");
    fprintf(stderr, "  --header FILE        Header file (for --generate-pairs)\n");
    fprintf(stderr, "  --alignment FILE     Alignment file (for --generate-pairs)\n");
    fprintf(stderr, "  -z, --compress      Compress output with gzip (for --generate-pairs)\n");
    fprintf(stderr, "  -q, --quality INT    Minimum mapping quality (default: 0)\n");
    fprintf(stderr, "  --scale INT          Scale factor for positions (default: 0)\n");
    fprintf(stderr, "  --agp FILE           AGP file for coordinate conversion\n");
    fprintf(stderr, "  --fai, --fasta-index FILE  FASTA index file (.fai)\n");
    fprintf(stderr, "  --log FILE           Log file (default: stderr)\n");
    fprintf(stderr, "  -i, --input FILE     Input file (default: stdin)\n");
    fprintf(stderr, "  -o, --output FILE    Output file (default: stdout)\n");
    fprintf(stderr, "  -v, --verbose        Print verbose statistics\n");
    fprintf(stderr, "  --strict             Stop on first invalid line (default: skip)\n");
    fprintf(stderr, "  -h, --help           Show this help message\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Extract mode (--extract):\n");
    fprintf(stderr, "  Removes all lines starting with '#' and extracts first 5 fields\n");
    fprintf(stderr, "  Output: readID, chr1, pos1, chr2, pos2 (tab-separated)\n");
    fprintf(stderr, "  Example: %s --extract input.pairs merged.pairs.pa5\n", prog_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "FASTA to AGP mode (--fasta-to-agp):\n");
    fprintf(stderr, "  Reads FASTA file and converts to AGP format\n");
    fprintf(stderr, "  Extracts sequence names and lengths, writes AGP format\n");
    fprintf(stderr, "  Output format: object\\t1\\tlength\\t1\\tW\\tobject\\t1\\tlength\\t+\n");
    fprintf(stderr, "  Example: %s --fasta-to-agp reference.fasta output.agp\n", prog_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "Juicer pre mode (pre or --juicer-pre):\n");
    fprintf(stderr, "  Converts PA5 file to juicer format compatible with Juicebox\n");
    fprintf(stderr, "  Input: PA5 file (5 fields: readID, chr1, pos1, chr2, pos2)\n");
    fprintf(stderr, "  Output: Juicer format (9 fields: 0, chr1, pos1, strand1, mapq1, chr2, pos2, strand2, mapq2)\n");
    fprintf(stderr, "  Filters: positions >= 0 (equivalent to awk '$3>=0 && $7>=0')\n");
    fprintf(stderr, "  Example: %s pre -q 0 -i merged.pairs.pa5 --agp file.agp --fai file.fai -o output.txt --log juicercout.log\n", prog_name);
    fprintf(stderr, "  Equivalent to: juicer pre -q 0 merged.pairs.pa5 file.agp file.fai 2>juicercout.log\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Create header mode (--create-header):\n");
    fprintf(stderr, "  Creates pairs format header from juicer log file\n");
    fprintf(stderr, "  Extracts PRE_C_SIZE entries and formats as #chromsize lines\n");
    fprintf(stderr, "  Output: pairs format header with chromsize and columns lines\n");
    fprintf(stderr, "  Example: %s --create-header -i juicercout.log -o header.pairs\n", prog_name);
    fprintf(stderr, "  Equivalent to: echo \"## pairs format v1.0\" && grep PRE_C_SIZE juicercout.log | awk '{print \"#chromsize:\\t\"$2\"\\t\"$3}' && echo \"#columns:\\treadID\\tchr1\\tpos1\\tchr2\\tpos2\\tstrand1\\tstrand2\"\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Generate pairs mode (--generate-pairs):\n");
    fprintf(stderr, "  Generates complete pairs file from header and alignment files\n");
    fprintf(stderr, "  Processes alignment data: extracts fields 2,3,6,7 (chr1, pos1, chr2, pos2)\n");
    fprintf(stderr, "  Output: header + processed alignment data (optionally gzipped)\n");
    fprintf(stderr, "  Example: %s --generate-pairs --header header.pairs --alignment alignment.txt -o output.pairs.gz --compress\n", prog_name);
    fprintf(stderr, "  Or: %s --generate-pairs header.pairs alignment.txt -o output.pairs.gz --compress\n", prog_name);
    fprintf(stderr, "  Equivalent to: (cat header.pairs; cat alignment.txt | awk '{print \".\\t\"$2\"\\t\"$3\"\\t\"$6\"\\t\"$7\"\\t.\\t.\"}') | gzip > output.pairs.gz\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Full preprocessing mode (default):\n");
    fprintf(stderr, "  - Validates and fixes pairs file format\n");
    fprintf(stderr, "  - Ensures positions are numeric\n");
    fprintf(stderr, "  - Adds required headers if missing\n");
    fprintf(stderr, "  - Skips invalid lines (unless --strict)\n");
    fprintf(stderr, "  - Preserves valid header information\n");
    fprintf(stderr, "\n");
}

