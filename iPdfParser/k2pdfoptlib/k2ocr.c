/*
** k2ocr.c       k2pdfopt OCR functions
**
** Copyright (C) 2014  http://willus.com
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Affero General Public License as
** published by the Free Software Foundation, either version 3 of the
** License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Affero General Public License for more details.
**
** You should have received a copy of the GNU Affero General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
**
*/

#include "k2pdfopt.h"

#ifdef HAVE_OCR_LIB
static int k2ocr_gocr_inited=0;
#if (defined(HAVE_TESSERACT_LIB))
static int k2ocr_tess_inited=0;
static int k2ocr_tess_status=0;
#endif
static void k2ocr_ocrwords_fill_in(MASTERINFO *masterinfo,OCRWORDS *words,BMPREGION *region,
                                   K2PDFOPT_SETTINGS *k2settings);
static void ocrword_fillin_mupdf_info(OCRWORD *word,BMPREGION *region);
#endif
static void k2ocr_ocrwords_get_from_ocrlayer(MASTERINFO *masterinfo,OCRWORDS *words,
                                             BMPREGION *region,K2PDFOPT_SETTINGS *k2settings);
static int ocrword_map_to_bitmap(OCRWORD *word,MASTERINFO *masterinfo,BMPREGION *region,
                                 K2PDFOPT_SETTINGS *k2settings,int *index,int *i2);
static int wrectmap_srcword_inside(WRECTMAP *wrectmap,OCRWORD *word,BMPREGION *region,int *i2);
static void wtextchars_group_by_words(WTEXTCHARS *wtcs,OCRWORDS *words,
                                      K2PDFOPT_SETTINGS *k2settings);
static void wtextchars_add_one_row(WTEXTCHARS *wtcs,int i0,int i1,OCRWORDS *words);


void k2ocr_init(K2PDFOPT_SETTINGS *k2settings)

    {
#ifdef HAVE_OCR_LIB
    if (!k2settings->dst_ocr)
        return;
    /* v2.15--don't need to do this--it's in k2pdfopt_settings_init. */
    /*
    if (!k2ocr_inited)
        {
        ocrwords_init(&k2settings->dst_ocrwords);
        k2ocr_inited=1;
        }
    */
#if (!defined(HAVE_TESSERACT_LIB) && defined(HAVE_GOCR_LIB))
    if (k2settings->dst_ocr=='t')
        {
        k2printf(TTEXT_WARN "\a** Tesseract not compiled into this version.  Using GOCR. **"
                TTEXT_NORMAL "\n\n");
        k2settings->dst_ocr='g';
        }
#endif
#if (defined(HAVE_TESSERACT_LIB) && !defined(HAVE_GOCR_LIB))
    if (k2settings->dst_ocr=='g')
        {
        k2printf(TTEXT_WARN "\a** GOCR not compiled into this version.  Using Tesseract. **"
                TTEXT_NORMAL "\n\n");
        k2settings->dst_ocr='t';
        }
#endif
#ifdef HAVE_TESSERACT_LIB
#ifdef HAVE_GOCR_LIB
    if (k2settings->dst_ocr=='t')
        {
#endif
        /* v2.15 fix--specific variable for Tesseract init status */
        if (!k2ocr_tess_inited)
            {
            k2printf(TTEXT_BOLD);
            k2ocr_tess_status=ocrtess_init(NULL,
                              k2settings->dst_ocr_lang[0]=='\0'?NULL:k2settings->dst_ocr_lang,stdout);
            k2printf(TTEXT_NORMAL);
            if (k2ocr_tess_status)
                k2printf(TTEXT_WARN "Could not find Tesseract data" TTEXT_NORMAL " (env var TESSDATA_PREFIX = %s).\nUsing GOCR v0.49.\n\n",getenv("TESSDATA_PREFIX")==NULL?"(not assigned)":getenv("TESSDATA_PREFIX"));
            else
                k2printf("\n");
            k2ocr_tess_inited=1;
            }
#ifdef HAVE_GOCR_LIB
        }
    else
#endif
#endif
#ifdef HAVE_GOCR_LIB
        {
        if (k2settings->dst_ocr=='g')
            {
            if (!k2ocr_gocr_inited)
                {
                k2printf(TTEXT_BOLD "GOCR v0.49 OCR Engine" TTEXT_NORMAL "\n\n");
                k2ocr_gocr_inited=1;
                }
            }
        }
#endif
#ifdef HAVE_MUPDF_LIB
    /* Could announce MuPDF virtual OCR here, but I think it will just confuse people. */
    /*
    if (k2settings->dst_ocr=='m')
        k2printf(TTEXT_BOLD "Using MuPDF \"Virtual\" OCR" TTEXT_NORMAL "\n\n");
    */
#endif
#endif /* HAVE_OCR_LIB */
    }

