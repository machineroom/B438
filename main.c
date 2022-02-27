
#include <stdio.h>
#include <math.h>
#include "LKIO.H"
#include <stdint.h>
#include <cstring>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>
#include "c011.h"
#include <gflags/gflags.h>

#define JOBCOM 0L
#define PRBCOM 1L
#define DATCOM 2L
#define RSLCOM 3L
#define FLHCOM 4L

#define TRUE 1
#define FALSE 0

/* HOST code */

int load_buf(char *, int);
void boot_transputer(void);

DEFINE_bool (verbose, false, "print verbose messages during initialisation");

int main(int argc, char **argv) {
    int i,aok = 1;
    char *s;
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    init_lkio();
    boot_transputer();
    if (FLAGS_verbose) {
        c011_dump_stats("done boot");
    }
    /*if (word_out(0x41) != 0) {
        printf(" -- timeout sending 0x41 test\n");
        exit(1);           
    }
    uint32_t test;
    if (word_in(&test)) {
        printf(" -- timeout getting test word (main)\n");
        exit(1);
    }
    printf("\nfrom transputer code:");
    printf("\n\tword : 0x%X\n",test);
    sleep(5);*/
    return(0);
}


void memdump (char *buf, int cnt) {
    char *p = buf;
    int byte_cnt=0;
    char str[200] = {'\0'};
    while (cnt > 0) {
        char b[10];
        sprintf (b, "0x%02X ", *p++);
        strcat (str,b);
        if (byte_cnt++==16) {
            printf ("%s\n", str);
            str[0] = '\0';
            byte_cnt=0;
        }
        cnt--;
    }
    printf ("%s\n", str);
}

#include "FLBOOT.ARR"
#include "FLLOAD.ARR"
#include "IDENT.ARR"
#include "MANDEL.ARR"

void boot_transputer(void)
{   uint32_t ack, nnodes, word0;

    rst_adpt();
    if (FLAGS_verbose) printf("Booting...\n");
    if (!load_buf(FLBOOT,sizeof(FLBOOT))) exit(1);
    //daughter sends back an ACK word on the booted link
    if (word_in(&ack)) {
        printf(" -- timeout getting ACK\n");
        exit(1);
    }
    printf("ack = 0x%X\n", ack);
    if (ack != 0xB007EEED) {
        printf ("boot ACK not as expected (B007EEED)!\n");
        exit(1);
    }
    sleep(1);
    if (FLAGS_verbose) printf("Loading...\n");      
    if (!load_buf(FLLOAD,sizeof(FLLOAD))) exit(1);
    if (FLAGS_verbose) printf("ID'ing...\n");
    // IDENT will give master node ID 0 and other nodes ID 1
    if (!load_buf(IDENT,sizeof(IDENT))) exit(1);
    if (byte_out(0))
    {
        printf(" -- timeout sending execute\n");
        exit(1);
    }
    if (byte_out(0))
    {
        printf(" -- timeout sending id\n");
        exit(1);
    }
    if (word_in(&nnodes)) {
        printf(" -- timeout getting nnodes (IDENT)\n");
        exit(1);
    }
    printf("\nfrom IDENT");
    printf("\n\tnodes found: %d\n",nnodes);
    if (FLAGS_verbose) printf("\nSending mandel-code");
    if (!load_buf(MANDEL,sizeof(MANDEL))) exit(1);
    if (byte_out(0))
    {
        printf("\n -- timeout sending execute");
        exit(1);
    }
    if (word_in(&nnodes)) {
        printf(" -- timeout getting nnodes (MANDEL)\n");
        exit(1);
    }
    printf("\nfrom transputer code:");
    printf("\n\tnodes found: %d\n",nnodes);
}

/* return TRUE if loaded ok, FALSE if error. */
int load_buf (char *buf, int bcnt) {
    int wok;
    uint8_t len;

    printf ("\ndump buffer that wil be sent to tranny\n");
    memdump (buf, bcnt);
    do {
        len = (bcnt > 255) ? 255 : bcnt;
        bcnt -= len;
        wok = byte_out(len);
        while (len-- && wok==0) {
            uint8_t byte = (uint8_t)*buf++;
            wok = byte_out(byte);
        } 
    } while (bcnt && wok==0);
    if (wok) {
        printf(" -- timeout loading network\n");
        return FALSE;
    } else {
        return TRUE;
    }
}


