void  RGB2NV21(unsigned char *RgbBuf,unsigned char *yuvBuf,int nWidth,int nHeight)  
{  
    int i, j;  
    unsigned char*bufY, *bufU, *bufV, *bufRGB;   
    bufY = yuvBuf;  
    bufV = yuvBuf + nWidth * nHeight;  
    bufU = bufV + 1;  
   // bufU= bufV+ (nWidth* nHeight* 1/4);  
     
    unsigned char y, u, v, r, g, b;  
    unsigned int ylen = nWidth * nHeight;  
    unsigned int ulen = (nWidth * nHeight)/4;  
    unsigned int vlen = (nWidth * nHeight)/4;   
    for (j = 0; j<nHeight;j++)  
    {  
        bufRGB = RgbBuf + nWidth * (nHeight - 1 - j) * 3 ;  
        for (i = 0;i<nWidth;i++)  
        {  
            int pos = nWidth * i + j;  
            b = *(bufRGB++);  
            g = *(bufRGB++);  
            r = *(bufRGB++);  
            y = (unsigned char)( ( 66 * r + 129 * g +  25 * b + 128) >> 8) + 16  ;            
            u = (unsigned char)( ( -38 * r -  74 * g + 112 * b + 128) >> 8) + 128 ;            
            v = (unsigned char)( ( 112 * r -  94 * g -  18 * b + 128) >> 8) + 128 ; 
	    if(y>255)
		 y=255;
	    if(y<0)
		 y=0;
	      *(bufY++)=y;
         
            if (j%2==0&&i%2 ==0)  
            {  
                if (u>255)  
                {  
                    u=255;  
                }  
                if (u<0)  
                {  
                    u = 0;  
                }  
               *bufU = u;  
		  bufU+= 2;
		//*(bufU++) =u; 
                //存u分量  
            }  
            else  
            {  
                //存v分量  
                if (i%2==0)  
                {  
                    if (v>255)  
                    {  
                        v = 255;  
                    }  
                    if (v<0)  
                    {  
                        v = 0;  
                    }  
		//*(bufV++) =v;
		  *bufV=v;  
		      bufV+= 2;
                }  
            }  
        }  
    }   
} 