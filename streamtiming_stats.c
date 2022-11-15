/** @file streamtiming_stats.c
 */

#include <math.h>
#include <ncurses.h>
#include <sched.h>

#include "CommandLineInterface/CLIcore.h"

#include "COREMOD_memory/COREMOD_memory.h"
#include "COREMOD_tools/COREMOD_tools.h"
#include "timediff.h"

// ==========================================
// Forward declaration(s)
// ==========================================

errno_t info_image_streamtiming_stats_disp(double *tdiffvarray,
        long    NBsamples,
        double  tdiffvmax,
        long    tdiffcntmax);

//
//
errno_t info_image_streamtiming_stats(
    imageID ID, int sem, long NBsamplesmax, float samplestimeout, int buffinit)
{
    long cnt;

    static int     initflag = 0;
    static double *tdiffvarray;

    if(initflag == 0)
    {
        initflag    = 1;
        tdiffvarray = (double *) malloc(sizeof(double) * NBsamplesmax);
    }

    // warmup
    for(cnt = 0; cnt < SEMAPHORE_MAXVAL; cnt++)
    {
        sem_wait(data.image[ID].semptr[sem]);
    }

    // collect timing data
    //long cnt0 = data.image[ID].md[0].cnt0;

    struct timespec tstart;
    struct timespec t0;
    struct timespec t1;
    struct timespec tdiff;
    double          tdiffv;

    sem_wait(data.image[ID].semptr[sem]);
    clock_gettime(CLOCK_REALTIME, &tstart);
    t0.tv_sec  = tstart.tv_sec;
    t0.tv_nsec = tstart.tv_nsec;

    double tdiffvmax   = 0.0;
    long   tdiffcntmax = 0;

    int loopOK = 1;

    static long framecnt     = 0;
    static long framecntbuff = 0;
    //static long NBsamples    = 0;

    if(buffinit == 1)
    {
        framecnt     = 0;
        framecntbuff = 0;
    }

    while(loopOK == 1)
    {
        //for (long framecnt = 0; framecnt < NBsamplesmax; framecnt++)
        sem_wait(data.image[ID].semptr[sem]);
        clock_gettime(CLOCK_REALTIME, &t1);
        tdiff                 = info_time_diff(t0, t1);
        tdiffv                = 1.0 * tdiff.tv_sec + 1.0e-9 * tdiff.tv_nsec;
        tdiffvarray[framecnt] = tdiffv;
        t0.tv_sec             = t1.tv_sec;
        t0.tv_nsec            = t1.tv_nsec;

        if(tdiffv > tdiffvmax)
        {
            tdiffvmax   = tdiffv;
            tdiffcntmax = framecnt;
        }
        framecnt++;
        framecntbuff++;
        if(framecntbuff > NBsamplesmax)
        {
            framecntbuff = NBsamplesmax;
        }
        if(framecnt >= NBsamplesmax)
        {
            framecnt = 0;
        }

        tdiff              = info_time_diff(tstart, t1);
        double tdiffstartv = 1.0 * tdiff.tv_sec + 1.0e-9 * tdiff.tv_nsec;
        if(tdiffstartv > samplestimeout)
        {
            loopOK = 0;
        }
    }

    info_image_streamtiming_stats_disp(tdiffvarray,
                                       framecntbuff,
                                       tdiffvmax,
                                       tdiffcntmax);

    //    free(tdiffvarray);

    return RETURN_SUCCESS;
}




