#include <cstring>

#include <osg/Notify>

#include <simgear/debug/logstream.hxx>

/**
* merge OSG output into our logging system, so it gets recorded to file,
* and so we can display a GUI console with renderer issues, especially
* shader compilation warnings and errors.
*/
class NotifyLogger : public osg::NotifyHandler
{
public:
    // note this callback will be invoked by OSG from multiple threads.
    // fortunately our Simgear logging implementation already handles
    // that internally, so we simply pass the message on.
    virtual void notify(osg::NotifySeverity severity, const char* message) {
        // Detect whether a osg::Reference derived object is deleted with a non-zero
        // reference count. In this case trigger a segfault to get a stack trace.
        if (strstr(message, "the final reference count was")) {
            // as this is going to segfault ignore the translation of severity and always output the message.
            SG_LOG(SG_GL, SG_ALERT, message);
#ifndef DEBUG
            throw std::string(message);
            //int* trigger_segfault = 0;
            //*trigger_segfault     = 0;
#endif
            return;
        }
        char*tmessage = strdup(message);
        if (tmessage) {
            char*lf = strrchr(tmessage, '\n');
            if (lf)
                *lf = 0;

            SG_LOG(SG_OSG, translateSeverity(severity), tmessage);
            free(tmessage);
        }
    }

private:
    sgDebugPriority translateSeverity(osg::NotifySeverity severity) {
        switch (severity) {
        case osg::ALWAYS:
        case osg::FATAL:      return SG_ALERT;
        case osg::WARN:       return SG_WARN;
        case osg::NOTICE:
        case osg::INFO:       return SG_INFO;
        case osg::DEBUG_FP:
        case osg::DEBUG_INFO: return SG_DEBUG;
        default:              return SG_ALERT;
        }
    }
};
