/*
** pagelist.c    Functions to parse comma-delimited page-list string.
**
** Copyright (C) 2013  http://willus.com
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

static void pagelist_n1n2_adjust(int *n1,int *n2,int flags);
static int pagelist_next_pages(char *pagelist,int maxpages,int *index,
                               int *n1,int *n2,int *flags);
static int evenoddcheck(int c,int *flags);


int pagelist_valid_page_range(char *s)

    {
    int i;

    for (i=0;s[i]!='\0';i++)
        {
        if (s[i]==' ' || s[i]=='\t' || s[i]==',' || s[i]=='-' 
             || tolower(s[i])=='o' || tolower(s[i])=='e' || (s[i]>='0' && s[i]<='9'))
            continue;
        else
            break;
        }
    return(s[i]=='\0');
    }


int pagelist_includes_page(char *pagelist,int pageno,int maxpages)

    {
    int i,n;

    /* Sort of arbitrary */
    if (maxpages < 0)
        maxpages = 99999;
    n=pagelist_count(pagelist,maxpages);
    for (i=0;i<n;i++)
        if (pagelist_page_by_index(pagelist,i,maxpages)==pageno)
             return(1);
    return(0);
    }


int pagelist_page_by_index(char *pagelist,int index,int maxpages)

    {
    int n1,n2,i,j,s,flags;

// printf("@pagelist_page_by_index('%s',%d,%d)\n",pagelist,index,maxpages);
    if (pagelist[0]=='\0')
        return(index+1);
    i=0;
    while (pagelist_next_pages(pagelist,maxpages,&i,&n1,&n2,&flags))
        {
        if (n1<=0 && n2<=0)
            continue;
        s = (n2>=n1) ? 1 : -1;
        if (flags!=3)
            s *= 2;
        n2 += s;
        for (j=n1;j!=n2;j+=s)
            {
            if (j<1)
                continue;
            if (maxpages>0 && j>maxpages)
                continue;
            if (index==0)
                return(j);
            index--;
            }
        }
    return(-1);
    }


int pagelist_count(char *pagelist,int maxpages)

    {
    int n1,n2,i,count,flags;

// printf("@pagelist_count('%s',%d)\n",pagelist,maxpages);
    if (pagelist[0]=='\0')
        return(maxpages);
    count=0;
    i=0;
    while (pagelist_next_pages(pagelist,maxpages,&i,&n1,&n2,&flags))
        {
        if (n1<=0 && n2<=0)
            continue;
        if (n1>n2)
            {
            int t;
            t=n1;
            n1=n2;
            n2=t;
            }
        if ((maxpages>0 && n1>maxpages) || n2<1)
            continue;
        if (n1<1)
            n1=1;
        if (maxpages>0 && n2>maxpages)
            n2=maxpages;
        if (flags==3)
            count += n2-n1+1;
        else if (n2>=n1)
            count += (n2-n1+2)/2;
        }
    return(count);
    }


static void pagelist_n1n2_adjust(int *n1,int *n2,int flags)

    {
    if (flags==2)
        {
        if ((*n2)>=(*n1))
            {
            (*n1)=((*n1)+1)&(~1);
            (*n2)=(*n2)&(~1);
            }
        else
            {
            (*n1)=(*n1)&(~1);
            (*n2)=((*n2)+1)&(~1);
            }
        }
    else if (flags==1)
        {
        if ((*n2)>=(*n1))
            {
            (*n1)=(*n1)|1;
            (*n2)=((*n2)-1)|1;
            }
        else
            {
            (*n1)=((*n1)-1)|1;
            (*n2)=(*n2)|1;
            }
        }
    }

/*
** (*flags)&1 ==> Odds
** (*flags)&2 ==> Evens
*/
static int pagelist_next_pages(char *pagelist,int maxpages,int *index,
                               int *n1,int *n2,int *flags)

    {
    int i,j;
    char buf[128];

    i=(*index);
    (*flags)=3; /* Even and odd */
    if (evenoddcheck(pagelist[i],flags))
        {
        i++;
        buf[0]='\0';
        }
    else
        {
        for (j=0;j<126 && pagelist[i]>='0' && pagelist[i]<='9';i++)
            buf[j++]=pagelist[i];
        buf[j]='\0';
        if (evenoddcheck(pagelist[i],flags))
            i++;
        }
    if (buf[0]=='\0')
        {
        if (pagelist[i]=='-')
            (*n1)=1;
        else
            {
            (*n1)=-1;
            (*n2)=-1;
            (*index)=i;
            if ((*flags)!=3)
                {
                (*n1)=1;
                (*n2)=maxpages;
                pagelist_n1n2_adjust(n1,n2,(*flags));
                if (pagelist[i]!='\0')
                    (*index)=(*index)+1;
                return(1);
                }
            if (pagelist[i]=='\0')
                return(0);
            (*index)=(*index)+1;
            return(1);
            }
        }
    else
        (*n1)=atoi(buf);
    if (pagelist[i]!='-')
        (*n2)=(*n1);
    else
        {
        if (evenoddcheck(pagelist[i+1],flags))
            {
            i+=2;
            buf[0]='\0';
            }
        else
            {
            for (i++,j=0;j<126 && pagelist[i]>='0' && pagelist[i]<='9';i++)
                buf[j++]=pagelist[i];
            buf[j]='\0';
            if (evenoddcheck(pagelist[i],flags))
                i++;
            }
        if (buf[0]=='\0')
            (*n2)=maxpages;
        else
            (*n2)=atoi(buf);
        }
    if (pagelist[i]!='\0')
        i++;
    (*index)=i;
    pagelist_n1n2_adjust(n1,n2,(*flags));
    return(1);
    }


static int evenoddcheck(int c,int *flags)

    {
    c=tolower(c);
    if (c=='e')
        {
        (*flags) = 2;
        return(1);
        }
    if (c=='o')
        {
        (*flags) = 1;
        return(1);
        }
    return(0);
    }
