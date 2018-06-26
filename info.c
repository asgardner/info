/**
 * @file    info.c
 * @brief   Information about images
 * 
 * Computes information about images
 *  
 * @author  O. Guyon
 * @date    7 Jul 2017
 *
 * 
 * @bug No known bugs.
 * 
 */

#define _GNU_SOURCE

#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h> //for the mkdir options
#include <sys/mman.h>

#ifdef __MACH__
#include <mach/mach_time.h>
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 0
int clock_gettime(int clk_id, struct timespec *t){
    mach_timebase_info_data_t timebase;
    mach_timebase_info(&timebase);
    uint64_t time;
    time = mach_absolute_time();
    double nseconds = ((double)time * (double)timebase.numer)/((double)timebase.denom);
    double seconds = ((double)time * (double)timebase.numer)/((double)timebase.denom * 1e9);
    t->tv_sec = seconds;
    t->tv_nsec = nseconds;
    return 0;
}
#else
#include <time.h>
#endif

#include <sched.h>

#include <fcntl.h> 
#include <termios.h>
#include <sys/types.h> //pid
#include <sys/wait.h> //pid

#include <ncurses.h>
#include <fcntl.h> 

#include <fitsio.h>  /* required by every program that uses CFITSIO  */

#include "CommandLineInterface/CLIcore.h"
#include "00CORE/00CORE.h"
#include "COREMOD_tools/COREMOD_tools.h"
#include "COREMOD_memory/COREMOD_memory.h"
#include "COREMOD_arith/COREMOD_arith.h"
#include "COREMOD_iofits/COREMOD_iofits.h"
#include "fft/fft.h"

#include "info/info.h"





#define SWAP(a,b) temp=(a);(a)=(b);(b)=temp;



//extern DATA data;

static int INITSTATUS_info = 0;



static int wcol, wrow; // window size

static long long cntlast;
static struct timespec tlast;


static int info_image_monitor(const char *ID_name, double frequ);



// CLI commands
//
// function CLI_checkarg used to check arguments
// 1: float
// 2: long
// 3: string
// 4: existing image
//

int_fast8_t info_profile_cli()
{
  if(CLI_checkarg(1,4)+CLI_checkarg(2,3)+CLI_checkarg(3,1)+CLI_checkarg(4,1)+CLI_checkarg(5,1)+CLI_checkarg(6,2)==0)
    {
      profile(data.cmdargtoken[1].val.string, data.cmdargtoken[2].val.string, data.cmdargtoken[3].val.numf, data.cmdargtoken[4].val.numf, data.cmdargtoken[5].val.numf, data.cmdargtoken[6].val.numl);
      return 0;
    }
  else
    return 1;
}


int_fast8_t info_image_monitor_cli()
{
  if(CLI_checkarg(1,4)+CLI_checkarg(2,1)==0)
    {
      info_image_monitor(data.cmdargtoken[1].val.string, data.cmdargtoken[2].val.numf);
      return 0;
    }
  else
    return 1;
}

int_fast8_t info_image_stats_cli()
{
  if(CLI_checkarg(1,4)==0)
    {
      info_image_stats(data.cmdargtoken[1].val.string, "");
      return 0;
    }
  else
    return 1;
}


int_fast8_t info_cubestats_cli()
{
  if(CLI_checkarg(1,4)+CLI_checkarg(2,4)+CLI_checkarg(3,3)==0)
    {
      info_cubestats(data.cmdargtoken[1].val.string, data.cmdargtoken[2].val.string, data.cmdargtoken[3].val.string);
      return 0;
    }
  else
    return 1;
}



int_fast8_t info_image_statsf_cli()
{
  if(CLI_checkarg(1,4)==0)
    {
      info_image_stats(data.cmdargtoken[1].val.string, "fileout");
      return 0;
    }
  else
    return 1;
}





int_fast8_t info_cubeMatchMatrix_cli()
{
	if(CLI_checkarg(1,4)+CLI_checkarg(2,5)==0)
	{
		info_cubeMatchMatrix(data.cmdargtoken[1].val.string, data.cmdargtoken[2].val.string);
		return 0;
	}
	else
		return 1;
}









void __attribute__ ((constructor)) libinit_info()
{
	if ( INITSTATUS_info == 0 )
	{
		init_info();
		RegisterModule(__FILE__, "milk", "Image information and statistics");
		INITSTATUS_info = 1; 
	}
}



int init_info()
{

/* =============================================================================================== */
/*                                                                                                 */
/* 1. GENERAL IMAGE STATS & INFO                                                                   */
/*                                                                                                 */
/* =============================================================================================== */

	RegisterCLIcommand("imstats", __FILE__, info_image_stats_cli, "image stats", "<image>", "imgstats im1", "int info_image_stats(const char *ID_name, \"\")");
	
	RegisterCLIcommand("cubestats", __FILE__,  info_cubestats_cli, "image cube stats", "<3Dimage> <mask> <output file>", "cubestats imc immask imc_stats.txt", "long info_cubestats(const char *ID_name, const char *IDmask_name, const char *outfname)");

	RegisterCLIcommand("imstatsf", __FILE__,  info_image_statsf_cli, "image stats with file output", "<image>", "imgstatsf im1","int info_image_stats(const char *ID_name, \"fileout\")");

/* =============================================================================================== */
/*                                                                                                 */
/* 2. IMAGE MONITORING                                                                             */
/*                                                                                                 */
/* =============================================================================================== */

	RegisterCLIcommand("imgmon", __FILE__,  info_image_monitor_cli, "image monitor", "<image> <frequ>", "imgmon im1 30", "int info_image_monitor(const char *ID_name, double frequ)");


/* =============================================================================================== */
/*                                                                                                 */
/* 3. SIMPLE PROCESSING                                                                            */
/*                                                                                                 */
/* =============================================================================================== */

	RegisterCLIcommand("profile", __FILE__, info_profile_cli, "radial profile", "<image> <output file> <xcenter> <ycenter> <step> <Nbstep>", "profile psf psf.prof 256 256 1.0 100", "int profile(const char *ID_name, const char *outfile, double xcenter, double ycenter, double step, long nb_step)");


	RegisterCLIcommand("cubeslmatch", __FILE__, info_cubeMatchMatrix_cli, "compute sqsum differences between slices", "<imagecube> <output file>", "cubeslmatch incube outim", "long info_cubeMatchMatrix(const char* IDin_name, const char* IDout_name)");


    return 0;
}




struct timespec info_time_diff(struct timespec start, struct timespec end)
{
  struct timespec temp;
  if ((end.tv_nsec-start.tv_nsec)<0) {
    temp.tv_sec = end.tv_sec-start.tv_sec-1;
    temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
  } else {
    temp.tv_sec = end.tv_sec-start.tv_sec;
    temp.tv_nsec = end.tv_nsec-start.tv_nsec;
  }
  return temp;
}



int kbdhit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;
     
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
 
    ch = getchar();
 
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
 
    if(ch != EOF)
      {
	//     ungetc(ch, stdin);
        return 1;
      }
    
    return 0;
}


int print_header(const char *str, char c)
{
    long n;
    long i;

    attron(A_BOLD);
    n = strlen(str);
    for(i=0; i<(wcol-n)/2; i++)
        printw("%c",c);
    printw("%s", str);
    for(i=0; i<(wcol-n)/2-1; i++)
        printw("%c",c);
    printw("\n");
    attroff(A_BOLD);

    return(0);
}




