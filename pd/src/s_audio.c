/* Copyright (c) 2003, Miller Puckette and others.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/*  machine-independent (well, mostly!) audio layer.  Stores and recalls
    audio settings from argparse routine and from dialog window. 
*/

#include "m_pd.h"
#include "s_stuff.h"
#include <stdio.h>
#ifdef UNISTD
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define SYS_DEFAULTCH 2
#define SYS_MAXCH 100
typedef long t_pa_sample;
#define SYS_SAMPLEWIDTH sizeof(t_pa_sample)
#define SYS_BYTESPERCHAN (DEFDACBLKSIZE * SYS_SAMPLEWIDTH) 
#define SYS_XFERSAMPS (SYS_DEFAULTCH*DEFDACBLKSIZE)
#define SYS_XFERSIZE (SYS_SAMPLEWIDTH * SYS_XFERSAMPS)
#define MAXNDEV 20
#define DEVDESCSIZE 80

static void audio_getdevs(char *indevlist, int *nindevs,
    char *outdevlist, int *noutdevs, int *canmulti, 
        int maxndev, int devdescsize);

    /* these are set in this file when opening audio, but then may be reduced,
    even to zero, in the system dependent open_audio routines. */
int sys_inchannels;
int sys_outchannels;
int sys_advance_samples;        /* scheduler advance in samples */
int sys_blocksize = 0;          /* audio I/O block size in sample frames */
int sys_audioapi = API_DEFAULT;

static int sys_meters;          /* true if we're metering */
static float sys_inmax;         /* max input amplitude */
static float sys_outmax;        /* max output amplitude */

    /* exported variables */
int sys_schedadvance;   /* scheduler advance in microseconds */
float sys_dacsr;

t_sample *sys_soundout;
t_sample *sys_soundin;

    /* the "state" is normally one if we're open and zero otherwise; 
    but if the state is one, we still haven't necessarily opened the
    audio hardware; see audio_isopen() below. */
static int audio_state;

    /* last requested parameters */
static int audio_naudioindev = -1;
static int audio_audioindev[MAXAUDIOINDEV];
static int audio_audiochindev[MAXAUDIOINDEV];
static int audio_naudiooutdev = -1;
static int audio_audiooutdev[MAXAUDIOOUTDEV];
static int audio_audiochoutdev[MAXAUDIOOUTDEV];
static int audio_rate;
static int audio_advance;

static int audio_isopen(void)
{
    return (audio_state &&
        ((audio_naudioindev > 0 && audio_audiochindev[0] > 0) 
            || (audio_naudiooutdev > 0 && audio_audiochoutdev[0] > 0)));
}

void sys_get_audio_params(
    int *pnaudioindev, int *paudioindev, int *chindev,
    int *pnaudiooutdev, int *paudiooutdev, int *choutdev,
    int *prate, int *padvance)
{
    int i;
    *pnaudioindev = audio_naudioindev;
    for (i = 0; i < MAXAUDIOINDEV; i++)
        paudioindev[i] = audio_audioindev[i],
            chindev[i] = audio_audiochindev[i]; 
    *pnaudiooutdev = audio_naudiooutdev;
    for (i = 0; i < MAXAUDIOOUTDEV; i++)
        paudiooutdev[i] = audio_audiooutdev[i],
            choutdev[i] = audio_audiochoutdev[i]; 
    *prate = audio_rate;
    *padvance = audio_advance;
}

void sys_save_audio_params(
    int naudioindev, int *audioindev, int *chindev,
    int naudiooutdev, int *audiooutdev, int *choutdev,
    int rate, int advance)
{
    int i;
    audio_naudioindev = naudioindev;
    for (i = 0; i < MAXAUDIOINDEV; i++)
        audio_audioindev[i] = audioindev[i],
            audio_audiochindev[i] = chindev[i]; 
    audio_naudiooutdev = naudiooutdev;
    for (i = 0; i < MAXAUDIOOUTDEV; i++)
        audio_audiooutdev[i] = audiooutdev[i],
            audio_audiochoutdev[i] = choutdev[i]; 
    audio_rate = rate;
    audio_advance = advance;
}

    /* init routines for any API which needs to set stuff up before
    any other API gets used.  This is only true of OSS so far. */
