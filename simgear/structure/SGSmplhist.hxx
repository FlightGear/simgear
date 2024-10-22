// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1988 Free Software Foundation, written by Dirk Grunwald (grunwald@cs.uiuc.edu)

#ifndef SampleHistogram_h
#define SampleHistogram_h 1

#include <iosfwd>
#include "SGSmplstat.hxx"

extern const int SampleHistogramMinimum;
extern const int SampleHistogramMaximum;

class SampleHistogram:public SampleStatistic
{
protected:
  short howManyBuckets;
  int *bucketCount;
  double *bucketLimit;

public:

    SampleHistogram (double low, double hi, double bucketWidth = -1.0);

   virtual ~SampleHistogram ();

  virtual void reset ();
  virtual void operator += (double);

  int similarSamples (double);

  int buckets ();

  double bucketThreshold (int i);
  int inBucket (int i);
  void printBuckets (std::ostream &);

};


inline int SampleHistogram::buckets ()
{
  return (howManyBuckets);
}

inline double SampleHistogram::bucketThreshold (int i)
{
  if (i < 0 || i >= howManyBuckets)
    error ("invalid bucket access");
  return (bucketLimit[i]);
}

inline int SampleHistogram::inBucket (int i)
{
  if (i < 0 || i >= howManyBuckets)
    error ("invalid bucket access");
  return (bucketCount[i]);
}

#endif
