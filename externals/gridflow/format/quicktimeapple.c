/*
	$Id: quicktimeapple.c 3904 2008-06-18 20:32:46Z matju $

	GridFlow
	Copyright (c) 2001-2008 by Mathieu Bouchard

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	See file ../COPYING for further informations on licensing terms.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <QuickTime/QuickTime.h>
#include <QuickTime/Movies.h>
#include <QuickTime/QuickTimeComponents.h>
#include "../gridflow.h.fcs"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <CoreServices/CoreServices.h>

typedef ComponentInstance VideoDigitizerComponent, VDC;
typedef ComponentResult   VideoDigitizerError,     VDE;

#if 0
//enum {VDCType='vdig', vdigInterfaceRev=2 };
//enum {ntscIn=0, currentIn=0, palIn, secamIn, ntscReallyIn };
//enum {compositeIn, sVideoIn, rgbComponentIn, rgbComponentSyncIn, yuvComponentIn, yuvComponentSyncIn, tvTunerIn, sdiIn};
//enum {vdPlayThruOff, vdPlayThruOn};
//enum {vdDigitizerBW, vdDigitizerRGB};
//enum {vdBroadcastMode, vdVTRMode};
//enum {vdUseAnyField, vdUseOddField, vdUseEvenField};
//enum {vdTypeBasic, vdTypeAlpha, vdTypeMask, vdTypeKey};
/*enum {digiInDoesNTSC, digiInDoesPAL, digiInDoesSECAM, skip 4,
  digiInDoesGenLock, digiInDoesComposite, digiInDoesSVideo, digiInDoesComponent,
  digiInVTR_Broadcast, digiInDoesColor, digiInDoesBW, skip 17,
  digiInSignalLock};*/
/*bitset {digiOutDoes1, digiOutDoes2, digiOutDoes4,
         digiOutDoes8, digiOutDoes16, digiOutDoes32, 
  digiOutDoesDither, digiOutDoesStretch, digiOutDoesShrink,
  digiOutDoesMask, skip 1, 
  digiOutDoesDouble, digiOutDoesQuad, digiOutDoesQuarter, digiOutDoesSixteenth,
  digiOutDoesRotate, digiOutDoesHorizFlip, digiOutDoesVertFlip, digiOutDoesSkew,
  digiOutDoesBlend, digiOutDoesWarp, digiOutDoesHW_DMA,
  digiOutDoesHWPlayThru, digiOutDoesILUT, digiOutDoesKeyColor,
  digiOutDoesAsyncGrabs, digiOutDoesUnreadableScreenBits, 
  digiOutDoesCompress, digiOutDoesCompressOnly,
  digiOutDoesPlayThruDuringCompress, digiOutDoesCompressPartiallyVisible,
  digiOutDoesNotNeedCopyOfCompressData};*/
/*struct DigitizerInfo {
  short vdigType;
  long inputCapabilityFlags, outputCapabilityFlags;
  long inputCurrentFlags,    outputCurrentFlags;
  short slot;
  GDHandle gdh, maskgdh;
  short minDestHeight, minDestWidth;
  short maxDestHeight, maxDestWidth;
  short blendLevels;
  long reserved;};*/
/*struct VdigType { long digType, reserved;};*/
/*struct VdigTypeList { short count; VdigType list[1];};*/
/*struct VdigBufferRec { PixMapHandle dest; Point location; long reserved;};*/
/*struct VdigBufferRecList {
  short count; MatrixRecordPtr matrix; RgnHandle mask; VdigBufferRec list[1];};*/
//typedef VdigBufferRecList *VdigBufferRecListPtr;
//typedef VdigBufferRecListPtr *VdigBufferRecListHandle;
//typedef CALLBACK_API(void,VdigIntProcPtr)(long flags, long refcon);
//typedef STACK_UPP_TYPE(VdigIntProcPtr);
/*struct VDCompressionList {
  CodecComponent codec; CodecType cType; Str63 typeName, name;
  long formatFlags, compressFlags, reserved;};*/
