/* Copyright (c) 1997-1999 Miller Puckette and others.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* Clock functions (which should move, but where?) and MIDI queueing */

#define PD_PLUSPLUS_FACE
#include "desire.h"
#ifdef UNISTD
#include <unistd.h>
#include <sys/time.h>
#ifdef HAVE_BSTRING_H
#include <bstring.h>
#endif
#endif
#ifdef MSW
#include <winsock.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <wtypes.h>
#endif
#include <string.h>
#include <stdio.h>
#include <signal.h>

struct t_midiqelem {
    double time;
    int portno;
    unsigned char onebyte;
    unsigned char byte1;
    unsigned char byte2;
    unsigned char byte3;
};

#define MIDIQSIZE 1024

t_midiqelem midi_outqueue[MIDIQSIZE];
int midi_outhead, midi_outtail;
t_midiqelem midi_inqueue[MIDIQSIZE];
int midi_inhead, midi_intail;
static double sys_midiinittime;

#ifndef API_DEFAULT
#define API_DEFAULT 0
#endif
int sys_midiapi = API_DEFAULT;

/* this is our current estimate for at what "system" real time the
   current logical time's output should occur. */
static double sys_dactimeminusrealtime;
/* same for input, should be schduler advance earlier. */
static double sys_adctimeminusrealtime;

static double sys_newdactimeminusrealtime = -1e20;
static double sys_newadctimeminusrealtime = -1e20;
static double sys_whenupdate;

void sys_initmidiqueue() {
    sys_midiinittime = clock_getlogicaltime();
    sys_dactimeminusrealtime = sys_adctimeminusrealtime = 0;
}

/* this is called from the OS dependent code from time to time when we
   think we know the delay (outbuftime) in seconds, at which the last-output
   audio sample will go out the door. */
void sys_setmiditimediff(double inbuftime, double outbuftime) {
    double dactimeminusrealtime = .001 * clock_gettimesince(sys_midiinittime) - outbuftime - sys_getrealtime();
    double adctimeminusrealtime = .001 * clock_gettimesince(sys_midiinittime) +  inbuftime - sys_getrealtime();
    if (dactimeminusrealtime > sys_newdactimeminusrealtime) sys_newdactimeminusrealtime = dactimeminusrealtime;
    if (adctimeminusrealtime > sys_newadctimeminusrealtime) sys_newadctimeminusrealtime = adctimeminusrealtime;
    if (sys_getrealtime() > sys_whenupdate) {
        sys_dactimeminusrealtime = sys_newdactimeminusrealtime;
        sys_adctimeminusrealtime = sys_newadctimeminusrealtime;
        sys_newdactimeminusrealtime = -1e20;
        sys_newadctimeminusrealtime = -1e20;
        sys_whenupdate = sys_getrealtime() + 1;
    }
}

/* return the logical time of the DAC sample we believe is currently going out, based on how much "system time"
   has elapsed since the last time sys_setmiditimediff got called. */
static double sys_getmidioutrealtime() {return sys_getrealtime() + sys_dactimeminusrealtime;}
static double sys_getmidiinrealtime () {return sys_getrealtime() + sys_adctimeminusrealtime;}

static void sys_putnext () {
    t_midiqelem &m = midi_outqueue[midi_outtail];
    int portno = m.portno;
#ifdef USEAPI_ALSA
    if (sys_midiapi == API_ALSA) {
        if (m.onebyte) sys_alsa_putmidibyte(portno, m.byte1);
        else sys_alsa_putmidimess(portno, m.byte1, m.byte2, m.byte3);
    } else
#endif /* ALSA */
    {
        if (m.onebyte) sys_putmidibyte(portno, m.byte1);
        else sys_putmidimess(portno, m.byte1, m.byte2, m.byte3);
    }
    midi_outtail  = (midi_outtail + 1 == MIDIQSIZE ? 0 : midi_outtail + 1);
}

/*  #define TEST_DEJITTER */