/*
** v2.15--call close/free functions even if k2settings->dst_ocr not set--may have
**        been called from previous GUI conversion.
*/
void k2ocr_end(K2PDFOPT_SETTINGS *k2settings)

    {
#ifdef HAVE_OCR_LIB
#ifdef HAVE_TESSERACT_LIB
    if (k2ocr_tess_inited)
        ocrtess_end();
#endif
    ocrwords_free(&k2settings->dst_ocrwords);
#endif /* HAVE_OCR_LIB */
    }


#ifdef HAVE_OCR_LIB
/*
** Find words in src bitmap and put into words structure.
** If ocrcolumns > maxcols, looks for multiple columns
**
** wrectmaps has information that maps the bitmap pixel space to the original
** source page space so that the location of the words can be used to extract
** the text from a native PDF document if possible (dst_ocr=='m').
**
*/
void k2ocr_ocrwords_fill_in_ex(MASTERINFO *masterinfo,OCRWORDS *words,BMPREGION *region,
                               K2PDFOPT_SETTINGS *k2settings)

    {
    PAGEREGIONS *pageregions,_pageregions;
    int i,maxlevels;

#if (WILLUSDEBUGX & 0x10000)
bmpregion_write(region,"bmpregion_major.png");
#endif

    if (k2settings->ocr_max_columns<0 || k2settings->ocr_max_columns <= k2settings->max_columns)
        {
#if (WILLUSDEBUGX & 32)
printf("Call #1. k2ocr_ocrwords_fill_in\n");
#endif
        k2ocr_ocrwords_fill_in(masterinfo,words,region,k2settings);
        return;
        }
    /* Parse region into columns for OCR */
    pageregions=&_pageregions;
    pageregions_init(pageregions);
    if (k2settings->ocr_max_columns==2 || k2settings->max_columns>1)
        maxlevels = 2;
    else
        maxlevels = 3;
    pageregions_find_columns(pageregions,region,k2settings,masterinfo,maxlevels);
    for (i=0;i<pageregions->n;i++)
        {
        /*
        ** If using built-in source-file OCR layer, don't need to scan bitmap
        ** for text rows.
        */
printf("\nwrectmaps->n=%d, dst_ocr='%c'\n",region->wrectmaps->n,k2settings->dst_ocr);
        if (k2settings->dst_ocr!='m' || region->wrectmaps->n!=1)
            bmpregion_find_textrows(&pageregions->pageregion[i].bmpregion,k2settings,0,1);
        pageregions->pageregion[i].bmpregion.wrectmaps = region->wrectmaps;
#if (WILLUSDEBUGX & 32)
printf("Call #2. k2ocr_ocrwords_fill_in, pr = %d of %d\n",i+1,pageregions->n);
#endif
        k2ocr_ocrwords_fill_in(masterinfo,words,&pageregions->pageregion[i].bmpregion,k2settings);
        }
    pageregions_free(pageregions);
    }

    