//typedef VDCompressionList *   VDCompressionListPtr;
//typedef VDCompressionListPtr *VDCompressionListHandle;
/*bitset {
  dmaDepth1, dmaDepth2, dmaDepth4, dmaDepth8, dmaDepth16, dmaDepth32,
  dmaDepth2Gray, dmaDepth4Gray, dmaDepth8Gray};*/
//enum {kVDIGControlledFrameRate=-1};
//bitset {vdDeviceFlagShowInputsAsDevices, vdDeviceFlagHideDevice};
/*bitset {
  vdFlagCaptureStarting, vdFlagCaptureStopping,
  vdFlagCaptureIsForPreview, vdFlagCaptureIsForRecord,
  vdFlagCaptureLowLatency, vdFlagCaptureAlwaysUseTimeBase,
  vdFlagCaptureSetSettingsBegin, vdFlagCaptureSetSettingsEnd};*/
/*\class VDC
VDE VDGetMaxSrcRect   (short inputStd, Rect *maxSrcRect)
VDE VDGetActiveSrcRect(short inputStd, Rect *activeSrcRect)
VDE VD[GS]etDigitizerRect(Rect *digitizerRect)
VDE VDGetVBlankRect(short inputStd, Rect *vBlankRect)
VDE VDGetMaskPixMap(PixMapHandlemaskPixMap)
VDE VDGetPlayThruDestination(PixMapHandle *    dest, Rect *destRect, MatrixRecord *    m, RgnHandle *mask)
VDE VDUseThisCLUT(CTabHandle colorTableHandle)
VDE VD[SG*]etInputGammaValue(Fixed channel1, Fixed channel2, Fixed channel3)
VDE VD[GS]etBrightness(uint16 *)
VDE VD[GS]etContrast(uint16 *)
VDE VD[GS]etHue(uint16 *)
VDE VD[GS]etSharpness(uint16 *)
VDE VD[GS]etSaturation(uint16 *)
VDE VDGrabOneFrame(VDC ci)
VDE VDGetMaxAuxBuffer(PixMapHandle *pm, Rect *r)
VDE VDGetDigitizerInfo(DigitizerInfo *info)
VDE VDGetCurrentFlags(long *inputCurrentFlag, long *outputCurrentFlag)
VDE VD[SG*]etKeyColor(long index)
VDE VDAddKeyColor(long *index)
VDE VDGetNextKeyColor(long  index)
VDE VD[GS]etKeyColorRange(RGBColor minRGB, RGBColor maxRGB)
VDE VDSetDigitizerUserInterrupt(long  flags, VdigIntUPP userInterruptProc, long  refcon)
VDE VD[SG*]etInputColorSpaceMode(short colorSpaceMode)
VDE VD[SG*]etClipState(short clipEnable)
VDE VDSetClipRgn(RgnHandle clipRegion)
VDE VDClearClipRgn(RgnHandle clipRegion)
VDE VDGetCLUTInUse(CTabHandle *colorTableHandle)
VDE VD[SG*]etPLLFilterType(short pllType)
VDE VDGetMaskandValue(uint16 blendLevel, long *mask, long *value)
VDE VDSetMasterBlendLevel(uint16 *blendLevel)
VDE VDSetPlayThruDestination(PixMapHandledest, RectPtr destRect, MatrixRecordPtr m, RgnHandle mask)
VDE VDSetPlayThruOnOff(short state)
VDE VD[SG*]etFieldPreference(short fieldFlag)
VDE VDPreflightDestination(Rect *digitizerRect, PixMap **dest, RectPtr destRect, MatrixRecordPtr m)
VDE VDPreflightGlobalRect(GrafPtr theWindow, Rect *globalRect)
VDE VDSetPlayThruGlobalRect(GrafPtr theWindow, Rect *globalRect)
VDE VDSetInputGammaRecord(VDGamRecPtrinputGammaPtr)
VDE VDGetInputGammaRecord(VDGamRecPtr *inputGammaPtr)
VDE VD[SG]etBlackLevelValue(uint16 *)
VDE VD[SG]etWhiteLevelValue(uint16 *)
VDE VDGetVideoDefaults(uint16 *blackLevel, uint16 *whiteLevel, uint16 *brightness, uint16 *hue, uint16 *saturation, uint16 *contrast, uint16 *sharpness)
VDE VDGetNumberOfInputs(short *inputs)
VDE VDGetInputFormat(short input, short *format)
VDE VD[SG*]etInput(short input)
VDE VDSetInputStandard(short inputStandard)
VDE VDSetupBuffers(VdigBufferRecListHandle bufferList)
VDE VDGrabOneFrameAsync(short buffer)
VDE VDDone(short buffer)
VDE VDSetCompression(OSTypecompressType, short depth, Rect *bounds, CodecQspatialQuality, CodecQtemporalQuality, long  keyFrameRate)
VDE VDCompressOneFrameAsync(VDC ci)
VDE VDCompressDone(UInt8 *queuedFrameCount, Ptr *theData, long *dataSize, UInt8 *similarity, TimeRecord *t)
VDE VDReleaseCompressBuffer(Ptr bufferAddr)
VDE VDGetImageDescription(ImageDescriptionHandle desc)
VDE VDResetCompressSequence(VDC ci)
VDE VDSetCompressionOnOff(Boolean)
VDE VDGetCompressionTypes(VDCompressionListHandle h)
VDE VDSetTimeBase(TimeBase t)
VDE VDSetFrameRate(Fixed framesPerSecond)
VDE VDGetDataRate(long *milliSecPerFrame, Fixed *framesPerSecond, long *bytesPerSecond)
VDE VDGetSoundInputDriver(Str255 soundDriverName)
VDE VDGetDMADepths(long *depthArray, long *preferredDepth)
VDE VDGetPreferredTimeScale(TimeScale *preferred)
VDE VDReleaseAsyncBuffers(VDC ci)
VDE VDSetDataRate(long  bytesPerSecond)
VDE VDGetTimeCode(TimeRecord *atTime, void *timeCodeFormat, void *timeCodeTime)
VDE VDUseSafeBuffers(Boolean useSafeBuffers)
VDE VDGetSoundInputSource(long videoInput, long *soundInput)
VDE VDGetCompressionTime(OSTypecompressionType, short depth, Rect *srcRect, CodecQ *spatialQuality, CodecQ *temporalQuality, ulong *compressTime)
VDE VDSetPreferredPacketSize(long preferredPacketSizeInBytes)
VDE VD[SG*]etPreferredImageDimensions(long  width, long  height)
VDE VDGetInputName(long videoInput, Str255 name)
VDE VDSetDestinationPort(CGrafPtr destPort)
VDE VDGetDeviceNameAndFlags(Str255 outName, UInt32 *outNameFlags)
VDE VDCaptureStateChanging(UInt32inStateFlags)
VDE VDGetUniqueIDs(UInt64 *outDeviceID, UInt64 *outInputID)
VDE VDSelectUniqueIDs(const UInt64 *inDeviceID, const UInt64 *inInputID)
*/
#endif