void sys_pollmidioutqueue() {
#ifdef TEST_DEJITTER
    static int db = 0;
#endif
    double midirealtime = sys_getmidioutrealtime();
#ifdef TEST_DEJITTER
    if (midi_outhead == midi_outtail) db = 0;
#endif
    while (midi_outhead != midi_outtail) {
#ifdef TEST_DEJITTER
        if (!db) {
            post("out: del %f, midiRT %f logicaltime %f, RT %f dacminusRT %f",
                (midi_outqueue[midi_outtail].time - midirealtime),
                    midirealtime, .001 * clock_gettimesince(sys_midiinittime),
                        sys_getrealtime(), sys_dactimeminusrealtime);
            db = 1;
        }
#endif
        if (midi_outqueue[midi_outtail].time <= midirealtime)
            sys_putnext();
        else break;
    }
}

static void sys_queuemidimess(int portno, int onebyte, int a, int b, int c) {
    int newhead = midi_outhead +1;
    if (newhead == MIDIQSIZE) newhead = 0;
    /* if FIFO is full flush an element to make room */
    if (newhead == midi_outtail) sys_putnext();
    t_midiqelem m = midi_outqueue[midi_outhead];
    m.portno = portno;
    m.onebyte = onebyte;
    m.byte1 = a;
    m.byte2 = b;
    m.byte3 = c;
    m.time = .001 * clock_gettimesince(sys_midiinittime);
    midi_outhead = newhead;
    sys_pollmidioutqueue();
}

#define MIDI_NOTEON 144
#define MIDI_POLYAFTERTOUCH 160
#define MIDI_CONTROLCHANGE 176
#define MIDI_PROGRAMCHANGE 192
#define MIDI_AFTERTOUCH 208
#define MIDI_PITCHBEND 224

void outmidi_noteon(int portno, int channel, int pitch, int velo) {
    if (pitch < 0) pitch = 0; else if (pitch > 127) pitch = 127;
    if (velo < 0) velo = 0;   else if (velo > 127) velo = 127;
    sys_queuemidimess(portno, 0, MIDI_NOTEON + (channel & 0xf), pitch, velo);
}

void outmidi_controlchange(int portno, int channel, int ctl, int value) {
    if (ctl < 0) ctl = 0; else if (ctl > 127) ctl = 127;
    if (value < 0) value = 0; else if (value > 127) value = 127;
    sys_queuemidimess(portno, 0, MIDI_CONTROLCHANGE + (channel & 0xf),
        ctl, value);
}

void outmidi_programchange(int portno, int channel, int value) {
    if (value < 0) value = 0; else if (value > 127) value = 127;
    sys_queuemidimess(portno, 0, MIDI_PROGRAMCHANGE + (channel & 0xf), value, 0);
}

void outmidi_pitchbend(int portno, int channel, int value) {
    if (value < 0) value = 0; else if (value > 16383) value = 16383;
    sys_queuemidimess(portno, 0, MIDI_PITCHBEND + (channel & 0xf), (value & 127), ((value>>7) & 127));
}

void outmidi_aftertouch(int portno, int channel, int value) {
    if (value < 0) value = 0; else if (value > 127) value = 127;
    sys_queuemidimess(portno, 0, MIDI_AFTERTOUCH + (channel & 0xf), value, 0);
}

void outmidi_polyaftertouch(int portno, int channel, int pitch, int value) {
    if (pitch < 0) pitch = 0; else if (pitch > 127) pitch = 127;
    if (value < 0) value = 0; else if (value > 127) value = 127;
    sys_queuemidimess(portno, 0, MIDI_POLYAFTERTOUCH + (channel & 0xf), pitch, value);
}

void outmidi_byte(int portno, int value) {
#ifdef USEAPI_ALSA
  if (sys_midiapi == API_ALSA) sys_alsa_putmidibyte(portno, value); else
#endif
  sys_putmidibyte(portno, value);
}

