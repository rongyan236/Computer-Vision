/*------------------------------------------------------------------------------------------*\ 
   Copyright (C) 2015.11.15 
   Author: tfygg
\*------------------------------------------------------------------------------------------*/

#if !defined BGFG
#define BGFG

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "videoprocess.h"

class interFrameDiff : public FrameProcessor 
{
	cv::Mat currentFrame;
	cv::Mat previousFrame;
	cv::Mat foreground;

	bool number;

  public:

	interFrameDiff():number(1) {}

	// processing method
	void process(cv:: Mat &frame, cv:: Mat &output, cv::Mat &backFrame) {

		// convert to gray-level image
		cv::cvtColor(frame, currentFrame, CV_BGR2GRAY); 
		// initialize background to 1st frame

		if (number)
		{
			currentFrame.convertTo(backFrame, CV_8U);
			currentFrame.convertTo(output, CV_8U);
			number = false;
			
		}
		else
		{

			// compute difference between current image and background
			cv::absdiff(backFrame,currentFrame,foreground);
			
			// apply threshold to foreground image
			cv::threshold(foreground,output,20,255,cv::THRESH_BINARY_INV);

			currentFrame.convertTo(backFrame, CV_8U);
		}
	}
};

#endif