int printstatus(long ID)
{
    struct timespec tnow;
    struct timespec tdiff;
    double tdiffv;
    char str[500];
    char str1[500];

    long j;
    double frequ;
    long NBhistopt = 20;
    long *vcnt;
    long h;
    long cnt, i, ii;

    int customcolor;

    float minPV = 60000;
    float maxPV = 0;
    double average;
    double imtotal;

    int atype;
    char line1[200];

    double tmp;
    double RMS = 0.0;

    double RMS01 = 0.0;
    long vcntmax;
    int semval;
    long s;


    printw("%s  ", data.image[ID].name);

    atype = data.image[ID].md[0].atype;

    if(atype==_DATATYPE_UINT8)
        printw("type:  UINT8             ");
    if(atype==_DATATYPE_INT8)
        printw("type:  INT8              ");

    if(atype==_DATATYPE_UINT16)
        printw("type:  UINT16            ");
    if(atype==_DATATYPE_INT16)
        printw("type:  INT16             ");

    if(atype==_DATATYPE_UINT32)
        printw("type:  UINT32            ");
    if(atype==_DATATYPE_INT32)
        printw("type:  INT32             ");

    if(atype==_DATATYPE_UINT64)
        printw("type:  UINT64            ");
    if(atype==_DATATYPE_INT64)
        printw("type:  INT64             ");

    if(atype==_DATATYPE_FLOAT)
        printw("type:  FLOAT              ");

    if(atype==_DATATYPE_DOUBLE)
        printw("type:  DOUBLE             ");

    if(atype==_DATATYPE_COMPLEX_FLOAT)
        printw("type:  COMPLEX_FLOAT      ");

    if(atype==_DATATYPE_COMPLEX_DOUBLE)
        printw("type:  COMPLEX_DOUBLE     ");

    sprintf(str, "[ %6ld", (long) data.image[ID].md[0].size[0]);

    for(j=1; j<data.image[ID].md[0].naxis; j++)
    {
        sprintf(str1, "%s x %6ld", str, (long) data.image[ID].md[0].size[j]);
        strcpy(str, str1);
    }
    sprintf(str1, "%s]", str);
    strcpy(str, str1);

    printw("%-28s\n", str);




    clock_gettime(CLOCK_REALTIME, &tnow);
    tdiff = info_time_diff(tlast, tnow);
    clock_gettime(CLOCK_REALTIME, &tlast);

    tdiffv = 1.0*tdiff.tv_sec + 1.0e-9*tdiff.tv_nsec;
    frequ = (data.image[ID].md[0].cnt0-cntlast)/tdiffv;
    cntlast = data.image[ID].md[0].cnt0;



    printw("[write %d] ", data.image[ID].md[0].write);
    printw("[status %2d] ", data.image[ID].md[0].status);
    printw("[cnt0 %8d] [%6.2f Hz] ", data.image[ID].md[0].cnt0, frequ);
    printw("[cnt1 %8d]\n", data.image[ID].md[0].cnt1);
 
    printw("[%ld sems ", data.image[ID].md[0].sem);
   for(s=0;s<data.image[ID].md[0].sem;s++)
    {
        sem_getvalue(data.image[ID].semptr[s], &semval);   
        printw(" % 3d ", semval);
    }
    printw("]");
    
    
    sem_getvalue(data.image[ID].semlog, &semval);   
        printw(" [semlog % 3d] ", semval);
    

    printw("\n");



    average = arith_image_mean(data.image[ID].name);
    imtotal = arith_image_total(data.image[ID].name);

	if(atype==_DATATYPE_FLOAT)
		printw("median %12g   ", arith_image_median(data.image[ID].name));

    printw("average %12g    total = %12g\n", imtotal/data.image[ID].md[0].nelement, imtotal);



    vcnt = (long*) malloc(sizeof(long)*NBhistopt);




    if(atype==_DATATYPE_FLOAT)
    {
        minPV = data.image[ID].array.F[0];
        maxPV = minPV;

        for(h=0; h<NBhistopt; h++)
            vcnt[h] = 0;
        for(ii=0; ii<data.image[ID].md[0].nelement; ii++)
        {
            if(data.image[ID].array.F[ii]<minPV)
                minPV = data.image[ID].array.F[ii];
            if(data.image[ID].array.F[ii]>maxPV)
                maxPV = data.image[ID].array.F[ii];
            tmp = (1.0*data.image[ID].array.F[ii]-average);
            RMS += tmp*tmp;
            h = (long) (1.0*NBhistopt*((float) (data.image[ID].array.F[ii]-minPV))/(maxPV-minPV));
            if((h>-1)&&(h<NBhistopt))
                vcnt[h]++;
        }
    }

    if(atype==_DATATYPE_UINT16)
    {
        minPV = data.image[ID].array.UI16[0];
        maxPV = minPV;

        for(h=0; h<NBhistopt; h++)
            vcnt[h] = 0;
        for(ii=0; ii<data.image[ID].md[0].nelement; ii++)
        {
            if(data.image[ID].array.UI16[ii]<minPV)
                minPV = data.image[ID].array.UI16[ii];
            if(data.image[ID].array.UI16[ii]>maxPV)
                maxPV = data.image[ID].array.UI16[ii];
            tmp = (1.0*data.image[ID].array.UI16[ii]-average);
            RMS += tmp*tmp;
            h = (long) (1.0*NBhistopt*((float) (data.image[ID].array.UI16[ii]-minPV))/(maxPV-minPV));
            if((h>-1)&&(h<NBhistopt))
                vcnt[h]++;
        }
    }
    
    if(atype==_DATATYPE_INT16)
    {
        minPV = data.image[ID].array.SI16[0];
        maxPV = minPV;

        for(h=0; h<NBhistopt; h++)
            vcnt[h] = 0;
        for(ii=0; ii<data.image[ID].md[0].nelement; ii++)
        {
            if(data.image[ID].array.SI16[ii]<minPV)
                minPV = data.image[ID].array.SI16[ii];
            if(data.image[ID].array.SI16[ii]>maxPV)
                maxPV = data.image[ID].array.SI16[ii];
            tmp = (1.0*data.image[ID].array.SI16[ii]-average);
            RMS += tmp*tmp;
            h = (long) (1.0*NBhistopt*((float) (data.image[ID].array.SI16[ii]-minPV))/(maxPV-minPV));
            if((h>-1)&&(h<NBhistopt))
                vcnt[h]++;
        }
    }



    RMS = sqrt(RMS/data.image[ID].md[0].nelement);
    RMS01 = 0.9*RMS01 + 0.1*RMS;

    printw("RMS = %12.6g     ->  %12.6g\n", RMS, RMS01);

    print_header(" PIXEL VALUES ", '-');
    printw("min - max   :   %12.6e - %12.6e\n", minPV, maxPV);

    if(data.image[ID].md[0].nelement>25)
    {
        vcntmax = 0;
        for(h=0; h<NBhistopt; h++)
            if(vcnt[h]>vcntmax)
                vcntmax = vcnt[h];


        for(h=0; h<NBhistopt; h++)
        {
            customcolor = 1;
            if(h==NBhistopt-1)
                customcolor = 2;
            sprintf(line1, "[%12.4e - %12.4e] %7ld", (minPV + 1.0*(maxPV-minPV)*h/NBhistopt), (minPV + 1.0*(maxPV-minPV)*(h+1)/NBhistopt), vcnt[h]);

            printw("%s", line1); //(minPV + 1.0*(maxPV-minPV)*h/NBhistopt), (minPV + 1.0*(maxPV-minPV)*(h+1)/NBhistopt), vcnt[h]);
            attron(COLOR_PAIR(customcolor));

            cnt=0;
            i = 0;
            while((cnt<wcol-strlen(line1)-1)&&(i<vcnt[h]))
            {
                printw(" ");
                i += (long) (vcntmax/(wcol-strlen(line1)))+1;
                cnt++;
            }
            attroff(COLOR_PAIR(customcolor));
            printw("\n");
        }
    }
    else
    {
        if(data.image[ID].md[0].atype == _DATATYPE_FLOAT)
        {
            for(ii=0; ii<data.image[ID].md[0].nelement; ii++)
            {
                printw("%3ld  %f\n", ii, data.image[ID].array.F[ii]);
            }
        }

        if(data.image[ID].md[0].atype == _DATATYPE_UINT16)
        {
            for(ii=0; ii<data.image[ID].md[0].nelement; ii++)
            {
                printw("%3ld  %5u\n", ii, data.image[ID].array.UI16[ii]);
            }
        }
        
        if(data.image[ID].md[0].atype == _DATATYPE_INT16)
        {
            for(ii=0; ii<data.image[ID].md[0].nelement; ii++)
            {
                printw("%3ld  %5d\n", ii, data.image[ID].array.SI16[ii]);
            }
        }

    }

    free(vcnt);


    return(0);
}






int info_pixelstats_smallImage(long ID, long NBpix)
{
    long ii;

    if(data.image[ID].md[0].atype == _DATATYPE_FLOAT)
    {
        for(ii=0; ii<NBpix; ii++)
        {
            printw("%3ld  %f\n", ii, data.image[ID].array.F[ii]);
        }
    }

    if(data.image[ID].md[0].atype == _DATATYPE_UINT16)
    {
        for(ii=0; ii<NBpix; ii++)
        {
            printw("%3ld  %5u\n", ii, data.image[ID].array.UI16[ii]);
        }
    }

    return(0);
}





