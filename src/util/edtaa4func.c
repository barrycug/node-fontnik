/*
 * edtaa4()
 *
 * Sweep-and-update Euclidean distance transform of an
 * image. Positive pixels are treated as object pixels,
 * zero or negative pixels are treated as background.
 * An attempt is made to treat antialiased edges correctly.
 * The input image must have pixels in the range [0,1],
 * and the antialiased image should be a box-filter
 * sampling of the ideal, crisp edge.
 * If the antialias region is more than 1 pixel wide,
 * the result from this transform will be inaccurate.
 *
 * By Stefan Gustavson (stefan.gustavson@gmail.com).
 *
 * Originally written in 1994, based on a verbal
 * description of the SSED8 algorithm published in the
 * PhD dissertation of Ingemar Ragnemalm. This is his
 * algorithm, I only implemented it in C.
 *
 * Updated in 2004 to treat border pixels correctly,
 * and cleaned up the code to improve readability.
 *
 * Updated in 2009 to handle anti-aliased edges.
 */

 /*
 Copyright (C) 2009 Stefan Gustavson (stefan.gustavson@gmail.com)

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

The GNU General Public License is available on <http://www.gnu.org/licenses/>.
 */

#include <util/edtaa4func.h>

/*
 * Compute the local gradient at edge pixels using convolution filters.
 * The gradient is computed only at edge pixels. At other places in the
 * image, it is never used, and it's mostly zero anyway.
 */
void computegradient(double *img, int w, int h, double *gx, double *gy)
{
    int i,j,k,p,q;
    double glength, phi, phiscaled, ascaled, errsign, pfrac, qfrac, err0, err1, err;
#define SQRT2 1.4142136
    for(i = 1; i < h-1; i++) {
        for(j = 1; j < w-1; j++) {
            k = i*w + j;
            if((img[k]>0.0) && (img[k]<1.0)) { // Compute gradient for edge pixels only
                gx[k] = -img[k-w-1] - SQRT2*img[k-1] - img[k+w-1] + img[k-w+1] + SQRT2*img[k+1] + img[k+w+1];
                gy[k] = -img[k-w-1] - SQRT2*img[k-w] - img[k-w+1] + img[k+w-1] + SQRT2*img[k+w] + img[k+w+1];
                glength = (gx[k]*gx[k] + gy[k]*gy[k]);
                if(glength > 0.0) { // Avoid division by zero
                    glength = sqrt(glength);
                    gx[k]=gx[k]/glength;
                    gy[k]=gy[k]/glength;
                }
            }
        }
    }
    // TODO: Compute values for gx, gy also along edges and at corners.
    // (These are zero for now, which is OK but gives less accuracy.)
    // Four different pairs of asymmetric 2x2 kernels are needed for this.
}

/*
 * A somewhat tricky function to approximate the distance to an edge in a
 * certain pixel, with consideration to either the local gradient (gx,gy)
 * or the direction to the pixel (dx,dy) and the pixel greyscale value a.
 * The latter alternative, using (dx,dy), is the metric used by edtaa2().
 * Using a local estimate of the edge gradient (gx,gy) yields much better
 * accuracy at and near edges, and reduces the error even at distant pixels
 * provided that the gradient direction is accurately estimated.
 */
double edgedf(double gx, double gy, double a)
{
    double df, glength, temp, a1;

    if ((gx == 0) || (gy == 0)) { // Either A) gu or gv are zero, or B) both
        df = 0.5-a;  // Linear approximation is A) correct or B) a fair guess
    } else {
        glength = sqrt(gx*gx + gy*gy);
        if(glength>0) {
            gx = gx/glength;
            gy = gy/glength;
        }
        /* Everything is symmetric wrt sign and transposition,
         * so move to first octant (gx>=0, gy>=0, gx>=gy) to
         * avoid handling all possible edge directions.
         */
        gx = fabs(gx);
        gy = fabs(gy);
        if(gx<gy) {
            temp = gx;
            gx = gy;
            gy = temp;
        }
        a1 = 0.5*gy/gx;
        if (a < a1) { // 0 <= a < a1
            df = 0.5*(gx + gy) - sqrt(2.0*gx*gy*a);
        } else if (a < (1.0-a1)) { // a1 <= a <= 1-a1
            df = (0.5-a)*gx;
        } else { // 1-a1 < a <= 1
            df = -0.5*(gx + gy) + sqrt(2.0*gx*gy*(1.0-a));
        }
    }    
    return df;
}

