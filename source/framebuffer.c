/*
   showbmp.c
   
*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>


//14byte
typedef struct
{
  char cfType[2];         /* �ļ�����, ����Ϊ "BM" (0x4D42)*/
  char cfSize[4];         /* �ļ��Ĵ�С(�ֽ�) */
  char cfReserved[4];     /* ����, ����Ϊ 0 */
  char cfoffBits[4];      /* λͼ����������ļ�ͷ��ƫ����(�ֽ�)*/
}__attribute__((packed)) BITMAPFILEHEADER;       /* �ļ�ͷ�ṹ */

//40byte
typedef struct
{
  char ciSize[4];         /* size of BITMAPINFOHEADER */
  char ciWidth[4];        /* λͼ���(����) */
  char ciHeight[4];       /* λͼ�߶�(����) */
  char ciPlanes[2];       /* Ŀ���豸��λƽ����, ������Ϊ1 */
  char ciBitCount[2];     /* ÿ�����ص�λ��, 1,4,8��24 */
  char ciCompress[4];     /* λͼ���е�ѹ������,0=��ѹ�� */
  char ciSizeImage[4];    /* ͼ���С(�ֽ�) */
  char ciXPelsPerMeter[4];/* Ŀ���豸ˮƽÿ�����ظ��� */
  char ciYPelsPerMeter[4];/* Ŀ���豸��ֱÿ�����ظ��� */
  char ciClrUsed[4];      /* λͼʵ��ʹ�õ���ɫ�����ɫ�� */
  char ciClrImportant[4]; /* ��Ҫ��ɫ�����ĸ��� */
}__attribute__((packed)) BITMAPINFOHEADER;       /* λͼ��Ϣͷ�ṹ */



typedef struct
{
  unsigned short blue:5;
  unsigned short green:5;
  unsigned short red:5;
  unsigned short rev:1;
}__attribute__((packed)) PIXEL;

BITMAPFILEHEADER FileHead;
BITMAPINFOHEADER InfoHead;


char *fbp = 0;
int xres = 0;
int yres = 0;
int bits_per_pixel = 0;

int  show_bmp  ( char *bmpfile );
long chartolong ( char * string, int length );
/******************************************************************************
*
******************************************************************************/
int main ( int argc, char *argv[] )
{
    int fbfd = 0;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long int screensize = 0;

    // Open the file for reading and writing
    fbfd = open("/dev/fb0", O_RDWR);
    if (!fbfd)
    {
        printf("Error cannot open framebuffer device.\n");
        exit(1);
    }
 else       printf("open framebuffer device.\n");

    // Get fixed screen information
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo))
    {
        printf("Error reading fixed information.\n");
        exit(2);
    }

    // Get variable screen information
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo))
    {
        printf("Error reading variable information.\n");
        exit(3);
    }

    printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel );
    xres = vinfo.xres;
    yres = vinfo.yres;
    bits_per_pixel = vinfo.bits_per_pixel;

    // Figure out the size of the screen in bytes
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    // Map the device to memory
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
                       fbfd, 0);
    if ((int)fbp == -1)
    {
        printf("Error failed to map framebuffer device to memory.\n");
        exit(4);
    }
printf("sizeof header=%d\n", sizeof(BITMAPFILEHEADER));
printf("into show_bmp fun\n");

      show_bmp( argv[1] );

    munmap(fbp, screensize);
    close(fbfd);
    return 0;
}

/******************************************************************************
*
******************************************************************************/
int show_bmp( char *bmpfile )
{
        FILE *fp;
        int rc;
        int ciBitCount, ciWidth, ciHeight;
        int line_x, line_y;
        long int location = 0, BytesPerLine = 0;
        char tmp[1024*10];

   
        /* ��λͼ�ļ� */
        fp = fopen( bmpfile, "rb" );
        if (fp == NULL)
        {
                return( -1 );
        }
        
        /* ��ȡλͼ�ļ�ͷ */
        rc = fread( &FileHead, sizeof(BITMAPFILEHEADER),1, fp );
        if ( rc != 1)
        {
		printf("read header error!\n");
                fclose( fp );
                return( -2 );
        }
        
        /* �ж�λͼ������ */
        if (memcmp(FileHead.cfType, "BM", 2) != 0)
        {
		printf("it's not a BMP file\n");
                fclose( fp );
                return( -3 );
        }

        /* ��ȡλͼ��Ϣͷ */
        rc = fread( (char *)&InfoHead, sizeof(BITMAPINFOHEADER),1, fp );
        if ( rc != 1)
        {
		printf("read infoheader error!\n");
                fclose( fp );
                return( -4 );
        }

        ciWidth    = (int) chartolong( InfoHead.ciWidth,    4 );
        ciHeight   = (int) chartolong( InfoHead.ciHeight,   4 );
        ciBitCount = (int) chartolong( InfoHead.ciBitCount, 4 );

	fseek(fp, (int)chartolong(FileHead.cfoffBits, 4), SEEK_SET);
	BytesPerLine = (ciWidth * ciBitCount + 31) / 32 * 4;
        
	printf("width=%d, height=%d, bitCount=%d, offset=%d\n", ciWidth, ciHeight, ciBitCount, (int)chartolong(FileHead.cfoffBits, 4));
        line_x = line_y = 0;


	while(!feof(fp))
	{
		PIXEL pix;
		unsigned short int tmp;
		rc = fread( (char *)&pix, 1, sizeof(unsigned short int), fp );
		if (rc != sizeof(unsigned short int) )
		{ break; }
		location = line_x * bits_per_pixel / 8 + (ciHeight - line_y - 1) * xres * bits_per_pixel / 8;

		tmp=pix.red<<11 | pix.green<<6 | pix.blue;
		*((unsigned short int*)(fbp + location)) = tmp;

		line_x++;
		if (line_x == ciWidth )
		{
			line_x = 0;
			line_y++;
			
			if(line_y==ciHeight-1)
			{
				break;
			}
		}
	}

    
    
  
        fclose( fp );
        return( 0 );
}

/******************************************************************************
*
******************************************************************************/
long chartolong( char * string, int length )
{
        long number;
        
        if (length <= 4)
        {
                memset( &number, 0x00, sizeof(long) );
                memcpy( &number, string, length );
        }
        
        return( number );
}