/*
** Find words in src bitmap and put into words structure.
*/
static void k2ocr_ocrwords_fill_in(MASTERINFO *masterinfo,OCRWORDS *words,BMPREGION *region,
                                   K2PDFOPT_SETTINGS *k2settings)

    {
    int i;
    /* static char *funcname="ocrwords_fill_in"; */

/*
k2printf("@ocrwords_fill_in (%d x %d)...tr=%d\n",region->bmp->width,region->bmp->height,region->textrows.n);
if (region->textrows.n==0)
{
bmpregion_write(region,"out.png");
exit(10);
}
*/
#if (WILLUSDEBUGX & 32)
k2printf("@ocrwords_fill_in...\n");
#endif
/*
{
char filename[256];
count++;
sprintf(filename,"out%03d.png",count);
bmp_write(src,filename,stdout,100);
}
*/

    if (region->bmp->width<=0 || region->bmp->height<=0)
        return;

#if (WILLUSDEBUGX & 32)
k2printf("    %d row%s of text, dst_ocr='%c'\n",region->textrows.n,region->textrows.n==1?"":"s",k2settings->dst_ocr);
#endif
    if (k2settings->dst_ocr=='m')
        {
        k2ocr_ocrwords_get_from_ocrlayer(masterinfo,words,region,k2settings);
        return;
        }
    /* Go text row by text row */
    for (i=0;i<region->textrows.n;i++)
        {
        BMPREGION _newregion,*newregion;
        int j,rowbase,r1,r2;
        double lcheight;
        TEXTWORDS *textwords;

        newregion=&_newregion;
        bmpregion_init(newregion);
        bmpregion_copy(newregion,region,0);
        r1=newregion->r1=region->textrows.textrow[i].r1;
        r2=newregion->r2=region->textrows.textrow[i].r2;
#if (WILLUSDEBUGX & 32)
printf("    @%3d,%3d = %3d x %3d\n",
region->textrows.textrow[i].c1,
region->textrows.textrow[i].r1,
region->textrows.textrow[i].c2-region->textrows.textrow[i].c1+1,
region->textrows.textrow[i].r2-region->textrows.textrow[i].r1+1);
#endif
        newregion->bbox.type = REGION_TYPE_TEXTLINE;
        rowbase=region->textrows.textrow[i].rowbase;
        lcheight=region->textrows.textrow[i].lcheight;
        /* Sanity check on rowbase, lcheight */
        if ((double)(rowbase-r1)/(r2-r1) < .5)
            rowbase = r1+(r2-r1)*0.7;
        if (lcheight/(r2-r1) < .33)
            lcheight = 0.33;
        /* Break text row into words (also establishes lcheight) */
        bmpregion_one_row_find_textwords(newregion,k2settings,0);
        textwords = &newregion->textrows;
        /* Add each word */
#if (WILLUSDEBUGX & 32)
printf("textwords->n=%d\n",textwords->n);
#endif
        for (j=0;j<textwords->n;j++)
            {
            char wordbuf[256];

#if (WILLUSDEBUGX & 32)
k2printf("j=%d of %d\n",j,textwords->n);
printf("dst_ocr='%c', ocrtessstatus=%d\n",k2settings->dst_ocr,k2ocr_tess_status);
#endif
            /* Don't OCR if "word" height exceeds spec */
            if ((double)(textwords->textrow[j].r2-textwords->textrow[j].r1+1)/region->dpi
                     > k2settings->ocr_max_height_inches)
                continue;
#if (WILLUSDEBUGX & 32)
{
static int counter=1;
int i;
char filename[256];
WILLUSBITMAP *bmp,_bmp;
bmp=&_bmp;
bmp_init(bmp);
bmp->width=textwords->textrow[j].c2-textwords->textrow[j].c1+1;
bmp->height=textwords->textrow[j].r2-textwords->textrow[j].r1+1;
bmp->bpp=8;
bmp_alloc(bmp);
for (i=0;i<256;i++)
bmp->red[i]=bmp->green[i]=bmp->blue[i]=i;
for (i=0;i<bmp->height;i++)
{
unsigned char *s,*d;
s=bmp_rowptr_from_top(newregion->bmp8,textwords->textrow[j].r1+i)+textwords->textrow[j].c1;
d=bmp_rowptr_from_top(bmp,i);
memcpy(d,s,bmp->width);
}
sprintf(filename,"word%04d.png",counter);
bmp_write(bmp,filename,stdout,100);
bmp_free(bmp);
k2printf("%5d. ",counter);
fflush(stdout);
#endif
            wordbuf[0]='\0';
#ifdef HAVE_TESSERACT_LIB
#ifdef HAVE_GOCR_LIB
            if (k2settings->dst_ocr=='t' && !k2ocr_tess_status)
#else
            if (!k2ocr_tess_status)
#endif
                ocrtess_single_word_from_bmp8(wordbuf,255,newregion->bmp8,
                                          textwords->textrow[j].c1,
                                          textwords->textrow[j].r1,
                                          textwords->textrow[j].c2,
                                          textwords->textrow[j].r2,3,0,1,NULL);
#ifdef HAVE_GOCR_LIB
            else if (k2settings->dst_ocr=='g')
#endif
#endif
#ifdef HAVE_GOCR_LIB
                jocr_single_word_from_bmp8(wordbuf,255,newregion->bmp8,
                                            textwords->textrow[j].c1,
                                            textwords->textrow[j].r1,
                                            textwords->textrow[j].c2,
                                            textwords->textrow[j].r2,0,1);
#endif
#ifdef HAVE_MUPDF_LIB
            if (k2settings->dst_ocr=='m')
                {
                wordbuf[0]='m'; /* Dummy value for now */
                wordbuf[1]='\0';
                }
#endif
#if (WILLUSDEBUGX & 32)
printf("..");
fflush(stdout);
if (wordbuf[0]!='\0')
{
char filename[256];
FILE *f;
sprintf(filename,"word%04d.txt",counter);
f=fopen(filename,"wb");
fprintf(f,"%s\n",wordbuf);
fclose(f);
k2printf("%s\n",wordbuf);
}
else
k2printf("(OCR failed)\n");
counter++;
}
#endif
            if (wordbuf[0]!='\0')
                {
                OCRWORD word;

                ocrword_init(&word);
                word.c=textwords->textrow[j].c1;
                /* Use same rowbase for whole row */
                word.r=rowbase;
                word.maxheight=rowbase-textwords->textrow[j].r1;
                word.w=textwords->textrow[j].c2-textwords->textrow[j].c1+1;
                word.h=textwords->textrow[j].r2-textwords->textrow[j].r1+1;
                word.lcheight=lcheight;
                word.rot=0;
                word.text=wordbuf;
                ocrword_fillin_mupdf_info(&word,region);
#if (WILLUSDEBUGX & 32)
k2printf("'%s', r1=%d, rowbase=%d, h=%d\n",wordbuf,
                             textwords->textrow[j].r1,
                             textwords->textrow[j].rowbase,
                             textwords->textrow[j].r2-textwords->textrow[j].r1+1);
printf("words=%p\n",words);
printf("words->n=%d\n",words->n);
#endif
                ocrwords_add_word(words,&word);
#if (WILLUSDEBUGX & 32)
printf("word added.\n");
#endif
                }
            }
        bmpregion_free(newregion);
        }
#if (WILLUSDEBUGX & 32)
printf("Done k2ocr_ocrwords_fill_in()\n");
#endif
    }