// Adjust distance values near edges based on the local edge gradient
void postprocess(double *img, double *gximg, double *gyimg, int w, int h, short *distx, short *disty, double *dist)
{
    int i, c, n;
    double a, gx, gy, dx, dy, df, t, u, v, uvmargin;

    for(i = 0; i < w*h; i++) {
        dx = distx[i];
        dy = disty[i];
        if((dx==0) && (dy==0)) continue; // Don't touch edges, they are OK
        c = i - disty[i]*w - distx[i]; // Index of closest edge pixel
        a = img[c];
        gx = gximg[c];
        gy = gyimg[c];
        if(dist[i] < 3.0) { // We can improve things only near edges (3.0 is ad hoc)
            df = edgedf(gx, gy, a);
            t = dy*gx - dx*gy; // Offset to orthogonal hit point in edge tangent direction
            u = -df*gx + t*gy; // (u,v) pixel-local coordinates of orthogonal hit point
            v = -df*gy - t*gx; // (pixel center is at (0,0), pixel is a unit square)
            uvmargin = 1.0; // Margin +/- 0.5 is formally correct, but allow some overshoot
            if ((fabs(u) <= uvmargin) && (fabs(v) <= uvmargin)) {
                // Use estimated orthogonal distance to edge based on gradient
                dist[i] = sqrt((dx+u)*(dx+u) + (dy+v)*(dy+v));
            }
        }
    }
}

double distaa4(double *img, double *gximg, double *gyimg, int w, int c, int xc, int yc, int xi, int yi)
{
  int closest;
  double a, gx, gy, dx, dy, di, df;
  
  closest = c-xc-yc*w; // Index to the edge pixel pointed to from c
  a = img[closest];    // Grayscale value at the edge pixel
  gx = gximg[closest]; // X gradient component at the edge pixel
  gy = gyimg[closest]; // Y gradient component at the edge pixel
  
  if(a > 1.0) a = 1.0;
  if(a < 0.0) a = 0.0; // Clip grayscale values outside the range [0,1]
  if(a == 0.0) return 1000000.0; // Not an object pixel, return "very far" ("don't know yet")

  dx = (double)xi;
  dy = (double)yi;
  di = sqrt(dx*dx + dy*dy); // Length of integer vector, like a traditional EDT
  if(di==0) { // Use local gradient only at edges
      // Estimate based on local gradient only
      df = edgedf(gx, gy, a);
  } else {
      // Estimate gradient based on direction to edge (accurate for large di)
      df = edgedf(dx, dy, a);
  }
  return di + df; // Same metric as edtaa2, except at edges (where di=0)
}

// Shorthand macro: add ubiquitous parameters dist, img and w and call distaa4()
#define DISTAA(c,xc,yc,xi,yi) (distaa4(img, gx, gy, w, c, xc, yc, xi, yi))