#ifdef USEAPI_OSS
void oss_init(void);
#endif

static void audio_init( void)
{
    static int initted = 0;
    if (initted)
        return;
    initted = 1;
#ifdef USEAPI_OSS
    oss_init();
#endif
} 

    /* set channels and sample rate.  */

void sys_setchsr(int chin, int chout, int sr)
{
    int nblk;
    int inbytes = (chin ? chin : 2) * (DEFDACBLKSIZE*sizeof(float));
    int outbytes = (chout ? chout : 2) * (DEFDACBLKSIZE*sizeof(float));

    sys_inchannels = chin;
    sys_outchannels = chout;
    sys_dacsr = sr;
    sys_advance_samples = (sys_schedadvance * sys_dacsr) / (1000000.);
    if (sys_advance_samples < 3 * DEFDACBLKSIZE)
        sys_advance_samples = 3 * DEFDACBLKSIZE;

    if (sys_soundin)
        free(sys_soundin);
    sys_soundin = (t_float *)malloc(inbytes);
    memset(sys_soundin, 0, inbytes);

    if (sys_soundout)
        free(sys_soundout);
    sys_soundout = (t_float *)malloc(outbytes);
    memset(sys_soundout, 0, outbytes);

    if (sys_verbose)
        post("input channels = %d, output channels = %d",
            sys_inchannels, sys_outchannels);
    canvas_resume_dsp(canvas_suspend_dsp());
}

/* ----------------------- public routines ----------------------- */

    /* open audio devices (after cleaning up the specified device and channel
    vectors).  The audio devices are "zero based" (i.e. "0" means the first
    one.)  We also save the cleaned-up device specification so that we
    can later re-open audio and/or show the settings on a dialog window. */

