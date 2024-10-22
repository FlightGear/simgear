// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#ifndef SG_VIDEO_ENCODER_STANDALONE
    #include <simgear/debug/logstream.hxx>
    #include <simgear/timing/rawprofile.hxx>
#endif

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include <assert.h>
#include <iostream>
#include <sstream>


std::ostream& operator<< (std::ostream& out, const AVRational& r)
{
    return out << "(" << r.num << "/" << r.den << ")";
}

/* Convenience exception type. Use like:
    throw ExceptionStream << "something failed e=" << e;
 */
struct ExceptionStream : std::exception
{
    mutable std::ostringstream  m_buffer;
    mutable std::string         m_buffer2;
    
    ExceptionStream()
    {
    }
    
    ExceptionStream(const ExceptionStream& rhs)
    {
        m_buffer << rhs.m_buffer.str();
    }
    
    template<typename T>
    const ExceptionStream& operator<< (const T& t) const
    {
        m_buffer << t;
        return *this;
    }
    
    const char* what() const noexcept
    {
        if (m_buffer2 == "")
        {
            m_buffer2 = m_buffer.str();
        }
        return m_buffer2.c_str();
    }
    
    void prefix(const std::string& s)
    {
        what();
        m_buffer2 = s + m_buffer2;
    }
};


static const char* select(const char** names, int names_num, double select)
{
    int i = select * names_num;
    if (i == names_num) i -= 1;
    return names[i];
}


/* Video encoder which uses ffmpeg libraries. */
struct FfmpegEncoder final
{
    double              m_quality = 0;
    double              m_speed = 0;
    int                 m_bitrate = 0;
    
    struct SwsContext*  m_sws_context = nullptr;
    AVFrame*            m_frame_yuv = nullptr;
    const AVCodec*      m_codec = nullptr;
    AVCodecContext*     m_codec_context = nullptr;
    AVStream*           m_stream = nullptr;
    AVPacket*           m_packet = nullptr;
    AVFormatContext*    m_format_context = nullptr;
    
    /* These are only used in exception text. */
    std::string     m_path;
    std::string     m_codec_name;
    
    double  m_t = 0;
    int64_t m_t_int_prev = 0;
    bool    m_have_written_header = false;
    bool    m_log_sws_scale_stats;
    
    /* Constructor.
    
    Args:
        path
            Name of output file. Container type is inferred from suffix using
            avformat_alloc_output_context2()). List of supported containers can
            be found with 'ffmpeg -formats'.
        codec_name
            Name of codec, passed to avcodec_find_encoder_by_name(). List of
            supported codecs can be found with 'ffmpeg -codecs'.
        quality
            Encoding quality in range 0..1 or -1 to use codec's default.
        speed:
            Encoding speed in range 0..1 or -1 to use codec's default.
        bitrate
            Target bitratae in bits/sec or -1 to use codex's default.
        log_sws_scale_stats
            If true we write summary timing stats for our calls of sws_scale()
            to SG_LOG().
    
    Throws exception if we cannot set up encoding, e.g. unrecognised
    codec. Other configuration errors may be detected only when encode() is
    called.
    */
    FfmpegEncoder(
            const std::string& path,
            const std::string& codec_name,
            double quality,
            double speed,
            int bitrate,
            bool log_sws_scale_stats=false
            )
    {
        assert(quality == -1 || (quality >= 0 && quality <= 1));
        assert(speed == -1 || (speed >= 0 && speed <= 1));
        
        m_path = path;
        m_codec_name = codec_name;
        
        m_quality = quality;
        m_speed = speed;
        m_bitrate = bitrate;
        
        m_log_sws_scale_stats = log_sws_scale_stats;
        
        try
        {
            m_codec = avcodec_find_encoder_by_name(codec_name.c_str());
            if (!m_codec)
            {
                throw ExceptionStream() << "avcodec_find_encoder_by_name() failed to find codec_name='"
                        << codec_name << "'";
            }
            assert(m_codec);

            avformat_alloc_output_context2(
                    &m_format_context,
                    nullptr /*oformat*/,
                    nullptr /*format_name*/,
                    path.c_str()
                    );
            if (!m_format_context)
            {
                throw ExceptionStream() << "avformat_alloc_output_context2() failed to recognise path='"
                        << path << "'";
            }
            assert(m_format_context);

            if (!(m_format_context->oformat->flags & AVFMT_NOFILE))
            {
                int e = avio_open(&m_format_context->pb, path.c_str(), AVIO_FLAG_WRITE);
                if (e < 0)
                {
                    throw ExceptionStream() << "avio_open() failed, e=" << e;
                }
                assert(e >= 0);
            }

            m_stream = avformat_new_stream(m_format_context, nullptr);
            if (!m_stream)
            {
                throw ExceptionStream() << "avformat_new_stream() returned null.";
            }
            assert(m_stream);
            m_stream->id = m_format_context->nb_streams - 1;
        }
        catch (ExceptionStream& e)
        {
            e.prefix("Video encoding failed (path=" + m_path + " codec=" + m_codec_name + "): ");
            clearall();
            throw;
        }
    }