void outmidi_mclk(int portno) {sys_queuemidimess(portno, 1, 0xf8, 0,0);}

/* ------------------------- MIDI input queue handling ------------------ */
typedef struct midiparser {
    int mp_status;
    int mp_gotbyte1;
    int mp_byte1;
} t_midiparser;

#define MIDINOTEOFF       0x80  /* 2 following 'data bytes' */
#define MIDINOTEON        0x90  /* 2 */
#define MIDIPOLYTOUCH     0xa0  /* 2 */
#define MIDICONTROLCHANGE 0xb0  /* 2 */
#define MIDIPROGRAMCHANGE 0xc0  /* 1 */
#define MIDICHANNELTOUCH  0xd0  /* 1 */
#define MIDIPITCHBEND     0xe0  /* 2 */
#define MIDISTARTSYSEX    0xf0  /* (until F7) */
#define MIDITIMECODE      0xf1  /* 1 */
#define MIDISONGPOS       0xf2  /* 2 */
#define MIDISONGSELECT    0xf3  /* 1 */
#define MIDIRESERVED1     0xf4  /* ? */
#define MIDIRESERVED2     0xf5  /* ? */
#define MIDITUNEREQUEST   0xf6  /* 0 */
#define MIDIENDSYSEX      0xf7  /* 0 */
#define MIDICLOCK         0xf8  /* 0 */
#define MIDITICK          0xf9  /* 0 */
#define MIDISTART         0xfa  /* 0 */
#define MIDICONT          0xfb  /* 0 */
#define MIDISTOP          0xfc  /* 0 */
#define MIDIACTIVESENSE   0xfe  /* 0 */
#define MIDIRESET         0xff  /* 0 */

static void sys_dispatchnextmidiin() {
    static t_midiparser parser[MAXMIDIINDEV], *parserp;
    int portno = midi_inqueue[midi_intail].portno, byte = midi_inqueue[midi_intail].byte1;
    if (!midi_inqueue[midi_intail].onebyte) bug("sys_dispatchnextmidiin");
    if (portno < 0 || portno >= MAXMIDIINDEV) bug("sys_dispatchnextmidiin 2");
    parserp = parser + portno;
    outlet_setstacklim();
    if (byte >= 0xf8) {
	inmidi_realtimein(portno, byte);
    } else {
        inmidi_byte(portno, byte);
        if (byte & 0x80) {
            if (byte == MIDITUNEREQUEST || byte == MIDIRESERVED1 || byte == MIDIRESERVED2) {
                    parserp->mp_status = 0;
            } else if (byte == MIDISTARTSYSEX) {
                inmidi_sysex(portno, byte);
                parserp->mp_status = byte;
            } else if (byte == MIDIENDSYSEX) {
                inmidi_sysex(portno, byte);
                parserp->mp_status = 0;
            } else {
                parserp->mp_status = byte;
            }
            parserp->mp_gotbyte1 = 0;
        } else {
            int cmd = (parserp->mp_status >= 0xf0 ? parserp->mp_status :
                (parserp->mp_status & 0xf0));
            int chan = (parserp->mp_status & 0xf);
            int byte1 = parserp->mp_byte1, gotbyte1 = parserp->mp_gotbyte1;
            switch (cmd) {
            case MIDINOTEOFF:
                if (gotbyte1) inmidi_noteon(portno, chan, byte1, 0), parserp->mp_gotbyte1 = 0;
                else parserp->mp_byte1 = byte, parserp->mp_gotbyte1 = 1;
                break;
            case MIDINOTEON:
                if (gotbyte1) inmidi_noteon(portno, chan, byte1, byte), parserp->mp_gotbyte1 = 0;
                else parserp->mp_byte1 = byte, parserp->mp_gotbyte1 = 1;
                break;
            case MIDIPOLYTOUCH:
                if (gotbyte1) inmidi_polyaftertouch(portno, chan, byte1, byte), parserp->mp_gotbyte1 = 0;
                else parserp->mp_byte1 = byte, parserp->mp_gotbyte1 = 1;
                break;
            case MIDICONTROLCHANGE:
                if (gotbyte1) inmidi_controlchange(portno, chan, byte1, byte), parserp->mp_gotbyte1 = 0;
                else parserp->mp_byte1 = byte, parserp->mp_gotbyte1 = 1;
                break;
            case MIDIPROGRAMCHANGE:
                inmidi_programchange(portno, chan, byte);
                break;
            case MIDICHANNELTOUCH:
                inmidi_aftertouch(portno, chan, byte);
                break;
            case MIDIPITCHBEND:
                if (gotbyte1) inmidi_pitchbend(portno, chan, ((byte << 7) + byte1)), parserp->mp_gotbyte1 = 0;
                else parserp->mp_byte1 = byte, parserp->mp_gotbyte1 = 1;
                break;
            case MIDISTARTSYSEX:
                inmidi_sysex(portno, byte);
                break;
                /* other kinds of messages are just dropped here.  We'll
                need another status byte before we start letting MIDI in
                again (no running status across "system" messages). */
            case MIDITIMECODE: break;    /* 1 data byte*/
            case MIDISONGPOS: break;      /* 2 */
            case MIDISONGSELECT: break;   /* 1 */
            }
        }
    }
    midi_intail  = (midi_intail + 1 == MIDIQSIZE ? 0 : midi_intail + 1);
}