static OSErr callback(ComponentInstanceRecord*, char*, long int, long int*, long int, TimeValue, short int, long int) {
	post("FormatQuickTimeCamera callback");
	return noErr;
}

\class FormatQuickTimeCamera : Format {
  P<Dim> dim;
  uint8 *buf;
  uint8 *buf2;
  VDC vdc;
  int m_newFrame; 
  SeqGrabComponent m_sg;
  SGChannel m_vc;
  short m_pixelDepth;
  Rect rect;
  GWorldPtr m_srcGWorld;
  PixMapHandle m_pixMap;
  Ptr m_baseAddr;
  long m_rowBytes;
  int m_quality;
//int m_colorspace;
  \constructor (t_symbol *mode) {
	//vdc = SGGetVideoDigitizerComponent(c);
	dim = new Dim(240,320,4);
	OSErr e;
	rect.top=rect.left=0;
	rect.bottom=dim->v[0]; rect.right=dim->v[1];
	int n=0;
	Component c = 0;
	ComponentDescription cd;
	cd.componentType = SeqGrabComponentType;
	cd.componentSubType = 0;
	cd.componentManufacturer = 0;
	cd.componentFlags = 0;
	cd.componentFlagsMask = 0;
	for(;;) {
		c = FindNextComponent(c, &cd);
		if (!c) break;
		ComponentDescription cd2;
		Ptr name=0,info=0,icon=0;
		GetComponentInfo(c,&cd2,&name,&info,&icon);
		post("Component #%d",n);
		char *t = (char *)&cd.componentType;
		post(" type='%c%c%c%c'",t[3],t[2],t[1],t[0]);
		t = (char *)&cd.componentSubType;
		post("  subtype='%c%c%c%c'",t[3],t[2],t[1],t[0]);
		post("  name=%08x, *name='%*s'",name, *name, name+1);
		post("  info=%08x, *info='%*s'",info, *name, info+1);
		n++;
	}
	post("number of components: %d",n);
	m_sg = OpenDefaultComponent(SeqGrabComponentType, 0);
	if(!m_sg) RAISE("could not open default component");
	e=SGInitialize(m_sg);
	if(e!=noErr) RAISE("could not initialize SG");
	e=SGSetDataRef(m_sg, 0, 0, seqGrabDontMakeMovie);
	if (e!=noErr) RAISE("dataref failed");
	e=SGNewChannel(m_sg, VideoMediaType, &m_vc);
	if(e!=noErr) post("could not make new SG channel");
	e=SGSetChannelBounds(m_vc, &rect);
	if(e!=noErr) post("could not set SG ChannelBounds");
	e=SGSetChannelUsage(m_vc, seqGrabPreview);
	if(e!=noErr) post("could not set SG ChannelUsage");
        e=SGSetDataProc(m_sg,NewSGDataUPP(callback),0);
	if (e!=noErr) post("could not set SG DataProc");
	//  m_rowBytes = m_vidXSize*4;
	switch (3) {
	  case 0: e=SGSetChannelPlayFlags(m_vc, channelPlayNormal); break;
	  case 1: e=SGSetChannelPlayFlags(m_vc, channelPlayHighQuality); break;
	  case 2: e=SGSetChannelPlayFlags(m_vc, channelPlayFast); break;
	  case 3: e=SGSetChannelPlayFlags(m_vc, channelPlayAllData); break;
	}
	int dataSize = dim->prod();
	buf = new uint8[dataSize];
	buf2 = new uint8[dataSize];
	m_rowBytes = dim->prod(1);
	e=QTNewGWorldFromPtr (&m_srcGWorld,k32ARGBPixelFormat,&rect,NULL,NULL,0,buf,m_rowBytes);
	if (0/*yuv*/) {
		int dataSize = dim->prod()*2/4;
		buf = new uint8[dataSize];
		m_rowBytes = dim->prod(1)*2/4;
		e=QTNewGWorldFromPtr (&m_srcGWorld,k422YpCbCr8CodecType,&rect,NULL,NULL,0,buf,m_rowBytes);
	}
	if (e!=noErr) RAISE("error #%d at QTNewGWorldFromPtr",e);
	if (!m_srcGWorld) RAISE("Could not allocate off screen");
	SGSetGWorld(m_sg,(CGrafPtr)m_srcGWorld, NULL);
	//SGStartPreview(m_sg);
	e=SGStartRecord(m_sg);
	if (e!=noErr) RAISE("error #%d at SGStartRecord",e);
  }
  ~FormatQuickTimeCamera() {
    if (m_vc) if (::SGDisposeChannel(m_sg, m_vc)) RAISE("SGDisposeChannel");
    if (m_sg) {
      if (::CloseComponent(m_sg)) RAISE("CloseComponent");
      if (m_srcGWorld) ::DisposeGWorld(m_srcGWorld);
    }
  }
  \decl 0 bang ();
  \grin 0 int
};

