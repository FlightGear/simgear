// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <memory>
#include <string>

#include <osg/GraphicsContext>

namespace simgear
{

/* Compressed video encoder.

Generated video contains information about frame times, and also copes with
changes to the width and/or height of the frames.

So replay will replicate variable frame rates and window resizing. */
struct VideoEncoder final
{
    /* Constructor; sets things up to write compressed video to file <path>.
    
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
    VideoEncoder(
            const std::string& path,
            const std::string& codec,
            double quality,
            double speed,
            int bitrate,
            bool log_sws_scale_stats=false
            );
    
    /* Appends gc's current bitmap to compressed video. Works by scheduling a
    callback with gc->add(). <dt> must be non-zero.
    
    Throws exception if error has occurred previously - for example sometimes a
    configuration doesn't fail until we start sending frames. */
    void encode(double dt, osg::GraphicsContext* gc);
    
    ~VideoEncoder();    // non-virtual intentional
    
    private:
    osg::ref_ptr<struct VideoEncoderInternal>   m_internal;
};

}
