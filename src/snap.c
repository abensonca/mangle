/*------------------------------------------------------------------------------
� A J S Hamilton 2001
------------------------------------------------------------------------------*/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "manglefn.h"
#include "defaults.h"

/* getopt options */
const char *optstr = "dqSa:b:t:y:m:s:e:v:p:i:o:H";

/* local functions */
void	usage(void);
#ifdef	GCC
int snap(int npoly, polygon *poly[npoly]);
#else
int snap(int npoly, polygon *poly[/*npoly*/]);
#endif

/*------------------------------------------------------------------------------
  Main program.
*/
int main(int argc, char *argv[])
{
    int ifile, nadj, nfiles, npoly, npolys, i, flag;
    polygon *polys[NPOLYSMAX];

    /* default output format */
    fmt.out = keywords[POLYGON];
    /* default is to renumber output polygons with new id numbers */
    fmt.newid = 'n';

    /* parse arguments */
    parse_args(argc, argv);

    /* at least one input and output filename required as arguments */
    if (argc - optind < 2) {
	if (optind > 1 || argc - optind == 1) {
	    fprintf(stderr, "%s requires at least 2 arguments: polygon_infile and polygon_outfile\n", argv[0]);
	    usage();
	    exit(1);
	} else {
	    usage();
	    exit(0);
	}
    }

    msg("---------------- snap ----------------\n");

    /* snap angles */
    scale(&axtol, axunit, 's');
    scale(&btol, bunit, 's');
    scale(&thtol, thunit, 's');
    axunit = 's';
    bunit = 's';
    thunit = 's';
    msg("snap angles: axis %g%c latitude %g%c edge %g%c\n", axtol, axunit, btol, bunit, thtol, thunit);
    scale(&axtol, axunit, 'r');
    scale(&btol, bunit, 'r');
    scale(&thtol, thunit, 'r');
    axunit = 'r';
    bunit = 'r';
    thunit = 'r';

    /* tolerance angle for multiple intersections */
    if (mtol != 0.) {
	scale(&mtol, munit, 's');
	munit = 's';
	msg("multiple intersections closer than %g%c will be treated as coincident\n", mtol, munit);
	scale(&mtol, munit, 'r');
	munit = 'r';
    }

    /* advise data format */
    advise_fmt(&fmt);

    /* read polygons */
    npoly = 0;
    nfiles = argc - 1 - optind;
    for (ifile = optind; ifile < optind + nfiles; ifile++) {
	npolys = rdmask(argv[ifile], &fmt, NPOLYSMAX - npoly, &polys[npoly]);
	if (npolys == -1) exit(1);
	npoly += npolys;
    }
    if (nfiles >= 2) {
	msg("total of %d polygons read\n", npoly);
    }
    if (npoly == 0) {
	msg("STOP\n");
	exit(0);
    }

    if (fmt.healpix_out == 1) {
      if (fmt.nweights == 0) fmt.nweights = npoly/nfiles;
      /* check whether fmt.nweights is a valid number based on the HEALPix pixelization scheme */
      for (i = 1; i <= 8192; i = 2*i) {
	if (12*pow(i,2) == fmt.nweights) {
	  flag = 1;
	  break;
	}
	else {
	  flag = 0;
	  continue;
	}
      }
      if (flag == 0) {
	fprintf(stderr, "ERROR: %d is not a valid number of HEALPix polygons\n", fmt.nweights);
	fprintf(stderr, "see the HEALPix website at http://healpix.jpl.nasa.gov for more information\n");
	fprintf(stderr, "the -H argument will thus be ignored\n");
	fmt.healpix_out = 0;
      }
      else msg("polygon_outfile will be formatted for subsequent use in rasterize, with nweights = %d\n", fmt.nweights);
    }

    /* adjust boundaries of polygons */
    nadj = snap(npoly, polys);
    if(nadj==-1) exit(1);

    /* write polygons */
    ifile = argc - 1;
    npoly = wrmask(argv[ifile], &fmt, npoly, polys);
    if (npoly == -1) exit(1);

    return(0);
}

