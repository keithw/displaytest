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

static BMDConfig		g_config;
class CaptureAndView : public IDeckLinkInputCallback
{
public:
	CaptureAndView(XWindow &xw, WPixmap &xp, XImage &xi, GraphicsContext &gc) : 
		m_refCount(1), 
		window(xw), 
		picture(xp),
		image(xi),
		gc(gc) 
	{}

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv) { return E_NOINTERFACE; }
	virtual ULONG STDMETHODCALLTYPE AddRef(void) {}
	virtual ULONG STDMETHODCALLTYPE  Release(void) {}
	virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents events, IDeckLinkDisplayMode *mode, BMDDetectedVideoInputFormatFlags formatFlags) {}
	virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame* videoFrame, IDeckLinkAudioInputPacket* audioFrame) {
		void*	frameBytes;
		void*	audioFrameBytes;

		// Handle Video Frame
		if (videoFrame) {
			if (videoFrame->GetFlags() & bmdFrameHasNoInputSource) {
				printf("Frame received (#%lu) - No input signal detected\n", g_frameCount);
			} else {
				printf("Frame received - Size: %li bytes\n",
				videoFrame->GetRowBytes() * videoFrame->GetHeight());


				if (videoFrame->GetHeight() == 720) {
					videoFrame->GetBytes(&image.data());
				} else {
					printf("Frame is not 720p\n", );
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


int main(int argc, char *argv[])
{
	HRESULT							result;
	int								exitStatus = 1;
	int								idx;

	IDeckLinkIterator*				deckLinkIterator = NULL;
	IDeckLink*						deckLink = NULL;
	IDeckLinkInput*					deckLinkInput = NULL;

	IDeckLinkAttributes*			deckLinkAttributes = NULL;
	bool							formatDetectionSupported;

	IDeckLinkDisplayModeIterator*	displayModeIterator = NULL;
	IDeckLinkDisplayMode*			displayMode = NULL;
	char*							displayModeName = NULL;
	BMDDisplayModeSupport			displayModeSupported;

	CaptureAndView*					inputViewer = NULL;

	pthread_mutex_init(&g_sleepMutex, NULL);
	pthread_cond_init(&g_sleepCond, NULL);

	signal(SIGINT, sigfunc);
	signal(SIGTERM, sigfunc);
	signal(SIGHUP, sigfunc);

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
		fprintf(stderr, "Unable to get DeckLink device %u\n", g_config.m_deckLinkIndex);
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

	// Configure the capture callback
	inputViewer = new CaptureAndView(window, picture, image, gc);
	deckLinkInput->SetCallback(inputViewer);

	// Block main thread until signal occurs
	while (!g_do_exit)
	{
		// Start capturing
		result = g_deckLinkInput->EnableVideoInput(displayMode->GetDisplayMode(), bmdFormat8BitARGB, bmdVideoInputEnableFormatDetection);
		if (result != S_OK)
		{
			fprintf(stderr, "Failed to enable video input. Is another application using the card?\n");
			goto bail;
		}

		result = g_deckLinkInput->StartStreams();
		if (result != S_OK)
			goto bail;

		// All Okay.
		exitStatus = 0;

		pthread_mutex_lock(&g_sleepMutex);
		pthread_cond_wait(&g_sleepCond, &g_sleepMutex);
		pthread_mutex_unlock(&g_sleepMutex);

		fprintf(stderr, "Stopping Capture\n");
		g_deckLinkInput->StopStreams();
		g_deckLinkInput->DisableAudioInput();
		g_deckLinkInput->DisableVideoInput();
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