int info_image_streamtiming_stats_disp(
	double *tdiffvarray, 
	long NBsamples, 
	float *percarray, 
	long *percNarray, 
	long NBperccnt, 
	long percMedianIndex, 
	long cntdiff,
	long part,
	long NBpart
	)
{
    long perccnt;
	float RMSval = 0.0;
	float AVEval = 0.0;

    // process timing data
    quick_sort_double(tdiffvarray, NBsamples);

	long i;
	for(i=0; i<NBsamples; i++)
	{
		AVEval += tdiffvarray[i];
		RMSval +=  tdiffvarray[i]*tdiffvarray[i];
	}
	RMSval = RMSval/NBsamples;
	AVEval /= NBsamples;
	RMSval = sqrt(RMSval - AVEval*AVEval);
	
    printw("\n NBsamples = %ld  (cntdiff = %ld)   part %3ld/%3ld   NBperccnt = %ld\n\n", NBsamples, cntdiff, part, NBpart, NBperccnt);



    float perc;
    for(perccnt=0; perccnt<NBperccnt; perccnt++)
    {
        if(perccnt==percMedianIndex)
        {
            attron(A_BOLD);
            printw("%6.3f% \%  %6.3f% \%  [%10ld] [%10ld]    %10.3f us\n",
                   100.0*percarray[perccnt],
                   100.0*(1.0-percarray[perccnt]),
                   percNarray[perccnt],
                   NBsamples - percNarray[perccnt],
                   1.0e6*tdiffvarray[percNarray[perccnt]]);
            attroff(A_BOLD);
        }
        else
        {
			if(tdiffvarray[percNarray[perccnt]] > 1.2 * tdiffvarray[percNarray[percMedianIndex]])
				attron(A_BOLD|COLOR_PAIR(4));
            if(tdiffvarray[percNarray[perccnt]] > 1.5 * tdiffvarray[percNarray[percMedianIndex]])
                attron(A_BOLD|COLOR_PAIR(5));
            if(tdiffvarray[percNarray[perccnt]] > 1.99 * tdiffvarray[percNarray[percMedianIndex]])
                attron(A_BOLD|COLOR_PAIR(6));

            printw("%6.3f% \%  %6.3f% \%  [%10ld] [%10ld]    %10.3f us   %+10.3f us\n",
                   100.0*percarray[perccnt],
                   100.0*(1.0-percarray[perccnt]),
                   percNarray[perccnt],
                   NBsamples - percNarray[perccnt],
                   1.0e6*tdiffvarray[percNarray[perccnt]], 
                   1.0e6*(tdiffvarray[percNarray[perccnt]]-tdiffvarray[percNarray[percMedianIndex]]) );
        }
    }
    attroff(A_BOLD|COLOR_PAIR(4));

	printw("\n  Average Time Interval = %10.3f us    -> frequ = %10.3f Hz\n", 1.0e6*AVEval, 1.0/AVEval);
	printw("                    RMS = %10.3f us  ( %5.3f \%)\n", 1.0e6*RMSval, 100.0*RMSval/AVEval);

    return 0;
}





//
// will drive semaphore to zero, monitor timing
//
// nbiter = -1 for infinite
//
int info_image_streamtiming_stats(const char *ID_name, int sem, long NBsamples, long part, long NBpart)
{
    long ID;
    long cnt;
    struct timespec t0;
    struct timespec t1;
    struct timespec tdiff;
    double tdiffv;


    static double *tdiffvarray;

    static long perccnt;
    static long NBperccnt;
    static long perccntMAX = 1000;
	static int percMedianIndex;
	
    static float *percarray;
    static long *percNarray;


    if(part == 0)
    {
        percarray = (float*) malloc(sizeof(float)*perccntMAX);
        percNarray = (long*) malloc(sizeof(long)*perccntMAX);
        tdiffvarray = (double*) malloc(sizeof(double)*NBsamples);


        perccnt = 0;

        float p;
        float pval;
        long N;

        for(N=1; N<5; N++)
        {
            if(N < NBsamples)
            {
                percNarray[perccnt] = N;
                percarray[perccnt] = 1.0*N/NBsamples;
                perccnt++;
            }
        }


        for(p=0.0001; p<0.1; p*=10.0)
        {
            N = (long) (p * NBsamples);
            if((N>percNarray[perccnt-1])&&(perccnt<perccntMAX-1))
            {
                percNarray[perccnt] = N;
                percarray[perccnt] = 1.0*N/NBsamples;
                perccnt++;
            }
        }

        for(p=0.1; p<0.41; p+=0.1)
        {
            N = (long) (p*NBsamples);
            if((N>percNarray[perccnt-1])&&(perccnt<perccntMAX-1))
            {
                percNarray[perccnt] = N;
                percarray[perccnt] = 1.0*N/NBsamples;
                perccnt++;
            }
        }

        p = 0.5;
        N = (long) (p*NBsamples);
        percNarray[perccnt] = N;
        percarray[perccnt] = 1.0*N/NBsamples;
        percMedianIndex = perccnt;
        perccnt++;

        for(p=0.6; p<0.91; p+=0.1)
        {
            N = (long) (p*NBsamples);
            if((N>percNarray[perccnt-1])&&(perccnt<perccntMAX-1))
            {
                percNarray[perccnt] = N;
                percarray[perccnt] = 1.0*N/NBsamples;
                perccnt++;
            }
        }


        for(p=0.9; p<0.999; p = p + 0.5*(1.0-p))
        {
            N = (long) (p*NBsamples);
            if((N>percNarray[perccnt-1])&&(perccnt<perccntMAX-1))
            {
                percNarray[perccnt] = N;
                percarray[perccnt] = 1.0*N/NBsamples;
                perccnt++;
            }
        }

        long N1;
        for(N1=5; N1>0; N1--)
        {
            N = NBsamples - N1;
            if((N>percNarray[perccnt-1])&&(perccnt<perccntMAX-1)&&(N>0))
            {
                percNarray[perccnt] = N;
                percarray[perccnt] = 1.0*N/NBsamples;
                perccnt++;
            }
        }
        NBperccnt = perccnt;



        int RT_priority = 80; //any number from 0-99
        struct sched_param schedpar;

        schedpar.sched_priority = RT_priority;
#ifndef __MACH__
        sched_setscheduler(0, SCHED_FIFO, &schedpar);
#endif



        ID = image_ID(ID_name);
    } // end of part=0 case


    // warmup
    for(cnt=0; cnt<SEMAPHORE_MAXVAL; cnt++)
        sem_wait(data.image[ID].semptr[sem]);

    // collect timing data
    long cnt0 = data.image[ID].md[0].cnt0;

    sem_wait(data.image[ID].semptr[sem]);
    clock_gettime(CLOCK_REALTIME, &t0);

    for(cnt=0; cnt<NBsamples; cnt++)
    {
        sem_wait(data.image[ID].semptr[sem]);
        clock_gettime(CLOCK_REALTIME, &t1);
        tdiff = info_time_diff(t0, t1);
        tdiffv = 1.0*tdiff.tv_sec + 1.0e-9*tdiff.tv_nsec;
        tdiffvarray[cnt] = tdiffv;
        t0.tv_sec  = t1.tv_sec;
        t0.tv_nsec = t1.tv_nsec;
    }
    long cntdiff = data.image[ID].md[0].cnt0 - cnt0 - 1;

    info_image_streamtiming_stats_disp(tdiffvarray, NBsamples, percarray, percNarray, NBperccnt, percMedianIndex, cntdiff, part, NBpart);

	if(part == NBpart-1)
	{
		free(percarray);
		free(percNarray);
		free(tdiffvarray);
	}

    return 0;
}







int info_image_monitor(
    const char *ID_name,
    double frequ
)
{
    long ID;
    long mode = 0; // 0 for large image, 1 for small image
    long NBpix;
    long npix;
    int sem;
    long cnt;

    int MonMode = 0;
    char monstring[200];

    // 0: image summary
    // 1: timing info for sem


    ID = image_ID(ID_name);
    if(ID==-1)
    {
        printf("Image %s not found in memory\n\n", ID_name);
        fflush(stdout);
    }
    else
    {
        npix = data.image[ID].md[0].nelement;

        /*  Initialize ncurses  */
        if ( initscr() == NULL ) {
            fprintf(stderr, "Error initialising ncurses.\n");
            exit(EXIT_FAILURE);
        }
        getmaxyx(stdscr, wrow, wcol);		/* get the number of rows and columns */
        cbreak();
        keypad(stdscr, TRUE);		/* We get F1, F2 etc..		*/
        nodelay(stdscr, TRUE);
        curs_set(0);
        noecho();			/* Don't echo() while we do getch */



        if(npix<100)
            mode = 1;

        NBpix = npix;
        if(NBpix > wrow)
            NBpix = wrow-4;


        start_color();
        init_pair(1, COLOR_BLACK, COLOR_WHITE);
        init_pair(2, COLOR_BLACK, COLOR_RED);
        init_pair(3, COLOR_GREEN, COLOR_BLACK);
        init_pair(4, COLOR_YELLOW, COLOR_BLACK);
        init_pair(5, COLOR_RED, COLOR_BLACK);
        init_pair(6, COLOR_BLACK, COLOR_RED);
                
        long NBtsamples = 1024;

        cnt = 0;
        int loopOK = 1;
        int freeze = 0;
        
        int part = 0;
        int NBpart = 1;
        
        while( loopOK == 1 )
        {
            usleep((long) (1000000.0/frequ));
            int ch = getch();

            if(freeze==0)
            {
                attron(A_BOLD);
                sprintf(monstring, "Mode %d   PRESS x TO STOP MONITOR", MonMode);
                print_header(monstring, '-');
                attroff(A_BOLD);
            }

            switch (ch)
            {
            case 'f':
                if(freeze==0)
                    freeze = 1;
                else
                    freeze = 0;
                break;

            case 'x':
                loopOK=0;
                break;

            case 's':
                MonMode = 0; // summary
                break;

            case '0':
                MonMode = 1; // Sem timing
                sem = 0;
                break;

            case '1':
                MonMode = 1; // Sem timing
                sem = 1;
                break;

            case '2':
                MonMode = 1; // Sem timing
                sem = 2;
                break;
                
            case '+':
                if(MonMode == 1)
					NBtsamples *= 2;				
                break;
                
            case '-':
                if(MonMode == 1)
					NBtsamples /= 2;				
                break;

            case KEY_UP:
                if(MonMode == 1)
					NBpart++;				
                break;
                
            case KEY_DOWN:
                if(MonMode == 1)
					NBpart--;				
                break;           
            
            }

            if(freeze==0)
            {
                if(MonMode == 0)
                {
                    clear();
                    printstatus(ID);
                }

                if(MonMode == 1)
                {
                   // clear();
                    
                    if(part>NBpart-1)
						part = 0;
                    info_image_streamtiming_stats(ID_name, sem, NBtsamples, part, NBpart);
                    part ++;
                    if(part>NBpart-1)
						part = 0;
                }


                refresh();

                cnt++;
            }
        }
        endwin();
    }
    return 0;
}






