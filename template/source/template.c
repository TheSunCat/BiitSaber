#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

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
	printf("\x1b[2;0H");


	printf("Hello Stream!");

    // loop so i can connect the wiimote before calling SetMotionPlus to enabled.
    // this needs to be called after the wiimote connected
    while(1)
    {
        WPAD_ScanPads();
        u32 gcPressed = WPAD_ButtonsDown(0);
        if (gcPressed & WPAD_BUTTON_A) break;

        printf("Scanning pads... %i", gcPressed);
    }

    WPAD_SetDataFormat(0, WPAD_FMT_BTNS_ACC_IR);

    printf("set motion plus on all wiimotes : %d\n", WPAD_SetMotionPlus(-1, 1));

    WPADData* wd;
    u32 type;

	while(1) {

		// Call WPAD_ScanPads each loop, this reads the latest controller states
		WPAD_ScanPads();

        //WPAD_ReadPending(WPAD_CHAN_ALL, countevs);

        // clear the screen (augh)
        printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");

        int err = WPAD_Probe(0, &type);
        switch(err) {
            case WPAD_ERR_NO_CONTROLLER:
                printf("WIIMOTE NOT CONNECTED\n");
                break;
            case WPAD_ERR_NOT_READY:
                printf("WIIMOTE NOT READY\n");
                break;
            case WPAD_ERR_NONE:
                printf("WIIMOTE READY\n");
                break;
            default:
                printf("UNK WIIMOTE STATE %d\n", err);
        }
        //printf("EVENT COUNT: %d\n", evctr);

        if(err == WPAD_ERR_NONE) {
            wd = WPAD_Data(0);
            printf("DATA ERR: %d\n\n",wd->err);

            printf("ACCEL:\n");
            printf("X: %3d\n", wd->accel.x);
            printf("Y: %3d\n", wd->accel.y);
            printf("Z: %3d\n", wd->accel.z);

            printf("\n");
            printf("ORIENT:\n");
            printf("PITCH: %.02f\n", wd->orient.pitch);
            printf("YAW: %.02f\n", wd->orient.yaw); // frozen at 0.00
            printf("ROLL: %.02f\n", wd->orient.roll);

            printf("\n");
            printf("MP:\n");
            printf("PITCH: %.2hd\n", wd->exp.mp.rx); // frozen at 0.00
            printf("YAW: %.2hd\n", wd->exp.mp.ry); // frozen at 0.00
            printf("ROLL: %.2hd\n", wd->exp.mp.rz); // frozen at 0.00

            printf("\n");
            printf("GFORCE:\n");
            printf("X: %.02f\n", wd->gforce.x);
            printf("Y: %.02f\n", wd->gforce.y);
            printf("Z: %.02f\n", wd->gforce.z);
        }


		// WPAD_ButtonsDown tells us which buttons were pressed in this loop
		// this is a "one shot" state which will not fire again until the button has been released
		u32 pressed = WPAD_ButtonsDown(0);

		// We return to the launcher application via exit
		if ( pressed & WPAD_BUTTON_HOME ) exit(0);

        sleep(3);

		// Wait for the next frame
		VIDEO_WaitVSync();
	}

	return 0;
}