void sys_open_audio(int naudioindev, int *audioindev, int nchindev,
    int *chindev, int naudiooutdev, int *audiooutdev, int nchoutdev,
    int *choutdev, int rate, int advance, int enable)
{
    int i, *ip;
    int defaultchannels = SYS_DEFAULTCH;
    int inchans, outchans, nrealindev, nrealoutdev;
    int realindev[MAXAUDIOINDEV], realoutdev[MAXAUDIOOUTDEV];
    int realinchans[MAXAUDIOINDEV], realoutchans[MAXAUDIOOUTDEV];

    char indevlist[MAXNDEV*DEVDESCSIZE], outdevlist[MAXNDEV*DEVDESCSIZE];
    int indevs = 0, outdevs = 0, canmulti = 0;
    audio_getdevs(indevlist, &indevs, outdevlist, &outdevs, &canmulti,
        MAXNDEV, DEVDESCSIZE);

    if (sys_externalschedlib)
    {
        return;
    }
        /* if we're already open close it */
    if (sys_inchannels || sys_outchannels)
        sys_close_audio();

    if (rate < 1)
        rate = DEFAULTSRATE;
    if (advance <= 0)
        advance = DEFAULTADVANCE;
     audio_init();
        /* Since the channel vector might be longer than the
        audio device vector, or vice versa, we fill the shorter one
        in to match the longer one.  Also, if both are empty, we fill in
        one device (the default) and two channels. */ 
    if (naudioindev == -1)
    {           /* no input audio devices specified */
        if (nchindev == -1)
        {
            if (indevs >= 1)
            {
                nchindev=1;
                chindev[0] = defaultchannels;
                naudioindev = 1;
                audioindev[0] = DEFAULTAUDIODEV;
            }
            else naudioindev = nchindev = 0;
        }
        else
        {
            for (i = 0; i < MAXAUDIOINDEV; i++)
                audioindev[i] = i;
            naudioindev = nchindev;
        }
    }
    else
    {
        if (nchindev == -1)
        {
            nchindev = naudioindev;
            for (i = 0; i < naudioindev; i++)
                chindev[i] = defaultchannels;
        }
        else if (nchindev > naudioindev)
        {
            for (i = naudioindev; i < nchindev; i++)
            {
                if (i == 0)
                    audioindev[0] = DEFAULTAUDIODEV;
                else audioindev[i] = audioindev[i-1] + 1;
            }
            naudioindev = nchindev;
        }
        else if (nchindev < naudioindev)
        {
            for (i = nchindev; i < naudioindev; i++)
            {
                if (i == 0)
                    chindev[0] = defaultchannels;
                else chindev[i] = chindev[i-1];
            }
            naudioindev = nchindev;
        }
    }

    if (naudiooutdev == -1)
    {           /* not set */
        if (nchoutdev == -1)
        {
            if (outdevs >= 1)
            {
                nchoutdev=1;
                choutdev[0]=defaultchannels;
                naudiooutdev=1;
                audiooutdev[0] = DEFAULTAUDIODEV;
            }
            else nchoutdev = naudiooutdev = 0;
        }
        else
        {
            for (i = 0; i < MAXAUDIOOUTDEV; i++)
                audiooutdev[i] = i;
            naudiooutdev = nchoutdev;
        }
    }
    else
    {
        if (nchoutdev == -1)
        {
            nchoutdev = naudiooutdev;
            for (i = 0; i < naudiooutdev; i++)
                choutdev[i] = defaultchannels;
        }
        else if (nchoutdev > naudiooutdev)
        {
            for (i = naudiooutdev; i < nchoutdev; i++)
            {
                if (i == 0)
                    audiooutdev[0] = DEFAULTAUDIODEV;
                else audiooutdev[i] = audiooutdev[i-1] + 1;
            }
            naudiooutdev = nchoutdev;
        }
        else if (nchoutdev < naudiooutdev)
        {
            for (i = nchoutdev; i < naudiooutdev; i++)
            {
                if (i == 0)
                    choutdev[0] = defaultchannels;
                else choutdev[i] = choutdev[i-1];
            }
            naudiooutdev = nchoutdev;
        }
    }
    
        /* count total number of input and output channels */
    for (i = nrealindev = inchans = 0; i < naudioindev; i++)
        if (chindev[i] > 0)
    {
        realinchans[nrealindev] = chindev[i];
        realindev[nrealindev] = audioindev[i];
        inchans += chindev[i];
        nrealindev++;
    }
    for (i = nrealoutdev = outchans = 0; i < naudiooutdev; i++)
        if (choutdev[i] > 0)
    {
        realoutchans[nrealoutdev] = choutdev[i];
        realoutdev[nrealoutdev] = audiooutdev[i];
        outchans += choutdev[i];
        nrealoutdev++;
    }
        /* if no input or output devices seem to have been specified,
        this really means just disable audio, which we now do. */
    if (!inchans && !outchans)
        enable = 0;
    sys_schedadvance = advance * 1000;
    sys_setchsr(inchans, outchans, rate);
    sys_log_error(ERR_NOTHING);
    if (enable)
    {
#ifdef USEAPI_PORTAUDIO
        if (sys_audioapi == API_PORTAUDIO)
        {
            int blksize = (sys_blocksize ? sys_blocksize : 64);
            pa_open_audio(inchans, outchans, rate, sys_soundin, sys_soundout,
                blksize, sys_advance_samples/blksize,
                    (naudiooutdev > 0 ? audioindev[0] : 0),
                        (naudiooutdev > 0 ? audiooutdev[0] : 0));
        }
else
#endif
#ifdef USEAPI_JACK
        if (sys_audioapi == API_JACK) 
            jack_open_audio((nrealindev > 0 ? realinchans[0] : 0),
                (nrealoutdev > 0 ? realoutchans[0] : 0), rate);

        else
#endif    
#ifdef USEAPI_OSS
        if (sys_audioapi == API_OSS)
            oss_open_audio(nrealindev, realindev, nrealindev, realinchans,
                nrealoutdev, realoutdev, nrealoutdev, realoutchans, rate);
        else
#endif
#ifdef USEAPI_ALSA
            /* for alsa, only one device is supported; it may
            be open for both input and output. */
        if (sys_audioapi == API_ALSA)
            alsa_open_audio(nrealindev, audioindev, nrealindev, realinchans,
                nrealoutdev, audiooutdev, nrealoutdev, realoutchans, rate);
        else 
#endif
#ifdef USEAPI_SGI
        if (sys_audioapi == API_SGI)
        {
            xtern int sgi_open_audio(int nindev,  int *indev,  int nchin,
                int *chin,  int noutdev, int *outdev, int nchout, int *chout,
                int rate);
            sgi_open_audio(naudioindev, audioindev, nchindev, chindev,
                    naudiooutdev, audiooutdev, nchoutdev, choutdev, rate);
        }
        else
#endif
#ifdef USEAPI_MMIO
        if (sys_audioapi == API_MMIO)
            mmio_open_audio(nrealindev, audioindev, nrealindev, realinchans,
                nrealoutdev, audiooutdev, nrealoutdev, realoutchans, rate);
        else
#endif
            post("unknown audio API specified");
    }
    sys_save_audio_params(naudioindev, audioindev, chindev,
        naudiooutdev, audiooutdev, choutdev, sys_dacsr, advance);
    if (sys_inchannels == 0 && sys_outchannels == 0)
        enable = 0;
    audio_state = enable;
    sys_vgui("set pd_whichapi %d\n",  (audio_isopen() ? sys_audioapi : 0));
    sched_set_using_dacs(enable);
}

