// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1997 Curtis L. Olson - http://www.flightgear.org/~curt

/**
 * @file
 * @brief Implements a simple linear least squares best fit routine
 */

#include <stdio.h>

#include "leastsqs.hxx"


/* 
Least squares fit:

y = b0 + b1x

     n*sum(xi*yi) - (sum(xi)*sum(yi))
b1 = --------------------------------
     n*sum(xi^2) - (sum(xi))^2


b0 = sum(yi)/n - b1*(sum(xi)/n)
*/

double sum_xi, sum_yi, sum_xi_2, sum_xi_yi;
int sum_n;

void least_squares(double *x, double *y, int n, double *m, double *b) {
    int i;

    sum_xi = sum_yi = sum_xi_2 = sum_xi_yi = 0.0;
    sum_n = n;

    for ( i = 0; i < n; i++ ) {
	sum_xi += x[i];
	sum_yi += y[i];
	sum_xi_2 += x[i] * x[i];
	sum_xi_yi += x[i] * y[i];
    }

    /* printf("sum(xi)=%.2f  sum(yi)=%.2f  sum(xi^2)=%.2f  sum(xi*yi)=%.2f\n",
	   sum_xi, sum_yi, sum_xi_2, sum_xi_yi); */

    *m = ( (double)sum_n * sum_xi_yi - sum_xi * sum_yi ) / 
	( (double)sum_n * sum_xi_2 - sum_xi * sum_xi );
    *b = (sum_yi / (double)sum_n) - (*m) * (sum_xi / (double)sum_n);

    /* printf("slope = %.2f  intercept = %.2f\n", *m, *b); */
}


/* incrimentally update existing values with a new data point */
void least_squares_update(double x, double y, double *m, double *b) {
    ++sum_n;

    sum_xi += x;
    sum_yi += y;
    sum_xi_2 += x * x;
    sum_xi_yi += x * y;

    /* printf("sum(xi)=%.2f  sum(yi)=%.2f  sum(xi^2)=%.2f  sum(xi*yi)=%.2f\n",
	   sum_xi, sum_yi, sum_xi_2, sum_xi_yi); */

    *m = ( (double)sum_n * sum_xi_yi - sum_xi * sum_yi ) / 
	( (double)sum_n * sum_xi_2 - sum_xi * sum_xi );
    *b = (sum_yi / (double)sum_n) - (*m) * (sum_xi / (double)sum_n);

    /* printf("slope = %.2f  intercept = %.2f\n", *m, *b); */
}


/* 
  return the least squares error:

              (y[i] - y_hat[i])^2
              -------------------
                      n
 */
double least_squares_error(double *x, double *y, int n, double m, double b) {
    int i;
    double error, sum;

    sum = 0.0;

    for ( i = 0; i < n; i++ ) {
	error = y[i] - (m * x[i] + b);
	sum += error * error;
	// printf("%.2f %.2f\n", error, sum);
    }

    return ( sum / (double)n );
}


/* 
  return the maximum least squares error:

              (y[i] - y_hat[i])^2
 */
double least_squares_max_error(double *x, double *y, int n, double m, double b){
    int i;
    double error, max_error;

    max_error = 0.0;

    for ( i = 0; i < n; i++ ) {
	error = y[i] - (m * x[i] + b);
	error = error * error;
	if ( error > max_error ) {
	    max_error = error;
	}
    }

    return ( max_error );
}