// /System/Library/Frameworks/CoreServices.framework/Frameworks/CarbonCore.framework/Headers/Components.h

static int nn(int c) {return c?c:' ';}

/*
pascal Boolean pix_videoDarwin :: SeqGrabberModalFilterProc (DialogPtr theDialog, const EventRecord *theEvent, short *itemHit, long refCon){
    Boolean	handled = false;
    if ((theEvent->what == updateEvt) &&
        ((WindowPtr) theEvent->message == (WindowPtr) refCon)) {
        BeginUpdate ((WindowPtr) refCon);
        EndUpdate ((WindowPtr) refCon);
        handled = true;
    } 
    WindowRef awin = GetDialogWindow(theDialog);
    ShowWindow (awin);
    SetWindowClass(awin,kUtilityWindowClass);
    //ChangeWindowAttributes(awin,kWindowStandardHandlerAttribute,0);
    //SGPanelEvent(m_sg,m_vc,theDialog,0,theEvent,itemHit,&handled);
    //AEProcessAppleEvent (theEvent);
    return handled;
}
void pix_videoDarwin :: DoVideoSettings() {
    Rect newActiveVideoRect;
    Rect curBounds, curVideoRect, newVideoRect;
    ComponentResult err;
    SGModalFilterUPP seqGragModalFilterUPP;
    err = SGGetChannelBounds (m_vc, &curBounds);
    err = SGGetVideoRect (m_vc, &curVideoRect);
    err = SGPause (m_sg, true);
    seqGragModalFilterUPP = (SGModalFilterUPP)NewSGModalFilterUPP(SeqGrabberModalFilterProc);
    err = SGSettingsDialog(m_sg, m_vc, 0, NULL, seqGrabSettingsPreviewOnly, seqGragModalFilterUPP, (long)m_srcGWorld);
    DisposeSGModalFilterUPP(seqGragModalFilterUPP);
    err = SGGetVideoRect (m_vc, &newVideoRect);
    err = SGGetSrcVideoBounds (m_vc, &newActiveVideoRect);
    err = SGPause (m_sg, false);
}
*/