long brighter(const char *ID_name, double value) /* number of pixels brighter than value */
{
    int ID;
    long ii,jj;
    long naxes[2];
    long brighter, fainter;

    ID = image_ID(ID_name);
    naxes[0] = data.image[ID].md[0].size[0];
    naxes[1] = data.image[ID].md[0].size[1];

    brighter = 0;
    fainter = 0;
    for (jj = 0; jj < naxes[1]; jj++)
        for (ii = 0; ii < naxes[0]; ii++) {
            if(data.image[ID].array.F[jj*naxes[0]+ii]>value)
                brighter++;
            else
                fainter++;
        }
    printf("brighter %ld   fainter %ld\n", brighter, fainter );

    return(brighter);
}


int img_nbpix_flux(const char *ID_name)
{
  int ID;
  long ii,jj;
  long naxes[2];
  double value = 0;
  double *array;
  long nelements,i;

  ID = image_ID(ID_name);
  naxes[0] = data.image[ID].md[0].size[0];
  naxes[1] = data.image[ID].md[0].size[1];    
  nelements = naxes[0]*naxes[1];
  
  array = (double*) malloc(naxes[1]*naxes[0]*sizeof(double));
  for (jj = 0; jj < naxes[1]; jj++) 
    for (ii = 0; ii < naxes[0]; ii++)
      array[jj*naxes[0]+ii] = data.image[ID].array.F[jj*naxes[0]+ii];

  quick_sort_double(array, nelements);
  
  for(i=0;i<nelements;i++)
    {
      value += array[i];
      printf("%ld  %20.18e\n", i, value);
    }

  free(array);
  return(0);
}

float img_percentile_float(const char *ID_name, float p)
{
    int ID;
    long ii;
    long naxes[2];
    float value = 0;
    float *array;
    long nelements;
    long n;

    ID = image_ID(ID_name);
    naxes[0] = data.image[ID].md[0].size[0];
    naxes[1] = data.image[ID].md[0].size[1];
    nelements = naxes[0]*naxes[1];

    array = (float*) malloc(nelements*sizeof(float));
    for (ii = 0; ii < nelements; ii++)
        array[ii] = data.image[ID].array.F[ii];

    quick_sort_float(array, nelements);

    n = (long) (p*naxes[1]*naxes[0]);
    if(n>(nelements-1))
        n = (nelements-1);
    if(n<0)
        n = 0;
    value = array[n];
    free(array);

    printf("percentile %f = %f (%ld)\n", p, value, n);

    return(value);
}


double img_percentile_double(const char *ID_name, double p)
{
    int ID;
    long ii;
    long naxes[2];
    double value = 0;
    double *array;
    long nelements;
    long n;

    ID = image_ID(ID_name);
    naxes[0] = data.image[ID].md[0].size[0];
    naxes[1] = data.image[ID].md[0].size[1];
    nelements = naxes[0]*naxes[1];

    array = (double*) malloc(nelements*sizeof(double));
    for (ii = 0; ii < nelements; ii++)
        array[ii] = data.image[ID].array.F[ii];

    quick_sort_double(array, nelements);

    n = (long) (p*naxes[1]*naxes[0]);
    if(n>(nelements-1))
        n = (nelements-1);
    if(n<0)
        n = 0;
    value = array[n];
    free(array);

    return(value);
}


double img_percentile(const char *ID_name, double p)
{
    long ID;
    int atype;
    double value = 0.0;

    ID = image_ID(ID_name);
    atype = data.image[ID].md[0].atype;

    if(atype==_DATATYPE_FLOAT)
        value = (double) img_percentile_float(ID_name, (float) p);
    if(atype==_DATATYPE_DOUBLE)
        value = img_percentile_double(ID_name, p);

    return value;
}



int img_histoc_float(const char *ID_name, const char *fname)
{
  FILE *fp;
  int ID;
  long ii,jj;
  long naxes[2];
  float value = 0;
  float *array;
  long nelements;

  ID = image_ID(ID_name);
  naxes[0] = data.image[ID].md[0].size[0];
  naxes[1] = data.image[ID].md[0].size[1];    
  nelements = naxes[0]*naxes[1];
  
  array = (float*) malloc(naxes[1]*naxes[0]*sizeof(float));
  for (jj = 0; jj < naxes[1]; jj++) 
    for (ii = 0; ii < naxes[0]; ii++)
      array[jj*naxes[0]+ii] = data.image[ID].array.F[jj*naxes[0]+ii];

  quick_sort_float(array, nelements);
  
  if((fp=fopen(fname,"w"))==NULL)
    {
      printf("ERROR: cannot open file \"%s\"\n",fname);
      exit(0);
    }
  value = 0.0;
  for(ii=0;ii<nelements;ii++)
    {
      value += array[ii];
      if(ii>0.99*nelements)
	fprintf(fp,"%ld %g %g\n", nelements-ii, value, array[ii]);
    }

  fclose(fp);
  free(array);

  return(0);
}

int img_histoc_double(const char *ID_name, const char *fname)
{
  FILE *fp;
  int ID;
  long ii,jj;
  long naxes[2];
  double value = 0;
  double *array;
  long nelements;

  ID = image_ID(ID_name);
  naxes[0] = data.image[ID].md[0].size[0];
  naxes[1] = data.image[ID].md[0].size[1];    
  nelements = naxes[0]*naxes[1];
  
  array = (double*) malloc(naxes[1]*naxes[0]*sizeof(double));
  for (jj = 0; jj < naxes[1]; jj++) 
    for (ii = 0; ii < naxes[0]; ii++)
      array[jj*naxes[0]+ii] = data.image[ID].array.F[jj*naxes[0]+ii];

  quick_sort_double(array, nelements);
  
  if((fp=fopen(fname,"w"))==NULL)
    {
      printf("ERROR: cannot open file \"%s\"\n",fname);
      exit(0);
    }
  value = 0.0;
  for(ii=0;ii<nelements;ii++)
    {
      value += array[ii];
      if(ii>0.99*nelements)
	fprintf(fp,"%ld %g %g\n",nelements-ii,value,array[ii]);
    }

  fclose(fp);
  free(array);

  return(0);
}


int make_histogram(const char *ID_name, const char *ID_out_name, double min, double max, long nbsteps)
{
    int ID,ID_out;
    long ii,jj;
    long naxes[2];
    long n;

    ID = image_ID(ID_name);
    naxes[0] = data.image[ID].md[0].size[0];
    naxes[1] = data.image[ID].md[0].size[1];

    create_2Dimage_ID(ID_out_name,nbsteps,1);
    ID_out = image_ID(ID_out_name);
    for (jj = 0; jj < naxes[1]; jj++)
        for (ii = 0; ii < naxes[0]; ii++)
        {
            n = (long) ((data.image[ID].array.F[jj*naxes[0]+ii]-min)/(max-min)*nbsteps);
            if((n>0)&&(n<nbsteps))
                data.image[ID_out].array.F[n] += 1;
        }
    return(0);
}


double ssquare(const char *ID_name)
{
    int ID;
    long ii,jj;
    long naxes[2];
    double ssquare;

    ID = image_ID(ID_name);
    naxes[0] = data.image[ID].md[0].size[0];
    naxes[1] = data.image[ID].md[0].size[1];

    ssquare = 0;
    for (jj = 0; jj < naxes[1]; jj++)
        for (ii = 0; ii < naxes[0]; ii++) {
            ssquare = ssquare + data.image[ID].array.F[jj*naxes[0]+ii]*data.image[ID].array.F[jj*naxes[0]+ii];
        }
    return(ssquare);
}