void sys_pollmidiinqueue() {
#ifdef TEST_DEJITTER
    static int db = 0;
#endif
    double logicaltime = .001 * clock_gettimesince(sys_midiinittime);
#ifdef TEST_DEJITTER
    if (midi_inhead == midi_intail)
        db = 0;
#endif
    while (midi_inhead != midi_intail) {
#ifdef TEST_DEJITTER
        if (!db) {
            post("in del %f, logicaltime %f, RT %f adcminusRT %f",
                (midi_inqueue[midi_intail].time - logicaltime),
                    logicaltime, sys_getrealtime(), sys_adctimeminusrealtime);
            db = 1;
        }
#endif
#if 0
        if (midi_inqueue[midi_intail].time <= logicaltime - 0.007)
            post("late %f", 1000 * (logicaltime - midi_inqueue[midi_intail].time));
#endif
        if (midi_inqueue[midi_intail].time <= logicaltime) {
#if 0
            post("diff %f", 1000* (logicaltime - midi_inqueue[midi_intail].time));
#endif
            sys_dispatchnextmidiin();
        }
        else break;
    }
}

    /* this should be called from the system dependent MIDI code when a byte
    comes in, as a result of our calling sys_poll_midi.  We stick it on a
    timetag queue and dispatch it at the appropriate logical time. */


void sys_midibytein(int portno, int byte) {
    static int warned = 0;
    int newhead = midi_inhead+1;
    if (newhead == MIDIQSIZE) newhead = 0;
            /* if FIFO is full flush an element to make room */
    if (newhead == midi_intail) {
        if (!warned) {
            post("warning: MIDI timing FIFO overflowed");
            warned = 1;
        }
        sys_dispatchnextmidiin();
    }
    midi_inqueue[midi_inhead].portno = portno;
    midi_inqueue[midi_inhead].onebyte = 1;
    midi_inqueue[midi_inhead].byte1 = byte;
    midi_inqueue[midi_inhead].time = sys_getmidiinrealtime();
    midi_inhead = newhead;
    sys_pollmidiinqueue();
}

void sys_pollmidiqueue() {
#if 0
    static double lasttime;
    double newtime = sys_getrealtime();
    if (newtime - lasttime > 0.007)
        post("delay %d", (int)(1000 * (newtime - lasttime)));
    lasttime = newtime;
#endif
#ifdef USEAPI_ALSA
      if (sys_midiapi == API_ALSA) sys_alsa_poll_midi();
      else
#endif /* ALSA */
    sys_poll_midi();    /* OS dependent poll for MIDI input */
    sys_pollmidioutqueue();
    sys_pollmidiinqueue();
}

