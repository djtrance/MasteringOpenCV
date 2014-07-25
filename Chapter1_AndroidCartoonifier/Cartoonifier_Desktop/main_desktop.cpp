/*****************************************************************************
*   Cartoonifier_Desktop.cpp, for Desktop.
*   Converts a real-life camera stream to look like a cartoon.
*   This file is for a desktop executable, but the cartoonifier can also be used in an Android / iOS project.
******************************************************************************
*   by Shervin Emami, 5th Dec 2012 (shervin.emami@gmail.com)
*   http://www.shervinemami.info/
******************************************************************************
*   Ch1 of the book "Mastering OpenCV with Practical Computer Vision Projects"
*   Copyright Packt Publishing 2012.
*   http://www.packtpub.com/cool-projects-with-opencv/book
*****************************************************************************/


// Try to set the camera resolution. Note that this only works for some cameras on
// some computers and only for some drivers, so don't rely on it to work!
const int DESIRED_CAMERA_WIDTH = 160;
const int DESIRED_CAMERA_HEIGHT = 120;

const int NUM_STICK_FIGURE_ITERATIONS = 40; // Sets how long the stick figure face should be shown for skin detection.

const char *windowName = "Cartoonifier";   // Name shown in the GUI window.


// Set to true if you want to see line drawings instead of paintings.
bool m_sketchMode = false;
// Set to true if you want to change the skin color of the character to an alien color.
bool m_alienMode = false;
// Set to true if you want an evil "bad" character instead of a "good" character.
bool m_evilMode = false;
// Set to true if you want to see many windows created, showing various debug info. Set to 0 otherwise.
bool m_debugMode = false;

bool m_enableDisplay = false;
bool m_enableOctoscroller = true;
bool m_enablePalette = true;
bool m_enablePalette2 = true;

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>


// Include OpenCV's C++ Interface
#include "opencv2/opencv.hpp"

// Include the rest of our code!
//#include "detectObject.h"       // Easily detect faces or eyes (using LBP or Haar Cascades).
#include "cartoon.h"            // Cartoonify a photo.
#include "ImageUtils.h"      // Shervin's handy OpenCV utility functions.

using namespace cv;
using namespace std;

int m_stickFigureIterations = 0;  // Draws a stick figure outline for where the user's face should be.

#if !defined VK_ESCAPE
    #define VK_ESCAPE 0x1B      // Escape character (27)
#endif



// Get access to the webcam.
void initWebcam(VideoCapture &videoCapture, int cameraNumber)
{
    // Get access to the default camera.
    try {   // Surround the OpenCV call by a try/catch block so we can give a useful error message!
        videoCapture.open(cameraNumber);
    } catch (cv::Exception &e) {}
    if ( !videoCapture.isOpened() ) {
        cerr << "ERROR: Could not access the camera!" << endl;
        exit(1);
    }
    cout << "Loaded camera " << cameraNumber << "." << endl;
}


// Keypress event handler. Note that it should be a 'char' and not an 'int' to better support Linux.
void onKeypress(char key)
{
    switch (key) {
    case 's':
        m_sketchMode = !m_sketchMode;
        cout << "Sketch / Paint mode: " << m_sketchMode << endl;
        break;
    case 'a':
        m_alienMode = !m_alienMode;
        cout << "Alien / Human mode: " << m_alienMode << endl;
        // Display a stick figure outline when alien skin is enabled,
        // so the user puts their face in the correct place.
        if (m_alienMode) {
            m_stickFigureIterations = NUM_STICK_FIGURE_ITERATIONS;
        }
        break;
    case 'e':
        m_evilMode = !m_evilMode;
        cout << "Evil / Good mode: " << m_evilMode << endl;
        break;
    case 'd':
        m_debugMode = !m_debugMode;
        cout << "Debug mode: " << m_debugMode << endl;
        break;
    }
}

#define PALETTE_SIZE 10
#define RED 2
#define GREEN 0
#define BLUE 1
float dist(uint8_t a[], Vec3b b) {
    //return(sqrt(pow((float)a[0]-b[0],2)+pow((float)a[1]-b[1],2)+pow((float)a[2]-b[2],2)));
    //return(abs((float)a[0]-b[0])+abs((float)a[1]-b[1])+abs((float)a[2]-b[2]));
    return((float)((a[0]^(uint8_t)b[RED])+(a[1]^(uint8_t)b[GREEN])+(a[2]^(uint8_t)b[BLUE])));
}