static void ocrword_fillin_mupdf_info(OCRWORD *word,BMPREGION *region)

    {
    int xp,yp; /* Top left of word on passed bitmap */
    int xc,yc; /* Center of word */
    int i;
    double pix2ptsw,pix2ptsh;
    WRECTMAP *wrmap;
    WRECTMAPS *wrectmaps;

    wrectmaps=region->wrectmaps;
#if (WILLUSDEBUGX & 0x400)
printf("Fillin: wrectmaps->n=%d\n",wrectmaps==NULL ? 0 : wrectmaps->n);
#endif
    if (wrectmaps==NULL || wrectmaps->n==0)
        {
        word->x0=word->c;
        word->y0=word->r-word->maxheight;
        word->w0=word->w;
        word->h0=word->h;
        word->rot0_deg=region->rotdeg;
        word->pageno=0;
        return;
        }
    xp=word->c;
    yp=word->r-word->maxheight;
    xc=xp+word->w/2;
    yc=yp+word->h/2;
#if (WILLUSDEBUGX & 0x400)
printf("    word xc,yc = (%d,%d)\n",xc,yc);
#endif
    for (i=0;i<wrectmaps->n;i++)
        {
#if (WILLUSDEBUGX & 0x400)
printf("    wrectmap[%d]= (%g,%g) , %g x %g\n",i,
wrectmaps->wrectmap[i].coords[1].x,
wrectmaps->wrectmap[i].coords[1].y,
wrectmaps->wrectmap[i].coords[2].x,
wrectmaps->wrectmap[i].coords[2].y);
#endif
        if (wrectmap_inside(&wrectmaps->wrectmap[i],xc,yc))
            break;
        }
    if (i>=wrectmaps->n)
        {
#if (WILLUSDEBUGX & 0x400)
printf("Not inside.\n");
#endif
        i=0;
        }
    wrmap=&wrectmaps->wrectmap[i];
    word->pageno = wrmap->srcpageno;
    word->rot0_deg = wrmap->srcrot;
    pix2ptsw = 72./wrmap->srcdpiw;
    pix2ptsh = 72./wrmap->srcdpih;
    word->x0 = (wrmap->coords[0].x + (xp-wrmap->coords[1].x))*pix2ptsw;
    word->y0 = (wrmap->coords[0].y + (yp-wrmap->coords[1].y))*pix2ptsh;
    word->w0 = word->w*pix2ptsw;
    word->h0 = word->h*pix2ptsh;
#if (WILLUSDEBUGX & 0x400)
printf("Word: (%5.1f,%5.1f) = %5.1f x %5.1f (page %2d)\n",word->x0,word->y0,word->w0,word->h0,word->pageno);
#endif
    }
#endif /* HAVE_OCR_LIB */