/******************** dialog window and device listing ********************/

#ifdef USEAPI_ALSA
void midi_alsa_init();
#endif
#ifndef USEAPI_PORTMIDI
#ifdef USEAPI_OSS
void midi_oss_init();
#endif
#endif

/* last requested parameters */
static int midi_nmidiindev;
static int midi_midiindev[MAXMIDIINDEV];
static int midi_nmidioutdev;
static int midi_midioutdev[MAXMIDIOUTDEV];

void sys_get_midi_apis(char *buf) {
    int n = 0;
    strcpy(buf, "{ ");
    sprintf(buf + strlen(buf), "{default-MIDI %d} ", API_DEFAULT); n++;
#ifdef USEAPI_ALSA
    sprintf(buf + strlen(buf), "{ALSA-MIDI %d} ", API_ALSA); n++;
#endif
    strcat(buf, "}");
}
void sys_get_midi_params(int *pnmidiindev, int *pmidiindev, int *pnmidioutdev, int *pmidioutdev) {
    *pnmidiindev  = midi_nmidiindev;  for (int i=0; i<MAXMIDIINDEV;  i++) pmidiindev [i] = midi_midiindev[i];
    *pnmidioutdev = midi_nmidioutdev; for (int i=0; i<MAXMIDIOUTDEV; i++) pmidioutdev[i] = midi_midioutdev[i];
}
static void sys_save_midi_params(int nmidiindev, int *midiindev, int nmidioutdev, int *midioutdev) {
    midi_nmidiindev  = nmidiindev;  for (int i=0; i<MAXMIDIINDEV;  i++) midi_midiindev [i] = midiindev[i];
    midi_nmidioutdev = nmidioutdev; for (int i=0; i<MAXMIDIOUTDEV; i++) midi_midioutdev[i] = midioutdev[i];
}

void sys_open_midi(int nmidiindev, int *midiindev, int nmidioutdev, int *midioutdev, int enable) {
#ifdef USEAPI_ALSA
    midi_alsa_init();
#endif
#ifndef USEAPI_PORTMIDI
#ifdef USEAPI_OSS
    midi_oss_init();
#endif
#endif
    if (enable) {
#ifdef USEAPI_ALSA
        if (sys_midiapi == API_ALSA) sys_alsa_do_open_midi(nmidiindev, midiindev, nmidioutdev, midioutdev);
        else
#endif /* ALSA */
            sys_do_open_midi(nmidiindev, midiindev, nmidioutdev, midioutdev);
    }
    sys_save_midi_params(nmidiindev, midiindev, nmidioutdev, midioutdev);
    sys_vgui("set pd_whichmidiapi %d\n", sys_midiapi);
}

/* open midi using whatever parameters were last used */
void sys_reopen_midi() {
    int nmidiindev, midiindev[MAXMIDIINDEV];
    int nmidioutdev, midioutdev[MAXMIDIOUTDEV];
    sys_get_midi_params(&nmidiindev, midiindev, &nmidioutdev, midioutdev);
    sys_open_midi(nmidiindev, midiindev, nmidioutdev, midioutdev, 1);
}

#define MAXNDEV 50
#define DEVDESCSIZE 80
#define DEVONSET 1  /* To agree with command line flags, normally start at 1 */

void sys_listmididevs() {
    char indevlist[MAXNDEV*DEVDESCSIZE], outdevlist[MAXNDEV*DEVDESCSIZE];
    int nindevs = 0, noutdevs = 0;
#ifdef USEAPI_ALSA
    if (sys_midiapi == API_ALSA)
      midi_alsa_getdevs(indevlist, &nindevs, outdevlist, &noutdevs, MAXNDEV, DEVDESCSIZE);
    else
#endif /* ALSA */
    midi_getdevs(indevlist, &nindevs, outdevlist, &noutdevs, MAXNDEV, DEVDESCSIZE);
    if (!nindevs) post("no midi input devices found");
    else post("MIDI input devices:");  for (int i=0;  i<nindevs; i++) post("%d. %s", i+1,         indevlist + i * DEVDESCSIZE);
    if (!noutdevs) post("no midi output devices found");
    else post("MIDI output devices:"); for (int i=0; i<noutdevs; i++) post("%d. %s", i+DEVONSET, outdevlist + i * DEVDESCSIZE);
}

