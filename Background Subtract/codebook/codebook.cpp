#include <cv.h>
#include <highgui.h>

int CVCONTOUR_APPROX_LEVEL = 2;   
int CVCLOSE_ITR = 1;

#define CV_CVX_WHITE	CV_RGB(0xff,0xff,0xff)
#define CV_CVX_BLACK	CV_RGB(0x00,0x00,0x00)


#define CHANNELS 3		
// ���ô����ͼ��ͨ����,Ҫ��С�ڵ���ͼ�����ͨ����

///////////////////////////////////////////////////////////////////////////
// ����Ϊ�뱾��Ԫ�����ݽṹ
// ����ͼ��ʱÿ�����ض�Ӧһ���뱾,ÿ���뱾�п������ɸ���Ԫ

typedef struct ce {
	uchar	learnHigh[CHANNELS];	// High side threshold for learning
	// ����Ԫ��ͨ���ķ�ֵ����(ѧϰ����)
	uchar	learnLow[CHANNELS];		// Low side threshold for learning
	// ����Ԫ��ͨ���ķ�ֵ����
	// ѧϰ���������һ�������ظ�ͨ��ֵx[i],���� learnLow[i]<=x[i]<=learnHigh[i],������ؿɺϲ��ڴ���Ԫ
	uchar	max[CHANNELS];			// High side of box boundary
	// ���ڴ���Ԫ�������и�ͨ�������ֵ
	uchar	min[CHANNELS];			// Low side of box boundary
	// ���ڴ���Ԫ�������и�ͨ������Сֵ
	int		t_last_update;			// This is book keeping to allow us to kill stale entries
	// ����Ԫ���һ�θ��µ�ʱ��,ÿһ֡Ϊһ����λʱ��,���ڼ���stale
	int		stale;					// max negative run (biggest period of inactivity)
	// ����Ԫ�������ʱ��,����ɾ���涨ʱ�䲻���µ���Ԫ,�����뱾
} code_element;						// ��Ԫ�����ݽṹ

typedef struct code_book {
	code_element	**cb;
	// ��Ԫ�Ķ�άָ��,���Ϊָ����Ԫָ�������ָ��,ʹ�������Ԫʱ����Ҫ���ظ�����Ԫ,ֻ��Ҫ�򵥵�ָ�븳ֵ����
	int				numEntries;
	// ���뱾����Ԫ����Ŀ
	int				t;				// count every access
	// ���뱾���ڵ�ʱ��,һ֡Ϊһ��ʱ�䵥λ
} codeBook;							// �뱾�����ݽṹ

codeBook* TcodeBook;//�����������ص����鼯��

