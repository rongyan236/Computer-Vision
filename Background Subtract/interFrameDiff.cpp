/*------------------------------------------------------------------------------------------*\ 
   Copyright (C) 2015.11.15 
   Author: tfygg
\*------------------------------------------------------------------------------------------*/


#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/background_segm.hpp>

#include "videoprocess.h"
#include "interFrameDiff.h"

int main()
{
	// Create video procesor instance
	VideoProcess processor;

	// Create background/foreground segmentor 
	interFrameDiff segmentor;

	// Open video file
	processor.setInput("bike.avi");

	// set frame processor
	processor.setFrameProcessor(&segmentor);

	// Declare a window to display the video
	processor.displayInput("Input");
	processor.displayBackground("Background");
	processor.displayOutput("Foreground");
	
	// Play the video at the original frame rate
	processor.setDelay(1000./processor.getFrameRate());

	// Start the process
	processor.run();

	cv::waitKey();
}