\def 0 bang () {
    GridOutlet out(this,0,dim);
    int n = dim->prod()/4;
    for (int i=0; i<n; i++) ((uint32 *)buf2)[i] = ((uint32 *)buf)[i] >> 8;
    out.send(dim->prod(),buf2);
    SGIdle(m_sg);
}

GRID_INLET(FormatQuickTimeCamera,0) {
	RAISE("Unimplemented. Sorry.");
//!@#$
	if (in->dim->n != 3)      RAISE("expecting 3 dimensions: rows,columns,channels");
	if (in->dim->get(2) != 3) RAISE("expecting 3 channels (got %d)",in->dim->get(2));
	in->set_chunk(0);
} GRID_FLOW {
} GRID_FINISH {
} GRID_END
\end class FormatQuickTimeCamera {install_format("#io.quicktimecamera",4,"");}

\class FormatQuickTimeApple : Format {
	Movie movie;
	TimeValue time;
	short movie_file;
	GWorldPtr gw; /* just like an X11 Image or Pixmap, maybe. */
	uint8 *buffer;
	P<Dim> dim;
	int nframe, nframes;
	\constructor (t_symbol *mode, string filename) {
		/*vdc=0;*/ movie=0; time=0; movie_file=0; gw=0; buffer=0; dim=0; nframe=0; nframes=0;
		int err;
		filename = gf_find_file(filename);
		FSSpec fss;
		FSRef fsr;
		err = FSPathMakeRef((const UInt8 *)filename.data(), &fsr, NULL);      if (err) goto err;
		err = FSGetCatalogInfo(&fsr, kFSCatInfoNone, NULL, NULL, &fss, NULL); if (err) goto err;
		err = OpenMovieFile(&fss,&movie_file,fsRdPerm);                       if (err) goto err;
		NewMovieFromFile(&movie, movie_file, NULL, NULL, newMovieActive, NULL);
		Rect r;
		GetMovieBox(movie, &r);
		post("handle=%d movie=%d tracks=%d", movie_file, movie, GetMovieTrackCount(movie));
		post("duration=%d; timescale=%d cHz", (long)GetMovieDuration(movie), (long)GetMovieTimeScale(movie));
		nframes = GetMovieDuration(movie); /* i don't think so */
		post("rect=((%d..%d),(%d..%d))", r.top, r.bottom, r.left, r.right);
		OffsetRect(&r, -r.left, -r.top);
		SetMovieBox(movie, &r);
		dim = new Dim(r.bottom-r.top, r.right-r.left, 4);
		SetMoviePlayHints(movie, hintsHighQuality, hintsHighQuality);
		buffer = new uint8[dim->prod()];
		err = QTNewGWorldFromPtr(&gw, k32ARGBPixelFormat, &r, NULL, NULL, 0, buffer, dim->prod(1));
		if (err) goto err;
		return;
	err:
	//	RAISE("can't open file `%s': error #%d (%s)", filename.data(), err, rb_str_ptr(rb_funcall(mGridFlow,SI(macerr),1,INT2NUM(err))));
		RAISE("can't open file `%s': error #%d (0x%08x)", filename.data(), err, err);
	}
	~FormatQuickTimeApple() {
		if (movie) {
			DisposeMovie(movie);
			DisposeGWorld(gw);
			CloseMovieFile(movie_file);
		}
	}
	\decl 0 codec (string c);
	\decl 0 colorspace (string c);
	\decl 0 bang ();
	\decl 0 seek (int frame);
	\decl 0 rewind ();
	\grin 0
};