double rms_dev(const char *ID_name)
{
    int ID;
    long ii,jj;
    long naxes[2];
    double ssquare,rms;
    double constant;

    ID = image_ID(ID_name);
    naxes[0] = data.image[ID].md[0].size[0];
    naxes[1] = data.image[ID].md[0].size[1];

    ssquare = 0;
    constant = arith_image_total(ID_name)/naxes[0]/naxes[1];
    for (jj = 0; jj < naxes[1]; jj++)
        for (ii = 0; ii < naxes[0]; ii++) {
            ssquare = ssquare + (data.image[ID].array.F[jj*naxes[0]+ii]-constant)*(data.image[ID].array.F[jj*naxes[0]+ii]-constant);
        }
    rms = sqrt(ssquare/naxes[1]/naxes[0]);
    return(rms);
}





// option "fileout" : output to file imstat.info.txt
int info_image_stats(
		const char *ID_name, 
		const char *options
		)
{
    int ID;
    long ii,jj,j;
    double min,max;
    double rms;
    long nelements;
    double tot;
    double *array;
    long iimin,iimax;
    int atype;
    long tmp_long;
    char type[20];
    char vname[200];
    double xtot,ytot;
    double vbx,vby;
    FILE *fp;
    int mode = 0;

    // printf("OPTIONS = %s\n",options);
    if (strstr(options,"fileout")!=NULL)
        mode = 1;

    if(mode == 1)
        fp = fopen("imstat.info.txt","w");


    ID = image_ID_noaccessupdate(ID_name);
    if(ID!=-1)
    {
        nelements =  data.image[ID].md[0].nelement;

        atype = data.image[ID].md[0].atype;
        tmp_long = data.image[ID].md[0].nelement*TYPESIZE[atype];
        printf("\n");
        printf("Image size (->imsize0...):     [");
        printf("% ld", (long) data.image[ID].md[0].size[0]);
        j = 0;
        sprintf(vname,"imsize%ld",j);

        create_variable_ID(vname,1.0*data.image[ID].md[0].size[j]);
        for(j=1; j<data.image[ID].md[0].naxis; j++)
        {
            printf(" %ld", (long) data.image[ID].md[0].size[j]);
            sprintf(vname,"imsize%ld",j);
            create_variable_ID(vname,1.0*data.image[ID].md[0].size[j]);
        }
        printf(" ]\n");

        printf("write = %d   cnt0 = %ld   cnt1 = %ld\n", data.image[ID].md[0].write, data.image[ID].md[0].cnt0, data.image[ID].md[0].cnt1);



    switch ( atype ) {

    case _DATATYPE_FLOAT:
        sprintf(type,"  FLOAT");
        break;

    case _DATATYPE_INT8:
        sprintf(type,"   INT8");
        break;

    case _DATATYPE_UINT8:
        sprintf(type,"  UINT8");
        break;

    case _DATATYPE_INT16:
        sprintf(type,"  INT16");
        break;

    case _DATATYPE_UINT16:
        sprintf(type," UINT16");
        break;

    case _DATATYPE_INT32:
        sprintf(type,"  INT32");
        break;

    case _DATATYPE_UINT32:
        sprintf(type," UINT32");
        break;

    case _DATATYPE_INT64:
        sprintf(type,"  INT64");
        break;

    case _DATATYPE_UINT64:
        sprintf(type," UINT64");
        break;

    case _DATATYPE_DOUBLE:
        sprintf(type," DOUBLE");
        break;

	case _DATATYPE_COMPLEX_FLOAT:
		sprintf(type,"CFLOAT");
		break;

	case _DATATYPE_COMPLEX_DOUBLE:
		sprintf(type,"CDOUBLE");
		break;

    default:
		sprintf(type,"??????");
        break;
    }




        printf("type:            %s\n", type);
        printf("Memory size:     %ld Kb\n", (long) tmp_long/1024);
        //      printf("Created:         %f\n", data.image[ID].creation_time);
        //      printf("Last access:     %f\n", data.image[ID].last_access);

	if(atype == _DATATYPE_FLOAT)
	{

        min = data.image[ID].array.F[0];
        max = data.image[ID].array.F[0];

        iimin = 0;
        iimax = 0;
        for (ii = 0; ii < nelements; ii++)
        {
            if (min > data.image[ID].array.F[ii])
            {
                min = data.image[ID].array.F[ii];
                iimin = ii;
            }
            if (max < data.image[ID].array.F[ii])
            {
                max = data.image[ID].array.F[ii];
                iimax = ii;
            }
        }

        array = (double*) malloc(nelements*sizeof(double));
        tot = 0.0;

        rms = 0.0;
        for (ii = 0; ii < nelements; ii++)
        {
            if(isnan(data.image[ID].array.F[ii])!=0)
            {
                printf("element %ld is NAN -> replacing by 0\n", ii);
                data.image[ID].array.F[ii] = 0.0;
            }
            tot += data.image[ID].array.F[ii];
            rms += data.image[ID].array.F[ii]*data.image[ID].array.F[ii];
            array[ii] = data.image[ID].array.F[ii];
        }
        rms = sqrt(rms);

        printf("minimum         (->vmin)     %20.18e [ pix %ld ]\n", min, iimin);
        if(mode == 1)
            fprintf(fp,"minimum                  %20.18e [ pix %ld ]\n", min, iimin);
        create_variable_ID("vmin",min);
        printf("maximum         (->vmax)     %20.18e [ pix %ld ]\n", max, iimax);
        if(mode == 1)
            fprintf(fp,"maximum                  %20.18e [ pix %ld ]\n", max, iimax);
        create_variable_ID("vmax",max);
        printf("total           (->vtot)     %20.18e\n",tot);
        if(mode == 1)
            fprintf(fp,"total                    %20.18e\n",tot);
        create_variable_ID("vtot",tot);
        printf("rms             (->vrms)     %20.18e\n",rms);
        if(mode == 1)
            fprintf(fp,"rms                      %20.18e\n",rms);
        create_variable_ID("vrms",rms);
        printf("rms per pixel   (->vrmsp)    %20.18e\n",rms/sqrt(nelements));
        if(mode == 1)
            fprintf(fp,"rms per pixel            %20.18e\n",rms/sqrt(nelements));
        create_variable_ID("vrmsp",rms/sqrt(nelements));
        printf("rms dev per pix (->vrmsdp)   %20.18e\n",sqrt(rms*rms/nelements - tot*tot/nelements/nelements));
        create_variable_ID("vrmsdp",sqrt(rms*rms/nelements - tot*tot/nelements/nelements));
        printf("mean            (->vmean)    %20.18e\n",tot/nelements);
        if(mode == 1)
            fprintf(fp,"mean                     %20.18e\n",tot/nelements);
        create_variable_ID("vmean",tot/nelements);

        if(data.image[ID].md[0].naxis==2)
        {
            xtot = 0.0;
            ytot = 0.0;
            for(ii=0; ii<data.image[ID].md[0].size[0]; ii++)
                for(jj=0; jj<data.image[ID].md[0].size[1]; jj++)
                {
                    xtot += data.image[ID].array.F[jj*data.image[ID].md[0].size[0]+ii]*ii;
                    ytot += data.image[ID].array.F[jj*data.image[ID].md[0].size[0]+ii]*jj;
                }
            vbx = xtot/tot;
            vby = ytot/tot;
            printf("Barycenter x    (->vbx)      %20.18f\n",vbx);
            if(mode == 1)
                fprintf(fp,"photocenterX             %20.18e\n",vbx);
            create_variable_ID("vbx",vbx);
            printf("Barycenter y    (->vby)      %20.18f\n",vby);
            if(mode == 1)
                fprintf(fp,"photocenterY             %20.18e\n",vby);
            create_variable_ID("vby",vby);
        }

        quick_sort_double(array, nelements);
        printf("\n");
        printf("percentile values:\n");

        printf("1  percent      (->vp01)     %20.18e\n",array[(long) (0.01*nelements)]);
        if(mode == 1)
            fprintf(fp,"percentile01             %20.18e\n",array[(long) (0.01*nelements)]);
        create_variable_ID("vp01",array[(long) (0.01*nelements)]);

        printf("5  percent      (->vp05)     %20.18e\n",array[(long) (0.05*nelements)]);
        if(mode == 1)
            fprintf(fp,"percentile05             %20.18e\n",array[(long) (0.05*nelements)]);
        create_variable_ID("vp05",array[(long) (0.05*nelements)]);

        printf("10 percent      (->vp10)     %20.18e\n",array[(long) (0.1*nelements)]);
        if(mode == 1)
            fprintf(fp,"percentile10             %20.18e\n",array[(long) (0.10*nelements)]);
        create_variable_ID("vp10",array[(long) (0.1*nelements)]);

        printf("20 percent      (->vp20)     %20.18e\n",array[(long) (0.2*nelements)]);
        if(mode == 1)
            fprintf(fp,"percentile20             %20.18e\n",array[(long) (0.20*nelements)]);
        create_variable_ID("vp20",array[(long) (0.2*nelements)]);

        printf("50 percent      (->vp50)     %20.18e\n",array[(long) (0.5*nelements)]);
        if(mode == 1)
            fprintf(fp,"percentile50             %20.18e\n",array[(long) (0.50*nelements)]);
        create_variable_ID("vp50",array[(long) (0.5*nelements)]);

        printf("80 percent      (->vp80)     %20.18e\n",array[(long) (0.8*nelements)]);
        if(mode == 1)
            fprintf(fp,"percentile80             %20.18e\n",array[(long) (0.80*nelements)]);
        create_variable_ID("vp80",array[(long) (0.8*nelements)]);

        printf("90 percent      (->vp90)     %20.18e\n",array[(long) (0.9*nelements)]);
        if(mode == 1)
            fprintf(fp,"percentile90             %20.18e\n",array[(long) (0.90*nelements)]);
        create_variable_ID("vp90",array[(long) (0.9*nelements)]);

        printf("95 percent      (->vp95)     %20.18e\n",array[(long) (0.95*nelements)]);
        if(mode == 1)
            fprintf(fp,"percentile95             %20.18e\n",array[(long) (0.95*nelements)]);
        create_variable_ID("vp95",array[(long) (0.95*nelements)]);

        printf("99 percent      (->vp99)     %20.18e\n",array[(long) (0.99*nelements)]);
        if(mode == 1)
            fprintf(fp,"percentile99             %20.18e\n",array[(long) (0.99*nelements)]);
        create_variable_ID("vp99",array[(long) (0.99*nelements)]);

        printf("99.5 percent    (->vp995)    %20.18e\n",array[(long) (0.995*nelements)]);
        if(mode == 1)
            fprintf(fp,"percentile995            %20.18e\n",array[(long) (0.995*nelements)]);
        create_variable_ID("vp995",array[(long) (0.995*nelements)]);

        printf("99.8 percent    (->vp998)    %20.18e\n",array[(long) (0.998*nelements)]);
        if(mode == 1)
            fprintf(fp,"percentile998            %20.18e\n",array[(long) (0.998*nelements)]);
        create_variable_ID("vp998",array[(long) (0.998*nelements)]);

        printf("99.9 percent    (->vp999)    %20.18e\n",array[(long) (0.999*nelements)]);
        if(mode == 1)
            fprintf(fp,"percentile999            %20.18e\n",array[(long) (0.999*nelements)]);
        create_variable_ID("vp999",array[(long) (0.999*nelements)]);

        printf("\n");
        free(array);
    }
	}


    if(mode == 1)
        fclose(fp);

    return(0);
}



