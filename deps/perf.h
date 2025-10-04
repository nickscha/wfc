/* perf.h - v0.5 - public domain data structures - nickscha 2025

A C89 standard compliant, single header, nostdlib (no C Standard Library) simple performance profiler.

LICENSE

  Placed in the public domain and also MIT licensed.
  See end of file for detailed license information.

*/
#ifndef PERF_H
#define PERF_H

/* #############################################################################
 * # COMPILER SETTINGS
 * #############################################################################
 */
/* Check if using C99 or later (inline is supported) */
#if __STDC_VERSION__ >= 199901L
#define PERF_INLINE inline
#define PERF_API static
#elif defined(__GNUC__) || defined(__clang__)
#define PERF_INLINE __inline__
#define PERF_API static
#elif defined(_MSC_VER)
#define PERF_INLINE __inline
#define PERF_API static
#else
#define PERF_INLINE
#define PERF_API static
#endif

PERF_API PERF_INLINE unsigned long perf_strlen(char *str)
{
    char *s = str;
    while (*s)
    {
        s++;
    }
    return (unsigned long)(s - str);
}

PERF_API PERF_INLINE unsigned long perf_append_string(char *dest, unsigned long current_len, unsigned long max_len, char *src)
{
    unsigned long i = 0;
    unsigned long remaining = max_len > current_len ? max_len - current_len : 0;

    if (remaining <= 1) /* Not enough space for even a null terminator */
    {
        return 0;
    }

    /* Leave one byte for the null terminator */
    while (src[i] != '\0' && i < remaining - 1)
    {
        dest[current_len + i] = src[i];
        i++;
    }
    dest[current_len + i] = '\0';

    return i;
}

/* #############################################################################
 * # WIN32 PLATFORM
 * #############################################################################
 */
#ifdef _WIN32
#ifndef _WINDOWS_

typedef union LARGE_INTEGER
{
    unsigned long LowPart;
    long HighPart;
} LARGE_INTEGER;

#define PERF_WIN32_API(r) __declspec(dllimport) r __stdcall

PERF_WIN32_API(int)
QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency);
PERF_WIN32_API(int)
QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount);
PERF_WIN32_API(void *)
GetStdHandle(unsigned long nStdHandle);
PERF_WIN32_API(int)
WriteConsoleA(void *hConsoleOutput, void *lpBuffer, unsigned long nNumberOfCharsToWrite, unsigned long *lpNumberOfCharsWritten, void *lpReserved);

#endif /* _WINDOWS_ (windows.h) */

PERF_API PERF_INLINE double perf_ll_to_double(unsigned long low_part, long high_part)
{
    /* The value of 2 to the power of 32, as a double constant. */
    double two_power_32 = 4294967296.0;

    return ((double)high_part * two_power_32) + (double)low_part;
}

PERF_API PERF_INLINE double perf_platform_current_time_nanoseconds(void)
{
    static LARGE_INTEGER perf_count_frequency;
    static int perf_count_frequency_initialized = 0;

    LARGE_INTEGER counter;
    double counterValue;
    double frequencyValue;

    if (!perf_count_frequency_initialized)
    {
        QueryPerformanceFrequency(&perf_count_frequency);
        perf_count_frequency_initialized = 1;
    }

    QueryPerformanceCounter(&counter);

    /* Convert the 64-bit counter value into a double for precision */
    counterValue = perf_ll_to_double(counter.LowPart, counter.HighPart);

    /* Convert the 64-bit frequency value into a double */
    frequencyValue = perf_ll_to_double(perf_count_frequency.LowPart, perf_count_frequency.HighPart);

    return (counterValue * 1000000000.0) / frequencyValue;
}

PERF_API PERF_INLINE void perf_platform_print(char *str)
{
    unsigned long written;
    void *hConsole = GetStdHandle((unsigned long)-11);
    WriteConsoleA(hConsole, str, perf_strlen(str), &written, ((void *)0));
}

#endif /* _WIN32 */

/* #############################################################################
 * # LINUX/POSIX PLATFORM
 * #############################################################################
 */
#ifdef __linux__
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef __timespec_defined
#define __timespec_defined
struct timespec
{
    long tv_sec;  /* seconds */
    long tv_nsec; /* nanoseconds */
};
#endif

#ifndef __clockid_t_defined
typedef int clockid_t;
#define __clockid_t_defined
#endif

#define CLOCK_MONOTONIC 1
#define SYS_clock_gettime 228 /* On x86_64, check for other architectures */
#define SYS_write 1           /* On x86_64, check for other architectures */
#define STDOUT_FILENO 1

extern long syscall(long number, ...);