\def 0 seek (int frame) {nframe=frame;}
\def 0 rewind () {_0_seek(0,0,0);}

\def 0 bang () {
	CGrafPtr savedPort;
	GDHandle savedDevice;
	SetMovieGWorld(movie,gw,GetGWorldDevice(gw));
	Rect r;
	GetMovieBox(movie,&r);
	PixMapHandle pixmap = GetGWorldPixMap(gw);
	short flags = nextTimeStep;
	if (nframe>=nframes) {outlet_bang(bself->te_outlet); return;}
	if (nframe==0) flags |= nextTimeEdgeOK;
	TimeValue duration;
	OSType mediaType = VisualMediaCharacteristic;
	GetMovieNextInterestingTime(movie,
		flags,1,&mediaType,time,0,&time,&duration);
	if (time<0) {
		time=0;
		outlet_bang(bself->te_outlet);
		return;
	}
//	post("quicktime frame #%d; time=%d duration=%d", nframe, (long)time, (long)duration);
	SetMovieTimeValue(movie,nframe*duration);
	MoviesTask(movie,0);
	GridOutlet out(this,0,dim);
	uint32 *bufu32 = (uint32 *)buffer;
	int n = dim->prod()/4;
	int i;
	if (is_le()) {
		for (; i<n; i++) {
			bufu32[i+0]=bufu32[i+0]>>8;
		}
	} else {
		for (i=0; i<n&-4; i+=4) {
			bufu32[i+0]=(bufu32[i+0]<<8)+(bufu32[i+0]>>24);
			bufu32[i+1]=(bufu32[i+1]<<8)+(bufu32[i+1]>>24);
			bufu32[i+2]=(bufu32[i+2]<<8)+(bufu32[i+2]>>24);
			bufu32[i+3]=(bufu32[i+3]<<8)+(bufu32[i+3]>>24);
		}
		for (; i<n; i++) {
			bufu32[i+0]=(bufu32[i+0]<<8)+(bufu32[i+0]>>24);
		}
	}
	out.send(dim->prod(),buffer);
	int nf=nframe;
	nframe++;
	//return INT2NUM(nf);
}

GRID_INLET(FormatQuickTimeApple,0) {
	RAISE("Unimplemented. Sorry.");
//!@#$
	if (in->dim->n != 3)
		RAISE("expecting 3 dimensions: rows,columns,channels");
	if (in->dim->get(2) != 3)
		RAISE("expecting 3 channels (got %d)",in->dim->get(2));
	in->set_chunk(0);
} GRID_FLOW {
} GRID_FINISH {
} GRID_END

\def 0 codec      (string c) { RAISE("Unimplemented. Sorry."); }
\def 0 colorspace (string c) { RAISE("Unimplemented. Sorry."); }

\classinfo {
	EnterMovies();
	install_format("#io.quicktime",4,"mov");
}
\end class FormatQuickTimeApple
void startup_quicktimeapple () {
	\startall
}