// mask pixel values are 0 or 1
// prints:
//		index
//		min
//		max
//		total
//		average
//		tot power
//		RMS
long info_cubestats(const char *ID_name, const char *IDmask_name, const char *outfname)
{
	long ID, IDm;
	long ii, jj, kk;
	float min, max, tot, tot2;
	long xysize;
	FILE *fp;
	int init = 0;
	float mtot;
	float val;

	int COMPUTE_CORR = 1;
	long kcmax = 100;
	double valn1, valn2, v1, v2, valxp, vcorr;
	long k1, k2, kc;
	
	ID = image_ID(ID_name);
	if(data.image[ID].md[0].naxis != 3)
		{
			printf("ERROR: info_cubestats requires 3D image\n");
			exit(0);
		}
	
	IDm = image_ID(IDmask_name);
	
	
	xysize = data.image[ID].md[0].size[0]*data.image[ID].md[0].size[1];
	
	mtot = 0.0;
	for(ii=0;ii<xysize;ii++)
		mtot += data.image[IDm].array.F[ii];
	
	
	fp = fopen(outfname, "w");
	for(kk=0; kk<data.image[ID].md[0].size[2]; kk++)
	{
		init = 0;
		tot = 0.0;
		tot2 = 0.0;
		for(ii=0;ii<xysize;ii++)
			{
				if(data.image[IDm].array.F[ii]>0.5)
				{
					val = data.image[ID].array.F[kk*xysize+ii];
					if(init==0)
					{
						init = 1;
						min = val;
						max = val;
					}
					if(val>max)
						max = val;
					if(val<min)
						min = val;
					tot += val;
					tot2 += val*val;
				}
			}
		fprintf(fp, "%5ld  %20f  %20f  %20f  %20f  %20f  %20f\n", kk, min, max, tot, tot/mtot, tot2, sqrt((tot2-tot*tot/mtot)/mtot));
	}
	fclose(fp);
    
    
    if(COMPUTE_CORR == 1)
    {
		fp = fopen("corr.txt", "w");
		for(kc=1; kc<kcmax; kc++)
		{
			vcorr = 0.0;
			for(kk=0; kk<data.image[ID].md[0].size[2]-kc; kk++)
				{
					k1 = kk;
					k2 = kk+kc;
					valn1 = 0.0;
					valn2 = 0.0;
					valxp = 0.0;
					for(ii=0;ii<xysize;ii++)
						{
							if(data.image[IDm].array.F[ii]>0.5)
								{
									v1 = data.image[ID].array.F[k1*xysize+ii];
									v2 = data.image[ID].array.F[k2*xysize+ii];
									valn1 += v1*v1;
									valn2 += v2*v2;
									valxp += v1*v2;
								}
						}
					vcorr += valxp/sqrt(valn1*valn2);
				}
			vcorr /= data.image[ID].md[0].size[2]-kc;
			fprintf(fp, "%3ld   %g\n", kc, vcorr);
		}
		fclose(fp);
	}
    
	
	return(ID);
}



double img_min(const char *ID_name)
{
  int ID;
  long ii;
  double min;

  ID = image_ID(ID_name);

  min = data.image[ID].array.F[0];
  for (ii = 0; ii < data.image[ID].md[0].nelement; ii++) 
    if (min > data.image[ID].array.F[ii])
      min = data.image[ID].array.F[ii];

   return(min);
}

double img_max(const char *ID_name)
{
  int ID;
  long ii;
  double max;

  ID = image_ID(ID_name);

  max = data.image[ID].array.F[0];
    for (ii = 0; ii < data.image[ID].md[0].nelement; ii++) 
      if (max < data.image[ID].array.F[ii])
	max = data.image[ID].array.F[ii];

   return(max);
}





int profile(const char *ID_name, const char *outfile, double xcenter, double ycenter, double step, long nb_step)
{
  int ID;
  long ii,jj;
  long naxes[2];
  long nelements;
  double distance;
  double *dist;
  double *mean;
  double *rms;
  long *counts;
  FILE *fp;
  long i;

	int *mask;
	long IDmask; // if profmask exists

  ID = image_ID(ID_name);
  naxes[0] = data.image[ID].md[0].size[0];
  naxes[1] = data.image[ID].md[0].size[1];    
  nelements = naxes[0] * naxes[1]; 
  dist = (double*) malloc(nb_step*sizeof(double));
  mean = (double*) malloc(nb_step*sizeof(double));
  rms = (double*) malloc(nb_step*sizeof(double));
  counts = (long*) malloc(nb_step*sizeof(long));
  
  mask = (int*) malloc(sizeof(int)*nelements);
  
	IDmask = image_ID("profmask");
	if(IDmask != -1)
	{
		for(ii=0;ii<nelements;ii++)
			{
				if(data.image[IDmask].array.F[ii]>0.5)
					mask[ii] = 1;
				else
					mask[ii] = 0;
			}
	}
	else
		for(ii=0;ii<nelements;ii++)
			mask[ii] = 1;
  
  //  if( Debug )
  //printf("Function profile. center = %f %f, step = %f, NBstep = %ld\n",xcenter,ycenter,step,nb_step);
  
  for (i=0;i<nb_step;i++)
    {
      dist[i] = 0.0;
      mean[i] = 0.0;
      rms[i] = 0.0;
      counts[i] = 0;
    }

  if ((fp=fopen(outfile,"w"))==NULL)
    printf("error : can't open file %s\n",outfile);
 
  for (jj = 0; jj < naxes[1]; jj++) 
    for (ii = 0; ii < naxes[0]; ii++){
      distance = sqrt((1.0*ii-xcenter)*(1.0*ii-xcenter)+(1.0*jj-ycenter)*(1.0*jj-ycenter));
      i = (long) (distance/step);
      if((i<nb_step)&&(mask[jj*naxes[0]+ii]==1))
		{
		dist[i] += distance;
		mean[i] += data.image[ID].array.F[jj*naxes[0]+ii];
		rms[i] += data.image[ID].array.F[jj*naxes[0]+ii]*data.image[ID].array.F[jj*naxes[0]+ii];
		counts[i] += 1;
		}
    }



  for (i=0;i<nb_step;i++)
    {
      dist[i] /= counts[i];
      mean[i] /= counts[i];
      rms[i] = 0.0;
    }

  for (jj = 0; jj < naxes[1]; jj++) 
    for (ii = 0; ii < naxes[0]; ii++){
      distance = sqrt((1.0*ii-xcenter)*(1.0*ii-xcenter)+(1.0*jj-ycenter)*(1.0*jj-ycenter));
      i = (long) distance/step;
      if((i<nb_step)&&(mask[jj*naxes[0]+ii]==1))
	{
	  rms[i] += (data.image[ID].array.F[jj*naxes[0]+ii]-mean[i])*(data.image[ID].array.F[jj*naxes[0]+ii]-mean[i]);
	  //	  counts[i] += 1;
	}
    }

  
  for (i=0;i<nb_step;i++)
    {
		if(counts[i]>0)
		{
			      //     dist[i] /= counts[i];
			// mean[i] /= counts[i];
			// rms[i] = sqrt(rms[i]-1.0*counts[i]*mean[i]*mean[i])/sqrt(counts[i]);
			rms[i] = sqrt(rms[i]/counts[i]);
			fprintf(fp,"%.18f %.18g %.18g %ld %ld\n", dist[i], mean[i], rms[i], counts[i], i);
		}
    }
  

  fclose(fp);
	free(mask);

  free(counts);
  free(dist);
  free(mean);
  free(rms);
  return(0);
}





