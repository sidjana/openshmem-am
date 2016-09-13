

#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <unistd.h>

#define MAX_LEN 100

int
bindSelf2Core()
{
   static int  bound_coreid=-1, is_bound_coreid_set=0;
   FILE* init_fptr;
   char coreid_str[MAX_LEN];
   if (is_bound_coreid_set)
      return bound_coreid;
   else {
      int i;
      sprintf(coreid_str,"/proc/%d/stat",getpid());
      init_fptr = fopen(coreid_str,"r");
      for(i=0; i<39; i++) // core number is the 39th token
          fscanf(init_fptr, "%s ", coreid_str);
      fclose(init_fptr);  
      cpu_set_t  mask;
      CPU_ZERO(&mask);
      CPU_SET(atoi(coreid_str), &mask);
      sched_setaffinity(0, sizeof(mask), &mask);
      bound_coreid = atoi(coreid_str);
      //is_bound_coreid_set=1;
      return (bound_coreid);
   }
}