    /* Used by constructor if throwing because error occured, and by
    destructor. */
    void clearall()
    {
        if (m_codec_context)
        {
            eof();
        }
        if (m_format_context && m_have_written_header)
        {
            av_write_trailer(m_format_context);
        }
        
        clear();
        if (m_format_context)
        {
            avformat_free_context(m_format_context);   /* Also frees m_stream. */
            m_format_context = nullptr;
        }
    }
    
    /* Destructor flushes any remaining encoded video and cleans up. */
    ~FfmpegEncoder()    // non-virtual intentional
    {
        clearall();
    }
    
    /* Clear state that depends on frame size. */
    void clear()
    {
        av_frame_free(&m_frame_yuv);
        sws_freeContext(m_sws_context);
        m_sws_context = nullptr;
        avcodec_free_context(&m_codec_context);
        av_packet_free(&m_packet);
        /* m_codec doesn't need freeing. */
    }
    
    /* Set state that depends on frame size. Throws if error occurs. */
    void set(int width, int height)
    {
        try
        {
            int e;

            /* Create YUV frame. */
            m_frame_yuv = av_frame_alloc();
            m_frame_yuv->format = AV_PIX_FMT_YUV420P;
            m_frame_yuv->width = width;
            m_frame_yuv->height = height;
            e = av_frame_get_buffer(m_frame_yuv, 0);
            if (e < 0)
            {
                throw ExceptionStream() << "av_frame_get_buffer() failed: " << e;
            }
            assert(e >= 0);

            /* Create RGB => YUV converter. */
            m_sws_context = sws_getContext(
                    width, height, AV_PIX_FMT_RGB24,
                    width, height, (enum AVPixelFormat) m_frame_yuv->format,
                    SWS_BILINEAR,
                    nullptr /*srcFilter*/,
                    nullptr /*dstFilter*/,
                    nullptr /*param*/
                    );
            if (!m_sws_context)
            {
                throw ExceptionStream() << "sws_getContext() failed for " << width << "x" << height;
            }
            assert(m_sws_context);

            /* Create codec context. */
            m_codec_context = avcodec_alloc_context3(m_codec);
            if (!m_codec_context)
            {
                throw ExceptionStream() << "avcodec_alloc_context3() failed";
            }
            m_codec_context->codec_id = m_codec->id;
            if (m_bitrate > 0)
            {
                m_codec_context->bit_rate = m_bitrate;
            }
            SG_LOG(SG_GENERAL, SG_DEBUG, "m_codec_context->bit_rate=" << m_codec_context->bit_rate);
            /* Resolution must be a multiple of two. */
            m_codec_context->width = width / 2 * 2;
            m_codec_context->height = height / 2 * 2;
            m_codec_context->time_base = AVRational{ 1, 60 };
            //m_codec_context->gop_size = 12; /* emit one intra m_frame every twelve frames at most */
            m_codec_context->pix_fmt = AV_PIX_FMT_YUV420P;
            /* Some formats want m_stream headers to be separate. */
            if (m_format_context->oformat->flags & AVFMT_GLOBALHEADER)
            {
                m_codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            }
            
            /* Set dictionary entries. */
            AVDictionary* dictionary = nullptr;
            if (m_codec->id == AV_CODEC_ID_H264)
            {
                if (m_quality != -1)
                {
                    /* -12-51, default 23.0. */
                    std::string q = std::to_string( (1-m_quality) * (51+12) - 12);
                    SG_LOG(SG_GENERAL, SG_ALERT, "crf m_quality=" << m_quality << " => " << q);
                    e = av_dict_set(&dictionary, "crf", q.c_str(), 0 /*flags*/); /* -12-51, default 23.0. */
                }
            }
            if (m_codec->id == AV_CODEC_ID_H265)
            {
                /* Reduce verbose output. */
                e = av_dict_set(&dictionary, "x265-params", "log-level=error", 0 /*flags*/);
                assert(e >= 0);
                
                if (m_quality != -1)
                {
                    /* 0-51, default 28.0. */
                    std::string q = std::to_string( (1-m_quality) * 51);
                    SG_LOG(SG_GENERAL, SG_DEBUG, "crf m_quality=" << m_quality << " => " << q);
                    e = av_dict_set(&dictionary, "crf", q.c_str(), 0 /*flags*/);
                    assert(e >= 0);
                }
            }
            if (m_codec->id == AV_CODEC_ID_THEORA)
            {
                SG_LOG(SG_GENERAL, SG_DEBUG, "AV_CODEC_ID_THEORA m_quality=" << m_quality);
                if (m_quality != -1)
                {
                    /* Enable constant quality mode. */
                    e = av_dict_set(&dictionary, "flags", "qscale", 0 /*flags*/);
                    assert(e >= 0);
                    
                    /* Quality scaling is a little obscure in
                    https://ffmpeg.org/ffmpeg-codecs.html#libtheora, but this
                    appears to work with our m_quality's 0..1 range: */
                    std::string q = std::to_string(m_quality * FF_QP2LAMBDA * 10);
                    e = av_dict_set(&dictionary, "global_quality", q.c_str(), 0 /*flags*/);
                    assert(e >= 0);
                }
            }
            if (m_codec->id == AV_CODEC_ID_H264 || m_codec->id == AV_CODEC_ID_H265)
            {
                if (m_speed != -1)
                {
                    /* Set preset to string derived from m_speed. */
                    static const char* speeds[] = {
                            "veryslow",
                            "slower",
                            "slow",
                            "medium",
                            "fast",
                            "faster",
                            "veryfast",
                            "superfast",
                            "ultrafast",
                            };
                    const char* speed = select(speeds, sizeof(speeds) / sizeof(speeds[0]), m_speed);
                    SG_LOG(SG_GENERAL, SG_DEBUG, "preset: " << m_speed <<" => " << speed);
                    e = av_dict_set(&dictionary, "preset", speed, 0 /*flags*/);
                    assert(e >= 0);
                }
            }
            
            /* Show dictionary. */
            SG_LOG(SG_GENERAL, SG_ALERT, "dictionary " << av_dict_count(dictionary) << ":");
            for (AVDictionaryEntry* t = av_dict_get(dictionary, "", nullptr, AV_DICT_IGNORE_SUFFIX);
                    t;
                    t = av_dict_get(dictionary, "", t, AV_DICT_IGNORE_SUFFIX)
                    )
            {
                SG_LOG(SG_GENERAL, SG_ALERT, "    " << t->key << "=" << t->value);
            }
            
            e = avcodec_open2(m_codec_context, m_codec, &dictionary);
            if (e < 0)
            {
                throw ExceptionStream() << "avcodec_open2() failed: " << e;
            }
            assert(e >= 0);
            e = avcodec_parameters_from_context(m_stream->codecpar, m_codec_context);
            if (e < 0)
            {
                throw ExceptionStream() << "avcodec_parameters_from_context() failed: " << e;
            }
            assert(e >= 0);
            
            /* Send header. */
            SG_LOG(SG_GENERAL, SG_DEBUG, "m_stream->time_base=" << m_stream->time_base);
            /* This appears to override m_stream->time_base to be 1/90,000. */
            e = avformat_write_header(m_format_context, &dictionary);
            if (e < 0)
            {
                throw ExceptionStream() << "avformat_write_header() failed: " << e;
            }
            m_have_written_header = true;
            /* Create packet. */
            m_packet = av_packet_alloc();
            if (!m_packet)
            {
                throw ExceptionStream() << "av_packet_alloc() failed";
            }
        }
        catch (ExceptionStream& e)
        {
            e.prefix("Video encoding failed (path=" + m_path + " codec=" + m_codec_name + "): ");
            clearall();
            throw;
        }
    }
    
