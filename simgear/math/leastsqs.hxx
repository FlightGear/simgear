// leastsqs.h -- Implements a simple linear least squares best fit routine
//
// Written by Curtis Olson, started September 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA  02111-1307, USA.
//
// $Id$
///


#ifndef _LEASTSQS_H
#define _LEASTSQS_H


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


/* 
Least squares fit:

y = b0 + b1x

     n*sum(xi*yi) - (sum(xi)*sum(yi))
b1 = --------------------------------
     n*sum(xi^2) - (sum(xi))^2


b0 = sum(yi)/n - b1*(sum(xi)/n)
*/

void least_squares(double *x, double *y, int n, double *m, double *b);

/* incrimentally update existing values with a new data point */
void least_squares_update(double x, double y, double *m, double *b);


/* 
  return the least squares error:

              (y[i] - y_hat[i])^2
              -------------------
                      n
*/
double least_squares_error(double *x, double *y, int n, double m, double b);


/* 
  return the maximum least squares error:

              (y[i] - y_hat[i])^2
*/
double least_squares_max_error(double *x, double *y, int n, double m, double b);


#endif // _LEASTSQS_H