/*
** In a contiguous rectangular region that is mapped to the PDF source file,
** find rows of text assuming a single column of text.
*/
static void k2ocr_ocrwords_get_from_ocrlayer(MASTERINFO *masterinfo,OCRWORDS *dwords,
                                             BMPREGION *region,K2PDFOPT_SETTINGS *k2settings)

    {
    static WTEXTCHARS *wtcs=NULL;
    static WTEXTCHARS _wtcs;
    static OCRWORDS *words;
    static OCRWORDS _words;
    static int pageno=-1;
    static char pdffile[512];
    int i;

#if (WILLUSDEBUGX & 0x10000)
printf("@k2ocr_ocrwords_get_from_ocrlayer.\n");
#endif
    if (wtcs==NULL)
        {
        wtcs=&_wtcs;
        wtextchars_init(wtcs);
        words=&_words;
        ocrwords_init(words);
        pdffile[0]='\0';
        }
    if (pageno!=masterinfo->pageinfo.srcpage || strcmp(pdffile,masterinfo->srcfilename))
        {
        ocrwords_free(words);
        wtextchars_clear(wtcs);
        wtextchars_fill_from_page(wtcs,masterinfo->srcfilename,masterinfo->pageinfo.srcpage,"");
        wtextchars_rotate_clockwise(wtcs,360-(int)masterinfo->pageinfo.srcpage_rot_deg);
        wtextchars_group_by_words(wtcs,words,k2settings);
        pageno=masterinfo->pageinfo.srcpage;
        strncpy(pdffile,masterinfo->srcfilename,511);
        pdffile[511]='\0';
        }
#if (WILLUSDEBUGX & 0x10000)
{
FILE *f;
int y0;
static int count=0;
WRECTMAPS *wrectmaps;
wrectmaps=region->wrectmaps;
printf("\n");
if (wrectmaps!=NULL)
{
printf("    Got %d word regions.\n",wrectmaps->n);
printf("    region = (%d,%d) - (%d,%d) pixels\n",region->c1,region->r1,region->c2,region->r2);
for (i=0;i<wrectmaps->n;i++)
{
printf("    wrectmap[%d]= srcorg=(%g,%g); dstorg=(%g,%g) , %g x %g\n",i,
wrectmaps->wrectmap[i].coords[0].x,
wrectmaps->wrectmap[i].coords[0].y,
wrectmaps->wrectmap[i].coords[1].x,
wrectmaps->wrectmap[i].coords[1].y,
wrectmaps->wrectmap[i].coords[2].x,
wrectmaps->wrectmap[i].coords[2].y);
printf("        srcpageno=%d\n",wrectmaps->wrectmap[i].srcpageno);
printf("        srcwidth=%d\n",wrectmaps->wrectmap[i].srcwidth);
printf("        srcheight=%d\n",wrectmaps->wrectmap[i].srcheight);
printf("        srcdpiw=%g\n",wrectmaps->wrectmap[i].srcdpiw);
printf("        srcdpih=%g\n",wrectmaps->wrectmap[i].srcdpih);
printf("        srcrot=%d\n",wrectmaps->wrectmap[i].srcrot);
}
}
y0=region->r2;
f=fopen("words.ep","w");
fprintf(f,"; srcdpi=%d, dstdpi=%d\n",k2settings->src_dpi,k2settings->dst_dpi);
fprintf(f,"/sm off\n/sc on\n/sd off\n");
fprintf(f,"/sa l \"Region (pixels)\" 2\n");
fprintf(f,"%d %d\n",region->c1,y0-region->r1);
fprintf(f,"%d %d\n",region->c2,y0-region->r1);
fprintf(f,"%d %d\n",region->c2,y0-region->r2);
fprintf(f,"%d %d\n",region->c1,y0-region->r2);
fprintf(f,"%d %d\n",region->c1,y0-region->r1);
fprintf(f,"//nc\n");
#endif

    /* Map word PDF positions (in source file) to destination bitmap pixels */
    /* I'm not sure this will work entirely correctly with left-to-right text */
#if (WILLUSDEBUGX & 0x10000)
printf("words->n=%d\n",words->n);
#endif
    for (i=0;i<words->n;i++)
        {
        int index,i2;

        for (i2=-1,index=0;1;index++)
            {
            OCRWORD _word,*word;
            int n,dn;

            /*
            ** Make copy of since the function call below can modify the value inside "word"
            */
            word=&_word;
            ocrword_copy(word,&words->word[i]); /* Allocates new memory */
            if (i2>=0)
                {
                ocrword_truncate(word,i2+1,word->n-1);
                dn = words->word[i].n-word->n;
#if (WILLUSDEBUGX & 0x10000)
printf("truncated word (%d-%d) = '%s'!\n",i2+1,word->n-1,word->text);
#endif
                }
            else
                dn=0;
            n=word->n;
            if (!ocrword_map_to_bitmap(word,masterinfo,region,k2settings,&index,&i2))
                {
                ocrword_free(word);
                break;
                }
#if (WILLUSDEBUGX & 0x10000)
printf("i2 = %d (dn=%d, n=%d); word='%s' (len=%d)\n",i2,dn,n,word->text,word->n);
#endif
            ocrwords_add_word(dwords,word);
            ocrword_free(word);
            if (i2>=n-1)
                break;
#if (WILLUSDEBUGX & 0x10000)
printf("continuing\n");
#endif
            i2+=dn;
#if (WILLUSDEBUGX & 0x10000)
fprintf(f,"/sa m 2 2\n");
fprintf(f,"/sa l \"'%s'\" 2\n",word->text);
fprintf(f,"%d %d\n",word->c,y0-word->r);
fprintf(f,"%d %d\n",word->c+word->w,y0-word->r);
fprintf(f,"%d %d\n",word->c+word->w,y0-(word->r-word->h));
fprintf(f,"%d %d\n",word->c,y0-(word->r-word->h));
fprintf(f,"%d %d\n",word->c,y0-word->r);
fprintf(f,"//nc\n");
#endif
            }
        }
    /* These are static--keep them around for the next call in case it's on the same page */
    /*
    ocrwords_free(words);
    wtextchars_free(wtcs);
    */
#if (WILLUSDEBUGX & 0x10000)
fclose(f);
wfile_written_info("words.ep",stdout);
count++;
/*
if (count==5)
exit(10);
*/
}
printf("ALL DONE.\n");
#endif
    }