    /* Sends new frame to encoder. Data must be RGB, 8 bits per channel. Will
    throw if error occurs. */
    void encode(int width, int height, int stride, void* input_raw, double dt)
    {
        if (!m_format_context)
        {
            throw ExceptionStream() << "Cannot encode after earlier error - m_format_context is null";
        }
        assert(dt > 0);
        
        bool restart = false;
        if (m_frame_yuv)
        {
            if (m_frame_yuv->width != width)     restart = true;
            if (m_frame_yuv->height != height)   restart = true;
        }
        else
        {
            /* This is the first frame. */
            restart = true;
        }
        
        if (restart)
        {
            if (m_frame_yuv)
            {
                /* Drain any remaining compressed data. */
                eof();
            }
            
            /* Set up new pipeline. */
            clear();
            set(width, height);
        }

        int e = av_frame_make_writable(m_frame_yuv);
        assert(e >= 0);
        
        /* Convert raw_input (RGB24) to m_frame_yuv (YUV420P).

        We also need to flip the image vertically so set m_input_linesize[0] to
        -ve and make m_input_data[0] point to the last line in input_raw. */
        #ifndef SG_VIDEO_ENCODER_STANDALONE
        static RawProfile raw_profile(1, "sws_scale() time: ");
        if (m_log_sws_scale_stats)  raw_profile.start();
        #endif
        int stride2 = -stride;
        const uint8_t* input_raw2 = (const uint8_t*) input_raw + stride * (height-1);
        sws_scale(
                m_sws_context,
                &input_raw2,
                &stride2,
                0 /*srcSliceY*/,
                height,
                m_frame_yuv->data,
                m_frame_yuv->linesize
                );
        #ifndef SG_VIDEO_ENCODER_STANDALONE
        if (m_log_sws_scale_stats)  raw_profile.stop();
        #endif

        /* Send m_frame_yuv to encoder. */
        m_t += dt;
        int64_t t_int = (int64_t) (m_t / m_codec_context->time_base.num * m_codec_context->time_base.den);
        int64_t dt_int = t_int - m_t_int_prev;
        m_t_int_prev = t_int;

        SG_LOG(SG_GENERAL, SG_DEBUG, ""
                << " avcodec_send_frame()"
                << " dt=" << dt
                << " m_t=" << m_t
                << " m_codec_context->time_base=" << m_codec_context->time_base
                << " m_stream->time_base=" << m_stream->time_base
                << " dt_int=" << dt_int
                << " t_int=" << t_int
                );

        m_frame_yuv->pts = t_int;
        
        e = avcodec_send_frame(m_codec_context, m_frame_yuv);
        SG_LOG(SG_GENERAL, SG_DEBUG, "m_stream->time_base=" << m_stream->time_base);
        assert(e >= 0);
        
        /* Process any available encoded video data. */
        drain();
    }
    
