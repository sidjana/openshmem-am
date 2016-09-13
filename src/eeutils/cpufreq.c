
#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpufreq.h"
#include <sys/types.h>
#include <unistd.h>
#include "errno.h"

#define MAX_LEN 100


static char temp_str[MAX_LEN];
static FILE *self_freq_fptr ;
struct shmbuf *shmp_self;



FILE*
getCoreCPUFreqDescriptor(int core_id, int caller_id)
{
    static cnt=0;
    cnt++;
    FILE *init_fptr=NULL;
    errno=0;
    
    sprintf(temp_str, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor", core_id);
    init_fptr = fopen(temp_str,"r+");
    if(init_fptr==NULL) {  
	printf("error opening governor file %d for core%d by core%d times=%d \n",
             errno,core_id, caller_id,cnt); 
	exit(EXIT_FAILURE); 
    }
   /* else
	printf("success opening governor file %d for core%d by core%d times=%d \n",
              errno,core_id, caller_id,cnt); 
   
    printf("%d:governor file write stat=%d\n",core_id,fprintf(init_fptr,"userspace"));
 */ fclose(init_fptr);
    sprintf(temp_str, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_setspeed", core_id);
    return  fopen(temp_str, "r+");
}


int 
initDVFSUtils()
{
   int mycoreid = bindSelf2Core();
   self_freq_fptr = getCoreCPUFreqDescriptor(mycoreid, mycoreid); 
   return mycoreid;
}



void
read_cpufreq(FILE* freq_fptr, char* data)
{
   fscanf(freq_fptr, "%s", data);
   fflush(freq_fptr);
   rewind(freq_fptr);
}


void
write_cpufreq(FILE* freq_fptr, char* data)
{
   fprintf(freq_fptr, "%s", data);
   fflush(freq_fptr);
   rewind(freq_fptr);
}


void 
write_self_cpufreq(char* data)
{
    write_cpufreq(self_freq_fptr,data);
}


/* Fortran interface */
void 
write_self_cpufreq_(char *data)
{
    write_self_cpufreq(data);
}


