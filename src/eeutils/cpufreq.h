#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <stdio.h>


int initDVFSUtils();
FILE* getCoreCPUFreqDescriptor(int core_id, int caller_id);
void read_cpufreq(FILE* freq_fptr, char* data);
void write_cpufreq(FILE* freq_fptr, char* data);
void write_self_cpufreq(char* data);