    /* End of video at current size. Not necessarily end of output - we create
    new encoder when input size changes. */
    void eof()
    {
        /* Send eof to m_codec_context and read final encoded data. */
        assert(m_codec_context);
        int e = avcodec_send_frame(m_codec_context, nullptr);
        assert(e >= 0);
        drain();
        clear();
    }
    
    /* Read all available compressed data and send to m_format_context. Returns
    0 on success or 1 at EOF. */
    int drain()
    {
        if (!m_codec_context || !m_packet || !m_stream || !m_format_context)
        {
            return 1;
        }
        for(;;)
        {
            int e = avcodec_receive_packet(m_codec_context, m_packet);
            if (e == AVERROR(EAGAIN))
            {
                SG_LOG(SG_GENERAL, SG_DEBUG, "AVERROR(EAGAIN)");
                return 0;
            }
            if (e == AVERROR_EOF)
            {
                SG_LOG(SG_GENERAL, SG_DEBUG, "AVERROR_EOF");
                return 1;
            }
            assert(e >= 0);
            
            /* rescale output packet timestamp values from codec to m_stream timebase. */
            av_packet_rescale_ts(m_packet, m_codec_context->time_base, m_stream->time_base);
            
            SG_LOG(SG_GENERAL, SG_DEBUG, ""
                    << " m_codec_context->time_base=" << m_codec_context->time_base
                    << " m_stream->time_base=" << m_stream->time_base
                    << " m_packet->pts=" << m_packet->pts
                    << " m_packet->dts=" << m_packet->dts
                    );
            
            m_packet->stream_index = m_stream->index;

            /* Write the compressed data. */
            e = av_interleaved_write_frame(m_format_context, m_packet);
            assert(e >= 0);
            
            /* packet is now blank (av_interleaved_write_frame() takes ownership of
             * its contents and resets packet), so that no unreferencing is necessary.
             * This would be different if one used av_write_frame(). */
        }
    }
};