/* Use CLOCK_MONOTONIC for stable high-resolution timing */
PERF_API PERF_INLINE double perf_platform_current_time_nanoseconds(void)
{
    struct timespec ts;
    syscall(SYS_clock_gettime, CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000000000.0 + (double)ts.tv_nsec;
}

/* Write directly to stdout without stdio buffering */
PERF_API PERF_INLINE void perf_platform_print(char *str)
{
    unsigned long len = perf_strlen(str);
    syscall(SYS_write, STDOUT_FILENO, str, len);
}

#endif /* __linux__ */

/* #############################################################################
 * # APPLE PLATFORM
 * #############################################################################
 */
#ifdef __APPLE__

#include <mach/mach_time.h>
#include <unistd.h>

PERF_API PERF_INLINE double perf_platform_current_time_nanoseconds(void)
{
    static mach_timebase_info_data_t timebase;
    uint64_t now = mach_absolute_time();
    if (timebase.denom == 0)
    {
        mach_timebase_info(&timebase);
    }
    /* Convert to nanoseconds */
    return (double)now * (double)timebase.numer / (double)timebase.denom;
}

PERF_API PERF_INLINE void perf_platform_print(char *str)
{
    unsigned long len = perf_strlen(str);
    write(STDOUT_FILENO, str, len);
}

#endif /* __APPLE__ */

#if defined(__x86_64__) || defined(__i386__)
PERF_API PERF_INLINE unsigned long perf_platform_current_cycle_count(void)
{
    unsigned long low_part = 0;
    unsigned long high_part = 0;
    __asm __volatile("rdtsc" : "=a"(low_part), "=d"(high_part));
    return ((unsigned long)((double)high_part * 4294967296.0 + (double)low_part));
}
#elif defined(__aarch64__) && defined(__APPLE__)
#include <mach/mach_time.h>
PERF_API PERF_INLINE unsigned long perf_platform_current_cycle_count(void)
{
    return (unsigned long)mach_absolute_time();
}
#endif

/* #############################################################################
 * # String Utility Functions
 * #############################################################################
 */
PERF_API PERF_INLINE void perf_int_to_string(int value, char *buffer, unsigned long max_len)
{
    char temp[12]; /* Max 11 chars for a 32-bit int + null */
    unsigned long i = 0;
    unsigned long j = 0;
    unsigned int uval;
    int is_negative = 0;

    if (max_len == 0)
    {
        return;
    }

    if (value < 0)
    {
        is_negative = 1;
        /* Handle INT_MIN safely without overflow */
        uval = (unsigned int)(-(value + 1)) + 1U;
    }
    else
    {
        uval = (unsigned int)value;
    }

    if (uval == 0)
    {
        temp[i++] = '0';
    }
    else
    {
        while (uval > 0)
        {
            temp[i++] = (char)('0' + (uval % 10));
            uval /= 10;
        }
    }

    if (is_negative)
    {
        temp[i++] = '-';
    }

    /* Reverse into the destination buffer, ensuring it fits */
    while (j < i && j < max_len - 1)
    {
        buffer[j] = temp[i - 1 - j];
        j++;
    }

    buffer[j] = '\0';
}

PERF_API PERF_INLINE void perf_ulong_to_string(unsigned long value, char *buffer, unsigned long max_len)
{
    char temp[21]; /* enough for 64-bit decimal representation */
    unsigned long i = 0;
    unsigned long j;
    unsigned long pad_count;

    if (max_len == 0)
    {
        return; /* can't write anything */
    }

    /* Build number in reverse */
    if (value == 0)
    {
        temp[i++] = '0';
    }
    else
    {
        while (value > 0 && i < sizeof(temp))
        {
            temp[i++] = (char)('0' + (value % 10));
            value /= 10;
        }
    }

    /* If number length > max_len-1, truncate to fit */
    if (i + 1 > max_len)
    {
        i = max_len - 1;
    }

    /* Calculate how many spaces to prepend */
    pad_count = (max_len - 1) - i;

    /* Fill padding spaces */
    for (j = 0; j < pad_count; ++j)
    {
        buffer[j] = ' ';
    }

    /* Reverse number into buffer after padding */
    for (j = 0; j < i; ++j)
    {
        buffer[pad_count + j] = temp[i - j - 1];
    }

    buffer[pad_count + i] = '\0';
}

PERF_API PERF_INLINE void perf_double_to_string(double value, char *buffer, unsigned long max_len, int precision)
{
    char temp[40]; /* holds the number without padding */
    unsigned long temp_index = 0;
    unsigned long j;
    unsigned long pad_count;
    int int_part;
    int negative = 0;

    if (max_len == 0)
    {
        return; /* can't write anything */
    }

    /* Handle sign */
    if (value < 0.0)
    {
        negative = 1;
        value = -value;
    }

    /* Extract integer part */
    int_part = (int)value;
    value -= (double)int_part;

    /* Convert integer part to string */
    if (int_part == 0)
    {
        temp[temp_index++] = '0';
    }
    else
    {
        char int_temp[32];
        unsigned long k = 0;
        while (int_part > 0 && k < sizeof(int_temp))
        {
            int_temp[k++] = (char)((int_part % 10) + '0');
            int_part /= 10;
        }
        /* reverse into temp */
        while (k > 0)
        {
            temp[temp_index++] = int_temp[--k];
        }
    }

    /* Decimal point + fractional part */
    if (precision > 0 && temp_index < sizeof(temp) - 1)
    {
        temp[temp_index++] = '.';
        while (precision-- > 0 && temp_index < sizeof(temp) - 1)
        {
            int digit;
            value *= 10.0;
            digit = (int)value;
            temp[temp_index++] = (char)(digit + '0');
            value -= (double)digit;
        }
    }

    /* Add sign at front of temp if negative */
    if (negative)
    {
        if (temp_index < sizeof(temp) - 1)
        {
            /* shift digits to make room for '-' */
            for (j = temp_index; j > 0; --j)
                temp[j] = temp[j - 1];
            temp[0] = '-';
            temp_index++;
        }
    }

    /* Null terminate temp */
    if (temp_index >= sizeof(temp))
    {
        temp_index = sizeof(temp) - 1;
    }
    temp[temp_index] = '\0';

    /* Truncate to fit max_len - 1 if necessary */
    if (temp_index > max_len - 1)
    {
        temp_index = max_len - 1;
    }

    /* Calculate padding spaces */
    pad_count = (max_len - 1) - temp_index;

    /* Fill spaces first */
    for (j = 0; j < pad_count; ++j)
    {
        buffer[j] = ' ';
    }

    /* Copy number */
    for (j = 0; j < temp_index; ++j)
    {
        buffer[pad_count + j] = temp[j];
    }

    /* Null terminate */
    buffer[pad_count + temp_index] = '\0';
}

/* #############################################################################
 * # PERF MAIN IMPLEMENTATION
 * #############################################################################
 */
#ifndef PERF_MAX_PRINT_BUFFER
#define PERF_MAX_PRINT_BUFFER 1024
#endif

#ifdef PERF_STATS_ENABLE

#ifndef PERF_STATS_ENTRIES_MAX
#define PERF_STATS_ENTRIES_MAX 1024 /* Max unique (file+line+name) combinations */
#endif

#ifndef PERF_STATS_NAME_MAX
#define PERF_STATS_NAME_MAX 512
#endif

typedef struct perf_stats_entry
{
    char file[128];                 /* File name */
    int line;                       /* Line number */
    char name[PERF_STATS_NAME_MAX]; /* Function or code block name */

    unsigned long count;

    unsigned long cycles_min;
    unsigned long cycles_max;
    unsigned long cycles_sum;

    double time_ms_min;
    double time_ms_max;
    double time_ms_sum;

} perf_stats_entry;

static perf_stats_entry perf_stats_entries[PERF_STATS_ENTRIES_MAX];
static unsigned long perf_stats_entry_count = 0;

PERF_API PERF_INLINE perf_stats_entry *perf_stats_get_entry(char *file, int line, char *name)
{
    unsigned long i;
    for (i = 0; i < perf_stats_entry_count; i++)
    {
        if (perf_stats_entries[i].line == line)
        {
            char *f1 = perf_stats_entries[i].file;
            char *f2 = file;
            while (*f1 && *f2 && *f1 == *f2)
            {
                f1++;
                f2++;
            }
            if (*f1 == '\0' && *f2 == '\0')
            {
                /* Now check name */
                char *n1 = perf_stats_entries[i].name;
                char *n2 = name;
                while (*n1 && *n2 && *n1 == *n2)
                {
                    n1++;
                    n2++;
                }
                if (*n1 == '\0' && *n2 == '\0')
                {
                    return &perf_stats_entries[i];
                }
            }
        }
    }

    /* If not found, create a new entry */
    if (perf_stats_entry_count < PERF_STATS_ENTRIES_MAX)
    {
        perf_stats_entry *e = &perf_stats_entries[perf_stats_entry_count++];
        unsigned long j = 0;

        /* Copy file */
        while (file[j] && j < sizeof(e->file) - 1)
        {
            e->file[j] = file[j];
            j++;
        }
        e->file[j] = '\0';

        /* Copy name */
        j = 0;
        while (name[j] && j < PERF_STATS_NAME_MAX - 1)
        {
            e->name[j] = name[j];
            j++;
        }

        e->name[j] = '\0';

        e->line = line;
        e->count = 0;

        e->cycles_min = ~0UL; /* Max unsigned long */
        e->cycles_max = 0;
        e->cycles_sum = 0;

        e->time_ms_min = 1e30; /* Huge number */
        e->time_ms_max = 0.0;
        e->time_ms_sum = 0.0;

        return e;
    }

    return 0; /* No space */
}

PERF_API PERF_INLINE void perf_stats_store_result(char *file, int line, unsigned long cycles, double time_ms, char *name)
{
    perf_stats_entry *e = perf_stats_get_entry(file, line, name);

    if (!e)
    {
        return; /* Out of slots */
    }

    e->count++;
    e->cycles_sum += cycles;
    e->time_ms_sum += time_ms;

    if (cycles < e->cycles_min)
    {
        e->cycles_min = cycles;
    }
    if (cycles > e->cycles_max)
    {
        e->cycles_max = cycles;
    }
    if (time_ms < e->time_ms_min)
    {
        e->time_ms_min = time_ms;
    }
    if (time_ms > e->time_ms_max)
    {
        e->time_ms_max = time_ms;
    }
}

PERF_API PERF_INLINE void perf_print_stats(void)
{
    unsigned long i;

    char buffer[PERF_MAX_PRINT_BUFFER];

    for (i = 0; i < perf_stats_entry_count; ++i)
    {
        unsigned long current_pos = 0;

        perf_stats_entry *e = &perf_stats_entries[i];
        unsigned long avg_cycles = e->count ? e->cycles_sum / e->count : 0;
        double avg_time_ms = e->count ? (e->time_ms_sum / (double)e->count) : 0.0;

        char line_str[5];
        char count_str[7];

        char cycles_min[12];
        char cycles_max[12];
        char cycles_avg[12];
        char cycles_sum[12];

        char time_min[12];
        char time_max[12];
        char time_avg[12];
        char time_sum[12];

        perf_int_to_string(e->line, line_str, sizeof(line_str));
        perf_ulong_to_string(e->count, count_str, sizeof(count_str));

        perf_ulong_to_string(e->cycles_min, cycles_min, sizeof(cycles_min));
        perf_ulong_to_string(e->cycles_max, cycles_max, sizeof(cycles_max));
        perf_ulong_to_string(avg_cycles, cycles_avg, sizeof(cycles_avg));
        perf_ulong_to_string(e->cycles_sum, cycles_sum, sizeof(cycles_sum));

        perf_double_to_string(e->time_ms_min, time_min, sizeof(time_min), 4);
        perf_double_to_string(e->time_ms_max, time_max, sizeof(time_max), 4);
        perf_double_to_string(avg_time_ms, time_avg, sizeof(time_avg), 4);
        perf_double_to_string(e->time_ms_sum, time_sum, sizeof(time_sum), 4);

        if (i == 0)
        {
            buffer[0] = '\0';
            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, e->file);
            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, ":");
            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, line_str);
            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, " [perf]\n");

            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, e->file);
            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, ":");
            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, line_str);
            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, " [perf] +-------------------------------------------------------+-------------------------------------------------------+\n");

            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, e->file);
            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, ":");
            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, line_str);
            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, " [perf] | cylces                                                | time_ms                                               |\n");

            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, e->file);
            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, ":");
            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, line_str);
            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, " [perf] +-------------+-------------+-------------+-------------+-------------+-------------+-------------+-------------+\n");

            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, e->file);
            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, ":");
            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, line_str);
            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, " [perf] |         min |         max |         avg |         sum |         min |         max |         avg |         sum |\n");

            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, e->file);
            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, ":");
            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, line_str);
            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, " [perf] +-------------+-------------+-------------+-------------+-------------+-------------+-------------+-------------+\n");
            current_pos = 0;
            perf_platform_print(buffer);
        }

        buffer[0] = '\0';

        current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, e->file);
        current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, ":");
        current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, line_str);
        current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, " [perf] | ");
        current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, cycles_min);
        current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, " | ");
        current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, cycles_max);
        current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, " | ");
        current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, cycles_avg);
        current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, " | ");
        current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, cycles_sum);
        current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, " | ");
        current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, time_min);
        current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, " | ");
        current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, time_max);
        current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, " | ");
        current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, time_avg);
        current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, " | ");
        current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, time_sum);
        current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, " | ");
        current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, count_str);
        current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, " x ");
        current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, e->name);
        current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, "\n");
        current_pos = 0;

        perf_platform_print(buffer);

        if (i == perf_stats_entry_count - 1)
        {
            buffer[0] = '\0';
            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, e->file);
            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, ":");
            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, line_str);
            current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, " [perf] +-------------+-------------+-------------+-------------+-------------+-------------+-------------+-------------+\n");
            current_pos = 0;
            perf_platform_print(buffer);
        }
    }
}
#else
PERF_API PERF_INLINE void perf_stats_store_result(char *file, int line, unsigned long cycles, double time_ms, char *name)
{
    (void)file;
    (void)line;
    (void)cycles;
    (void)time_ms;
    (void)name;
}
#endif /* PERF_STATS_ENABLE */

