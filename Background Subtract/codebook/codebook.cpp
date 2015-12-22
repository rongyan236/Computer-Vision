#include <cv.h>
#include <highgui.h>

int CVCONTOUR_APPROX_LEVEL = 2;   
int CVCLOSE_ITR = 1;

#define CV_CVX_WHITE	CV_RGB(0xff,0xff,0xff)
#define CV_CVX_BLACK	CV_RGB(0x00,0x00,0x00)


#define CHANNELS 3		
// 设置处理的图像通道数,要求小于等于图像本身的通道数

///////////////////////////////////////////////////////////////////////////
// 下面为码本码元的数据结构
// 处理图像时每个像素对应一个码本,每个码本中可有若干个码元

typedef struct ce {
	uchar	learnHigh[CHANNELS];	// High side threshold for learning
	// 此码元各通道的阀值上限(学习界限)
	uchar	learnLow[CHANNELS];		// Low side threshold for learning
	// 此码元各通道的阀值下限
	// 学习过程中如果一个新像素各通道值x[i],均有 learnLow[i]<=x[i]<=learnHigh[i],则该像素可合并于此码元
	uchar	max[CHANNELS];			// High side of box boundary
	// 属于此码元的像素中各通道的最大值
	uchar	min[CHANNELS];			// Low side of box boundary
	// 属于此码元的像素中各通道的最小值
	int		t_last_update;			// This is book keeping to allow us to kill stale entries
	// 此码元最后一次更新的时间,每一帧为一个单位时间,用于计算stale
	int		stale;					// max negative run (biggest period of inactivity)
	// 此码元最长不更新时间,用于删除规定时间不更新的码元,精简码本
} code_element;						// 码元的数据结构

typedef struct code_book {
	code_element	**cb;
	// 码元的二维指针,理解为指向码元指针数组的指针,使得添加码元时不需要来回复制码元,只需要简单的指针赋值即可
	int				numEntries;
	// 此码本中码元的数目
	int				t;				// count every access
	// 此码本现在的时间,一帧为一个时间单位
} codeBook;							// 码本的数据结构

codeBook* TcodeBook;//包括所有像素的码书集合

////////////////////////////////////////////////////////////////////
// int update_codebook(uchar *p, codeBook &c, unsigned cbBounds,,int numChannels)
// Updates the codebook entry with a new data point
// p Pointer to a YUV or HSI pixel
// c Codebook for this pixel
// cbBounds Learning bounds for codebook (cvBounds must be of length equal to numChannels)
// numChannels Number of color channels we’re learning
// codebook index
////////////////////////////////////////////////////////////////////
int update_codebook(uchar* p, codeBook& c, unsigned* cbBounds,int numChannels)
{
	int i = 0 ;
	unsigned int high[3],low[3];
	int n;
	for(n=0; n< numChannels; n++)
	{
		high[n] = *(p+n) + *(cbBounds+n);
		// *(p+n) 和 p[n] 结果等价,经试验*(p+n) 速度更快
		if(high[n] > 255) 
			high[n] = 255;
		low[n] = *(p+n)-*(cbBounds+n);
		if(low[n] < 0) 
			low[n] = 0;
		// 用p 所指像素通道数据,加减cbBonds中数值,作为此像素阀值的上下限
	}
	int matchChannel;

	// SEE IF THIS FITS AN EXISTING CODEWORD  测试是否有满足的码元
	for(i=0; i<c.numEntries; i++)
	{
		// 遍历此码本每个码元,测试p像素是否满足其中之一
		matchChannel = 0;
		for (n=0; n<numChannels; n++)
			//遍历每个通道
		{
			if((c.cb[i]->learnLow[n] <= *(p+n)) &&
				//Found an entry for this channel
				(*(p+n) <= c.cb[i]->learnHigh[n]))// 如果p 像素通道数据在该码元阀值上下限之间
			{
				matchChannel++;
			}
		}
		// 如果p 像素各通道都满足上面条件，则是背景
		if (matchChannel == numChannels)		// If an entry was found over all channels
		{
			// 更新该码元时间为当前时间
			c.cb[i]->t_last_update = c.t;
			// adjust this codeword for the first channel  调整该码元各通道最大最小值
			for (n=0; n<numChannels; n++)
			{
				if(c.cb[i]->max[n] < *(p+n))
				{
					c.cb[i]->max[n] = *(p+n);
				}
				else if(c.cb[i]->min[n] > *(p+n))
				{
					c.cb[i]->min[n] = *(p+n);
				}
			}

			break;
		}
	}

	// ENTER A NEW CODEWORD IF NEEDED  新建码元
	if(i == c.numEntries) //if no existing codeword found, make one
	{
		// 为该码元申请c.numEntries+1 个指向码元的指针
		code_element **foo = new code_element* [c.numEntries+1];
		
		for(int ii=0; ii<c.numEntries; ii++)
		{	// 将前c.numEntries 个指针指向已存在的每个码元
			foo[ii] = c.cb[ii];
		}
		// 申请一个新的码元
		foo[c.numEntries] = new code_element;
		// 删除c.cb 指针数组
		if(c.numEntries)
			delete [] c.cb;
		// 把foo 头指针赋给c.cb
		c.cb = foo;
		// 更新新码元各通道数据
		for(n=0; n<numChannels; n++)
		{
			c.cb[c.numEntries]->learnHigh[n] = high[n];
			c.cb[c.numEntries]->learnLow[n] = low[n];
			c.cb[c.numEntries]->max[n] = *(p+n);
			c.cb[c.numEntries]->min[n] = *(p+n);
		}
		c.cb[c.numEntries]->t_last_update = c.t;
		c.cb[c.numEntries]->stale = 0;
		c.numEntries += 1;
	}

		// OVERHEAD TO TRACK POTENTIAL STALE ENTRIES
	// 计算该码元的不更新时间stale
	for(int s=0; s<c.numEntries; s++)
	{
		// Track which codebook entries are going stale:
		int negRun = c.t - c.cb[s]->t_last_update;
		if(c.cb[s]->stale < negRun) // 计算该码元的不更新时间
			c.cb[s]->stale = negRun;
	}

	// SLOWLY ADJUST LEARNING BOUNDS
	// 如果像素通道数据在高低阀值范围内,但在码元阀值之外,则缓慢调整此码元学习界限
	for(n=0; n<numChannels; n++)
	{
		if(c.cb[i]->learnHigh[n] < high[n]) 
			c.cb[i]->learnHigh[n] += 1;
		if(c.cb[i]->learnLow[n] > low[n]) 
			c.cb[i]->learnLow[n] -= 1;
	}
	return(i);
}