int main(int argc, char *argv[])
{
    int octoSocket;
    unsigned octoAddress = (127 << 24) | (0 << 16) | (0 << 8) | (1 << 0);
    sockaddr_in octoAddr;
    octoAddr.sin_family = AF_INET;
    octoAddr.sin_addr.s_addr = htonl(octoAddress);
    octoAddr.sin_port = htons(9999);
    ssize_t octoSize = 128*128*3+1;
    uint8_t * octoData = (uint8_t *)calloc(octoSize, 1);
    octoData[0] = 0;

    const uint8_t paletteRed[] =   { 0x00, 0x20, 0x20, 0x80, 0x80, 0xe0, 0xe0, 0xe0 };
    const uint8_t paletteGreen[] = { 0x00, 0x20, 0x20, 0x20, 0x20, 0x80, 0x80, 0xe0 };
    const uint8_t paletteBlue[] =  { 0x00, 0x20, 0x20, 0x40, 0x60, 0x60, 0x60, 0xe0 };

    uint8_t palette[PALETTE_SIZE][3] =
    {
        {0x00, 0x00, 0x00},
        {0x2f, 0x00, 0x2f},
        {0x20, 0x40, 0x20},
        {0x80, 0x80, 0xb0},
        {0xe0, 0xb0, 0x00},
        {0xb0, 0x10, 0x20},
        {0xe0, 0x80, 0x80},
        {0xe0, 0x60, 0x20},
        {0xe0, 0xb0, 0x20},
        {0x80, 0xe0, 0xe0},
    };

    cout << "Cartoonifier, by Shervin Emami (www.shervinemami.info), June 2012." << endl;
    cout << "Converts real-life images to cartoon-like images." << endl;
    cout << "Compiled with OpenCV version " << CV_VERSION << endl;
    cout << endl;

    cout << "Keyboard commands (press in the GUI window):" << endl;
    cout << "    Esc:  Quit the program." << endl;
    cout << "    s:    change Sketch / Paint mode." << endl;
    cout << "    a:    change Alien / Human mode." << endl;
    cout << "    e:    change Evil / Good character mode." << endl;
    cout << "    d:    change debug mode." << endl;
    cout << endl;

    // Allow the user to specify a camera number, since not all computers will be the same camera number.
    int cameraNumber = 0;   // Change this if you want to use a different camera device.
    if (argc > 1) {
        cameraNumber = atoi(argv[1]);
    }

    // Get access to the camera.
    VideoCapture camera;
    initWebcam(camera, cameraNumber);

    // Try to set the camera resolution. Note that this only works for some cameras on
    // some computers and only for some drivers, so don't rely on it to work!
    camera.set(CV_CAP_PROP_FRAME_WIDTH, DESIRED_CAMERA_WIDTH);
    camera.set(CV_CAP_PROP_FRAME_HEIGHT, DESIRED_CAMERA_HEIGHT);

    // Create a GUI window for display on the screen.
    if(m_enableDisplay) {
        namedWindow(windowName); // Resizable window, might not work on Windows.
    }

    if(m_enableOctoscroller) {
        octoSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }

    // Run forever, until the user hits Escape to "break" out of this loop.
    while (true) {

        // Grab the next camera frame. Note that you can't modify camera frames.
        Mat cameraFrame;
        camera >> cameraFrame;
        if( cameraFrame.empty() ) {
            cerr << "ERROR: Couldn't grab the next camera frame." << endl;
            exit(1);
        }

        Mat displayedFrame = Mat(cameraFrame.size(), CV_8UC3);

        // Use debug type 2 (for desktop) if debug mode is enabled.
        int debugType = 0;
        if (m_debugMode)
            debugType = 2;

        // Run the cartoonifier filter using the selected mode.
        cartoonifyImage(cameraFrame, displayedFrame, m_sketchMode, m_alienMode, m_evilMode, debugType);

        // Show a stick-figure outline of a face for a short duration, so the user knows where to put their face.
        if (m_stickFigureIterations > 0) {
            drawFaceStickFigure(displayedFrame);
            m_stickFigureIterations--;
        }

        if(m_enableDisplay) {
            imshow(windowName, displayedFrame);
        }

        if(m_enableOctoscroller) {
            //Mat croppedImage = displayedFrame(Rect(16, 0, 128, 120));
            for(int i = 0; i < 128; i++) {
                for(int j = 0; j < 120; j++) {
                    if(m_enablePalette2) {
                        uint8_t red = ((uint8_t)displayedFrame.at<Vec3b>(j,i+16)[RED] >> 5);
                        octoData[1+3*(i+j*128)+3*8*128] = paletteRed[red];
                        uint8_t green = ((uint8_t)displayedFrame.at<Vec3b>(j,i+16)[GREEN] >> 5);
                        octoData[2+3*(i+j*128)+3*8*128] = paletteGreen[green];
                        uint8_t blue = ((uint8_t)displayedFrame.at<Vec3b>(j,i+16)[BLUE] >> 5);
                        octoData[3+3*(i+j*128)+3*8*128] = paletteBlue[blue];
                    } else if(m_enablePalette) {
                        float minDistance = dist(palette[0], displayedFrame.at<Vec3b>(j,i+16));
                        int index = 0;
                        for(int k = 1; k < PALETTE_SIZE; k++) {
                            float distance = dist(palette[k], displayedFrame.at<Vec3b>(j,i+16));
                            if(distance < minDistance) {
                                index = k;
                                minDistance = distance;
                            }
                        }
                        octoData[1+3*(i+j*128)+3*8*128] = palette[index][0];
                        octoData[2+3*(i+j*128)+3*8*128] = palette[index][1];
                        octoData[3+3*(i+j*128)+3*8*128] = palette[index][2];
                    } else {
                        octoData[1+3*(i+j*128)+3*8*128] = displayedFrame.at<Vec3b>(j,i+16)[RED];
                        octoData[2+3*(i+j*128)+3*8*128] = displayedFrame.at<Vec3b>(j,i+16)[GREEN];
                        octoData[3+3*(i+j*128)+3*8*128] = displayedFrame.at<Vec3b>(j,i+16)[BLUE];
                    }
                }
            }
            sendto(octoSocket, 
                (void *)octoData, 
                octoSize,
                0,
                (const sockaddr *)&octoAddr, 
                sizeof(sockaddr_in));
        }


        if(!m_enableOctoscroller) {
        // IMPORTANT: Wait for atleast 20 milliseconds, so that the image can be displayed on the screen!
        // Also checks if a key was pressed in the GUI window. Note that it should be a "char" to support Linux.
        char keypress = waitKey(20);  // This is needed if you want to see anything!
        if (keypress == VK_ESCAPE) {   // Escape Key
            // Quit the program!
            break;
        }
        // Process any other keypresses.
        if (keypress > 0) {
            onKeypress(keypress);
        }
        }

    }//end while

    return EXIT_SUCCESS;
}
