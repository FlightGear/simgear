// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2022 Julian Smith

/**
 * @brief Simple profiling support for hard-coded regions of code.
 *
 * To use, create a static RawProfile instance, and place calls to RawProfile.start() and
 * RawProfile.stop() around the code to be profiled. Each time RawProfile.stop() is called,
 * we update a rolling average time and periodically write out to `SG_LOG` and/or a property.
 *
 * @code
 * static RawProfile prof;
 * ...
 * prof.start();
 * ...
 * prof.stop();
 * @endcode
*/

#pragma once

#include <simgear/debug/logstream.hxx>
#include <simgear/props/props.hxx>

#include <chrono>

#include <math.h>


struct RawProfile
{
    /*
        damping_time:
            Time in seconds for average to change by factor of e.
        sglog_name:
            Prefix string when we write average to SG_LOG().
        sglog_interval:
            Interval in seconds between calls to SG_LOG() or zero to disable.
        prop:
            Property to update with average, or null to disable.
        prop_update_interval:
            Interval in seconds between updates of <prop>, or zero to disable.
    */
    RawProfile(
            double damping_time=1,
            const std::string& sglog_name="",
            double sglog_interval=2,
            SGPropertyNode* prop=nullptr,
            double prop_update_interval=1
            )
    {
        m_damping_time = damping_time;
        m_sglog_name = sglog_name;
        m_sglog_interval = sglog_interval;
        m_prop = prop;
    }
    
    typedef std::chrono::high_resolution_clock  clock_t;
    typedef std::chrono::time_point< clock_t>   time_point_t;
    
    void start()
    {
        m_t1 = clock_t::now();
    }
    
    void stop()
    {
        time_point_t t2 = clock_t::now();
        double duration = duration_as_double( t2 - m_t1);
        
        /* Update rolling average. */
        if (m_duration_average == -1)
        {
            m_duration_average = duration;
        }
        else
        {
            const double e = 2.718281828459045;
            double dt = duration_as_double(t2 - m_damping_last);
            m_duration_average = duration - (duration - m_duration_average) * pow(e, -dt/m_damping_time);
        }
        m_damping_last = t2;
        
        if (m_sglog_interval)
        {
            if (duration_as_double(t2 - m_sglog_last) >= m_sglog_interval)
            {
                m_sglog_last = t2;
                SG_LOG(SG_GENERAL, SG_ALERT, m_sglog_name << m_duration_average);
            }
        }
        if (m_prop_update_interval && m_prop)
        {
            if (duration_as_double(t2 - m_prop_update_last) >= m_prop_update_interval)
            {
                m_prop_update_last = t2;
                m_prop->setDoubleValue( m_duration_average);
            }
        }
    }
    
    double duration_as_double( time_point_t::duration dt)
    {
        typedef std::chrono::duration< double>  duration_fp_t;
        return std::chrono::duration_cast< duration_fp_t>(dt).count();
    }
    
    double              m_damping_time;
    time_point_t        m_t1;
    time_point_t        m_damping_last;
    double              m_duration_average = -1;
    
    std::string         m_sglog_name;
    double              m_sglog_interval;
    time_point_t        m_sglog_last;
    
    SGPropertyNode_ptr  m_prop;
    double              m_prop_update_interval;
    time_point_t        m_prop_update_last;
};

