// SPDX-License-Identifier: LGPL-2.1-or-later

#include "simgear_config.h"
#include "video-encoder.hxx"

#ifdef SG_FFMPEG

/* Video encoding is supported. */

#include "video-encoder-internal.hxx"

#include <simgear/debug/logstream.hxx>

#include <osg/Image>

#include <condition_variable>
#include <mutex>
#include <thread>


namespace simgear
{

/* Support for streaming video of gc's pixels to file. */
struct VideoEncoderInternal : osg::GraphicsOperation
{
    osg::ref_ptr<osg::Image>    m_image;
    FfmpegEncoder               m_ffmpeg_encoder;
    double                      m_dt = 0;
    int                         m_encoder_busy = 0;  /**/
    std::string                 m_exception;
    
    std::mutex                  m_mutex;
    std::condition_variable     m_condition_variable;
    std::thread                 m_thread;
    

    /* See FfmpegEncoder constructor for API. */
    VideoEncoderInternal(
            const std::string& path,
            const std::string& codec,
            double quality,
            double speed,
            int bitrate,
            bool log_sws_scale_stats
            )
    :
    osg::GraphicsOperation("VideoEncoderOperation", false /*keep*/),
    m_image(new osg::Image),
    m_ffmpeg_encoder(path, codec, quality, speed, bitrate, log_sws_scale_stats),
    m_thread(
            [this]
            {
                this->thread_fn();
            }
            )
    {
    }
    
    void thread_fn()
    {
        /* We repeatedly wait for a new frame to be available, signalled by
        m_encoder_busy being +1. */
        SG_LOG(SG_GENERAL, SG_DEBUG, "thread_fn() starting");
        for(;;)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_encoder_busy == -1)
            {
                /* This is us being told to quit. */
                break;
            }
            else if (m_encoder_busy == 1)
            {
                /* New frame needs encoding. */
                try
                {
                    m_ffmpeg_encoder.encode(
                            m_image->s(),
                            m_image->t(),
                            3 * m_image->s() /*stride*/,
                            m_image->data(),
                            m_dt
                            );
                }
                catch (std::exception& e)
                {
                    SG_LOG(SG_GENERAL, SG_ALERT, "Caught exception: " << e.what());
                    m_exception = e.what();
                    m_encoder_busy = -1;
                    m_condition_variable.notify_all();
                    break;
                }
                m_encoder_busy = 0;
                m_condition_variable.notify_all();
            }
            else
            {
                m_condition_variable.wait(lock);
            }
        }
        SG_LOG(SG_GENERAL, SG_DEBUG, "thread_fn() returning");
    }
    
    void operator() (osg::GraphicsContext* gc)
    {
        /* Called by OSG when <gc> is ready.  We wait for any existing frame
        encode to finish, then encode <gs>'s frame. */
        std::unique_lock<std::mutex> lock(m_mutex);
        for(;;)
        {
            if (m_encoder_busy == -1)
            {
                /* Encoding failed or finished. */
                SG_LOG(SG_GENERAL, SG_ALERT, "operator(): m_encoder_busy=" << m_encoder_busy);
                break;
            }
            else if (m_encoder_busy == 0)
            {
                m_image->readPixels(
                        0,
                        0,
                        gc->getTraits()->width,
                        gc->getTraits()->height,
                        GL_RGB,
                        GL_UNSIGNED_BYTE
                        );
                m_encoder_busy = 1;
                m_condition_variable.notify_all();
                break;
            }
            else
            {
                m_condition_variable.wait(lock);
            }
        }
    }
    
    void encode(double dt, osg::GraphicsContext* gc)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_exception.empty())
        {
            throw std::runtime_error(m_exception);
        }
        assert(dt != 0);
        m_dt = dt;
        gc->add(this);
    }
    
    ~VideoEncoderInternal()
    {
        {
            /* Set m_encoder_busy to -1 to stop our thread. */
            std::unique_lock<std::mutex> lock(m_mutex);
            m_encoder_busy = -1;
            m_condition_variable.notify_all();
        }
        m_thread.join();
    }
};

static void av_log(void* /*avcl*/, int level, const char* format, va_list va)
{
    if (level < 0)  return;
    sgDebugPriority sglevel;
    if (level < 20) sglevel = SG_ALERT;
    if (level < 28) sglevel = SG_WARN;
    if (level < 36) sglevel = SG_INFO;
    if (level < 44) sglevel = SG_DEBUG;
    if (level < 54) sglevel = SG_BULK;
    else sglevel = SG_BULK;
    #ifdef _WIN32
        /* Windows does not have vasprintf(). */
        char message[200];
        vsnprintf(message, sizeof(message), format, va);
    #else
        char* message = nullptr;
        vasprintf(&message, format, va);
    #endif
    SG_LOG(SG_VIEW, sglevel, "level=" << level << ": " << message);
    #ifndef _WIN32
        free(message);
    #endif
}

VideoEncoder::VideoEncoder(
        const std::string& path,
        const std::string& codec,
        double quality,
        double speed,
        int bitrate,
        bool log_sws_scale_stats
        )
{
    av_log_set_callback(av_log);
    m_internal = new VideoEncoderInternal(path,  codec, quality, speed, bitrate, log_sws_scale_stats);
}

VideoEncoder::~VideoEncoder()
{
}

void VideoEncoder::encode(double dt, osg::GraphicsContext* gc)
{
    if (dt == 0)
    {
        SG_LOG(SG_GENERAL, SG_ALERT, "Ignoring frame because dt is zero");
        return;
    }
    m_internal->encode(dt, gc);
}

}   /* namespace simgear. */

#else

/* SG_FFMPEG not defined - video encoding is not supported. */

namespace simgear
{

struct VideoEncoderInternal : osg::GraphicsOperation
{
};

VideoEncoder::VideoEncoder(
        const std::string& path,
        const std::string& codec,
        double quality,
        double speed,
        int bitrate,
        bool log_sws_scale_stats
        )
{
    throw std::runtime_error("Video encoding is not available in this build of Flightgear");
}

VideoEncoder::~VideoEncoder()
{
}

void VideoEncoder::encode(double dt, osg::GraphicsContext* gc)
{
    throw std::runtime_error("Video encoding is not available in this build of Flightgear");
}

}   /* namespace simgear. */

#endif