void edtaa4(double *img, double *gx, double *gy, int w, int h, short *distx, short *disty, double *dist)
{
  int x, y, i, c;
  int offset_u, offset_ur, offset_r, offset_rd,
  offset_d, offset_dl, offset_l, offset_lu;
  double olddist, newdist;
  int cdistx, cdisty, newdistx, newdisty;
  int changed;

  /* Initialize index offsets for the current image width */
  offset_u = -w;
  offset_ur = -w+1;
  offset_r = 1;
  offset_rd = w+1;
  offset_d = w;
  offset_dl = w-1;
  offset_l = -1;
  offset_lu = -w-1;

  /* Initialize the distance images */
  for(i=0; i<w*h; i++) {
    distx[i] = 0; // At first, all pixels point to
    disty[i] = 0; // themselves as the closest known.
    if(img[i] <= 0.0)
      {
	dist[i]= 1000000.0; // Big value, means "not set yet"
      }
    else if (img[i]<1.0) {
      dist[i] = edgedf(gx[i], gy[i], img[i]); // Gradient-assisted estimate
    }
    else {
      dist[i]= 0.0; // Inside the object
    }
  }

  /* Perform the transformation */
  do
    {
      changed = 0;

      /* Scan rows, except first row */
      for(y=1; y<h; y++)
        {

          /* move index to leftmost pixel of current row */
          i = y*w;

          /* scan right, propagate distances from above & left */

          /* Leftmost pixel is special, has no left neighbors */
          olddist = dist[i];
          if(olddist > 0) // If non-zero distance or not set yet
            {
	      c = i + offset_u; // Index of candidate for testing
	      cdistx = distx[c];
	      cdisty = disty[c];
              newdistx = cdistx;
              newdisty = cdisty+1;
              newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
              if(newdist < olddist)
                {
                  distx[i]=newdistx;
                  disty[i]=newdisty;
		  dist[i]=newdist;
                  olddist=newdist;
                  changed = 1;
                }

	      c = i+offset_ur;
	      cdistx = distx[c];
	      cdisty = disty[c];
              newdistx = cdistx-1;
              newdisty = cdisty+1;
              newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
              if(newdist < olddist)
                {
                  distx[i]=newdistx;
                  disty[i]=newdisty;
		  dist[i]=newdist;
                  changed = 1;
                }
            }
          i++;

          /* Middle pixels have all neighbors */
          for(x=1; x<w-1; x++, i++)
            {
              olddist = dist[i];
              if(olddist <= 0) continue; // No need to update further

	      c = i+offset_l;
	      cdistx = distx[c];
	      cdisty = disty[c];
              newdistx = cdistx+1;
              newdisty = cdisty;
              newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
              if(newdist < olddist)
                {
                  distx[i]=newdistx;
                  disty[i]=newdisty;
		  dist[i]=newdist;
                  olddist=newdist;
                  changed = 1;
                }

	      c = i+offset_lu;
	      cdistx = distx[c];
	      cdisty = disty[c];
              newdistx = cdistx+1;
              newdisty = cdisty+1;
              newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
              if(newdist < olddist)
                {
                  distx[i]=newdistx;
                  disty[i]=newdisty;
		  dist[i]=newdist;
                  olddist=newdist;
                  changed = 1;
                }

	      c = i+offset_u;
	      cdistx = distx[c];
	      cdisty = disty[c];
              newdistx = cdistx;
              newdisty = cdisty+1;
              newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
              if(newdist < olddist)
                {
                  distx[i]=newdistx;
                  disty[i]=newdisty;
		  dist[i]=newdist;
                  olddist=newdist;
                  changed = 1;
                }

	      c = i+offset_ur;
	      cdistx = distx[c];
	      cdisty = disty[c];
              newdistx = cdistx-1;
              newdisty = cdisty+1;
              newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
              if(newdist < olddist)
                {
                  distx[i]=newdistx;
                  disty[i]=newdisty;
		  dist[i]=newdist;
                  changed = 1;
                }
            }

          /* Rightmost pixel of row is special, has no right neighbors */
          olddist = dist[i];
          if(olddist > 0) // If not already zero distance
            {
	      c = i+offset_l;
	      cdistx = distx[c];
	      cdisty = disty[c];
              newdistx = cdistx+1;
              newdisty = cdisty;
              newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
              if(newdist < olddist)
                {
                  distx[i]=newdistx;
                  disty[i]=newdisty;
		  dist[i]=newdist;
                  olddist=newdist;
                  changed = 1;
                }

	      c = i+offset_lu;
	      cdistx = distx[c];
	      cdisty = disty[c];
              newdistx = cdistx+1;
              newdisty = cdisty+1;
              newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
              if(newdist < olddist)
                {
                  distx[i]=newdistx;
                  disty[i]=newdisty;
		  dist[i]=newdist;
                  olddist=newdist;
                  changed = 1;
                }

	      c = i+offset_u;
	      cdistx = distx[c];
	      cdisty = disty[c];
              newdistx = cdistx;
              newdisty = cdisty+1;
              newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
              if(newdist < olddist)
                {
                  distx[i]=newdistx;
                  disty[i]=newdisty;
		  dist[i]=newdist;
                  changed = 1;
                }
            }

          /* Move index to second rightmost pixel of current row. */
          /* Rightmost pixel is skipped, it has no right neighbor. */
          i = y*w + w-2;

          /* scan left, propagate distance from right */
          for(x=w-2; x>=0; x--, i--)
            {
              olddist = dist[i];
              if(olddist <= 0) continue; // Already zero distance

	      c = i+offset_r;
	      cdistx = distx[c];
	      cdisty = disty[c];
              newdistx = cdistx-1;
              newdisty = cdisty;
              newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
              if(newdist < olddist)
                {
                  distx[i]=newdistx;
                  disty[i]=newdisty;
		  dist[i]=newdist;
                  changed = 1;
                }
            }
        }
      
      /* Scan rows in reverse order, except last row */
      for(y=h-2; y>=0; y--)
        {
          /* move index to rightmost pixel of current row */
          i = y*w + w-1;

          /* Scan left, propagate distances from below & right */

          /* Rightmost pixel is special, has no right neighbors */
          olddist = dist[i];
          if(olddist > 0) // If not already zero distance
            {
	      c = i+offset_d;
	      cdistx = distx[c];
	      cdisty = disty[c];
              newdistx = cdistx;
              newdisty = cdisty-1;
              newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
              if(newdist < olddist)
                {
                  distx[i]=newdistx;
                  disty[i]=newdisty;
		  dist[i]=newdist;
                  olddist=newdist;
                  changed = 1;
                }

	      c = i+offset_dl;
	      cdistx = distx[c];
	      cdisty = disty[c];
              newdistx = cdistx+1;
              newdisty = cdisty-1;
              newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
              if(newdist < olddist)
                {
                  distx[i]=newdistx;
                  disty[i]=newdisty;
		  dist[i]=newdist;
                  changed = 1;
                }
            }
          i--;

          /* Middle pixels have all neighbors */
          for(x=w-2; x>0; x--, i--)
            {
              olddist = dist[i];
              if(olddist <= 0) continue; // Already zero distance

	      c = i+offset_r;
	      cdistx = distx[c];
	      cdisty = disty[c];
              newdistx = cdistx-1;
              newdisty = cdisty;
              newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
              if(newdist < olddist)
                {
                  distx[i]=newdistx;
                  disty[i]=newdisty;
		  dist[i]=newdist;
                  olddist=newdist;
                  changed = 1;
                }

	      c = i+offset_rd;
	      cdistx = distx[c];
	      cdisty = disty[c];
              newdistx = cdistx-1;
              newdisty = cdisty-1;
              newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
              if(newdist < olddist)
                {
                  distx[i]=newdistx;
                  disty[i]=newdisty;
		  dist[i]=newdist;
                  olddist=newdist;
                  changed = 1;
                }

	      c = i+offset_d;
	      cdistx = distx[c];
	      cdisty = disty[c];
              newdistx = cdistx;
              newdisty = cdisty-1;
              newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
              if(newdist < olddist)
                {
                  distx[i]=newdistx;
                  disty[i]=newdisty;
                  dist[i]=newdist;
                  olddist=newdist;
                  changed = 1;
                }

	      c = i+offset_dl;
	      cdistx = distx[c];
	      cdisty = disty[c];
              newdistx = cdistx+1;
              newdisty = cdisty-1;
              newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
              if(newdist < olddist)
                {
                  distx[i]=newdistx;
                  disty[i]=newdisty;
                  dist[i]=newdist;
                  changed = 1;
                }
            }
          /* Leftmost pixel is special, has no left neighbors */
          olddist = dist[i];
          if(olddist > 0) // If not already zero distance
            {
	      c = i+offset_r;
	      cdistx = distx[c];
	      cdisty = disty[c];
              newdistx = cdistx-1;
              newdisty = cdisty;
              newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
              if(newdist < olddist)
                {
                  distx[i]=newdistx;
                  disty[i]=newdisty;
                  dist[i]=newdist;
                  olddist=newdist;
                  changed = 1;
                }

	      c = i+offset_rd;
	      cdistx = distx[c];
	      cdisty = disty[c];
              newdistx = cdistx-1;
              newdisty = cdisty-1;
              newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
              if(newdist < olddist)
                {
                  distx[i]=newdistx;
                  disty[i]=newdisty;
		  dist[i]=newdist;
                  olddist=newdist;
                  changed = 1;
                }

	      c = i+offset_d;
	      cdistx = distx[c];
	      cdisty = disty[c];
              newdistx = cdistx;
              newdisty = cdisty-1;
              newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
              if(newdist < olddist)
                {
                  distx[i]=newdistx;
                  disty[i]=newdisty;
                  dist[i]=newdist;
                  changed = 1;
                }
            }

          /* Move index to second leftmost pixel of current row. */
          /* Leftmost pixel is skipped, it has no left neighbor. */
          i = y*w + 1;
          for(x=1; x<w; x++, i++)
            {
              /* scan right, propagate distance from left */
              olddist = dist[i];
              if(olddist <= 0) continue; // Already zero distance

	      c = i+offset_l;
	      cdistx = distx[c];
	      cdisty = disty[c];
              newdistx = cdistx+1;
              newdisty = cdisty;
              newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
              if(newdist < olddist)
                {
                  distx[i]=newdistx;
                  disty[i]=newdisty;
                  dist[i]=newdist;
                  changed = 1;
                }
            }
        }
    }
  while(changed); // Sweep until no more updates are made

  /* The transformation is completed. */

}
