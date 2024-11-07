#include <algorithm>

#include <osg/Notify>

#include <simgear/debug/logstream.hxx>

#if defined(IGNORE_OSG_MESSAGES)
const std::string osg_ignored_messages[] = {
   "0xde1",
   "0x806f",
};
#endif

/**
* merge OSG output into our logging system, so it gets recorded to file,
* and so we can display a GUI console with renderer issues, especially
* shader compilation warnings and errors.
*/
class SGNotifyHandler : public osg::NotifyHandler {
public:
    // note this callback will be invoked by OSG from multiple threads.
    // fortunately our Simgear logging implementation already handles
    // that internally, so we simply pass the message on.
    virtual void notify(osg::NotifySeverity severity, const char* message) {
        // avoid asserts when message is NULL or empty
        // https://gitlab.com/flightgear/flightgear/-/issues/3
        if (!message)
            return;

        std::string msg(message);
        if (msg.empty()) {
            return;
        }

        // Remove the newline character, if any. SG_LOG already adds its own
        if (msg.back() == '\n') {
            msg.pop_back();
        }

        // Detect if this is an ignored message
    #if defined(IGNORE_OSG_MESSAGES)
        auto it = std::find_if(std::begin(osg_ignored_messages),
                               std::end(osg_ignored_messages),
                               [&msg](const std::string& s) {
                                   return msg.find(s) != std::string::npos;
                               });
        if (it != std::end(osg_ignored_messages)) {
            return;
        }
    #endif
    
        // Detect whether a osg::Reference derived object is deleted with a non-zero
        // reference count. In this case trigger a segfault to get a stack trace.
        if (msg.find("the final reference count was") != std::string::npos) {
            // as this is going to segfault ignore the translation of severity and always output the message.
            SG_LOG(SG_GL, SG_ALERT, msg);
#ifndef DEBUG
            throw msg;
#endif
            return;
        }

        SG_LOG(SG_OSG, translateSeverity(severity), msg);
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