void sys_set_midi_api(int which) {
     sys_midiapi = which;
     if (sys_verbose) post("sys_midiapi %d", sys_midiapi);
}

void glob_midi_properties(t_pd *dummy, t_floatarg flongform);

void glob_midi_setapi(t_pd *dummy, t_floatarg f) {
    int newapi = int(f);
    if (newapi) {
        if (newapi == sys_midiapi) {
          //if (!midi_isopen()) s_reopen_midi();
        } else {
#ifdef USEAPI_ALSA
          if (sys_midiapi == API_ALSA) sys_alsa_close_midi();
          else
#endif
            sys_close_midi();
          sys_midiapi = newapi;
          /* bash device params back to default */
          midi_nmidiindev = midi_nmidioutdev = 1;
          //midi_midiindev[0] = midi_midioutdev[0] = DEFAULTMIDIDEV;
          //midi_midichindev[0] = midi_midichoutdev[0] = SYS_DEFAULTCH;
          sys_reopen_midi();
        }
        glob_midi_properties(0, 0);
    } else /* if (midi_isopen()) */ {
        sys_close_midi();
        //midi_state = 0;
    }
}

extern t_class *glob_pdobject;

/* start a midi settings dialog window */
void glob_midi_properties(t_pd *dummy, t_floatarg flongform) {
    /* these are the devices you're using: */
    int nindev, midiindev[MAXMIDIINDEV];
    int noutdev, midioutdev[MAXMIDIOUTDEV];
    char midiinstr[16*4+1],midioutstr[16*4+1],*s;
    /* these are all the devices on your system: */
    char indevlist[MAXNDEV*DEVDESCSIZE], outdevlist[MAXNDEV*DEVDESCSIZE];
    int nindevs = 0, noutdevs = 0;
    char  indevl[MAXNDEV*(DEVDESCSIZE+4)+80];
    char outdevl[MAXNDEV*(DEVDESCSIZE+4)+80];
    midi_getdevs(indevlist, &nindevs, outdevlist, &noutdevs, MAXNDEV, DEVDESCSIZE);
    *indevl=0;  for (int i=0; i< nindevs; i++) sprintf( indevl+strlen( indevl), "\"%s\" ",  indevlist + i * DEVDESCSIZE);
    *outdevl=0; for (int i=0; i<noutdevs; i++) sprintf(outdevl+strlen(outdevl), "\"%s\" ", outdevlist + i * DEVDESCSIZE);
    sys_get_midi_params(&nindev, midiindev, &noutdev, midioutdev);
    if (nindev > 1 || noutdev > 1) flongform = 1;
    s=midiinstr ; *s=0; for(int i=0; i<16; ++i) {sprintf(s,"%3d ", nindev>i && midiindev[i] >=0? midiindev[i]:-1); s+=strlen(s);}
    s=midioutstr; *s=0; for(int i=0; i<16; ++i) {sprintf(s,"%3d ",noutdev>i && midioutdev[i]>=0?midioutdev[i]:-1); s+=strlen(s);}
    sys_vgui("pdtk_midi_dialog %%s {%s} %s {%s} %s %d\n", indevl,midiinstr,outdevl,midioutstr,!!flongform);
}

