// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1988 Free Software Foundation, written by Dirk Grunwald (grunwald@cs.uiuc.edu)

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
