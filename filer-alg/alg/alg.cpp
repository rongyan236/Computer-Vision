
#if  defined(Hi3516)
#include <stdio.h>
#include "../Include/opencv2/core/core.hpp"
#include "../Include/opencv2/imgproc/imgproc.hpp"
#include "../Include/opencv2/highgui/highgui.hpp"

#include <math.h>

#include "alg.h"


using namespace cv;
using namespace std;


HI_S32 YV12ToBGR24_OpenCV(unsigned char* pYUV,unsigned char* pBGR24,int width,int height)
{
    cv::Mat dst(height,width,CV_8UC3,pBGR24);
    cv::Mat src(height + height/2,width,CV_8UC1,pYUV);
    cv::cvtColor(src,dst,CV_YUV420sp2RGB);
	return HI_SUCCESS;
}
HI_S32 NV21toRGB(unsigned char*pFrame,unsigned char* pRGB,int width,int height)
{   
      
      cv::Mat dst(height,width,CV_8UC3,pRGB);  
      cv::Mat src(height + height/2,width,CV_8UC1,pFrame);
	//  printf(" Mat end******************************** ***************ok\n");
     cv::cvtColor(src,dst,CV_YUV420sp2BGR);//hisi中使用CV_YUV420sp2BGR
	//  printf(" cvtColor end******************************** ***************ok\n");
      //cv::imwrite("1.bmp",dst);
	  return HI_SUCCESS;
}

HI_S32 RGB2NV21(unsigned char*pRGB,unsigned char* pFrame,int width,int height)
{   
      
      cv::Mat src(height,width,CV_8UC3,pRGB);  
      cv::Mat dst(height + height/2,width,CV_8UC1,pFrame);
	  printf(" Mat end1******************************** ***************ok\n");
     cv::cvtColor(src,dst,CV_BGR2YUV);//hisi中使用CV_YUV420sp2BGR
	  printf(" cvtColor1 end******************************** ***************ok\n");
	//  pFrame
    //  cv::imwrite("1-1.bmp",dst);
	  return HI_SUCCESS;
}

void rgb_1(Mat &input, Mat &B,Mat &G,Mat &R,Mat &R_G)
{
	  // create vector of 3 images
        vector<cv::Mat> planes;
        // split 1 3-channel image into 3 1-channel images
        split(input,planes);
	
       B = planes[0];    
       G = planes[1];  
       R = planes[2];  
	R_G = planes[2]-planes[1];
}

int  averGrey(InputArray _src)
{
      Mat src = _src.getMat();
      int  nr= src .rows; // number of rows
      int nc= src .cols; // total number of elements per line    
      int aver=0;
	  
	if (src.isContinuous())  {  
          // then no padded pixels  
          nc= nc*nr;   
          nr= 1;  // it is now a 1D array  
      }
      	  
      for (int j=0; j<nr; j++) 
      {  
          uchar* data= src.ptr<uchar>(j);    	  
          for (int i=0; i<nc; i++) 
	    {  
              aver+=data[i*nr+j];	
            }                   
       }  
	  
	if(nr== 1)
	    aver=aver/nc;
	else
	  aver=aver/nc/nr;
	  
  return aver ;
}


void BW_img(unsigned char*pRGB,unsigned char*pBW,int width,int height)
{
     Mat img(height,width,CV_8UC3,pRGB);
     Mat BW(height,width,CV_8UC1,pBW);
     Mat B1,G1,R1,R_G;
     double min,max;
     int a1,a2,a3;
	 
     rgb_1(img, B1,G1,R1,R_G);

     a1= averGrey(R_G);
     minMaxIdx(R_G,&min,&max);
     a2=(min+max)/2;
     a3=3*averGrey(R1)+2*averGrey(B1)-4*averGrey(G1);

     int  nr= R_G.rows; // number of rows
     int nc= R_G.cols * R_G.channels(); // total number of elements per line  
     int  nr_R= R1.rows; // number of rows
     int nc_R= R1.cols * R1.channels(); // total number of elements per line  
      if (R_G.isContinuous())  {  
          // then no padded pixels  
          nc= nc*nr;   
          nr= 1;  // it is now a 1D array  
        }
     if (R1.isContinuous())  {  
          // then no padded pixels  
          nc_R= nc_R*nr_R;   
          nr_R= 1;  // it is now a 1D array  
         }  
      //BW=B1.clone();

	 //便利一次速度下降很多,此步骤严重耗时
   for (int j=0; j<nr; j++) 
   {  
          uchar* data= R_G.ptr<uchar>(j);  
	   uchar* data_R= R1.ptr<uchar>(j);  
	   uchar* data_BW= BW.ptr<uchar>(j);  
          for (int i=0; i<nc; i++) 
	    {  
               if(data[i]>a1 && ( data[i]<a2 ) && ( data_R[i]<a3 ))
                      data_BW[i] = 255;
		   else
		   	  data_BW[i] = 0;
            }                   
      } 
  //  imshow("BW",BW);
   // imshow("img",img);
  //  cvWaitKey(0);
}