void sys_close_audio(void)
{
    if (sys_externalschedlib)
    {
        return;
    }
    if (!audio_isopen())
        return;
#ifdef USEAPI_PORTAUDIO
    if (sys_audioapi == API_PORTAUDIO)
        pa_close_audio();
    else 
#endif
#ifdef USEAPI_JACK
    if (sys_audioapi == API_JACK)
        jack_close_audio();
    else
#endif
#ifdef USEAPI_OSS
    if (sys_audioapi == API_OSS)
        oss_close_audio();
    else
#endif
#ifdef USEAPI_ALSA
    if (sys_audioapi == API_ALSA)
        alsa_close_audio();
    else
#endif
#ifdef USEAPI_SGI
    if (sys_audioapi == API_SGI)
    {
        extern void sgi_close_audio(void);
        sgi_close_audio();
    }
    else
#endif
#ifdef USEAPI_MMIO
    if (sys_audioapi == API_MMIO)
        mmio_close_audio();
    else
#endif
        post("sys_close_audio: unknown API %d", sys_audioapi);
    sys_inchannels = sys_outchannels = 0;
}

    /* open audio using whatever parameters were last used */
void sys_reopen_audio( void)
{
    int naudioindev, audioindev[MAXAUDIOINDEV], chindev[MAXAUDIOINDEV];
    int naudiooutdev, audiooutdev[MAXAUDIOOUTDEV], choutdev[MAXAUDIOOUTDEV];
    int rate, advance;
    sys_get_audio_params(&naudioindev, audioindev, chindev,
        &naudiooutdev, audiooutdev, choutdev, &rate, &advance);
    sys_open_audio(naudioindev, audioindev, naudioindev, chindev,
        naudiooutdev, audiooutdev, naudiooutdev, choutdev, rate, advance, 1);
}

int sys_send_dacs(void)
{
    if (sys_meters)
    {
        int i, n;
        float maxsamp;
        for (i = 0, n = sys_inchannels * DEFDACBLKSIZE, maxsamp = sys_inmax;
            i < n; i++)
        {
            float f = sys_soundin[i];
            if (f > maxsamp) maxsamp = f;
            else if (-f > maxsamp) maxsamp = -f;
        }
        sys_inmax = maxsamp;
        for (i = 0, n = sys_outchannels * DEFDACBLKSIZE, maxsamp = sys_outmax;
            i < n; i++)
        {
            float f = sys_soundout[i];
            if (f > maxsamp) maxsamp = f;
            else if (-f > maxsamp) maxsamp = -f;
        }
        sys_outmax = maxsamp;
    }

#ifdef USEAPI_PORTAUDIO
    if (sys_audioapi == API_PORTAUDIO)
        return (pa_send_dacs());
    else 
#endif
#ifdef USEAPI_JACK
      if (sys_audioapi == API_JACK) 
        return (jack_send_dacs());
    else
#endif
#ifdef USEAPI_OSS
    if (sys_audioapi == API_OSS)
        return (oss_send_dacs());
    else
#endif
#ifdef USEAPI_ALSA
    if (sys_audioapi == API_ALSA)
        return (alsa_send_dacs());
    else
#endif
#ifdef USEAPI_SGI
    if (sys_audioapi == API_SGI)
    {
        extern int sgi_send_dacs(void);
        return (sgi_send_dacs());
    }
    else
#endif
#ifdef USEAPI_MMIO
    if (sys_audioapi == API_MMIO)
        return (mmio_send_dacs());
    else
#endif
    post("unknown API");    
    return (0);
}