/*
** Determine pixel locations of OCR word on source bitmap region.
** Return 1 if any part of word is within region, 0 if not.
**
** WARNING:  Contents of "word" may be modified.
**
*/
static int ocrword_map_to_bitmap(OCRWORD *word,MASTERINFO *masterinfo,BMPREGION *region,
                                 K2PDFOPT_SETTINGS *k2settings,int *index,int *i2)

    {
    int i;
    WRECTMAPS *wrectmaps;

    wrectmaps=region->wrectmaps;
/*
#if (WILLUSDEBUGX & 0x10000)
printf("wrectmaps->n = %d\n",wrectmaps->n);
#endif
*/
    for (i=(*index);i<wrectmaps->n;i++)
        if (wrectmap_srcword_inside(&wrectmaps->wrectmap[i],word,region,i2))
            {
            word->rot0_deg=masterinfo->pageinfo.srcpage_rot_deg;
            word->pageno=masterinfo->pageinfo.srcpage;
            (*index)=i;
            return(1);
            }
    return(0);
    }


/*
** WARNING:  Contents of "word" may be modified.
*/
static int wrectmap_srcword_inside(WRECTMAP *wrectmap,OCRWORD *word,BMPREGION *region,int *index2)

    {
    double cx_word,cy_word;
    double x0_wrect,x1_wrect,y0_wrect,y1_wrect;
    double cx_wrect,cy_wrect;

    (*index2)=-1;
    /* compute word and wrectmap in src coords (pts) */
    /*
    ** (cx_word, cy_word) = position of center of word on source page, from top left,
    **                      in points.
    */
    cx_word=word->x0+word->w0/2.;
    cy_word=word->y0+word->h0/2.;
    /*
    ** x0_wrect, y0_wrect = position of upper-left region box on source page, in points.
    ** y1_wrect, y1_wrect = position of lower-right region box on source page, in points.
    */
    /* Original formula from before 7-13-2014--seems off.
    x0_wrect=(wrectmap->coords[0].x+(region->c1-wrectmap->coords[1].x))*72./wrectmap->srcdpiw;
    x1_wrect=(wrectmap->coords[0].x+(region->c2+1-wrectmap->coords[1].x))*72./wrectmap->srcdpiw;
    y0_wrect=(wrectmap->coords[0].y+(region->r1-wrectmap->coords[1].y))*72./wrectmap->srcdpih;
    y1_wrect=(wrectmap->coords[0].y+(region->r2+1-wrectmap->coords[1].y))*72./wrectmap->srcdpih;
    */
    x0_wrect=wrectmap->coords[0].x*72./wrectmap->srcdpiw;
    x1_wrect=x0_wrect + wrectmap->coords[2].x*72./wrectmap->srcdpiw;
    y0_wrect=wrectmap->coords[0].y*72./wrectmap->srcdpih;
    y1_wrect=y0_wrect + wrectmap->coords[2].y*72./wrectmap->srcdpih;
    cx_wrect=(x0_wrect+x1_wrect)/2.;
    cy_wrect=(y0_wrect+y1_wrect)/2.;

    /* If word box is completely outside, skip */
    if (word->x0 >= x1_wrect || (word->x0+word->w0) <= x0_wrect 
                             || word->y0 >= y1_wrect || (word->y0+word->h0) <= y0_wrect)
        return(0);
    if (((cx_word >= x0_wrect && word->x0 <= x1_wrect) || (cx_wrect >= word->x0 && cx_wrect <= word->x0+word->w0))
            && ((cy_word >= y0_wrect && cy_word <= y1_wrect) || (cy_wrect >= word->y0 && cy_wrect <= word->y0+word->h0)))
        {
        int i,i1,i2;

        /* Find out which letters are in/out */
        /* From left... */
#if (WILLUSDEBUGX & 0x10000)
printf("Word:  '%s'\n",word->text);
#endif
        for (i=0;i<word->n;i++)
            {
            double x0,x1,x2,dx;

            x0=(i==0)?0.:word->cpos[i-1];
            dx=word->cpos[i]-x0;
            x1=word->x0+x0+dx*0.2;
            x2=word->x0+x0+dx*0.8;
#if (WILLUSDEBUGX & 0x10000)
printf("    i1=%d, x1=%g, x2=%g, x0_rect=%g, x1_rect=%g\n",i,x1,x2,x0_wrect,x1_wrect);
#endif
            if ((x1>=x0_wrect && x1<=x1_wrect) || (x2>=x0_wrect && x2<=x1_wrect))
                break;
            }
        i1=i;
        if (i1>=word->n)
            return(0);
        /* From right... */
        for (i=word->n-1;i>i1;i--)
            {
            double x0,x1,x2,dx;

            x0=(i==0)?0.:word->cpos[i-1];
            dx=word->cpos[i]-x0;
            x1=word->x0+x0+dx*0.2;
            x2=word->x0+x0+dx*0.8;
#if (WILLUSDEBUGX & 0x10000)
printf("    i2=%d, x1=%g, x2=%g, x0_rect=%g, x1_rect=%g\n",i,x1,x2,x0_wrect,x1_wrect);
#endif
            if ((x1>=x0_wrect && x1<=x1_wrect) || (x2>=x0_wrect && x2<=x1_wrect))
                break;
            }
        i2=i;
        (*index2)=i2;
#if (WILLUSDEBUGX & 0x10000)
printf("    i1=%d, i2=%d, n=%d\n",i1,i2,word->n);
#endif
        if (i2<i1)
            return(0);
#if (WILLUSDEBUGX & 0x10000)
printf("    Chars %d to %d\n",i1+1,i2+1);
printf("    Region = (%g,%g) - (%g,%g)\n",x0_wrect,y0_wrect,x1_wrect,y1_wrect);
printf("    Word = (%g,%g) - (%g,%g)\n",word->x0,word->y0,word->x0+word->w0,word->y0+word->h0);
#endif
        word->c=word->x0*wrectmap->srcdpiw/72.-wrectmap->coords[0].x+wrectmap->coords[1].x+.5;
        word->r=((double)word->r/100.)*wrectmap->srcdpih/72.-wrectmap->coords[0].y+wrectmap->coords[1].y+.5;
        word->w=(int)(word->w0*wrectmap->srcdpiw/72.+.5);
        word->h=(int)(word->h0*wrectmap->srcdpih/72.+.5);
#if (WILLUSDEBUGX & 0x10000)
printf("    Word mapped to (%d,%d), %d x %d\n",word->c,word->r,word->w,word->h);
printf("    Word mxheight = %g\n",word->maxheight);
#endif
        word->maxheight=word->maxheight*wrectmap->srcdpih/72.;
        word->lcheight=word->maxheight/1.7;
        word->rot=0;
        if (i1>0 || i2<word->n-1)
            {
            ocrword_truncate(word,i1,i2);
#if (WILLUSDEBUGX & 0x10000)
aprintf(ANSI_YELLOW "    Truncated='%s' (len=%d)" ANSI_NORMAL "\n",word->text,word->n);
printf("    Word = (%g,%g) - (%g,%g)\n",word->x0,word->y0,word->x0+word->w0,word->y0+word->h0);
printf("    Word mapped to (%d,%d), %d x %d\n",word->c,word->r,word->w,word->h);
printf("    Word mxheight = %g\n",word->maxheight);
#endif
            }
        return(1);
        }
    return(0);
    }