#ifdef PERF_DISBALE_INTERMEDIATE_PRINT
PERF_API PERF_INLINE void perf_print_result(char *file, int line, unsigned long cycles, double time_ms, char *name)
{
    (void)file;
    (void)line;
    (void)cycles;
    (void)time_ms;
    (void)name;
}
#else
PERF_API PERF_INLINE void perf_print_result(char *file, int line, unsigned long cycles, double time_ms, char *name)
{
    char buffer[PERF_MAX_PRINT_BUFFER];
    char cycles_str[14];
    char time_ms_str[14];
    char line_str[12];
    unsigned long current_pos = 0;

    /* Format numbers into temporary buffers. */
    perf_ulong_to_string(cycles, cycles_str, sizeof(cycles_str));
    perf_double_to_string(time_ms, time_ms_str, sizeof(time_ms_str), 6);
    perf_int_to_string(line, line_str, sizeof(line_str));

    /* Build the final string safely and efficiently. */
    current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, file);
    current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, ":");
    current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, line_str);
    current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, " [perf] ");
    current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, cycles_str);
    current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, " cycles, ");
    current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, time_ms_str);
    current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, " ms, \"");
    current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, name);
    current_pos += perf_append_string(buffer, current_pos, PERF_MAX_PRINT_BUFFER, "\"\n");

    perf_platform_print(buffer);
}
#endif