/*------------------------------------------------------------------------------
*/
void usage(void)
{
    printf("usage:\n");
    printf("snap [-d] [-q] [-S] [-a<a>[u]] [-b<a>[u]] [-t<a>[u]] [-y<r>] [-m<a>[u]] [-s<n>] [-e<n>] [-vo|-vn] [-p[+|-][<n>]] [-i<f>[<n>][u]] [-o<f>[u]] polygon_infile1 [polygon_infile2 ...] polygon_outfile\n");
#include "usage.h"
}

/*------------------------------------------------------------------------------
*/
#include "parse_args.c"

/*------------------------------------------------------------------------------
  Make almost coincident caps of polygons coincide.

  Input: npoly = number of polygons to snap.
	 *poly[npoly] = array of npoly pointers to polygon structures.
  Return value: number of caps adjusted.
*/
int snap(int npoly, polygon *poly[/*npoly*/])
{
#define WARNMAX		8
  int i, inull, iprune, nadj, dnadj, warnmax;
  int *start;
  int *total;
  int p, max_pixel, ier;
  
  /* start by sorting polygons by pixel number*/
  poly_sort(npoly,poly,'p');

  /* allocate memory for pixel info arrays start and total */ 
  /* if only self-snapping, don't use pixelization */
  max_pixel=(selfsnap)? 1 : poly[npoly-1]->pixel+1; 
  start = (int *) malloc(sizeof(int) * max_pixel);
  if (!start) {
    fprintf(stderr, "snap_polys: failed to allocate memory for %d integers\n", max_pixel);
    return(-1);
  }
  total = (int *) malloc(sizeof(int) * max_pixel);
  if (!total) {
    fprintf(stderr, "snap_polys: failed to allocate memory for %d integers\n", max_pixel);
    return(-1);
  }
  
  /* if we're only doing self-snapping, don't use the pixelization info */ 
  if(selfsnap){
    start[0]=0;
    total[0]=npoly;
  }
  else{
    /* build lists of starting indices of each pixel and total number of polygons in each pixel*/
    ier=pixel_list(npoly, poly, max_pixel, start, total);
    if (ier == -1) {
      fprintf(stderr, "snap: error building pixel index lists\n");
      return(-1);
    }
  }
  
  /*turn off warning messages if using more than one pixel*/
  warnmax= (max_pixel<=1) ? WARNMAX : 0; 

  /* snap edges of polygons to each other */
  nadj=0;
  for(p=0;p<max_pixel;p++){
    if(total[p]==0) continue;
    dnadj=snap_polys(&fmt, total[p], &poly[start[p]], selfsnap, axtol, btol, thtol, ytol, mtol,((selfsnap)? warnmax : warnmax/2),0x0);
    if(dnadj==-1) return(-1);
    nadj+=dnadj;
  }
  
  /* prune polygons */
  inull = 0;
  for (i = 0; i < npoly; i++) {
    iprune = prune_poly(poly[i], mtol);
    if (iprune >= 2) {
      if (WARNMAX > 0 && inull == 0)
	msg("warning from snap: the following polygons have zero area:\n");
      if (inull < WARNMAX) {
	msg(" %d", (fmt.newid == 'o')? poly[i]->id : i);
      } else if (inull == WARNMAX) {
	msg(" ... more\n");
      }
      inull++;
    }
  }
  if (WARNMAX > 0 && inull > 0 && inull <= WARNMAX) msg("\n");
  if (inull > 0) msg("snap: %d snapped polygons have zero area (but are being retained)\n", inull);
  
  /* assign new polygon id numbers */
  if (fmt.newid == 'n') {
    for (i = 0; i < npoly; i++) {
      poly[i]->id = i;
    }
  }

  if (fmt.newid == 'p') {
    for (i = 0; i < npoly; i++) {
      poly[i]->id = poly[i]->pixel;
    }
  }

  
  msg("snap: total of %d caps adjusted\n", nadj);
  
  return(nadj);
}