int profile2im(const char *profile_name, long nbpoints, long size, double xcenter, double ycenter, double radius, const char *out)
{
  FILE *fp;
  long ID;
  double *profile_array;
  long i;
  long index;
  double tmp;
  long ii,jj;
  double r,x;

  ID = create_2Dimage_ID(out,size,size);
  profile_array = (double*) malloc(sizeof(double)*nbpoints);

  if((fp=fopen(profile_name,"r"))==NULL)
    {
      printf("ERROR: cannot open profile file \"%s\"\n",profile_name);
      exit(0);
    }
  for(i=0;i<nbpoints;i++)
    {
      if(fscanf(fp,"%ld %lf\n",&index,&tmp)!=2)
	{
	  printf("ERROR: fscanf, %s line %d\n",__FILE__,__LINE__);
	  exit(0);
	}
      profile_array[i] = tmp;
    }
  fclose(fp);

  for(ii=0;ii<size;ii++)
    for(jj=0;jj<size;jj++)
      {
	r = sqrt((1.0*ii-xcenter)*(1.0*ii-xcenter)+(1.0*jj-ycenter)*(1.0*jj-ycenter))/radius;
	i = (long) (r*nbpoints);
	x = r*nbpoints-i; // 0<x<1

	if(i+1<nbpoints)
	  {
	    data.image[ID].array.F[jj*size+ii] = (1.0-x)*profile_array[i]+x*profile_array[i+1];
	  }
	else
	  if(i<nbpoints)
	    data.image[ID].array.F[jj*size+ii] = profile_array[i];
      }

  free(profile_array);

  return(0);
}

int printpix(const char *ID_name, const char *filename)
{
  int ID;
  long ii,jj,kk;
  long nelements;
  long nbaxis;
  long naxes[3];
  FILE *fp;

  long iistep = 1;
  long jjstep = 1;

  ID = variable_ID("_iistep");
  if(ID!=-1)
      {
      iistep = (long) (0.1+data.variable[ID].value.f);
      printf("iistep = %ld\n", iistep);
    }
  ID = variable_ID("_jjstep");
  if(ID!=-1)
    {
      jjstep = (long) (0.1+data.variable[ID].value.f);
       printf("jjstep = %ld\n", jjstep);
    }

  if((fp=fopen(filename,"w"))==NULL)
    {
      printf("ERROR: cannot open file \"%s\"\n",filename);
      exit(0);
    }

  ID = image_ID(ID_name);
  nbaxis = data.image[ID].md[0].naxis;
  if(nbaxis==2)
    {
      naxes[0] = data.image[ID].md[0].size[0];
      naxes[1] = data.image[ID].md[0].size[1];    
      nelements = naxes[0] * naxes[1]; 
      for (ii = 0; ii < naxes[0]; ii+=iistep)
	{
	  for (jj = 0; jj < naxes[1]; jj+=jjstep)
	    {
	      //  fprintf(fp,"%f ",data.image[ID].array.F[jj*naxes[0]+ii]);
	      fprintf(fp,"%ld %ld %g\n",ii,jj,data.image[ID].array.F[jj*naxes[0]+ii]);
	    }
	  fprintf(fp,"\n");
	}      
    }
  if(nbaxis==3)
    {
      naxes[0] = data.image[ID].md[0].size[0];
      naxes[1] = data.image[ID].md[0].size[1];    
      naxes[2] = data.image[ID].md[0].size[2];    
      nelements = naxes[0] * naxes[1]; 
      for (ii = 0; ii < naxes[0]; ii+=iistep) 
	for (jj = 0; jj < naxes[1]; jj+=jjstep)
	  for (kk = 0; kk < naxes[2]; kk++)
	    {
	      fprintf(fp,"%ld %ld %ld %f\n",ii,jj,kk,data.image[ID].array.F[kk*naxes[1]*naxes[0]+jj*naxes[0]+ii]);
	    }
    
    }
  fclose(fp);

  return(0);
}

double background_photon_noise(const char *ID_name)
{
  int ID;
  long ii,jj;
  long naxes[2];
  double value1, value2, value3, value;
  double *array;
  long nelements;

  ID = image_ID(ID_name);
  naxes[0] = data.image[ID].md[0].size[0];
  naxes[1] = data.image[ID].md[0].size[1];    
  nelements = naxes[0]*naxes[1];
  
  array = (double*) malloc(naxes[1]*naxes[0]*sizeof(double));
  for (jj = 0; jj < naxes[1]; jj++) 
    for (ii = 0; ii < naxes[0]; ii++)
      array[jj*naxes[0]+ii] = data.image[ID].array.F[jj*naxes[0]+ii];

  quick_sort_double(array,nelements);
  
  /* uses the repartition function F of the normal distribution law */
  /* F(0) = 0.5 */
  /* F(-0.1 * sig) = 0.460172162723 */
  /* F(-0.2 * sig) = 0.420740290562 */
  /* F(-0.3 * sig) = 0.382088577811 */
  /* F(-0.4 * sig) = 0.34457825839 */
  /* F(-0.5 * sig) = 0.308537538726 */
  /* F(-0.6 * sig) = 0.27425311775 */
  /* F(-0.7 * sig) = 0.241963652223 */
  /* F(-0.8 * sig) = 0.211855398584 */
  /* F(-0.9 * sig) = 0.184060125347 */
  /* F(-1.0 * sig) = 0.158655253931 */
  /* F(-1.1 * sig) = 0.135666060946 */
  /* F(-1.2 * sig) = 0.115069670222 */
  /* F(-1.3 * sig) = 0.0968004845855 */

  /* calculation using F(-0.9*sig) and F(-1.3*sig) */
  value1 = array[(long) (0.184060125347*naxes[1]*naxes[0])]-array[(long) (0.0968004845855*naxes[1]*naxes[0])];
  value1 /= (1.3-0.9);
  printf("(-1.3 -0.9) %f\n",value1);

  /* calculation using F(-0.6*sig) and F(-1.3*sig) */
  value2 = array[(long) (0.27425311775*naxes[1]*naxes[0])]-array[(long) (0.0968004845855*naxes[1]*naxes[0])];
  value2 /= (1.3-0.6);
  printf("(-1.3 -0.6) %f\n",value2);

  /* calculation using F(-0.3*sig) and F(-1.3*sig) */
  value3 = array[(long) (0.382088577811*naxes[1]*naxes[0])]-array[(long) (0.0968004845855*naxes[1]*naxes[0])];
  value3 /= (1.3-0.3);
  printf("(-1.3 -0.3) %f\n",value3);

  value = value3;
  
  free(array);
  return(value);
}