float sys_getsr(void)
{
     return (sys_dacsr);
}

int sys_get_outchannels(void)
{
     return (sys_outchannels); 
}

int sys_get_inchannels(void) 
{
     return (sys_inchannels);
}

void sys_getmeters(float *inmax, float *outmax)
{
    if (inmax)
    {
        sys_meters = 1;
        *inmax = sys_inmax;
        *outmax = sys_outmax;
    }
    else
        sys_meters = 0;
    sys_inmax = sys_outmax = 0;
}

void sys_reportidle(void)
{
}

static void audio_getdevs(char *indevlist, int *nindevs,
    char *outdevlist, int *noutdevs, int *canmulti, 
        int maxndev, int devdescsize)
{
    audio_init();
#ifdef USEAPI_PORTAUDIO
    if (sys_audioapi == API_PORTAUDIO)
    {
        pa_getdevs(indevlist, nindevs, outdevlist, noutdevs, canmulti,
            maxndev, devdescsize);
    }
    else
#endif
#ifdef USEAPI_JACK
    if (sys_audioapi == API_JACK)
    {
        jack_getdevs(indevlist, nindevs, outdevlist, noutdevs, canmulti,
            maxndev, devdescsize);
    }
    else
#endif
#ifdef USEAPI_OSS
    if (sys_audioapi == API_OSS)
    {
        oss_getdevs(indevlist, nindevs, outdevlist, noutdevs, canmulti,
            maxndev, devdescsize);
    }
    else
#endif
#ifdef USEAPI_ALSA
    if (sys_audioapi == API_ALSA)
    {
        alsa_getdevs(indevlist, nindevs, outdevlist, noutdevs, canmulti,
            maxndev, devdescsize);
    }
    else
#endif
#ifdef USEAPI_SGI
    if (sys_audioapi == API_SGI)
    {
        extern void sgi_getdevs(char *indevlist, int *nindevs,
            char *outdevlist, int *noutdevs, int *canmulti, 
            int maxndev, int devdescsize);
        sgi_getdevs(indevlist, nindevs, outdevlist, noutdevs, canmulti,
            maxndev, devdescsize);
    }
    else
#endif
#ifdef USEAPI_MMIO
    if (sys_audioapi == API_MMIO)
    {
        mmio_getdevs(indevlist, nindevs, outdevlist, noutdevs, canmulti,
            maxndev, devdescsize);
    }
    else
#endif
    {
            /* this shouldn't happen once all the above get filled in. */
        int i;
        *nindevs = *noutdevs = 3;
        for (i = 0; i < 3; i++)
        {
            sprintf(indevlist + i * devdescsize, "input device #%d", i+1);
            sprintf(outdevlist + i * devdescsize, "output device #%d", i+1);
        }
        *canmulti = 0;
    }
}

#ifdef MSW
#define DEVONSET 0  /* microsoft device list starts at 0 (the "mapper"). */
#else               /* (see also MSW ifdef in sys_parsedevlist(), s_main.c)  */
#define DEVONSET 1  /* To agree with command line flags, normally start at 1 */
#endif

