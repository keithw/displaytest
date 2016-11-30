#ifndef __BLACKMAGIC_TEST__
#define __BLACKMAGIC_TEST__

#include "DeckLinkAPI.h"
#include "display.hh"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <csignal>

static pthread_mutex_t	g_sleepMutex;
static pthread_cond_t	g_sleepCond;
static bool				g_do_exit = false;

class CaptureAndView : public IDeckLinkInputCallback
{
public:
	CaptureAndView(XWindow &xw, XPixmap &xp, XImage &xi, GraphicsContext &gc) : 
		m_refCount(1), 
		window(xw), 
		picture(xp),
		image(xi),
		gc(gc) 
	{}

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID*) { return E_NOINTERFACE; }
	virtual ULONG STDMETHODCALLTYPE AddRef(void) { 	return __sync_add_and_fetch(&m_refCount, 1); }
	virtual ULONG STDMETHODCALLTYPE  Release(void) { 	
		int32_t newRefValue = __sync_sub_and_fetch(&m_refCount, 1);
		if (newRefValue == 0) {
			delete this;
			return 0;
		}
		return newRefValue; 
	}
	virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents, 
															  IDeckLinkDisplayMode *, 
															  BMDDetectedVideoInputFormatFlags) {
		return S_OK;
	}
	virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame* videoFrame, IDeckLinkAudioInputPacket*) {
		void*	frameBytes;
		// Handle Video Frame
		if (videoFrame) {
			if (videoFrame->GetFlags() & bmdFrameHasNoInputSource) {
				printf("Frame received  - No input signal detected\n");
			} else {
				printf("Frame received - Size: %li bytes\n",
				videoFrame->GetRowBytes() * videoFrame->GetHeight());


				if (videoFrame->GetHeight() == 720) {
					videoFrame->GetBytes(&frameBytes);
					memcpy(image.data_unsafe(), frameBytes, videoFrame->GetRowBytes() * videoFrame->GetHeight());
				} else {
					printf("Frame is not 720p\n");
				}
			}
		}

		return S_OK;
	}

private:
	int32_t				m_refCount;
	XWindow& 			window;
	XPixmap& 			picture;
	XImage& 			image;
	GraphicsContext& 	gc;
};

static void sigfunc(int signum)
{
	if (signum == SIGINT || signum == SIGTERM)
		g_do_exit = true;

	pthread_cond_signal(&g_sleepCond);
}


int main()
{
	HRESULT							result;
	int								exitStatus = 1;

	IDeckLinkIterator*				deckLinkIterator = NULL;
	IDeckLink*						deckLink = NULL;
	IDeckLinkInput*					deckLinkInput = NULL;

	IDeckLinkAttributes*			deckLinkAttributes = NULL;
	bool							formatDetectionSupported;

	IDeckLinkDisplayModeIterator*	displayModeIterator = NULL;
	IDeckLinkDisplayMode*			displayMode = NULL;
	char*							displayModeName = NULL;

	CaptureAndView*					inputViewer = NULL;

	pthread_mutex_init(&g_sleepMutex, NULL);
	pthread_cond_init(&g_sleepCond, NULL);

	signal(SIGINT, sigfunc);
	signal(SIGTERM, sigfunc);
	signal(SIGHUP, sigfunc);

	// if (!IsDeckLinkAPIPresent) {
	// 	printf("No deckLink API\n");
	// 	exit(1);
	// }
	/* construct a window */
  	XWindow window( 1280, 720 );
 	window.set_name( "RGB example" );

  	/* put the window on the screen */
  	window.map();

  	/* on the X server, construct a picture */
  	XPixmap picture( window );

  	/* in our program (the X client), construct an image */
  	XImage image( picture );
  	GraphicsContext gc( picture );


	// Get the DeckLink device
	deckLinkIterator = CreateDeckLinkIteratorInstance();
	if (!deckLinkIterator)
	{
		fprintf(stderr, "This application requires the DeckLink drivers installed.\n");
		goto bail;
	}

	result = deckLinkIterator->Next(&deckLink);

	if (result != S_OK || deckLink == NULL)
	{
		fprintf(stderr, "Could not find DeckLink device - result = %08x\n", result);
		goto bail;
	}

	// Get the input (capture) interface of the DeckLink device
	result = deckLink->QueryInterface(IID_IDeckLinkInput, (void**)&deckLinkInput);
	if (result != S_OK)
		goto bail;

	// Check the card supports format detection
	result = deckLink->QueryInterface(IID_IDeckLinkAttributes, (void**)&deckLinkAttributes);
	if (result == S_OK) {
		result = deckLinkAttributes->GetFlag(BMDDeckLinkSupportsInputFormatDetection, &formatDetectionSupported);
		if (result != S_OK || !formatDetectionSupported) {
			fprintf(stderr, "Format detection is not supported on this device\n");
			goto bail;
		}
	}

	result = deckLinkInput->GetDisplayModeIterator(&displayModeIterator);
	if (result != S_OK)
		goto bail;

	result = displayModeIterator->Next(&displayMode);

	if (result != S_OK || displayMode == NULL)
	{
		fprintf(stderr, "Unable to get display mode\n");
		goto bail;
	}

	// Get display mode name
	result = displayMode->GetName((const char**)&displayModeName);
	if (result != S_OK)
	{	}

	// Configure the capture callback
	inputViewer = new CaptureAndView(window, picture, image, gc);
	deckLinkInput->SetCallback(inputViewer);

	// Block main thread until signal occurs
	while (!g_do_exit)
	{
		// Start capturing
		result = deckLinkInput->EnableVideoInput(displayMode->GetDisplayMode(), bmdFormat8BitARGB, bmdVideoInputEnableFormatDetection);
		if (result != S_OK)
		{
			fprintf(stderr, "Failed to enable video input. Is another application using the card?\n");
			goto bail;
		}

		result = deckLinkInput->StartStreams();
		if (result != S_OK)
			goto bail;

		// All Okay.
		exitStatus = 0;

		pthread_mutex_lock(&g_sleepMutex);
		pthread_cond_wait(&g_sleepCond, &g_sleepMutex);
		pthread_mutex_unlock(&g_sleepMutex);

		fprintf(stderr, "Stopping Capture\n");
		deckLinkInput->StopStreams();
		deckLinkInput->DisableAudioInput();
		deckLinkInput->DisableVideoInput();
	}

bail:
	if (displayMode != NULL)
		displayMode->Release();

	if (displayModeIterator != NULL)
		displayModeIterator->Release();

	if (inputViewer != NULL)
		delete inputViewer;

	if (deckLinkInput != NULL) {
		deckLinkInput->Release();
		deckLinkInput = NULL;
	}

	if (deckLink != NULL)
		deckLink->Release();

	if (deckLinkIterator != NULL)
		deckLinkIterator->Release();

	return exitStatus;
}

#endif