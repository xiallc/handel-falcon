/* Tests setting basic MCA acquisition values and does a run. FalconX only. */

/*
 * This code accompanies the XIA Code and tests Handel via C.
 *
 * Copyright (c) 2005-2012 XIA LLC
 * All rights reserved
 *
 * Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided
 * that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above
 *     copyright notice, this list of conditions and the
 *     following disclaimer.
 *   * Redistributions in binary form must reproduce the
 *     above copyright notice, this list of conditions and the
 *     following disclaimer in the documentation and/or other
 *     materials provided with the distribution.
 *   * Neither the name of XIA LLC
 *     nor the names of its contributors may be used to endorse
 *     or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include <stdio.h>
#include <stdlib.h>

#include "handel.h"
#include "handel_errors.h"
#include "md_generic.h"

static void CHECK_ERROR(int status);


#ifdef _WIN32
#include <windows.h>
static void sleep(unsigned int msecs)
{
    Sleep((DWORD)msecs);
}
#else
#include <unistd.h>
#endif

#define MAXITEM_LEN 256

static void usage(const char* prog)
{
    printf("%s options\n", prog);
    printf(" -f file       : Handel INI file to load\n");
}


int main(int argc, char** argv)
{
    int status;
    int ignored = 0;

    /* Acquisition Values */
    double pt = 16.0;
    double thresh = 1000.0;
    double calib = 5900.0;
    double dr = 47200.0;
    double mappingMode = 0.0;

    unsigned long mcaLen = 0;

    unsigned long *mca = NULL;

    char ini[MAXITEM_LEN] = "t_api/sandbox/xia_test_helper.ini";
    int a;

    for (a = 1; a < argc; ++a) {
        if (argv[a][0] == '-') {
            switch (argv[a][1]) {
                case 'f':
                    ++a;
                    if (a >= argc) {
                        printf("error: no file provided\n");
                        exit (1);
                    }
                    strncpy(ini, argv[a], sizeof(ini) / sizeof(ini[0]));
                    break;

                default:
                    printf("error: invalid option: %s\n", argv[a]);
                    usage(argv[0]);
                    exit(1);
            }
        }
        else {
            printf("error: invalid option: %s\n", argv[a]);
            usage(argv[0]);
            exit(1);
        }
    }

    /* Setup logging here */
    printf("Configuring the Handel log file.\n");
    xiaSetLogLevel(MD_DEBUG);
    xiaSetLogOutput("handel.log");

    printf("Loading the .ini file.\n");
    status = xiaInit(ini);
    CHECK_ERROR(status);

    /* Boot hardware */
    printf("Starting up the hardware.\n");
    status = xiaStartSystem();
    CHECK_ERROR(status);

    /* Configure acquisition values */
    printf("Setting the acquisition values.\n");
    status = xiaSetAcquisitionValues(-1, "peaking_time", &pt);
    CHECK_ERROR(status);

    status = xiaSetAcquisitionValues(-1, "trigger_threshold", &thresh);
    CHECK_ERROR(status);

    status = xiaSetAcquisitionValues(-1, "calibration_energy", &calib);
    CHECK_ERROR(status);

    status = xiaSetAcquisitionValues(-1, "dynamic_range", &dr);
    CHECK_ERROR(status);

    status = xiaSetAcquisitionValues(-1, "mapping_mode", &mappingMode);
    CHECK_ERROR(status);

    /* Apply new acquisition values */
    printf("Applying the acquisition values.\n");
    status = xiaBoardOperation(0, "apply", &ignored);
    CHECK_ERROR(status);

    /* Start a run w/ the MCA cleared */
    printf("Starting the run.\n");
    status = xiaStartRun(-1, 0);
    CHECK_ERROR(status);

    printf("Waiting 5 seconds to collect data.\n");
    sleep(5000);

    printf("Stopping the run.\n");
    status = xiaStopRun(-1);
    CHECK_ERROR(status);

    /* Prepare to read out MCA spectrum */
    printf("Getting the MCA length.\n");
    status = xiaGetRunData(0, "mca_length", &mcaLen);
    CHECK_ERROR(status);

    /* If you don't want to dynamically allocate memory here,
     * then be sure to declare mca as an array of length 8192,
     * since that is the maximum length of the spectrum.
     */
    printf("Allocating memory for the MCA data.\n");
    mca = malloc(mcaLen * sizeof(unsigned long));

    if (!mca) {
        /* Error allocating memory */
        exit(1);
    }

    printf("Reading the MCA.\n");
    status = xiaGetRunData(0, "mca", mca);
    CHECK_ERROR(status);

    /* Display the spectrum, write it to a file, etc... */

    printf("Release MCA memory.\n");
    free(mca);

    printf("Cleaning up Handel.\n");
    status = xiaExit();
    CHECK_ERROR(status);

    return 0;
}


/*
 * This is just an example of how to handle error values.  A program
 * of any reasonable size should implement a more robust error
 * handling mechanism.
 */
static void CHECK_ERROR(int status)
{
    /* XIA_SUCCESS is defined in handel_errors.h */
    if (status != XIA_SUCCESS) {
        int status2;
        printf("Error encountered (exiting)! Status = %d\n", status);
        status2 = xiaExit();
        if (status2 != XIA_SUCCESS)
            printf("Handel exit failed, Status = %d\n", status2);
        exit(status);
    }
}
