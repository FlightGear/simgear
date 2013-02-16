// This may look like C code, but it is really -*- C++ -*-
/* 
Copyright (C) 1988 Free Software Foundation
    written by Dirk Grunwald (grunwald@cs.uiuc.edu)

This file is part of the GNU C++ Library.  This library is free
software; you can redistribute it and/or modify it under the terms of
the GNU Library General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your
option) any later version.  This library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU Library General Public License for more details.
You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef SampleStatistic_h

#define SampleStatistic_h 1


#undef min
#undef max

class SampleStatistic
{
protected:
  int n;
  double x;
  double x2;
  double minValue, maxValue;
  double totalTime, cumulativeTime;

public:
  SampleStatistic ();
  inline virtual ~ SampleStatistic ();
  virtual void reset ();

  virtual void operator += (double);
  int samples () const;
  double mean () const;
  double stdDev () const;
  double var () const;
  double min () const;
  double max () const;
  double total () const;
  double cumulative () const;
  double confidence (int p_percentage) const;
  double confidence (double p_value) const;

  void error (const char *msg);
};


inline SampleStatistic::SampleStatistic ()
{
  cumulativeTime = 0;
  reset ();
}

inline int SampleStatistic::samples () const
{
  return (n);
}

inline double SampleStatistic::min () const
{
  return (minValue);
}

inline double SampleStatistic::max () const
{
  return (maxValue);
}

inline double SampleStatistic::total () const
{
  return (totalTime);
}

inline double SampleStatistic::cumulative () const
{
  return (cumulativeTime);
}

inline SampleStatistic::~SampleStatistic ()
{
}

#endif