void FBcorrect(unsigned char*pBW,unsigned char*pRGB,unsigned char*pimg_dst,int width,int height)
{
         Mat BW(height,width,CV_8UC1,pBW);
	 Mat img(height,width,CV_8UC3,pRGB);
	 Mat img_dst(height,width,CV_8UC3,pimg_dst);
	//  imshow("BW",BW);
   	// imshow("img",img);
    	// cvWaitKey(0);
	 Mat img_BW,img1,BW2,img_BW2,dst_bg;	 
	 cvtColor(BW,img_BW,CV_GRAY2BGR);

	  img1=img_BW & img;

         Mat dst_tree = img1.clone();
         vector<Mat>  mv; 
         vector<Mat>  mv_end;
         split(img1,mv); 
         split(dst_tree,mv_end);

         mv_end[2] = mv[2]-(mv[1]+mv[0])*0.25;
         mv_end[0] = mv[0]- (mv[1]+mv[2])*0.25;
         mv_end[1] = mv[1];
         merge(mv_end,dst_tree);

         BW2=~BW;
         cvtColor(BW2, img_BW2, CV_GRAY2BGR);
         dst_bg= img_BW2 & img;
         img_dst= dst_bg + dst_tree;
       //  imshow("img_dst",img_dst);

}


void FBcorrect1(unsigned char*pBW,unsigned char*pRGB,unsigned char*pimg_dst1,int width,int height)
{
         Mat BW(height,width,CV_8UC1,pBW);
	 Mat img(height,width,CV_8UC3,pRGB);
	 Mat img_dst1(height,width,CV_8UC3,pimg_dst1);
	//  imshow("BW",BW);
   	// imshow("img",img);
    	// cvWaitKey(0);
	 Mat img_BW,img1;	 
	 cvtColor(BW,img_BW,CV_GRAY2BGR);

	  img1=img_BW & img;

         vector<Mat>  mv; 
         vector<Mat>  mv_end;
         split(img1,mv); 
         split(img_dst1,mv_end);
//vfp运算能力不强，慢0.01s
         mv_end[2] = mv[2]-(25*mv[1]+25*mv[0])/100;
         mv_end[0] = mv[0]- (25*mv[1]+25*mv[2])/100;
         mv_end[1] = mv[1];
	//mv_end[2] = mv[2]-(mv[1]+mv[0])*0.25;
        // mv_end[0] = mv[0]- (mv[1]+mv[2])*0.25;
        // mv_end[1] = mv[1];

         merge(mv_end,img_dst1);

}

void FBcorrect2(unsigned char*pBW,unsigned char*pRGB,unsigned char*pimg_dst1,unsigned char*pimg_dst,int width,int height)
{
         Mat BW(height,width,CV_8UC1,pBW);
	 Mat img(height,width,CV_8UC3,pRGB);
	 Mat img_dst1(height,width,CV_8UC3,pimg_dst1);
	 Mat img_dst2(height,width,CV_8UC3);
	 Mat img_dst(height,width,CV_8UC3,pimg_dst);
	//  imshow("BW",BW);
   	// imshow("img",img);
    	// cvWaitKey(0);
	 Mat BW2,img_BW2;	 

         BW2=~BW;
         cvtColor(BW2, img_BW2, CV_GRAY2BGR);
         img_dst2= img_BW2 & img;
         img_dst= img_dst1 + img_dst2;
       //  imshow("img_dst",img_dst);

}


#endif