////////////////////////////////////////////////////////////////////
// int update_codebook(uchar *p, codeBook &c, unsigned cbBounds,,int numChannels)
// Updates the codebook entry with a new data point
// p Pointer to a YUV or HSI pixel
// c Codebook for this pixel
// cbBounds Learning bounds for codebook (cvBounds must be of length equal to numChannels)
// numChannels Number of color channels we��re learning
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
		// *(p+n) �� p[n] ����ȼ�,������*(p+n) �ٶȸ���
		if(high[n] > 255) 
			high[n] = 255;
		low[n] = *(p+n)-*(cbBounds+n);
		if(low[n] < 0) 
			low[n] = 0;
		// ��p ��ָ����ͨ������,�Ӽ�cbBonds����ֵ,��Ϊ�����ط�ֵ��������
	}
	int matchChannel;

	// SEE IF THIS FITS AN EXISTING CODEWORD  �����Ƿ����������Ԫ
	for(i=0; i<c.numEntries; i++)
	{
		// �������뱾ÿ����Ԫ,����p�����Ƿ���������֮һ
		matchChannel = 0;
		for (n=0; n<numChannels; n++)
			//����ÿ��ͨ��
		{
			if((c.cb[i]->learnLow[n] <= *(p+n)) &&
				//Found an entry for this channel
				(*(p+n) <= c.cb[i]->learnHigh[n]))// ���p ����ͨ�������ڸ���Ԫ��ֵ������֮��
			{
				matchChannel++;
			}
		}
		// ���p ���ظ�ͨ���������������������Ǳ���
		if (matchChannel == numChannels)		// If an entry was found over all channels
		{
			// ���¸���Ԫʱ��Ϊ��ǰʱ��
			c.cb[i]->t_last_update = c.t;
			// adjust this codeword for the first channel  ��������Ԫ��ͨ�������Сֵ
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

	// ENTER A NEW CODEWORD IF NEEDED  �½���Ԫ
	if(i == c.numEntries) //if no existing codeword found, make one
	{
		// Ϊ����Ԫ����c.numEntries+1 ��ָ����Ԫ��ָ��
		code_element **foo = new code_element* [c.numEntries+1];
		
		for(int ii=0; ii<c.numEntries; ii++)
		{	// ��ǰc.numEntries ��ָ��ָ���Ѵ��ڵ�ÿ����Ԫ
			foo[ii] = c.cb[ii];
		}
		// ����һ���µ���Ԫ
		foo[c.numEntries] = new code_element;
		// ɾ��c.cb ָ������
		if(c.numEntries)
			delete [] c.cb;
		// ��foo ͷָ�븳��c.cb
		c.cb = foo;
		// ��������Ԫ��ͨ������
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
	// �������Ԫ�Ĳ�����ʱ��stale
	for(int s=0; s<c.numEntries; s++)
	{
		// Track which codebook entries are going stale:
		int negRun = c.t - c.cb[s]->t_last_update;
		if(c.cb[s]->stale < negRun) // �������Ԫ�Ĳ�����ʱ��
			c.cb[s]->stale = negRun;
	}

	// SLOWLY ADJUST LEARNING BOUNDS
	// �������ͨ�������ڸߵͷ�ֵ��Χ��,������Ԫ��ֵ֮��,������������Ԫѧϰ����
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
// During learning, after you��ve learned for some period of time,
// periodically call this to clear out stale codebook entries
//
// c Codebook to clean up
//
// Return
// number of entries cleared
//
int clear_stale_entries(codeBook &c)
{
	int staleThresh = c.t >> 1;			// �趨ˢ��ʱ��
	int *keep = new int [c.numEntries];	// ����һ���������
	int keepCnt = 0;					// ��¼��ɾ����Ԫ��Ŀ
	//SEE WHICH CODEBOOK ENTRIES ARE TOO STALE
	for (int i=0; i<c.numEntries; i++)
		// �����뱾��ÿ����Ԫ
	{
		if (c.cb[i]->stale > staleThresh)	
			// ����Ԫ�еĲ�����ʱ������趨��ˢ��ʱ��,����Ϊɾ�� Maximum Negative Run-Length (MNRL)
			keep[i] = 0; //Mark for destruction
		else
		{
			keep[i] = 1; //Mark to keep
			keepCnt += 1;
		}
	}
	// KEEP ONLY THE GOOD
	c.t = 0;						//Full reset on stale tracking
	// �뱾ʱ������
	code_element **foo = new code_element* [keepCnt];
	// �����СΪkeepCnt ����Ԫָ������
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
	// ��foo ͷָ���ַ����c.cb 
	int numCleared = c.numEntries - keepCnt;
	// ���������Ԫ����
	c.numEntries = keepCnt;
	// ʣ�����Ԫ��ַ
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
		return(255);	// p���ظ�ͨ��ֵ�����뱾������һ����Ԫ,�򷵻ذ�ɫ
	else
		return(0);
}
void connected_Components(IplImage *mask, int poly1_hull0, float perimScale, int *num, CvRect *bbs, CvPoint *centers)
{
	static CvMemStorage*	mem_storage	= NULL;
	static CvSeq*			contours	= NULL;

	//CLEAN UP RAW MASK
	cvMorphologyEx( mask, mask, NULL, NULL, CV_MOP_OPEN ,1);
	// �������ȸ�ʴ������,��ʴ����������,���Ϳ����޸��ѷ�
	cvMorphologyEx( mask, mask, NULL, NULL, CV_MOP_CLOSE,2);
	// �����������ͺ�ʴ,֮�����ڿ�����֮��,��Ϊ������ͺ��ٸ�ʴ,�ǲ�����ȥ����

	//FIND CONTOURS AROUND ONLY BIGGER REGIONS
	if( mem_storage==NULL ) mem_storage = cvCreateMemStorage(0);
	else cvClearMemStorage(mem_storage);

	CvContourScanner scanner = cvStartFindContours(mask,mem_storage,sizeof(CvContour),CV_RETR_EXTERNAL,CV_CHAIN_APPROX_SIMPLE);
	// ��֮ǰ�����۹�������������,���ֻ��ȡ����û��cvFindContours()���ķ���
	// ������Ҫ����������ֱ�Ӳ���,�������ַ������ܸ�ǿ��һ��
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
			// �滻����ɨ��������ȡ������
			numCont++;
		}
	}
	contours = cvEndFindContours( &scanner );

	// PAINT THE FOUND REGIONS BACK INTO THE IMAGE
	cvZero( mask );
	// ����ģͼ������
	// ��ģ: ָ����0��1��ɵ�һ��������ͼ��
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
	//��������
	cvNamedWindow("video", 1);
	cvNamedWindow("HSV�ռ�ͼ��",1);
	cvNamedWindow("foreground",1);
	//ʹ������������
	cvMoveWindow("video", 30, 0);
	cvMoveWindow("HSV�ռ�ͼ��", 360, 0);
	cvMoveWindow("foreground", 690, 0);
	//����Ƶ�ļ���
	//capture = cvCaptureFromFile("video/bike.avi");
	capture = cvCreateCameraCapture( 0 );
	if( !capture)
	{
		printf("Couldn't open the capture!");
		return -2;
	}

	int j;
	//��֡��ȡ��Ƶ
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
			//cvCvtColor(pFrame, pFrameHSV, CV_BGR2HSV);//ɫ�ʿռ�ת��
			// �����뱾�ռ䣬ÿ������һ���뱾��
			TcodeBook = new codeBook[width*height];

			for(j = 0; j < width*height; j++)
			{
				TcodeBook[j].numEntries = 0;
				TcodeBook[j].t = 0;
			}
		}

		if (nFrmNum<=30)// 30֡�ڽ��б���ѧϰ
		{
			//cvCvtColor(pFrame, pFrameHSV, CV_BGR2HSV);//ɫ�ʿռ�ת��
			cvCopyImage(pFrame,pFrameHSV);
			//ѧϰ����
			for(j = 0; j < width*height; j++)
				// ��ÿ�����ؽ��б���ѧϰ
				update_codebook((uchar*)pFrameHSV->imageData+j*nchannels, TcodeBook[j],&cbBounds,3);
		}
		else
		{
			//cvCvtColor(pFrame, pFrameHSV, CV_BGR2HSV);//ɫ�ʿռ�ת��
			cvCopyImage(pFrame,pFrameHSV);

			if(nFrmNum%20 == 0)
			{
				for(j = 0; j < width*height; j++)
					update_codebook((uchar*)pFrameHSV->imageData+j*nchannels, TcodeBook[j],&cbBounds,3);
			}
			if(nFrmNum%40 == 0)
			{
				for(j = 0; j < width*height; j++)
					clear_stale_entries(TcodeBook[j]);//ɾ���뱾�г¾ɵ���Ԫ
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
			cvShowImage("HSV�ռ�ͼ��", pFrameHSV);
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
	//���ٴ���
	cvDestroyWindow("video");
	cvDestroyWindow("HSV�ռ�ͼ��");
	cvDestroyWindow("foreground");
	return 0;
}