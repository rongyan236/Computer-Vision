void  RGB2YUV(unsigned char RgbBuf,UINT nWidth,UINT nHeight,unsigned char yuvBuf,unsigned long *len)  
{  
    int i, j;  
    unsigned char*bufY, *bufU, *bufV, *bufRGB,*bufYuv;  
    memset(yuvBuf,0,(unsigned int )*len);  
    bufY = yuvBuf;  
    bufV = yuvBuf + nWidth * nHeight;  
    bufU = bufV + 1;  
    *len = 0;   
    unsigned char y, u, v, r, g, b,testu,testv;  
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
            *(bufY++) = max( 0, min(y, 255 ));  
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
                *bufU =u;  
				bufU+= 2；
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
                    *bufV=v;  
					bufU+= 2；
                }  
            }  
        }  
    }  
    *len = nWidth * nHeight+(nWidth * nHeight)/2;   
} 