static void sys_listaudiodevs(void )
{
    char indevlist[MAXNDEV*DEVDESCSIZE], outdevlist[MAXNDEV*DEVDESCSIZE];
    int nindevs = 0, noutdevs = 0, i, canmulti = 0;

    audio_getdevs(indevlist, &nindevs, outdevlist, &noutdevs, &canmulti,
        MAXNDEV, DEVDESCSIZE);

    if (!nindevs)
        post("no audio input devices found");
    else
    {
        post("input devices:");
        for (i = 0; i < nindevs; i++)
            post("%d. %s", i + DEVONSET, indevlist + i * DEVDESCSIZE);
    }
    if (!noutdevs)
        post("no audio output devices found");
    else
    {
        post("output devices:");
        for (i = 0; i < noutdevs; i++)
            post("%d. %s", i + DEVONSET, outdevlist + i * DEVDESCSIZE);
    }
    post("API number %d\n", sys_audioapi);
}


    /* start an audio settings dialog window */
void glob_audio_properties(t_pd *dummy, t_floatarg flongform)
{
    char buf[1024 + 2 * MAXNDEV*(DEVDESCSIZE+4)];
        /* these are the devices you're using: */
    int naudioindev, audioindev[MAXAUDIOINDEV], chindev[MAXAUDIOINDEV];
    int naudiooutdev, audiooutdev[MAXAUDIOOUTDEV], choutdev[MAXAUDIOOUTDEV];
    int audioindev1, audioindev2, audioindev3, audioindev4,
        audioinchan1, audioinchan2, audioinchan3, audioinchan4,
        audiooutdev1, audiooutdev2, audiooutdev3, audiooutdev4,
        audiooutchan1, audiooutchan2, audiooutchan3, audiooutchan4;
    int rate, advance;
        /* these are all the devices on your system: */
    char indevlist[MAXNDEV*DEVDESCSIZE], outdevlist[MAXNDEV*DEVDESCSIZE];
    int nindevs = 0, noutdevs = 0, canmulti = 0, i;

    audio_getdevs(indevlist, &nindevs, outdevlist, &noutdevs, &canmulti,
        MAXNDEV, DEVDESCSIZE);

    sys_gui("set audio_indevlist {}\n");
    for (i = 0; i < nindevs; i++)
        sys_vgui("lappend audio_indevlist \"%s\"\n",
            indevlist + i * DEVDESCSIZE);

    sys_gui("set audio_outdevlist {}\n");
    for (i = 0; i < noutdevs; i++)
        sys_vgui("lappend audio_outdevlist \"%s\"\n",
            outdevlist + i * DEVDESCSIZE);

    sys_get_audio_params(&naudioindev, audioindev, chindev,
        &naudiooutdev, audiooutdev, choutdev, &rate, &advance);

    /* post("naudioindev %d naudiooutdev %d longform %f",
            naudioindev, naudiooutdev, flongform); */
    if (naudioindev > 1 || naudiooutdev > 1)
        flongform = 1;

    audioindev1 = (naudioindev > 0 &&  audioindev[0]>= 0 ? audioindev[0] : 0);
    audioindev2 = (naudioindev > 1 &&  audioindev[1]>= 0 ? audioindev[1] : 0);
    audioindev3 = (naudioindev > 2 &&  audioindev[2]>= 0 ? audioindev[2] : 0);
    audioindev4 = (naudioindev > 3 &&  audioindev[3]>= 0 ? audioindev[3] : 0);
    audioinchan1 = (naudioindev > 0 ? chindev[0] : 0);
    audioinchan2 = (naudioindev > 1 ? chindev[1] : 0);
    audioinchan3 = (naudioindev > 2 ? chindev[2] : 0);
    audioinchan4 = (naudioindev > 3 ? chindev[3] : 0);
    audiooutdev1 = (naudiooutdev > 0 && audiooutdev[0]>=0 ? audiooutdev[0] : 0);  
    audiooutdev2 = (naudiooutdev > 1 && audiooutdev[1]>=0 ? audiooutdev[1] : 0);  
    audiooutdev3 = (naudiooutdev > 2 && audiooutdev[2]>=0 ? audiooutdev[2] : 0);  
    audiooutdev4 = (naudiooutdev > 3 && audiooutdev[3]>=0 ? audiooutdev[3] : 0);  
    audiooutchan1 = (naudiooutdev > 0 ? choutdev[0] : 0);
    audiooutchan2 = (naudiooutdev > 1 ? choutdev[1] : 0);
    audiooutchan3 = (naudiooutdev > 2 ? choutdev[2] : 0);
    audiooutchan4 = (naudiooutdev > 3 ? choutdev[3] : 0);
    sprintf(buf,
"pdtk_audio_dialog %%s \
%d %d %d %d %d %d %d %d \
%d %d %d %d %d %d %d %d \
%d %d %d %d\n",
        audioindev1, audioindev2, audioindev3, audioindev4, 
        audioinchan1, audioinchan2, audioinchan3, audioinchan4, 
        audiooutdev1, audiooutdev2, audiooutdev3, audiooutdev4,
        audiooutchan1, audiooutchan2, audiooutchan3, audiooutchan4, 
        rate, advance, canmulti, (flongform != 0));
    gfxstub_deleteforkey(0);
    gfxstub_new(&glob_pdobject, (void *)glob_audio_properties, buf);
}

    /* new values from dialog window */