errno_t info_image_streamtiming_stats_disp(double *tdiffvarray,
        long    NBsamples,
        double  tdiffvmax,
        long    tdiffcntmax)
{
    float RMSval = 0.0;
    float AVEval = 0.0;

    static int percMedianIndex;

    static int    initflag = 0;
    static float *percarray;
    static long  *percNarray;
    static int    NBpercbin;

    if(initflag == 0)
    {
        initflag = 1;

        // printw("ALLOCATE arrays\n");
        NBpercbin  = 17;
        percarray  = (float *) malloc(sizeof(float) * NBpercbin);
        percNarray = (long *) malloc(sizeof(long) * NBpercbin);

        percarray[0]  = 0.001;
        percarray[1]  = 0.01;
        percarray[2]  = 0.02;
        percarray[3]  = 0.05;
        percarray[4]  = 0.10;
        percarray[5]  = 0.20;
        percarray[6]  = 0.30;
        percarray[7]  = 0.40;
        percarray[8]  = 0.50;
        percarray[9]  = 0.60;
        percarray[10] = 0.70;
        percarray[11] = 0.80;
        percarray[12] = 0.90;
        percarray[13] = 0.95;
        percarray[14] = 0.98;
        percarray[15] = 0.99;
        percarray[16] = 0.999;

        percMedianIndex = 8;
    }



    for(int pc = 0; pc < NBpercbin; pc++)
    {
        percNarray[pc] = (long)(percarray[pc] * NBsamples);
    }



    // process timing data
    quick_sort_double(tdiffvarray, NBsamples);

    long i;
    for(i = 0; i < NBsamples; i++)
    {
        AVEval += tdiffvarray[i];
        RMSval += tdiffvarray[i] * tdiffvarray[i];
    }
    RMSval = RMSval / NBsamples;
    AVEval /= NBsamples;
    RMSval = sqrt(RMSval - AVEval * AVEval);

    printw("\n NBsamples = %ld \n\n", NBsamples);




    for(int percbin = 0; percbin < NBpercbin; percbin++)
    {
        if(percbin == percMedianIndex)
        {
            attron(A_BOLD);
            #pragma GCC diagnostic ignored "-Wformat"
            printw(
                "%2d/%2d  %6.3f% \%  %6.3f% \%  [%6ld] [%6ld]    %10.3f us\n",
                percbin,
                NBpercbin,
                100.0 * percarray[percbin],
                100.0 * (1.0 - percarray[percbin]),
                percNarray[percbin],
                NBsamples - percNarray[percbin],
                1.0e6 * tdiffvarray[percNarray[percbin]]);
            attroff(A_BOLD);
        }
        else
        {
            if(tdiffvarray[percNarray[percbin]] >
                    1.2 * tdiffvarray[percNarray[percMedianIndex]])
            {
                attron(A_BOLD | COLOR_PAIR(4));
            }
            if(tdiffvarray[percNarray[percbin]] >
                    1.5 * tdiffvarray[percNarray[percMedianIndex]])
            {
                attron(A_BOLD | COLOR_PAIR(5));
            }
            if(tdiffvarray[percNarray[percbin]] >
                    1.99 * tdiffvarray[percNarray[percMedianIndex]])
            {
                attron(A_BOLD | COLOR_PAIR(6));
            }

            printw(
                "%2d/%2d  %6.3f% \%  %6.3f% \%  [%6ld] [%6ld]    %10.3f us   "
                " %+10.3f us\n",
                percbin,
                NBpercbin,
                100.0 * percarray[percbin],
                100.0 * (1.0 - percarray[percbin]),
                percNarray[percbin],
                NBsamples - percNarray[percbin],
                1.0e6 * tdiffvarray[percNarray[percbin]],
                1.0e6 * (tdiffvarray[percNarray[percbin]] -
                         tdiffvarray[percNarray[percMedianIndex]]));
        }
    }
    attroff(A_BOLD | COLOR_PAIR(4));

    printw("\n  Average Time Interval = %10.3f us    -> frequ = %10.3f Hz\n",
           1.0e6 * AVEval,
           1.0 / AVEval);
    printw("                    RMS = %10.3f us  ( %5.3f \%)\n",
           1.0e6 * RMSval,
           100.0 * RMSval / AVEval);
    printw("  Max delay : %10.3f us   frame # %ld\n",
           1.0e6 * tdiffvmax,
           tdiffcntmax);


    //free(percarray);
    //free(percNarray);

    return RETURN_SUCCESS;
}
