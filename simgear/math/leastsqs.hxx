// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1997 Curtis L. Olson - http://www.flightgear.org/~curt

/**
 * @file
 * @brief Implements a simple linear least squares best fit routine
 */

#ifndef _LEASTSQS_H
#define _LEASTSQS_H


#ifndef __cplusplus
# error This library requires C++
#endif


/**
Classical least squares fit:

\f[
    y = b_0 + b_1 * x
\f]

\f[
    b_1 = \frac{n * \sum_0^{i-1} (x_i*y_i) - \sum_0^{i-1} x_i* \sum_0^{i-1} y_i}
          {n*\sum_0^{i-1} x_i^2 - (\sum_0^{i-1} x_i)^2}
\f]

\f[
    b_0 = \frac{\sum_0^{i-1} y_i}{n} - b_1 * \frac{\sum_0^{i-1} x_i}{n}
\f]
*/
void least_squares(double *x, double *y, int n, double *m, double *b);


/**
 * Incrimentally update existing values with a new data point.
 */
void least_squares_update(double x, double y, double *m, double *b);


/**
  @return the least squares error:.
\f[

    \frac{(y_i - \hat{y}_i)^2}{n}
\f]
*/
double least_squares_error(double *x, double *y, int n, double m, double b);


/**
  @return the maximum least squares error.

\f[
    (y_i - \hat{y}_i)^2
\f]
*/
double least_squares_max_error(double *x, double *y, int n, double m, double b);


#endif // _LEASTSQS_H