static void wtextchars_group_by_words(WTEXTCHARS *wtcs,OCRWORDS *words,
                                      K2PDFOPT_SETTINGS *k2settings)

    {
    double vert_spacing_threshold;
    static char *funcname="wtextchars_group_by_words";
    int i,i0;

    if (wtcs->n<=0)
        return;    
    /* Manually sort characters in OCR layer */
    if (k2settings->dst_ocr_visibility_flags&32)
        {
        double *ch; /* population of character heights */

        willus_dmem_alloc_warn(39,(void **)&ch,sizeof(double)*wtcs->n,funcname,10);
        /* Sort by position */
        wtextchars_sort_vertically_by_position(wtcs,0);
        /* Create population character heights to determine row spacing threshold */
        for (i=0;i<wtcs->n-1;i++)
            ch[i]=wtcs->wtextchar[i].y2-wtcs->wtextchar[i].y1;
        sortd(ch,wtcs->n);
        vert_spacing_threshold=0.67*ch[wtcs->n/10];
        willus_dmem_free(39,&ch,funcname);
        }
    else
        vert_spacing_threshold=-1;
    /* Group characters row by row and add one row at a time */
    for (i0=0,i=1;i<wtcs->n;i++)
        {
        double dx,dy;
        int newrow;

        newrow=0;
        dy=wtcs->wtextchar[i].yp - wtcs->wtextchar[i-1].yp;
        dx=wtcs->wtextchar[i].xp - wtcs->wtextchar[i-1].xp;
        if (vert_spacing_threshold>0)
            {
            double h;

            h=wtcs->wtextchar[i].y2 - wtcs->wtextchar[i].y1;
            if (dy >= h*.75 || dy >= vert_spacing_threshold)
                {
                if (i-1>i0)
                    wtextchar_array_sort_horizontally_by_position(&wtcs->wtextchar[i0],i-i0+1);
                newrow=1;
                }
            }
        else
            {
            if (dx<0 || dy<0)
                newrow=1;
            }
        if (newrow)
            {
            wtextchars_add_one_row(wtcs,i0,i-1,words);
            i0=i;
            }
        }
    wtextchars_add_one_row(wtcs,i0,wtcs->n-1,words);
    }