int test_structure_function(const char *ID_name, long NBpoints, const char *ID_out)
{
  int ID,ID1,ID2;
  long ii1,ii2,jj1,jj2,i,ii,jj;
  long naxes[2];
  long nelements;
  double v1,v2;

  ID = image_ID(ID_name);
  naxes[0] = data.image[ID].md[0].size[0];
  naxes[1] = data.image[ID].md[0].size[1];    

  nelements = naxes[0]*naxes[1];

  ID1=create_2Dimage_ID("tmp1",naxes[0],naxes[1]);
  ID2=create_2Dimage_ID("tmp2",naxes[0],naxes[1]);
  
  for(i=0;i<NBpoints;i++)
    {
      ii1=(long) (data.INVRANDMAX*rand()*naxes[0]);
      jj1=(long) (data.INVRANDMAX*rand()*naxes[1]);
      ii2=(long) (data.INVRANDMAX*rand()*naxes[0]);
      jj2=(long) (data.INVRANDMAX*rand()*naxes[1]);
      v1=data.image[ID].array.F[jj1*naxes[0]+ii1];
      v2=data.image[ID].array.F[jj2*naxes[0]+ii2];
      ii=(ii1-ii2);
      if(ii<0)
	ii=-ii;
      jj=(jj1-jj2);
      if(jj<0)
	jj=-jj;
      data.image[ID1].array.F[jj*naxes[0]+ii] += (v1-v2)*(v1-v2);
      data.image[ID2].array.F[jj*naxes[0]+ii] += 1.0;
    }
  arith_image_div("tmp1","tmp2",ID_out);


  return(0);
}



int full_structure_function(const char *ID_name, long NBpoints, const char *ID_out)
{
  int ID,ID1,ID2;
  long ii1,ii2,jj1,jj2;
  long naxes[2];
  double v1,v2;
  long i=0;
  long STEP1=2;
  long STEP2=3;

  ID = image_ID(ID_name);
  naxes[0] = data.image[ID].md[0].size[0];
  naxes[1] = data.image[ID].md[0].size[1];    

  ID1=create_2Dimage_ID("tmp1",naxes[0],naxes[1]);
  ID2=create_2Dimage_ID("tmp2",naxes[0],naxes[1]);
  

  for(ii1=0;ii1<naxes[0];ii1+=STEP1)
    {
      printf(".");
      for(jj1=0;jj1<naxes[1];jj1+=STEP1)
	{
	  if(i<NBpoints)
	    {
	      i++;
	      fflush(stdout);
	      for(ii2=0;ii2<naxes[0];ii2+=STEP2)
		for(jj2=0;jj2<naxes[1];jj2+=STEP2)
		  if((ii2>ii1)&&(jj2>jj1))
		    {
		      v1=data.image[ID].array.F[jj1*naxes[0]+ii1];
		      v2=data.image[ID].array.F[jj2*naxes[0]+ii2];
		      data.image[ID1].array.F[(jj2-jj1)*naxes[0]+ii2-ii1] += (v1-v2)*(v1-v2);
		      data.image[ID2].array.F[(jj2-jj1)*naxes[0]+ii2-ii1] += 1.0;
		    }
	    }
	}
    }
  printf("\n");

  arith_image_div("tmp1","tmp2",ID_out);

  return(0);
}



int fft_structure_function(const char *ID_in, const char *ID_out)
{
	long ID;
	double value;
	long nelement;
	int atype;
	
	autocorrelation(ID_in, "stftmp");
	ID = image_ID("stftmp");
	nelement = data.image[ID].md[0].nelement;
	
	atype = data.image[ID].md[0].atype;
	if(atype==_DATATYPE_FLOAT)
		value = -data.image[ID].array.F[0];
	else
		value = -data.image[ID].array.D[0];
	
	arith_image_cstadd("stftmp",value,"stftmp1");
	delete_image_ID("stftmp");
	arith_image_cstmult("stftmp1",-2.0/sqrt(nelement),ID_out);
	delete_image_ID("stftmp1");

	return(0);
}





long info_cubeMatchMatrix(const char* IDin_name, const char* IDout_name)
{
	long IDout;
	long IDin;
	long xsize, ysize, zsize, xysize;
	long ii;
	
	long kk1, kk2;
	long double totv;
	long double v;
	double v1, v2;
	
	FILE *fpout;
	
	long ksize;
	double *array_matchV;
	long *array_matchii;
	long *array_matchjj;
	
	long ID0;
	long k;
	float zfrac = 0.01;
	long IDrmsim;
	
	long kdiffmin = 995;	
	long kdiffmax = 1005;
	long kmax = 10;
	
	IDin = image_ID(IDin_name);
	xsize = data.image[IDin].md[0].size[0];
	ysize = data.image[IDin].md[0].size[1];
	zsize = data.image[IDin].md[0].size[2];
	xysize = xsize*ysize;
	
	
	IDout = image_ID(IDout_name);

	if(IDout == -1)
	{
	create_2Dimage_ID(IDout_name, zsize, zsize);
	IDout = image_ID(IDout_name);
	
	fpout = fopen("outtest.txt", "w");
	fclose(fpout);
	
	printf("Computing differences - cube size is %ld %ld   %ld\n", zsize, zsize, xysize);
	printf("\n\n");
	for(kk1=0; kk1<zsize; kk1++)
	{
		printf("%4ld / %4ld    \n", kk1, zsize);
		fflush(stdout);
		for(kk2=kk1+1; kk2<zsize; kk2++)
			{
				totv = 0.0;
				for(ii=0;ii<xysize;ii++)
					{
						v1 = (double) data.image[IDin].array.F[kk1*xysize + ii];
						v2 = (double) data.image[IDin].array.F[kk2*xysize + ii];
						v = v1-v2;
						totv += v*v; 
//						printf("   %5ld  %20f  ->  %g\n", ii, (double) v, (double) totv);
					}
				printf("    %4ld %4ld   %g\n", kk1, kk2, (double) totv);
				
				fpout = fopen("outtest.txt", "a");
				fprintf(fpout, "%5ld  %20f  %5ld %5ld\n", kk2-kk1, (double) totv, kk1, kk2);
				fclose(fpout);
				
				data.image[IDout].array.F[kk2*zsize + kk1] = (float) totv;
			}
			
		save_fits(IDout_name, "!testout.fits");
	}
	printf("\n");
	}
	else
	{
		zsize = data.image[IDout].md[0].size[0];
	}
	
	ksize = (zsize-1)*(zsize)/2;
	array_matchV = (double*) malloc(sizeof(double)*ksize);
	array_matchii = (long*) malloc(sizeof(long)*ksize);
	array_matchjj = (long*) malloc(sizeof(long)*ksize);

	list_image_ID();
	printf("Reading %ld pixels from ID = %ld\n", (zsize-1)*(zsize)/2, IDout);
	
	ii = 0;
	
	
	
	for(kk1=0; kk1<zsize; kk1++)
		for(kk2=kk1+1; kk2<zsize; kk2++)
			{
				if(ii>ksize-1)
				{
					printf("ERROR: %ld %ld  %ld / %ld\n", kk1, kk2, ii, ksize);
					exit(0);
				}
				if(((double) data.image[IDout].array.F[kk2*zsize+kk1] > 1.0)&&(kk2-kk1 > kdiffmin)&&(kk2-kk1 < kdiffmax))
					{
						array_matchV[ii] = (double) data.image[IDout].array.F[kk2*zsize+kk1];
						array_matchii[ii] = kk1;
						array_matchjj[ii] = kk2;
						ii++;
					}				
			}
	ksize = ii;


	fpout = fopen("outtest.unsorted.txt", "w");
	for(ii=0;ii<ksize;ii++)
		fprintf(fpout, "%5ld  %5ld  %+5ld   %g\n", array_matchii[ii], array_matchjj[ii], array_matchjj[ii]-array_matchii[ii], array_matchV[ii]);
	fclose(fpout);

	
	quick_sort3ll_double(array_matchV, array_matchii, array_matchjj, ksize);
	
	fpout = fopen("outtest.sorted.txt", "w");
	for(ii=0;ii<ksize;ii++)
	{
			fprintf(fpout, "%5ld  %5ld  %+5ld   %g\n", array_matchii[ii], array_matchjj[ii], array_matchjj[ii]-array_matchii[ii], array_matchV[ii]);
	}
	fclose(fpout);


	ID0 = image_ID("imcfull");
	xsize = data.image[ID0].md[0].size[0];
	ysize = data.image[ID0].md[0].size[1];
	xysize = xsize*ysize;

	if(ID0 != -1)
	{
		printf("PROCESSING IMAGE  %ld pixels\n", xysize);
		
		IDrmsim = create_2Dimage_ID("imRMS", xsize, ysize);
		// kmax = (long) (zfrac*ksize);
		printf("KEEPING %ld out of %ld pairs\n", kmax, ksize);
		
		for(k=0;k<kmax;k++)
		{
			kk1 = array_matchii[k];
			kk2 = array_matchjj[k];
			for(ii=0;ii<xysize;ii++)
				{
					v1 = data.image[ID0].array.F[kk1*xysize+ii];
					v2 = data.image[ID0].array.F[kk2*xysize+ii];
					v = v1 - v2;
					data.image[IDrmsim].array.F[ii] += v*v;
				}
		}
		for(ii=0;ii<xysize;ii++)
			data.image[IDrmsim].array.F[ii] = sqrt(data.image[IDrmsim].array.F[ii]/kmax);
		save_fits("imRMS", "!imRMS.fits");
	}


	free(array_matchV);
	free(array_matchii);
	free(array_matchjj);
	
	return(IDout);
}