void glob_audio_dialog(t_pd *dummy, t_symbol *s, int argc, t_atom *argv)
{
    int naudioindev, audioindev[MAXAUDIOINDEV], chindev[MAXAUDIOINDEV];
    int naudiooutdev, audiooutdev[MAXAUDIOOUTDEV], choutdev[MAXAUDIOOUTDEV];
    int rate, advance, audioon, i, nindev, noutdev;
    int audioindev1, audioinchan1, audiooutdev1, audiooutchan1;
    int newaudioindev[4], newaudioinchan[4],
        newaudiooutdev[4], newaudiooutchan[4];
        /* the new values the dialog came back with: */
    int newrate = atom_getintarg(16, argc, argv);
    int newadvance = atom_getintarg(17, argc, argv);
    int statewas;

    for (i = 0; i < 4; i++)
    {
        newaudioindev[i] = atom_getintarg(i, argc, argv);
        newaudioinchan[i] = atom_getintarg(i+4, argc, argv);
        newaudiooutdev[i] = atom_getintarg(i+8, argc, argv);
        newaudiooutchan[i] = atom_getintarg(i+12, argc, argv);
    }

    for (i = 0, nindev = 0; i < 4; i++)
    {
        if (newaudioinchan[i])
        {
            newaudioindev[nindev] = newaudioindev[i];
            newaudioinchan[nindev] = newaudioinchan[i];
            /* post("in %d %d %d", nindev,
                newaudioindev[nindev] , newaudioinchan[nindev]); */
            nindev++;
        }
    }
    for (i = 0, noutdev = 0; i < 4; i++)
    {
        if (newaudiooutchan[i])
        {
            newaudiooutdev[noutdev] = newaudiooutdev[i];
            newaudiooutchan[noutdev] = newaudiooutchan[i];
            /* post("out %d %d %d", noutdev,
                newaudiooutdev[noutdev] , newaudioinchan[noutdev]); */
            noutdev++;
        }
    }

    sys_close_audio();
    sys_open_audio(nindev, newaudioindev, nindev, newaudioinchan,
        noutdev, newaudiooutdev, noutdev, newaudiooutchan,
        newrate, newadvance, 1);
}

void sys_listdevs(void )
{
#ifdef USEAPI_PORTAUDIO
    if (sys_audioapi == API_PORTAUDIO)
        sys_listaudiodevs();
    else 
#endif
#ifdef USEAPI_JACK
    if (sys_audioapi == API_JACK)
        jack_listdevs();
    else
#endif
#ifdef USEAPI_OSS
    if (sys_audioapi == API_OSS)
        sys_listaudiodevs();
    else
#endif
#ifdef USEAPI_ALSA
    if (sys_audioapi == API_ALSA)
        sys_listaudiodevs();
    else
#endif
#ifdef USEAPI_SGI
        extern void sgi_listaudiodevs(void);
    if (sys_audioapi == API_SGI)
        sgi_listaudiodevs();
    else
#endif
#ifdef USEAPI_MMIO
    if (sys_audioapi == API_MMIO)
        sys_listaudiodevs();
    else
#endif
    post("unknown API");    

    sys_listmididevs();
}