static void wtextchars_add_one_row(WTEXTCHARS *wtcs,int i0,int i1,OCRWORDS *words)

    {
    int i,j0;
    OCRWORD word;
    int *u16str;
    static char *funcname="wtextchars_add_one_row";

    ocrword_init(&word);
    willus_dmem_alloc_warn(40,(void **)&u16str,(i1-i0+1)*sizeof(int),funcname,10);
    willus_dmem_alloc_warn(41,(void **)&word.text,(i1-i0+2)*sizeof(int),funcname,10);
    willus_dmem_alloc_warn(42,(void **)&word.cpos,(i1-i0+2)*sizeof(double),funcname,10);
    for (j0=i=i0;i<=i1;i++)
        {
        int c,space;
        double h,dx;

        c=wtcs->wtextchar[i].ucs;
        /* If letters are physically separated, assume a space */
        if (i<i1)
            {
            h = fabs((wtcs->wtextchar[i+1].y2 - wtcs->wtextchar[i+1].y1)
                      +(wtcs->wtextchar[i].y2 - wtcs->wtextchar[i].y1)) / 2.;
            dx = wtcs->wtextchar[i+1].x1 - wtcs->wtextchar[i].x2;
            space = (dx > h*.05);
            }
        else
            space = 0;
        if (space || c==' ' || i==i1)
            {
            if (c==' ' && i==j0)
                j0++;
            else
                {
                int j1,j;
                double dx;

                j1=(c==' ')?i-1:i;
                word.r=(int)(wtcs->wtextchar[j0].yp*100.+.5);
                word.x0=wtcs->wtextchar[j0].x1;
                word.y0=wtcs->wtextchar[j0].y1;
                word.w0=wtcs->wtextchar[j1].x2-word.x0;
                word.h0=wtcs->wtextchar[j0].y2-word.y0;
                /*
                ** Narrow up the words slightly so that it is physically separated
                ** from other words--this makes selection work better in Sumatra.
                ** Note that only w0 is narrowed--the cpos[] values are not. (v2.20)
                */
                dx = word.h0*.05;
                if (word.w0<dx*4.)
                    dx = word.w0/10.;
                word.x0 += dx;
                word.w0 -= 2*dx;

                word.maxheight=0.;
                for (j=j0;j<=j1;j++)
                    {
                    word.cpos[j-j0]=wtcs->wtextchar[j].x2-word.x0;
                    u16str[j-j0]=wtcs->wtextchar[j].ucs;
                    if (wtcs->wtextchar[j].yp-wtcs->wtextchar[j].y1 > word.maxheight)
                        word.maxheight=(wtcs->wtextchar[j].yp-wtcs->wtextchar[j].y1);
                    }
                unicode_to_utf8(word.text,u16str,j1-j0+1);
                ocrwords_add_word(words,&word);
                j0= i+1;
                }
            }
        }
    willus_dmem_free(42,(double **)&word.cpos,funcname);
    willus_dmem_free(41,(double **)&word.text,funcname);
    willus_dmem_free(40,(double **)&u16str,funcname);
    }