///////////////////////////////////////////////////////////////////
//int clear_stale_entries(codeBook &c)
// During learning, after you’ve learned for some period of time,
// periodically call this to clear out stale codebook entries
//
// c Codebook to clean up
//
// Return
// number of entries cleared
//
int clear_stale_entries(codeBook &c)
{
	int staleThresh = c.t >> 1;			// 设定刷新时间
	int *keep = new int [c.numEntries];	// 申请一个标记数组
	int keepCnt = 0;					// 记录不删除码元数目
	//SEE WHICH CODEBOOK ENTRIES ARE TOO STALE
	for (int i=0; i<c.numEntries; i++)
		// 遍历码本中每个码元
	{
		if (c.cb[i]->stale > staleThresh)	
			// 如码元中的不更新时间大于设定的刷新时间,则标记为删除 Maximum Negative Run-Length (MNRL)
			keep[i] = 0; //Mark for destruction
		else
		{
			keep[i] = 1; //Mark to keep
			keepCnt += 1;
		}
	}
	// KEEP ONLY THE GOOD
	c.t = 0;						//Full reset on stale tracking
	// 码本时间清零
	code_element **foo = new code_element* [keepCnt];
	// 申请大小为keepCnt 的码元指针数组
	int k=0;
	for(int ii=0; ii<c.numEntries; ii++)
	{
		if(keep[ii])
		{
			foo[k] = c.cb[ii];
			//We have to refresh these entries for next clearStale
			foo[k]->t_last_update = 0;
			k++;
		}
	}
	// CLEAN UP
	//
	delete [] keep;
	delete [] c.cb;
	c.cb = foo;
	// 把foo 头指针地址赋给c.cb 
	int numCleared = c.numEntries - keepCnt;
	// 被清理的码元个数
	c.numEntries = keepCnt;
	// 剩余的码元地址
	return(numCleared);
}

////////////////////////////////////////////////////////////
// uchar background_diff( uchar *p, codeBook &c,
// int minMod, int maxMod)
// Given a pixel and a codebook, determine if the pixel is
// covered by the codebook
//
// p Pixel pointer (YUV interleaved)
// c Codebook reference
// numChannels Number of channels we are testing
// maxMod Add this (possibly negative) number onto