void sys_setblocksize(int n)
{
    if (n < 1)
        n = 1;
    if (n != (1 << ilog2(n)))
        post("warning: adjusting blocksize to power of 2: %d", 
            (n = (1 << ilog2(n))));
    sys_blocksize = n;
}

void sys_set_audio_api(int which)
{
     sys_audioapi = which;
     if (sys_verbose)
        post("sys_audioapi %d", sys_audioapi);
}

void glob_audio_setapi(void *dummy, t_floatarg f)
{
    int newapi = f;
    if (newapi)
    {
        if (newapi == sys_audioapi)
        {
            if (!audio_isopen())
                sys_reopen_audio();
        }
        else
        {
            sys_close_audio();
            sys_audioapi = newapi;
                /* bash device params back to default */
            audio_naudioindev = audio_naudiooutdev = 1;
            audio_audioindev[0] = audio_audiooutdev[0] = DEFAULTAUDIODEV;
            audio_audiochindev[0] = audio_audiochoutdev[0] = SYS_DEFAULTCH;
            sys_reopen_audio();
        }
        glob_audio_properties(0, 0);
    }
    else if (audio_isopen())
    {
        sys_close_audio();
        audio_state = 0;
        sched_set_using_dacs(0);
    }
}

    /* start or stop the audio hardware */
void sys_set_audio_state(int onoff)
{
    if (onoff)  /* start */
    {
        if (!audio_isopen())
            sys_reopen_audio();    
    }
    else
    {
        if (audio_isopen())
        {
            sys_close_audio();
            sched_set_using_dacs(0);
        }
    }
    audio_state = onoff;
}

void sys_get_audio_apis(char *buf)
{
    int n = 0;
    strcpy(buf, "{ ");
#ifdef USEAPI_OSS
    sprintf(buf + strlen(buf), "{OSS %d} ", API_OSS); n++;
#endif
#ifdef USEAPI_MMIO
    sprintf(buf + strlen(buf), "{\"standard (MMIO)\" %d} ", API_MMIO); n++;
#endif
#ifdef USEAPI_ALSA
    sprintf(buf + strlen(buf), "{ALSA %d} ", API_ALSA); n++;
#endif
#ifdef USEAPI_PORTAUDIO
#ifdef MSW
    sprintf(buf + strlen(buf),
        "{\"ASIO (via portaudio)\" %d} ", API_PORTAUDIO);
#else
#ifdef OSX
    sprintf(buf + strlen(buf),
        "{\"standard (portaudio)\" %d} ", API_PORTAUDIO);
#else
    sprintf(buf + strlen(buf), "{portaudio %d} ", API_PORTAUDIO);
#endif
#ifdef USEAPI_SGI
    sprintf(buf + strlen(buf), "{SGI %d} ", API_SGI); n++;
#endif
#endif
     n++;
#endif
#ifdef USEAPI_JACK
    sprintf(buf + strlen(buf), "{jack %d} ", API_JACK); n++;
#endif
    strcat(buf, "}");
        /* then again, if only one API (or none) we don't offer any choice. */
    if (n < 2)
        strcpy(buf, "{}");
    
}

#ifdef USEAPI_ALSA
void alsa_putzeros(int n);
void alsa_getzeros(int n);
void alsa_printstate( void);
#endif

    /* debugging */
void glob_foo(void *dummy, t_symbol *s, int argc, t_atom *argv)
{
    t_symbol *arg = atom_getsymbolarg(0, argc, argv);
    if (arg == gensym("restart"))
        sys_reopen_audio();
#ifdef USEAPI_ALSA
    else if (arg == gensym("alsawrite"))
    {
        int n = atom_getintarg(1, argc, argv);
        alsa_putzeros(n);
    }
    else if (arg == gensym("alsaread"))
    {
        int n = atom_getintarg(1, argc, argv);
        alsa_getzeros(n);
    }
    else if (arg == gensym("print"))
    {
        alsa_printstate();
    }
#endif
}
