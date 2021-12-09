#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include "wiimote.h"

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

// Block until A is pressed
void pressA()
{
    while(1)
    {
        WPAD_ScanPads();
        u32 gcPressed = WPAD_ButtonsDown(0);
        if (gcPressed & WPAD_BUTTON_A) break;

        usleep(100);
    }
}

int main(int argc, char **argv) {
	VIDEO_Init();
	WPAD_Init();

    Wiimote wiimote;

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
	VIDEO_SetBlack(false); // Make the display visible
	VIDEO_Flush();
	VIDEO_WaitVSync(); // Wait for Video setup to complete
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	printf("Hello world! Press A...");

    // loop to allow Wiimote to connect
    pressA();

    WPAD_SetDataFormat(-1, WPAD_FMT_BTNS_ACC_IR);

    printf("Set motion plus on all wiimotes: %d\n", WPAD_SetMotionPlus(-1, 1));

    WPAD_Rumble(-1, true);
    sleep(1);
    WPAD_Rumble(-1, false);

    printf("Place the Wiimote on a flat surface for calibration and press A.\n");
    pressA();
    printf("Calibrating...\n");
    wiimote.sensorCalibrate(40);
    printf("Calibrated! Press A to continue.");


    WPADData* wd;
    u32 type;

	while(1) {
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
            printf("X: %.02f\n", float(wd->accel.x));
            printf("Y: %.02f\n", float(wd->accel.y));
            printf("Z: %.02f\n", float(wd->accel.z));

            printf("\n");
            printf("ORIENT:\n");
            printf("PITCH: %.02f\n", wd->orient.pitch);
            printf("YAW: %.02f\n", wd->orient.yaw); // frozen at 0.00
            printf("ROLL: %.02f\n", wd->orient.roll);

            printf("\n");
            printf("MP:\n");
            printf("PITCH: %i\n", wd->exp.mp.rx);
            printf("YAW: %i\n", wd->exp.mp.ry);
            printf("ROLL: %i\n", wd->exp.mp.rz);

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