// max level when determining if new pixel is foreground
// minMod Subract this (possibly negative) number from
// min level when determining if new pixel is foreground
//
// NOTES:
// minMod and maxMod must have length numChannels,
// e.g. 3 channels => minMod[3], maxMod[3]. There is one min and
// one max threshold per channel.
//
// Return
// 0 => background, 255 => foreground
//
uchar background_diff(
					  uchar* p,
					  codeBook& c,
					  int numChannels,
					  int* minMod,
					  int* maxMod
					  )
{
	int i = 0 ;
	int matchChannel;
	// SEE IF THIS FITS AN EXISTING CODEWORD
	//
	for(i=0; i<c.numEntries; i++)
	{
		matchChannel = 0;
		for(int n=0; n<numChannels; n++) 
		{
			if((c.cb[i]->min[n] - minMod[n] <= *(p+n)) && (*(p+n) <= c.cb[i]->max[n] + maxMod[n])) 
			{
					matchChannel++; //Found an entry for this channel
			} 
			else 
			{
				break;
			}
		}
		if(matchChannel == numChannels) 
		{
			break; //Found an entry that matched all channels
		}
	}
	if(i >= c.numEntries) 
		return(255);	// p像素各通道值满足码本中其中一个码元,则返回白色
	else
		return(0);
}
void connected_Components(IplImage *mask, int poly1_hull0, float perimScale, int *num, CvRect *bbs, CvPoint *centers)
{
	static CvMemStorage*	mem_storage	= NULL;
	static CvSeq*			contours	= NULL;

	//CLEAN UP RAW MASK
	cvMorphologyEx( mask, mask, NULL, NULL, CV_MOP_OPEN ,1);
	// 开运算先腐蚀在膨胀,腐蚀可以清除噪点,膨胀可以修复裂缝
	cvMorphologyEx( mask, mask, NULL, NULL, CV_MOP_CLOSE,2);
	// 闭运算先膨胀后腐蚀,之所以在开运算之后,因为噪点膨胀后再腐蚀,是不可能去除的

	//FIND CONTOURS AROUND ONLY BIGGER REGIONS
	if( mem_storage==NULL ) mem_storage = cvCreateMemStorage(0);
	else cvClearMemStorage(mem_storage);

	CvContourScanner scanner = cvStartFindContours(mask,mem_storage,sizeof(CvContour),CV_RETR_EXTERNAL,CV_CHAIN_APPROX_SIMPLE);
	// 在之前已讨论过这种轮廓方法,如果只提取轮廓没有cvFindContours()来的方便
	// 但下面要对轮廓进行直接操作,看来这种方法功能更强大一点
	CvSeq* c;
	int numCont = 0;
	while( (c = cvFindNextContour( scanner )) != NULL )
	{
		double len = cvContourPerimeter( c );
		double q = (mask->height + mask->width) /perimScale;   //calculate perimeter len threshold
		if( len < q ) //Get rid of blob if it's perimeter is too small
		{
			cvSubstituteContour( scanner, NULL );
		}
		else //Smooth it's edges if it's large enough
		{
			CvSeq* c_new;
			if(poly1_hull0) 
				// Polygonal approximation of the segmentation
				c_new = cvApproxPoly(c, sizeof(CvContour), mem_storage,
							CV_POLY_APPROX_DP, CVCONTOUR_APPROX_LEVEL);
			else 
				// Convex Hull of the segmentation
				c_new = cvConvexHull2(c,mem_storage, CV_CLOCKWISE, 1);
			
			cvSubstituteContour(scanner, c_new);
			// 替换轮廓扫描器中提取的轮廓
			numCont++;
		}
	}
	contours = cvEndFindContours( &scanner );

	// PAINT THE FOUND REGIONS BACK INTO THE IMAGE
	cvZero( mask );
	// 将掩模图像清零
	// 掩模: 指是由0和1组成的一个二进制图像
	IplImage *maskTemp;
	//CALC CENTER OF MASS AND OR BOUNDING RECTANGLES
	if(num != NULL)
	{
		int N = *num, numFilled = 0, i=0;
		CvMoments moments;
		double M00, M01, M10;
		maskTemp = cvCloneImage(mask);
		for(i=0, c=contours; c != NULL; c = c->h_next,i++ )
		{
			if(i < N) //Only process up to *num of them
			{
				cvDrawContours(maskTemp,c,CV_CVX_WHITE, CV_CVX_WHITE,-1,CV_FILLED,8);
				//Find the center of each contour
				if(centers != NULL)
				{
					cvMoments(maskTemp,&moments,1);
					M00 = cvGetSpatialMoment(&moments,0,0);
					M10 = cvGetSpatialMoment(&moments,1,0);
					M01 = cvGetSpatialMoment(&moments,0,1);
					centers[i].x = (int)(M10/M00);
					centers[i].y = (int)(M01/M00);
				}
				//Bounding rectangles around blobs
				if(bbs != NULL)
				{
					bbs[i] = cvBoundingRect(c);
				}
				cvZero(maskTemp);
				numFilled++;
			}
			//Draw filled contours into mask
			cvDrawContours(mask,c,CV_CVX_WHITE,CV_CVX_WHITE,-1,CV_FILLED,8); //draw to central mask
		} //end looping over contours
		*num = numFilled;
		cvReleaseImage( &maskTemp);
	}
	else
	{
		for( c=contours; c != NULL; c = c->h_next )
		{
			cvDrawContours(mask,c,CV_CVX_WHITE, CV_CVX_BLACK,-1,CV_FILLED,8);
		}
	}
}
IplImage* pFrame = NULL;
IplImage* pFrameHSV = NULL;
IplImage* pFrImg = NULL;
CvCapture*	capture = NULL;
int nFrmNum = 0;
//IplImage* pFrImg = NULL;
//IplImage* pBkImg = NULL;

