// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1988 Free Software Foundation, written by Dirk Grunwald (grunwald@cs.uiuc.edu)

#include <simgear_config.h>

#include <iostream>
#include <fstream>
#include "SGSmplhist.hxx"
#include <math.h>

#ifndef HUGE_VAL
#ifdef HUGE
#define HUGE_VAL HUGE
#else
#include <float.h>
#define HUGE_VAL DBL_MAX
#endif
#endif

const int SampleHistogramMinimum = -2;
const int SampleHistogramMaximum = -1;

SampleHistogram::SampleHistogram (double low, double high, double width)
{
  if (high < low)
    {
      double t = high;
      high = low;
      low = t;
    }

  if (width == -1)
    {
      width = (high - low) / 10;
    }

  howManyBuckets = int ((high - low) / width) + 2;
  bucketCount = new int[howManyBuckets];
  bucketLimit = new double[howManyBuckets];
  double lim = low;
  for (int i = 0; i < howManyBuckets; i++)
    {
      bucketCount[i] = 0;
      bucketLimit[i] = lim;
      lim += width;
    }
  bucketLimit[howManyBuckets - 1] = HUGE_VAL;	/* from math.h */
}

SampleHistogram::~SampleHistogram ()
{
  if (howManyBuckets > 0)
    {
      delete[]bucketCount;
      delete[]bucketLimit;
    }
}

void SampleHistogram::operator += (double value)
{
  int i;
  for (i = 0; i < howManyBuckets; i++)
    {
      if (value < bucketLimit[i])
	break;
    }
  bucketCount[i]++;
  this->SampleStatistic::operator += (value);
}

int SampleHistogram::similarSamples (double d)
{
  int i;
  for (i = 0; i < howManyBuckets; i++)
    {
      if (d < bucketLimit[i])
	return (bucketCount[i]);
    }
  return (0);
}

void SampleHistogram::printBuckets (std::ostream & s)
{
  for (int i = 0; i < howManyBuckets; i++)
    {
      if (bucketLimit[i] >= HUGE_VAL)
	{
	  s << "< max : " << bucketCount[i] << "\n";
	}
      else
	{
	  s << "< " << bucketLimit[i] << " : " << bucketCount[i] << "\n";
	}
    }
}

void SampleHistogram::reset ()
{
  this->SampleStatistic::reset ();
  if (howManyBuckets > 0)
    {
      for (int i = 0; i < howManyBuckets; i++)
	{
	  bucketCount[i] = 0;
	}
    }
}