/* new values from dialog window */
void glob_midi_dialog(t_pd *dummy, t_symbol *s, int argc, t_atom *argv) {
    int i, nindev, noutdev, newmidiindev[16], newmidioutdev[16], alsadevin, alsadevout;
    for (int i=0; i<16; i++) {
        newmidiindev[i]  = atom_getintarg(i   , argc, argv);
        newmidioutdev[i] = atom_getintarg(i+16, argc, argv);
    }
    for (i=0, nindev=0;  i<16; i++) {if ( newmidiindev[i] >= 0) { newmidiindev[nindev]  =  newmidiindev[i]; nindev++;}}
    for (i=0, noutdev=0; i<16; i++) {if (newmidioutdev[i] >= 0) {newmidioutdev[noutdev] = newmidioutdev[i]; noutdev++;}}
    alsadevin = atom_getintarg(32, argc, argv);
    alsadevout = atom_getintarg(33, argc, argv);
#ifdef USEAPI_ALSA
    if (sys_midiapi == API_ALSA) {
        sys_alsa_close_midi();
        sys_open_midi(alsadevin, newmidiindev, alsadevout, newmidioutdev, 1);
    } else
#endif
    {
        sys_close_midi();
        sys_open_midi(nindev, newmidiindev, noutdev, newmidioutdev, 1);
    }
}

/* tb { */
void glob_midi_getindevs(t_pd *dummy, t_symbol *s, int ac, t_atom *av) {
    char indevlist[MAXNDEV*DEVDESCSIZE], outdevlist[MAXNDEV*DEVDESCSIZE];
    int nindevs = 0, noutdevs = 0; t_symbol *selector = gensym("midiindev"); t_symbol *pd = gensym("pd");
    t_atom argv[MAXNDEV]; int f = ac ? (int)atom_getfloatarg(0,ac,av) : -1;
    midi_getdevs(indevlist, &nindevs, outdevlist, &noutdevs, MAXNDEV, DEVDESCSIZE);
    if (f<0) {
        for (int i=0; i<nindevs; i++) SETSYMBOL(argv+i, gensym(indevlist + i * DEVDESCSIZE));
        typedmess(pd->s_thing, selector, nindevs, argv);
    } else if (f < nindevs) {
        SETSYMBOL(argv, gensym(indevlist + f * DEVDESCSIZE));
        typedmess(pd->s_thing, selector, 1, argv);
    }
}
void glob_midi_getoutdevs(t_pd *dummy, t_symbol *s, int ac, t_atom *av) {
    char indevlist[MAXNDEV*DEVDESCSIZE], outdevlist[MAXNDEV*DEVDESCSIZE];
    int nindevs = 0, noutdevs = 0; t_symbol *selector = gensym("midioutdev"); t_symbol *pd = gensym("pd");
    t_atom argv[MAXNDEV]; int f = ac ? (int)atom_getfloatarg(0,ac,av) : -1;
    midi_getdevs(indevlist, &nindevs, outdevlist, &noutdevs, MAXNDEV, DEVDESCSIZE);
    if (f<0) {
        for (int i=0; i<noutdevs; i++) SETSYMBOL(argv+i, gensym(outdevlist + i*DEVDESCSIZE));
        typedmess(pd->s_thing, selector, noutdevs, argv);
    } else if (f < noutdevs) {
        SETSYMBOL(argv, gensym(outdevlist + f*DEVDESCSIZE));
        typedmess(pd->s_thing, selector, 1, argv);
    }
}

#define FOO(N,D,M) \
    int nindev, midiindev[MAXMIDIINDEV], noutdev, midioutdev[MAXMIDIOUTDEV]; \
    t_atom argv[MAXNDEV]; sys_get_midi_params(&nindev, midiindev, &noutdev, midioutdev); \
    for (int i=0; i<N; i++) SETFLOAT(argv+i, D[i]); \
    typedmess(gensym("pd")->s_thing, gensym(M),  nindev,  argv);
void glob_midi_getcurrentindevs( t_pd *dummy) {FOO( nindev,midiindev ,"midicurrentindev" )}
void glob_midi_getcurrentoutdevs(t_pd *dummy) {FOO(noutdev,midioutdev,"midicurrentoutdev")}