#define PERF_PROFILE(func_call) PERF_PROFILE_WITH_NAME(func_call, #func_call)

#ifdef PERF_DISABLE
#define PERF_PROFILE_WITH_NAME(func_call, name) func_call;
#else
#define PERF_PROFILE_WITH_NAME(func_call, name)                                                                 \
    do                                                                                                          \
    {                                                                                                           \
        unsigned long perf_start_cycles, perf_end_cycles;                                                       \
        double perf_start_time_nano, perf_end_time_nano;                                                        \
        double perf_time_ms;                                                                                    \
        perf_start_time_nano = perf_platform_current_time_nanoseconds();                                        \
        perf_start_cycles = perf_platform_current_cycle_count();                                                \
        func_call;                                                                                              \
        perf_end_cycles = perf_platform_current_cycle_count();                                                  \
        perf_end_time_nano = perf_platform_current_time_nanoseconds();                                          \
        perf_time_ms = ((perf_end_time_nano - perf_start_time_nano) / 1000000.0);                               \
        perf_print_result(                                                                                      \
            __FILE__,                                                                                           \
            __LINE__,                                                                                           \
            perf_end_cycles - perf_start_cycles,                                                                \
            perf_time_ms,                                                                                       \
            (name));                                                                                            \
        perf_stats_store_result(__FILE__, __LINE__, perf_end_cycles - perf_start_cycles, perf_time_ms, (name)); \
    } while (0)
#endif

#endif /* PERF_H */

/*
   ------------------------------------------------------------------------------
   This software is available under 2 licenses -- choose whichever you prefer.
   ------------------------------------------------------------------------------
   ALTERNATIVE A - MIT License
   Copyright (c) 2025 nickscha
   Permission is hereby granted, free of charge, to any person obtaining a copy of
   this software and associated documentation files (the "Software"), to deal in
   the Software without restriction, including without limitation the rights to
   use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
   of the Software, and to permit persons to whom the Software is furnished to do
   so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
   ------------------------------------------------------------------------------
   ALTERNATIVE B - Public Domain (www.unlicense.org)
   This is free and unencumbered software released into the public domain.
   Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
   software, either in source code form or as a compiled binary, for any purpose,
   commercial or non-commercial, and by any means.
   In jurisdictions that recognize copyright laws, the author or authors of this
   software dedicate any and all copyright interest in the software to the public
   domain. We make this dedication for the benefit of the public at large and to
   the detriment of our heirs and successors. We intend this dedication to be an
   overt act of relinquishment in perpetuity of all present and future rights to
   this software under copyright law.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
   WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   ------------------------------------------------------------------------------
*/