unsigned cbBounds = 5;

int height,width;
int nchannels;
int minMod[3]={30,30,30}, maxMod[3]={30,30,30};

int main(int argc, char* argv[])
{
	//创建窗口
	cvNamedWindow("video", 1);
	cvNamedWindow("HSV空间图像",1);
	cvNamedWindow("foreground",1);
	//使窗口有序排列
	cvMoveWindow("video", 30, 0);
	cvMoveWindow("HSV空间图像", 360, 0);
	cvMoveWindow("foreground", 690, 0);
	//打开视频文件，
	//capture = cvCaptureFromFile("video/bike.avi");
	capture = cvCreateCameraCapture( 0 );
	if( !capture)
	{
		printf("Couldn't open the capture!");
		return -2;
	}

	int j;
	//逐帧读取视频
	while(pFrame = cvQueryFrame( capture ))
	{
		nFrmNum++;
		cvShowImage("video", pFrame);

		if (nFrmNum == 1)
		{
			height =  pFrame->height;
			width = pFrame->width;
			nchannels = pFrame->nChannels;
			pFrameHSV = cvCreateImage(cvSize(pFrame->width, pFrame->height), IPL_DEPTH_8U,3);
			pFrImg = cvCreateImage(cvSize(pFrame->width, pFrame->height), IPL_DEPTH_8U,1);
			//cvCvtColor(pFrame, pFrameHSV, CV_BGR2HSV);//色彩空间转化
			// 申请码本空间，每个像素一个码本。
			TcodeBook = new codeBook[width*height];

			for(j = 0; j < width*height; j++)
			{
				TcodeBook[j].numEntries = 0;
				TcodeBook[j].t = 0;
			}
		}

		if (nFrmNum<=30)// 30帧内进行背景学习
		{
			//cvCvtColor(pFrame, pFrameHSV, CV_BGR2HSV);//色彩空间转化
			cvCopyImage(pFrame,pFrameHSV);
			//学习背景
			for(j = 0; j < width*height; j++)
				// 对每个像素进行背景学习
				update_codebook((uchar*)pFrameHSV->imageData+j*nchannels, TcodeBook[j],&cbBounds,3);
		}
		else
		{
			//cvCvtColor(pFrame, pFrameHSV, CV_BGR2HSV);//色彩空间转化
			cvCopyImage(pFrame,pFrameHSV);

			if(nFrmNum%20 == 0)
			{
				for(j = 0; j < width*height; j++)
					update_codebook((uchar*)pFrameHSV->imageData+j*nchannels, TcodeBook[j],&cbBounds,3);
			}
			if(nFrmNum%40 == 0)
			{
				for(j = 0; j < width*height; j++)
					clear_stale_entries(TcodeBook[j]);//删除码本中陈旧的码元
			}
			for(j = 0; j < width*height; j++)
			{
				if(background_diff((uchar*)pFrameHSV->imageData+j*nchannels, TcodeBook[j],3,minMod,maxMod))
				{
					pFrImg->imageData[j] = 255;
				}
				else
				{
					pFrImg->imageData[j] = 0;
				}
			}

			//connected_Components(pFrImg,1,20,NULL,NULL, NULL);

			cvShowImage("foreground", pFrImg);
			cvShowImage("HSV空间图像", pFrameHSV);
		}
		if( cvWaitKey(30) >= 0 )
			break;   

	} // end of while-loop

	for(j = 0; j < width*height; j++)
	{
		if (!TcodeBook[j].cb)
			delete [] TcodeBook[j].cb;
	}
	if (!TcodeBook)
		delete [] TcodeBook;
	//销毁窗口
	cvDestroyWindow("video");
	cvDestroyWindow("HSV空间图像");
	cvDestroyWindow("foreground");
	return 0;
}