#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

extern int s_log_count;
extern char s_log_buffer[1024];

static void check_log() {
    if (s_log_count > 0) {
        puts(s_log_buffer);
        s_log_count = 0;
    }
}

static void mem_report()
{
    printf("MEM1: %p-%p, MEM2: %p-%p\n", SYS_GetArenaLo(), SYS_GetArenaHi(), SYS_GetArena2Lo(), SYS_GetArena2Hi());
}

static void *try_allocate(size_t size)
{
    void *ptr = malloc(size);
    check_log();
    if (ptr) {
        printf("Allocating %d bytes succeeded: %p\n", size, ptr);
    } else {
        printf("Allocating %d bytes failed\n", size);
    }
    return ptr;
}

static void try_free(void *ptr)
{
    if (!ptr) {
        puts("Asked to free a NULL pointer!\n");
        return;
    }

    printf("Freeing %p\n", ptr);
    free(ptr);
    check_log();
}

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------

	// Initialise the video system
	VIDEO_Init();

	// This function initialises the attached controllers
	WPAD_Init();

	// Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);

	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	// Initialise the console, required for printf
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);

	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb);

	// Make the display visible
	VIDEO_SetBlack(false);

	// Flush the video register changes to the hardware
	VIDEO_Flush();

	// Wait for Video setup to complete
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();


	// The console understands VT terminal escape codes
	// This positions the cursor on row 2, column 0
	// we can use variables for this with format codes too
	// e.g. printf ("\x1b[%d;%dH", row, column );
	//printf("\x1b[2;0H");


	printf("Hello World!");

    int step = 0;
	while(SYS_MainLoop()) {

		// Call WPAD_ScanPads each loop, this reads the latest controller states
		WPAD_ScanPads();

		// WPAD_ButtonsDown tells us which buttons were pressed in this loop
		// this is a "one shot" state which will not fire again until the button has been released
		u32 pressed = WPAD_ButtonsDown(0);

		// We return to the launcher application via exit
		if ( pressed & WPAD_BUTTON_HOME ) exit(0);

        if (step == 30) {
            check_log();
            void *mem1_10mb = try_allocate(10*1024*1024);
            /* This will get allocated in MEM2: */
            void *mem2_20mb = try_allocate(20*1024*1024);
            /* We would like to see this in MEM1, but alas */
            void *mem2_5mb = try_allocate(5*1024*1024);
            /* This will get allocated in MEM2: */
            void *mem2_1mb = try_allocate(1*1024*1024);

            try_free(mem2_20mb);
            try_free(mem1_10mb);
            mem_report();

            try_free(mem2_1mb);
            mem_report();
            try_free(mem2_5mb);
            mem_report();
        }
        step++;
		// Wait for the next frame
		VIDEO_WaitVSync();
	}

	return 0;
}
