//
//  context.h
//  PdfReflow
//
//  Created by KyleWong on 13-8-20.
//  Copyright (c) 2013年 KyleWong. All rights reserved.
//

#ifndef PdfReflow_context_h
#define PdfReflow_context_h

#include "k2pdfopt.h"
#include "leptonica.h"
static const int PATH_LENGTH = 256;
extern char gAppFolder[PATH_LENGTH];
typedef struct {
	float x0, y0;
	float x1, y1;
} BBox;

typedef struct KOPTContext
{
    int trim;
    int wrap;
    int indent;
    int rotate;
    int columns;
    int offset_x;
    int offset_y;
    int dev_dpi;
    int dev_width;
    int dev_height;
    int page_width;
    int page_height;
    int straighten;
    int justification;
    int read_max_width;
    int read_max_height;
    
    double zoom;
    double margin;
    double quality;
    double contrast;
    double defect_size;
    double line_spacing;
    double word_spacing;
    double shrink_factor;
    
    int precache;
    int wordfragment_count;
    BBox bbox;
    uint8_t *data;
    WILLUSBITMAP *src;
    WILLUSBITMAP *dst;
} KOPTContext;
//#define kContinuousReadCombineMode
#define kContinuousReadSinglePageMode

int masterinfo_break_point(MASTERINFO *masterinfo,K2PDFOPT_SETTINGS *k2settings,int maxsize);

void doPdfReflow(char *inPngPath,char *outPngPath,int devWidth,int devHeight,int *width,int *height,double *timeCost,int *wordFragmentCount);
int getPaddingHeight(int from,int to,int height);
void k2pdfopt_reflow_bmp(KOPTContext *kctx);
static void k2pdfopt_settings_init_from_koptcontext(K2PDFOPT_SETTINGS *k2settings, KOPTContext *kctx);
#ifdef __cplusplus
extern "C" {
#endif
    void get_file_ext(char *dest,char *src);
    void outputToFile(void *stream, const char *text, int len);
    char *getApplicationDir();
    int convertPdf2Text(char *pdfFilePath,int page,void *interfaceObject);
    void render(char *infilename,char *outfilename, int pagenumber, int zoom, int rotation);
#ifdef __cplusplus
}
#endif


#endif
