#ifdef SG_PROPS_UNTHREADSAFE

    #include "props-unsafe.cxx"

#else
// props.cxx - implementation of a property list.
// Started Fall 2000 by David Megginson, david@megginson.com
// This code is released into the Public Domain.
//
// See props.html for documentation [replace with URL when available].
//
// $Id$

#include <simgear_config.h>

#include "props.hxx"

#include <algorithm>
#include <limits>

#include <set>
#include <sstream>
#include <iomanip>
#include <iterator>
#include <exception> // can't use sg_exception because of PROPS_STANDALONE
#include <mutex>
#include <thread>

#include <stdio.h>
#include <string.h>

#if PROPS_STANDALONE
# include <iostream>
using std::cerr;
#else
# include <boost/algorithm/string/find_iterator.hpp>
# include <boost/algorithm/string/predicate.hpp>
# include <boost/functional/hash.hpp>
# include <boost/range.hpp>

# include <simgear/compiler.h>
# include <simgear/debug/logstream.hxx>
# include <simgear/sg_inlines.h>

# include "PropertyInterpolationMgr.hxx"
# include "vectorPropTemplates.hxx"

#endif

using std::endl;
using std::find;
using std::sort;
using std::vector;
using std::stringstream;

using namespace simgear;


/*
Locking support.

All code that reads/writes SGPropertyNode data has to create either a
SGPropertyLockShared or a SGPropertyLockExclusive.

To avoid deadlocks, code must never lock a node when holding lock on a
child/grandchild/... node.
*/

static bool         s_property_locking_first_time = true;
static bool         s_property_locking_active = true;
static bool         s_property_locking_verbose = false;
static bool         s_property_timing_active = false;
static bool         s_property_change_parent_listeners = false;

static SGPropertyNode* s_main_tree_root = nullptr;

#include "props_io.hxx"

struct SGPropertyLockListener : SGPropertyChangeListener
{
    SGPropertyLockListener(bool& out, const char* name)
    :
    m_out(out),
    m_name(name)
    {}

    void valueChanged(SGPropertyNode* node)
    {
        m_out = node->getBoolValue();
        std::cerr << __FILE__ << ":" << __LINE__ << ":"
                << " " << m_name << ":"
                << " m_out=" << m_out
                << "\n";
    }
    bool&       m_out;
    std::string m_name;
};

void SGPropertyLockControl(
        SGPropertyNode* active,
        SGPropertyNode* verbose,
        SGPropertyNode* timing,
        SGPropertyNode* parent_listeners
        )
{
        std::cerr << __FILE__ << ":" << __LINE__ << ":"
                << " active: " << active->getPath()
                << " verbose: " << verbose->getPath()
                << " timing: " << timing->getPath()
                << " parent_listeners: " << parent_listeners->getPath()
                << "\n";

    s_main_tree_root = active->getRootNode();
    active->setBoolValue(s_property_locking_active);
    active->addChangeListener(new SGPropertyLockListener(s_property_locking_active, "active"));

    verbose->setBoolValue(s_property_locking_verbose);
    verbose->addChangeListener(new SGPropertyLockListener(s_property_locking_verbose, "verbose"));

    timing->setBoolValue(s_property_timing_active);
    timing->addChangeListener(new SGPropertyLockListener(s_property_timing_active, "timing"));

    parent_listeners->setBoolValue(s_property_change_parent_listeners);
    parent_listeners->addChangeListener(new SGPropertyLockListener(s_property_change_parent_listeners, "parent-listeners"));
}

#undef SG_PROPS_GATHER_TIMING

#ifdef SG_PROPS_GATHER_TIMING

// Code for gathering timing information about use of locks. Unfortunately the
// act of measuring gives small but noticeable overheads (e.g. 17.7 fps to 18.6
// fps), so it's macro-ed out by default.
//
static double       s_property_locking_time_acquire = 0;
static double       s_property_locking_time_release = 0;
static double       s_property_locking_time_acquire_shared = 0;
static double       s_property_locking_time_release_shared = 0;
static long         s_property_locking_n_acquire = 0;
static long         s_property_locking_n_release = 0;
static long         s_property_locking_n_acquire_shared = 0;
static long         s_property_locking_n_release_shared = 0;

static long         ScopedTime_count = 0;
static const int    ScopedTime_sampling = 1000*1000+1;
static double       ScopedTime_t0 = 0;

// This is a copy of flightgear/src/Time/TimeManager.cxx:TimeUTC().
//
// SGTimeStamp() doesn't return UTC time on some systems, e.g. Linux with
// _POSIX_TIMERS > 0 uses _POSIX_MONOTONIC_CLOCK if available.
//
// So we define our own time function here.
//
static double TimeUTC()
{
    time_t      sec;
    unsigned    nsec;

    #ifdef _WIN32
        static bool qpc_init = false;
        static LARGE_INTEGER s_frequency;
        static BOOL s_use_qpc;
        if (!qpc_init) {
            s_use_qpc = QueryPerformanceFrequency(&s_frequency);
            qpc_init = true;
        }
        if (qpc_init && s_use_qpc) {
            LARGE_INTEGER now;
            QueryPerformanceCounter(&now);
            sec = now.QuadPart / s_frequency.QuadPart;
            nsec = (1000000000LL * (now.QuadPart - sec * s_frequency.QuadPart)) / s_frequency.QuadPart;
        }
        else {
            unsigned int msec = timeGetTime();
            sec = msec / 1000;
            nsec = (msec - sec * 1000) * 1000 * 1000;
        }
    #elif defined(_POSIX_TIMERS) && (0 < _POSIX_TIMERS)
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        sec = ts.tv_sec;
        nsec = ts.tv_nsec;
    #elif defined( HAVE_GETTIMEOFDAY ) // openbsd
        struct timeval current;
        gettimeofday(&current, NULL);
        sec = current.tv_sec;
        nsec = current.tv_usec * 1000;
    #elif defined( HAVE_GETLOCALTIME )
        SYSTEMTIME current;
        GetLocalTime(&current);
        sec = current.wSecond;
        nsec = current.wMilliseconds * 1000 * 1000;
    #else
        #error Unable to find UTC time.
    #endif
    return sec + nsec * 1.0e-9;
}

// Gathers timing information for a scoped block.
struct ScopedTime
{
    ScopedTime( bool shared, const char* name, double& total, long& n)
    {
        if (!s_property_timing_active) {
            m_total = nullptr;
            return;
        }
        ScopedTime_count += 1;
        if (ScopedTime_count % ScopedTime_sampling == 0) {
            m_time_start = TimeUTC();
            m_total = &total;
            m_n = &n;
            m_name = name;
            m_shared = shared;
        }
        else {
            m_total = nullptr;
        }
    }

    ~ScopedTime()
    {
        if (!m_total) {
            return;
        }
        double t = TimeUTC();
        double dt = t - m_time_start;
        *m_total += dt;
        *m_n += 1;
        int diagnostic_interval = 100;
        if (((*m_n) % diagnostic_interval) == 0) {
            if (ScopedTime_t0 == 0) {
                ScopedTime_t0 = t;
            }
            double t_projected = (*m_total) * ScopedTime_sampling;
            double t_projected_fraction = t_projected / (t - ScopedTime_t0);
            double n_per_sec_projected = (*m_n) * ScopedTime_sampling / (t - ScopedTime_t0);
            std::cerr << __FILE__ << ":" << __LINE__ << ":"
                    << " " << m_name << (m_shared ? "    shared" : " exclusive")
                    << ":"
                    << " n=" << *m_n
                    << " t=" << *m_total
                    << " average time=" << (*m_total) / (*m_n)
                    << " t_projected=" << t_projected << " (" << t_projected_fraction * 100 << "%)"
                    << " n_per_sec_projected=" << n_per_sec_projected
                    << " raw:"
                    << " t=" << (t - ScopedTime_t0)
                    << " ScopedTime_count=" << ScopedTime_count
                    << " ScopedTime_count/(t - ScopedTime_t0)=" << (ScopedTime_count / (t - ScopedTime_t0))
                    << "\n";
        }
    }

    double m_time_start;
    double* m_total;
    long* m_n;
    const char* m_name;
    bool m_shared;

};
#endif

/* Abstract lock for shared/exclusive locks. This base API is used by code that
doesn't care whether a lock is shared or exclusive, so it can be called by code
that has exclusive or shared locks. */
struct SGPropertyLock
{
    /* Returns true/false if environmental variable <name> is 1 or 0, otherwise
    <default_>. */
    static bool env_default(const char* name, bool default_)
    {
        bool ret;
        const char* value = getenv(name);
        if (!value) ret = default_;
        else if (!strcmp(value, "0"))  ret = false;
        else if (!strcmp(value, "1"))  ret = true;
        else {
            std::cerr << __FILE__ << ":" << __LINE__ << ":"
                    << " unrecognised " << name
                    << " should be 0 or 1: " << value
                    << "\n";
            ret = default_;
        }
        if (0) std::cerr << __FILE__ << ":" << __LINE__ << ":"
                << " name=" << name
                << " value=" << (value ? value : "<unset>")
                << " returning: " << ret
                << "\n";
        return ret;
    }

    static void init_static()
    {
        if (s_property_locking_first_time) {
            s_property_locking_first_time = false;
            s_property_locking_active = env_default("SG_PROPERTY_LOCKING", true);
            s_property_locking_verbose = env_default("SG_PROPERTY_LOCKING_VERBOSE", false);
        }
    }

    SGPropertyLock()
    {
        init_static();
    }

    /* These are provided for the rare situations where share/non-shared
    agnostic code needs to release/acquire a lock. */
    virtual void acquire() = 0;
    virtual void release() = 0;

    /* Acquires shared or exclusive lock on <mutex>, generating various
    diagnostics depending on s_property_locking_verbose. */
    void acquire_internal(const SGPropertyNode& node, bool shared)
    {
        #ifdef SG_PROPS_GATHER_TIMING
        ScopedTime  t(
                shared,
                "acquire",
                shared ? s_property_locking_time_acquire_shared : s_property_locking_time_acquire,
                shared ? s_property_locking_n_acquire_shared : s_property_locking_n_acquire
                );
        #endif
        if (!s_property_locking_active) {
            return;
        }

        if (!s_property_locking_verbose) {
            if (shared) node._mutex.lock_shared();
            else node._mutex.lock();
            return;
        }

        /* Verbose. Try non-blocking lock first. */
        try {
            bool ok = (shared) ? node._mutex.try_lock_shared() : node._mutex.try_lock();
            if (ok) return;
        }
        catch (std::exception& e) {
            std::cerr << __FILE__ << ":" << __LINE__ << ":"
                    << (shared ? "    shared" : " exclusive") << " try-lock failed."
                    << " &node=" << &node
                    << " _name=" << node._name
                    << ": " << e.what()
                    << "\n";
            throw;
        }

        /* Non-blocking call failed to acquire, so now do a blocking lock. */
        std::cerr << __FILE__ << ":" << __LINE__ << ":"
                << (shared ? "    shared" : " exclusive") << " lock contention"
                << " &node=" << &node
                << " _name=" << node._name
                << "\n";
        try {
            if (shared) node._mutex.lock_shared();
            else node._mutex.lock();
        }
        catch (std::exception& e) {
            std::cerr << __FILE__ << ":" << __LINE__ << ":"
                    << (shared ? "    shared" : " exclusive") << " lock failed:"
                    << " &node=" << &node
                    << " _name=" << node._name
                    << ": " << e.what()
                    << "\n";
            throw;
        }
    }

    void release_internal(const SGPropertyNode& node, bool shared)
    {
        #ifdef SG_PROPS_GATHER_TIMING
        ScopedTime  t(shared, "release",
                shared ? s_property_locking_time_release_shared : s_property_locking_time_release,
                shared ? s_property_locking_n_release_shared : s_property_locking_n_release
                );
        #endif
        if (!s_property_locking_active) {
            return;
        }
        if (shared) node._mutex.unlock_shared();
        else node._mutex.unlock();
    }

    const SGPropertyNode*   m_node = nullptr;
    bool                    m_own = false;
};

/* Scoped exclusive lock of a SGPropertyNode. */
struct SGPropertyLockExclusive : SGPropertyLock
{
    SGPropertyLockExclusive()
    :
    SGPropertyLock()
    {
    }

    SGPropertyLockExclusive(const SGPropertyNode& node)
    :
    SGPropertyLock()
    {
        assign(node);
    }

    void assign(const SGPropertyNode& node)
    {
        assert(!m_node);
        m_node = &node;
        acquire();
    }

    void acquire() override
    {
        assert(m_node);
        assert(!m_own);
        acquire_internal(*m_node, false /*shared*/);
        m_own = true;
    }
    void release() override
    {
        assert(m_own);
        release_internal(*m_node, false /*shared*/);
        m_own = false;
    }

    ~SGPropertyLockExclusive()
    {
        if (m_own) release();
    }

    private:
        SGPropertyLockExclusive(const SGPropertyLockExclusive&);
};

/* Scoped shared lock of a SGPropertyNode. */
struct SGPropertyLockShared : SGPropertyLock
{
    SGPropertyLockShared()
    :
    SGPropertyLock()
    {
    }

    /* Takes out shared lock for <node>. */
    SGPropertyLockShared(const SGPropertyNode& node)
    :
    SGPropertyLock()
    {
        assign(node);
    }

    /* Sets our SGPropertyNode and takes out a shared lock. Can only
    be called if we don't have a SGPropertyNode. */
    void assign(const SGPropertyNode& node)
    {
        assert(!m_node);
        m_node = &node;
        acquire();
    }

    void acquire() override
    {
        assert(m_node);
        assert(!m_own);
        acquire_internal(*m_node, true /*shared*/);
        m_own = true;
    }
    void release() override
    {
        assert(m_own);
        release_internal(*m_node, true /*shared*/);
        m_own = false;
    }

    ~SGPropertyLockShared()
    {
        if (m_own) release();
    }

    private:
        SGPropertyLockShared(const SGPropertyLockShared&);
};


struct SGPropertyNodeListeners
{
  /* Protect _num_iterators and _items. We use a recursive mutex to allow
  nested access to work as normal. */
  //std::recursive_mutex  _rmutex;

  /* This keeps a count of the current number of nested invocations of
  forEachListener(). If non-zero, other code higher up the stack is iterating
  _items[] so for example code must not erase items in the vector. */
  int _num_iterators = 0;

  std::vector<SGPropertyChangeListener *> _items;
};


////////////////////////////////////////////////////////////////////////
// Local classes.
////////////////////////////////////////////////////////////////////////

/**
 * Comparator class for sorting by index.
 */
class CompareIndices
{
public:
  int operator() (const SGPropertyNode_ptr n1, const SGPropertyNode_ptr n2) const {
    return (n1->getIndex() < n2->getIndex());
  }
};

class AliasChangeListener : public SGPropertyChangeListener
{
  public:
  AliasChangeListener(SGPropertyNode* dest) : _destination(dest)
  {
  }

  void valueChanged(SGPropertyNode* v) override;

  private:
  SGPropertyNode* _destination;
};

////////////////////////////////////////////////////////////////////////
// Local path normalization code.
////////////////////////////////////////////////////////////////////////

#if PROPS_STANDALONE
struct PathComponent
{
  string name;
  int index;
};
#endif

/**
 * Parse the name for a path component.
 *
 * Name: [_a-zA-Z][-._a-zA-Z0-9]*
 */

namespace
{
// Parsing property names is a profiling hotspot. The regular C
// library functions interact with the locale and are therefore quite
// heavyweight. We only support ASCII letters and numbers in property
// names, so use these simple functions instead.

inline bool isalpha_c(int c)
{
    return (c <= 'Z' && c >= 'A') || (c <= 'z' && c >= 'a');
}

inline bool isdigit_c(int c)
{
    return c <= '9' && c >= '0';
}

inline bool isspecial_c(int c)
{
    return  c == '_' ||  c == '-' || c == '.';
}
}

// Used to convert Range to std::string when generating diagnostics.
//
template<typename Range>
std::string RangeToString(const Range &range)
{
    std::string ret;
    for (auto c: range) ret += c;
    return ret;
}

template<typename Range>
inline Range
parse_name (const SGPropertyNode *node, const Range &path)
{
  typename Range::iterator i = path.begin();
  typename Range::iterator max = path.end();

  if (*i == '.') {
    i++;
    if (i != path.end() && *i == '.') {
      i++;
    }
    if (i != max && *i != '/') {
      throw std::runtime_error(
          std::string() + "Illegal character '" + *i + "'"
          + " after initial . or .. in leaf of property path: "
          + node->getPath() + '/' + RangeToString(path)
          );
    }
  }
  else if (isalpha_c(*i) || *i == '_') {
    i++;

    // The rules inside a name are a little
    // less restrictive.
    while (i != max) {
      if (isalpha_c(*i) || isdigit_c(*i) || isspecial_c(*i)) {
        // name += path[i];
      }
      else if (*i == '[' || *i == '/') {
	    break;
      }
      else {
        throw std::runtime_error(
            std::string() + "Illegal character '" + *i + "'"
            + " in leaf of property path"
            + " (may contain only ._- and alphanumeric characters)"
            + ": " + node->getPath() + '/' + RangeToString(path)
            );
      }
      i++;
    }
  }
  else {
    if (path.begin() == i) {
      throw std::runtime_error(
          std::string() + "Illegal character '" + *i + "'"
          + " at start of leaf of property path: "
          + node->getPath() + '/' + RangeToString(path)
          );
    }
  }
  return Range(path.begin(), i);
}

// Validate the name of a single node
inline bool validateName(const std::string& name)
{
  if (name.empty())
    return false;
  if (!isalpha(name[0]) && name[0] != '_')
    return false;
#if PROPS_STANDALONE
  std::string is_any_of("_-.");
  bool rv = true;
  for(unsigned i=1; i<name.length(); ++i) {
    if (!isalnum(name[i]) && is_any_of.find(name[i]) == std::string::npos)  {
      rv = false;
      break;
    }
  }
  return rv;
#else
  return std::all_of(name.begin() + 1, name.end(),
                     [](int c){ return isalpha_c(c) || isdigit_c(c) ||  isspecial_c(c); });
#endif
}

#if PROPS_STANDALONE
/**
 * Parse the name for a path component.
 *
 * Name: [_a-zA-Z][-._a-zA-Z0-9]*
 */
static inline const string
parse_name (const string &path, int &i)
{
  string name = "";
  int max = (int)path.size();

  if (path[i] == '.') {
    i++;
    if (i < max && path[i] == '.') {
      i++;
      name = "..";
    } else {
      name = ".";
    }
    if (i < max && path[i] != '/')
      throw std::invalid_argument("Illegal character after " + name);
  }

  else if (isalpha(path[i]) || path[i] == '_') {
    name += path[i];
    i++;

	      // The rules inside a name are a little
	      // less restrictive.
    while (i < max) {
      if (isalpha(path[i]) || isdigit(path[i]) || path[i] == '_' ||
      path[i] == '-' || path[i] == '.') {
        name += path[i];
      } else if (path[i] == '[' || path[i] == '/') {
        break;
      } else {
        throw std::invalid_argument("name may contain only ._- and alphanumeric characters");
      }
      i++;
    }
  }

  else {
    if (name.size() == 0)
      throw std::invalid_argument("name must begin with alpha or '_'");
  }

  return name;
}


/**
 * Parse the optional integer index for a path component.
 *
 * Index: "[" [0-9]+ "]"
 */
static inline int
parse_index (const string &path, int &i)
{
  int index = 0;

  if (path[i] != '[')
    return 0;
  else
    i++;

  for (int max = (int)path.size(); i < max; i++) {
    if (isdigit(path[i])) {
      index = (index * 10) + (path[i] - '0');
    } else if (path[i] == ']') {
      i++;
      return index;
    } else {
      break;
    }
  }

  throw std::invalid_argument("unterminated index (looking for ']')");
}

/**
 * Parse a single path component.
 *
 * Component: Name Index?
 */
static inline PathComponent
parse_component (const string &path, int &i)
{
  PathComponent component;
  component.name = parse_name(path, i);
  if (component.name[0] != '.')
    component.index = parse_index(path, i);
  else
    component.index = -1;
  return component;
}

/**
 * Parse a path into its components.
 */
static void
parse_path (const string &path, vector<PathComponent> &components)
{
  int pos = 0;
  int max = (int)path.size();

  // Check for initial '/'
  if (path[pos] == '/') {
    PathComponent root;
    root.name = "";
    root.index = -1;
    components.push_back(root);
    pos++;
    while (pos < max && path[pos] == '/')
      pos++;
  }

  while (pos < max) {
    components.push_back(parse_component(path, pos));
    while (pos < max && path[pos] == '/')
      pos++;
  }
}
#endif


////////////////////////////////////////////////////////////////////////
// Other static utility functions.
////////////////////////////////////////////////////////////////////////


static char* copy_string(const char* s)
{
    // assure there is data to be copied
    if (s == nullptr)
        return nullptr;

    size_t slen = strlen(s);
    // REVIEW: Memory Leak - 1,963 bytes in 47 blocks are indirectly lost
    char* copy = new char[slen + 1];

    // the source string length is known so no need to check for '\0'
    // when copying every single character
    memcpy(copy, s, slen);
    *(copy + slen) = '\0';
    return copy;
}

static bool
strings_equal (const char * s1, const char * s2)
{
  return !strcmp(s1, s2);
}


/**
 * Locate a child node by name and index.
 */
template<typename Itr>
static int
find_child(SGPropertyLock& lock, Itr begin, Itr end, int index, const PropertyList& nodes)
{
  size_t nNodes = nodes.size();
#if PROPS_STANDALONE
  for (int i = 0; i < nNodes; i++) {
    SGPropertyNode * node = nodes[i];
    if (node->getIndex() == index && strings_equal(node->getNameString(), begin))
      return i;
  }
#else
  boost::iterator_range<Itr> name(begin, end);
  for (size_t i = 0; i < nNodes; i++) {
    SGPropertyNode * node = nodes[i];

    // searching for a matching index is a lot less time consuming than
    // comparing two strings so do that first.
    if (node->getIndex() == index && boost::equals(node->getNameString(), name))
      return static_cast<int>(i);
  }
#endif
  return -1;
}

/**
 * Locate the child node with the highest index of the same name
 */
static int
find_last_child (SGPropertyLockExclusive& exclusive, const char * name, const PropertyList& nodes)
{
  size_t nNodes = nodes.size();
  int index = -1;

  for (size_t i = 0; i < nNodes; i++) {
    SGPropertyNode * node = nodes[i];
    if (node->getNameString() == name) {
        int idx = node->getIndex();
        if (idx > index) index = idx;
    }
  }
  return index;
}

/**
 * Get first unused index for child nodes with the given name
 */
static int
first_unused_index( SGPropertyLockExclusive& exclusive,
                    const char * name,
                    const PropertyList& nodes,
                    int min_index
                    )
{
  const char* nameEnd = name + strlen(name);

  for( int index = min_index; index < std::numeric_limits<int>::max(); ++index )
  {
    if( find_child(exclusive, name, nameEnd, index, nodes) < 0 )
      return index;
  }

  SG_LOG(SG_GENERAL, SG_ALERT, "Too many nodes: " << name);
  return -1;
}

/* Calls <callback> for each item in _listeners. We are careful to skip nullptr
entries in _listeners->items[], which can be created if listeners are removed
while we are iterating. */
static void forEachListener(
    SGPropertyLockExclusive& exclusive,
    SGPropertyNode* node,
    SGPropertyNodeListeners*& _listeners,
    std::function<void (SGPropertyChangeListener*)> callback
    )
{
  if (!_listeners) return;

  assert(_listeners->_num_iterators >= 0);
  _listeners->_num_iterators += 1;
  exclusive.release();

  {
    SGPropertyLockShared  shared(*node);
    /* We need to use an index here when iterating _listeners->_items, not an
    iterator. This is because a listener may add new listeners, causing the
    vector to be reallocated, which would invalidate any iterator. */
    for (size_t i = 0; i < _listeners->_items.size(); ++i) {
      auto listener = _listeners->_items[i];
      if (listener) {
        shared.release();
        try {
          callback(listener);
        }
        catch (std::exception& e) {
          SG_LOG(SG_GENERAL, SG_ALERT, "Ignoring exception from property callback: " << e.what());
        }
        shared.acquire();
      }
    }
  }

  exclusive.acquire();
  _listeners->_num_iterators -= 1;
  #ifdef __openbsd__
  if (_listeners->_num_iterators < 0) {
    std::cerr << __FILE__ << ":" << __LINE__ << ": _listeners->_num_iterators=" << _listeners->_num_iterators << "\n";
    _listeners->_num_iterators = 0;
    return;
  }
  #else
  assert(_listeners->_num_iterators >= 0);
  #endif

  if (_listeners->_num_iterators == 0) {

    /* Remove any items that have been set to nullptr. */
    _listeners->_items.erase(
        std::remove(
                _listeners->_items.begin(),
                _listeners->_items.end(),
                (SGPropertyChangeListener*) nullptr
                ),
        _listeners->_items.end()
        );
    if (_listeners->_items.empty()) {
      delete _listeners;
      _listeners = nullptr;
    }
  }
}


struct SGPropertyNodeImpl
{
    static bool get_bool(SGPropertyLock& lock, const SGPropertyNode& node)
    {
        if (node._tied)
            return static_cast<SGRawValue<bool>*>(node._value.val)->getValue();
        else
            return node._local_val.bool_val;
    }

    static int get_int(SGPropertyLock& lock, const SGPropertyNode& node)
    {
        if (node._tied)
            return static_cast<SGRawValue<int>*>(node._value.val)->getValue();
        else
            return node._local_val.int_val;
    }

    static int get_long(SGPropertyLock& lock, const SGPropertyNode& node)
    {
        if (node._tied)
            return static_cast<SGRawValue<long>*>(node._value.val)->getValue();
        else
            return node._local_val.long_val;
    }

    static float get_float(SGPropertyLock& lock, const SGPropertyNode& node)
    {
        if (node._tied)
            return static_cast<SGRawValue<float>*>(node._value.val)->getValue();
        else
            return node._local_val.float_val;
    }

    static double get_double(SGPropertyLock& lock, const SGPropertyNode& node)
    {
        if (node._tied)
            return static_cast<SGRawValue<double>*>(node._value.val)->getValue();
        else
            return node._local_val.double_val;
    }

    static const char* get_string(SGPropertyLock& lock, const SGPropertyNode& node)
    {
        if (node._tied)
            return static_cast<SGRawValue<const char*>*>(node._value.val)->getValue();
        else
            return node._local_val.string_val;
    }

    static bool
    set_bool(SGPropertyLockExclusive& exclusive, SGPropertyNode& node, bool val)
    {
        bool changed = false;
        if (node._tied) {
            exclusive.release();
            changed = static_cast<SGRawValue<bool>*>(node._value.val)->setValue(val);
            exclusive.acquire();
        }
        else {
            changed = true;
            node._local_val.bool_val = val;
        }
        if (changed) {
            SGPropertyNodeImpl::fireValueChanged(exclusive, node, &node);
        }
        return changed;
    }

    static bool
    set_int(SGPropertyLockExclusive& exclusive, SGPropertyNode& node, int val)
    {
        bool changed = false;
        if (node._tied) {
            exclusive.release();
            changed = static_cast<SGRawValue<int>*>(node._value.val)->setValue(val);
            exclusive.acquire();
        }
        else {
            changed = true;
            node._local_val.int_val = val;
        }
        if (changed) {
            SGPropertyNodeImpl::fireValueChanged(exclusive, node, &node);
        }
        return changed;
    }

    static bool
    set_long(SGPropertyLockExclusive& exclusive, SGPropertyNode& node, long val)
    {
        bool changed = false;
        if (node._tied) {
            exclusive.release();
            changed = static_cast<SGRawValue<long>*>(node._value.val)->setValue(val);
            exclusive.acquire();
        }
        else {
            changed = true;
            node._local_val.long_val = val;
        }
        if (changed) {
            SGPropertyNodeImpl::fireValueChanged(exclusive, node, &node);
        }
        return changed;
    }

    static bool
    set_float(SGPropertyLockExclusive& exclusive, SGPropertyNode& node, float val)
    {
        bool changed = false;
        if (node._tied) {
            exclusive.release();
            changed = static_cast<SGRawValue<float>*>(node._value.val)->setValue(val);
            exclusive.acquire();
        }
        else {
            changed = true;
            node._local_val.float_val = val;
        }
        if (changed) {
            SGPropertyNodeImpl::fireValueChanged(exclusive, node, &node);
        }
        return changed;
    }

    static bool
    set_double(SGPropertyLockExclusive& exclusive, SGPropertyNode& node, double val)
    {
        bool changed = false;
        if (node._tied) {
            exclusive.release();
            changed = static_cast<SGRawValue<double>*>(node._value.val)->setValue(val);
            exclusive.acquire();
        }
        else {
            changed = true;
            node._local_val.double_val = val;
        }
        if (changed) {
            SGPropertyNodeImpl::fireValueChanged(exclusive, node, &node);
        }
        return changed;
    }

    static bool
    set_string(SGPropertyLockExclusive& exclusive, SGPropertyNode& node, const char* val)
    {
        bool changed = false;
        if (node._tied) {
            exclusive.release();
            changed = static_cast<SGRawValue<const char*>*>(node._value.val)->setValue(val);
            exclusive.acquire();
        }
        else {
            changed = true;
            delete [] node._local_val.string_val;
            node._local_val.string_val = copy_string(val);
        }
        if (changed) {
            SGPropertyNodeImpl::fireValueChanged(exclusive, node, &node);
        }
        return changed;
    }

    static void
    appendNode(SGPropertyLockExclusive& exclusive, SGPropertyNode& parent, SGPropertyNode* child)
    {
        if (parent._attr & SGPropertyNode::VALUE_CHANGED_DOWN)
        {
            // Propagate to child nodes.
            child->setAttribute(SGPropertyNode::VALUE_CHANGED_DOWN, true);
            child->setAttribute(SGPropertyNode::VALUE_CHANGED_UP, true);
        }
        parent._children.push_back(child);
    }

    static SGPropertyNode*
    getChildImplCreate(SGPropertyLockExclusive& exclusive, SGPropertyNode& node0, const char* begin, const char* end, int index)
    {
        SGPropertyNode* node = getExistingChild(exclusive, node0, begin, end, index);

        if (node) {
            return node;
        }
        else {
            // REVIEW: Memory Leak - 2,028 (1,976 direct, 52 indirect) bytes in 13 blocks are definitely lost
            node = new SGPropertyNode(begin, end, index, &node0);
            SGPropertyNodeImpl::appendNode(exclusive, node0, node);
            exclusive.release();
            node0.fireChildAdded(node);
            exclusive.acquire();
            return node;
        }
    }

    static SGPropertyNode*
    getExistingChild(SGPropertyLock& lock, SGPropertyNode& node, const char* begin, const char* end, int index)
    {
        int pos = find_child(lock, begin, end, index, node._children);
        if (pos >= 0)
            return node._children[pos];
        return 0;
    }

    static SGPropertyNode*
    getChildImpl(SGPropertyLockShared& shared, SGPropertyNode& node, const char* begin, const char* end, int index)
    {
        return getExistingChild(shared, node, begin, end, index);
    }

    static SGPropertyNode*
    getChildImpl(SGPropertyNode& node, const char* begin, const char* end, int index, bool create)
    {
        if (create) {
            SGPropertyLockExclusive exclusive(node);
            return getChildImplCreate(exclusive, node, begin, end, index);
        }
        else {
            SGPropertyLockShared shared(node);
            return getChildImpl(shared, node, begin, end, index);
        }
    }

    /* Get the value as a string. */
    static std::string
    make_string(SGPropertyLock& lock, const SGPropertyNode& node, const char* defaultValue="")
    {
        if (!getAttribute(lock, node, SGPropertyNode::READ))
            return defaultValue;
        switch (node._type) {
        case props::ALIAS:
            {
            auto p = node._value.alias->target;
            lock.release();
            std::string ret = p->getStringValue();
            lock.acquire();
            return ret;
            }
        case props::BOOL:
            return get_bool(lock, node) ? "true" : "false";
        case props::STRING:
        case props::UNSPECIFIED:
            return get_string(lock, node);
        case props::NONE:
            return defaultValue;
        default:
            break;
        }
        stringstream sstr;
        switch (node._type) {
        case props::INT:
            sstr << get_int(lock, node);
            break;
        case props::LONG:
            sstr << get_long(lock, node);
            break;
        case props::FLOAT:
            sstr << get_float(lock, node);
            break;
        case props::DOUBLE:
            sstr << std::setprecision(10) << get_double(lock, node);
            break;
        case props::EXTENDED:
        {
            props::Type realType = node._value.val->getType();
            // Perhaps this should be done for all types?
            if (realType == props::VEC3D || realType == props::VEC4D)
                sstr.precision(10);
            static_cast<SGRawExtended*>(node._value.val)->printOn(sstr);
        }
            break;
        default:
            return defaultValue;
        }
        return sstr.str();
    }

    /**
     * Trace a write access for a property.
     */
    static void
    trace_write (SGPropertyLockExclusive& exclusive, const SGPropertyNode& node)
    {
        std::string value = SGPropertyNodeImpl::make_string(exclusive, node);
        bool own = exclusive.m_own;
        if (own) exclusive.release();
        {
            SG_LOG(SG_GENERAL, SG_ALERT, "TRACE: Write node " << node.getPath()
	                << ", value \"" << value << '"');
        }
        if (own) exclusive.acquire();
    }

    /**
     * Trace a read access for a property.
     *
     * Note that this releases and re-acquires <lock>.
     */
    static void
    trace_read(SGPropertyLock& lock, const SGPropertyNode& node)
    {
        // This is tricky; we need to release the <lock> lock because getPath()
        // will need to acquire locks of parent nodes, and claiming a
        // parent/grandparent/... node's lock after a child node's lock can deadlock.
        //
        // So we temporarily release <lock>. But this could break calling code in
        // subtle ways - *this could be modified by other threads.
        //
        // Hopefully we are only called for specific debugging purposes.
        //
        std::string value = SGPropertyNodeImpl::make_string(lock, node);
        lock.release();
        SG_LOG(SG_GENERAL, SG_ALERT, "TRACE: Read node " << node.getPath()
                << ", value \"" << value << '"');
        lock.acquire();
    }

    static bool getAttribute (SGPropertyLock& lock, const SGPropertyNode& node, SGPropertyNode::Attribute attr)
    {
        return ((node._attr & attr) != 0);
    }

    static void setChildrenUpDown(SGPropertyLockExclusive& exclusive, SGPropertyNode& node)
    {
        for (SGPropertyNode* child: node._children) {
            SGPropertyLockExclusive exclusive_child(*child);
            setAttributes(
                    exclusive_child,
                    *child,
                    child->_attr | SGPropertyNode::VALUE_CHANGED_UP | SGPropertyNode::VALUE_CHANGED_DOWN
                    );
        }
    }

    static void setAttributes(SGPropertyLockExclusive& exclusive, SGPropertyNode& node, int attr)
    {
        // Preserve VALUE_CHANGED_UP and VALUE_CHANGED_DOWN if they are already
        // set.
        //
        if (node._attr & SGPropertyNode::VALUE_CHANGED_UP)      attr |= SGPropertyNode::VALUE_CHANGED_UP;
        if (node._attr & SGPropertyNode::VALUE_CHANGED_DOWN)    attr |= SGPropertyNode::VALUE_CHANGED_DOWN;

        if ((attr & SGPropertyNode::VALUE_CHANGED_DOWN) && !(node._attr & SGPropertyNode::VALUE_CHANGED_DOWN)) {
            // We are changing VALUE_CHANGED_DOWN flag from 0 to 1, so set
            // VALUE_CHANGED_UP and VALUE_CHANGED_DOWN flags in all child
            // nodes.
            //
            setChildrenUpDown(exclusive, node);
        }

        node._attr = attr;
    }

    static void setAttribute(SGPropertyLockExclusive& exclusive, SGPropertyNode& node, SGPropertyNode::Attribute attr, bool state)
    {
        int attr_new = (state) ? (node._attr | attr) : (node._attr & ~attr);
        setAttributes(exclusive, node, attr_new);
    }

    static int getAttributes(SGPropertyLock& lock, SGPropertyNode& node)
    {
        return node._attr;
    }

    static void
    clearValue(SGPropertyLockExclusive& exclusive, SGPropertyNode& node)
    {
        if (node._type == props::ALIAS) {
            if (node._value.alias->listener) {
                exclusive.release();
                node._value.alias->target->removeChangeListener(node._value.alias->listener);
                exclusive.acquire();
            }
            delete node._value.alias;
            node._value.alias = nullptr;
        }
        else if (node._type != props::NONE) {
            switch (node._type) {
            case props::BOOL:
                node._local_val.bool_val = SGRawValue<bool>::DefaultValue();
                break;
            case props::INT:
                node._local_val.int_val = SGRawValue<int>::DefaultValue();
                break;
            case props::LONG:
                node._local_val.long_val = SGRawValue<long>::DefaultValue();
                break;
            case props::FLOAT:
                node._local_val.float_val = SGRawValue<float>::DefaultValue();
                break;
            case props::DOUBLE:
                node._local_val.double_val = SGRawValue<double>::DefaultValue();
                break;
            case props::STRING:
            case props::UNSPECIFIED:
                if (!node._tied) {
                    delete [] node._local_val.string_val;
                }
                node._local_val.string_val = 0;
                break;
            default: // avoid compiler warning
                break;
            }
            delete node._value.val;
            node._value.val = 0;
        }
        node._tied = false;
        node._type = props::NONE;
    }

    static std::string
    getStringValue(SGPropertyLock& lock, const SGPropertyNode& node)
    {
        // Shortcut for common case
        if (node._attr == (SGPropertyNode::READ|SGPropertyNode::WRITE) && node._type == props::STRING)
            return std::string(get_string(lock, node));

        if (getAttribute(lock, node, SGPropertyNode::TRACE_READ))
            trace_read(lock, node);
        if (!getAttribute(lock, node, SGPropertyNode::READ))
            return std::string(SGRawValue<const char *>::DefaultValue());
        return make_string(lock, node);
    }

    static bool
    getBoolValue(SGPropertyLock& lock, const SGPropertyNode& node, bool defaultValue=false)
    {
        // Shortcut for common case
        if (node._attr == (SGPropertyNode::READ|SGPropertyNode::WRITE) && node._type == props::BOOL)
            return get_bool(lock, node);

        if (getAttribute(lock, node, SGPropertyNode::TRACE_READ))
            trace_read(lock, node);   // Releases+acquire <lock>.
        if (!getAttribute(lock, node, SGPropertyNode::READ))
            return defaultValue;
        switch (node._type) {
        case props::ALIAS:
            {
            auto p = node._value.alias->target;
            lock.release();
            return p->getBoolValue(defaultValue);
            }
        case props::BOOL:
            return get_bool(lock, node);
        case props::INT:
            return get_int(lock, node) == 0 ? false : true;
        case props::LONG:
            return get_long(lock, node) == 0L ? false : true;
        case props::FLOAT:
            return get_float(lock, node) == 0.0 ? false : true;
        case props::DOUBLE:
            return get_double(lock, node) == 0.0L ? false : true;
        case props::STRING:
        case props::UNSPECIFIED:
            return (strings_equal(get_string(lock, node), "true") || getDoubleValue(lock, node, 0) != 0.0L);
        case props::NONE:
        default:
            return SGRawValue<bool>::DefaultValue();
        }
    }

    static int
    getIntValue(SGPropertyLock& lock, const SGPropertyNode& node, int defaultValue=0)
    {
         // Shortcut for common case
         if (node._attr == (SGPropertyNode::READ|SGPropertyNode::WRITE) && node._type == props::INT)
             return get_int(lock, node);

         if (getAttribute(lock, node, SGPropertyNode::TRACE_READ))
             trace_read(lock, node);
         if (!getAttribute(lock, node, SGPropertyNode::READ))
             return defaultValue;
         switch (node._type) {
         case props::ALIAS:
             {
             auto p = node._value.alias->target;
             lock.release();
             return p->getIntValue(defaultValue);
             }
         case props::BOOL:
             return int(get_bool(lock, node));
         case props::INT:
             return get_int(lock, node);
         case props::LONG:
             return int(get_long(lock, node));
         case props::FLOAT:
             return int(get_float(lock, node));
         case props::DOUBLE:
             return int(get_double(lock, node));
         case props::STRING:
         case props::UNSPECIFIED:
             /* fixme: use strtol*/
             return atoi(get_string(lock, node));
         case props::NONE:
         default:
             return SGRawValue<int>::DefaultValue();
         }
    }

    static long
    getLongValue(SGPropertyLock& lock, const SGPropertyNode& node, long defaultValue=0)
    {
        // Shortcut for common case
        if (node._attr == (SGPropertyNode::READ|SGPropertyNode::WRITE) && node._type == props::LONG)
            return get_long(lock, node);

        if (getAttribute(lock, node, SGPropertyNode::TRACE_READ))
            trace_read(lock, node);
        if (!getAttribute(lock, node, SGPropertyNode::READ))
            return defaultValue;
        switch (node._type) {
        case props::ALIAS:
            {
            auto p = node._value.alias->target;
            lock.release();
            return p->getLongValue();
            }
        case props::BOOL:
            return long(get_bool(lock, node));
        case props::INT:
            return long(get_int(lock, node));
        case props::LONG:
            return get_long(lock, node);
        case props::FLOAT:
            return long(get_float(lock, node));
        case props::DOUBLE:
            return long(get_double(lock, node));
        case props::STRING:
        case props::UNSPECIFIED:
            /* fixme: check for error. */
            return strtol(get_string(lock, node), 0, 0);
        case props::NONE:
        default:
            return SGRawValue<long>::DefaultValue();
        }
    }

    static float
    getFloatValue(SGPropertyLock& lock, const SGPropertyNode& node, float defaultValue=0)
    {
        // Shortcut for common case
        if (node._attr == (SGPropertyNode::READ|SGPropertyNode::WRITE) && node._type == props::FLOAT)
            return get_float(lock, node);

        if (getAttribute(lock, node, SGPropertyNode::TRACE_READ))
            trace_read(lock, node);
        if (!getAttribute(lock, node, SGPropertyNode::READ))
            return defaultValue;
        switch (node._type) {
        case props::ALIAS:
            {
            auto p = node._value.alias->target;
            lock.release();
            return p->getFloatValue();
            }
        case props::BOOL:
            return float(get_bool(lock, node));
        case props::INT:
            return float(get_int(lock, node));
        case props::LONG:
            return float(get_long(lock, node));
        case props::FLOAT:
            return get_float(lock, node);
        case props::DOUBLE:
            return float(get_double(lock, node));
        case props::STRING:
        case props::UNSPECIFIED:
            /* fixme: check for error. */
            return atof(get_string(lock, node));
        case props::NONE:
        default:
            return SGRawValue<float>::DefaultValue();
        }
    }

    static double
    getDoubleValue(SGPropertyLock& lock, const SGPropertyNode& node, double defaultValue=0)
    {
        // Shortcut for common case
        if (node._attr == (SGPropertyNode::READ|SGPropertyNode::WRITE) && node._type == props::DOUBLE)
            return get_double(lock, node);

        if (getAttribute(lock, node, SGPropertyNode::TRACE_READ))
            trace_read(lock, node);
        if (!getAttribute(lock, node, SGPropertyNode::READ))
            return defaultValue;

        switch (node._type) {
            case props::ALIAS:
                {
            auto p = node._value.alias->target;
            lock.release();
            return p->getDoubleValue(defaultValue);
                }
            case props::BOOL:
                return double(get_bool(lock, node));
            case props::INT:
                return double(get_int(lock, node));
            case props::LONG:
                return double(get_long(lock, node));
            case props::FLOAT:
                return double(get_float(lock, node));
            case props::DOUBLE:
                return get_double(lock, node);
            case props::STRING:
            case props::UNSPECIFIED:
            {
                char* end;
                const char* s = get_string(lock, node);
                double ret = strtod(s, &end);
                if (end == s)
                {
                    /* Unable to convert. Would prefer 'if (*end) ...' so
                    we don't return partial string conversion, but this is
                    backwards compatible. */
                    return defaultValue;
                }
                return ret;
            }
            case props::NONE:
            default:
                return SGRawValue<double>::DefaultValue();
        }
    }

    static bool
    setBoolValue(SGPropertyLockExclusive& exclusive, SGPropertyNode& node, bool value=false)
    {
        // Shortcut for common case
        if (node._attr == (SGPropertyNode::READ|SGPropertyNode::WRITE) && node._type == props::BOOL)
            return set_bool(exclusive, node, value);

        bool result = false;
        if (!getAttribute(exclusive, node, SGPropertyNode::WRITE)) return false;
        if (node._type == props::NONE || node._type == props::UNSPECIFIED) {
            clearValue(exclusive, node);
            node._tied = false;
            node._type = props::BOOL;
        }

        switch (node._type) {
            case props::ALIAS:
                {
            auto p = node._value.alias->target;
            exclusive.release();
            result = p->setBoolValue(value);
            exclusive.acquire();
            break;
                }
            case props::BOOL:
                result = set_bool(exclusive, node, value);
                break;
            case props::INT:
                result = set_int(exclusive, node, int(value));
                break;
            case props::LONG:
                result = set_long(exclusive, node, long(value));
                break;
            case props::FLOAT:
                result = set_float(exclusive, node, float(value));
                break;
            case props::DOUBLE:
                result = set_double(exclusive, node, double(value));
                break;
            case props::STRING:
            case props::UNSPECIFIED:
                result = set_string(exclusive, node, value ? "true" : "false");
                break;
            case props::NONE:
            default:
                break;
        }

        if (getAttribute(exclusive, node, SGPropertyNode::TRACE_WRITE)) {
            trace_write(exclusive, node);
        }
        return result;
    }

    static bool
    setIntValue(SGPropertyLockExclusive& exclusive, SGPropertyNode& node, int value)
    {
        // Shortcut for common case
        if (node._attr == (SGPropertyNode::READ|SGPropertyNode::WRITE) && node._type == props::INT)
            return set_int(exclusive, node, value);

        bool result = false;
        if (!getAttribute(exclusive, node, SGPropertyNode::WRITE)) return false;
        if (node._type == props::NONE || node._type == props::UNSPECIFIED) {
            clearValue(exclusive, node);
            node._type = props::INT;
            node._local_val.int_val = 0;
        }

        switch (node._type) {
            case props::ALIAS:
                {
            auto p = node._value.alias->target;
            exclusive.release();
            result = p->setIntValue(value);
            exclusive.acquire();
            break;
                }
            case props::BOOL:
                result = set_bool(exclusive, node, value == 0 ? false : true);
                break;
            case props::INT:
                result = set_int(exclusive, node, value);
                break;
            case props::LONG:
                result = set_long(exclusive, node, long(value));
                break;
            case props::FLOAT:
                result = set_float(exclusive, node, float(value));
                break;
            case props::DOUBLE:
                result = set_double(exclusive, node, double(value));
                break;
            case props::STRING:
            case props::UNSPECIFIED: {
                char buf[128];
                snprintf(buf, 128, "%d", value);
                result = set_string(exclusive, node, buf);
                break;
            }
            case props::NONE:
            default:
                break;
        }

        if (getAttribute(exclusive, node, SGPropertyNode::TRACE_WRITE))
            trace_write(exclusive, node);
        return result;
    }

    static bool
    setLongValue(SGPropertyLockExclusive& exclusive, SGPropertyNode& node, long value)
    {
        // Shortcut for common case
        if (node._attr == (SGPropertyNode::READ|SGPropertyNode::WRITE) && node._type == props::LONG)
            return set_long(exclusive, node, value);

        bool result = false;
        if (!getAttribute(exclusive, node, SGPropertyNode::WRITE)) return false;
        if (node._type == props::NONE || node._type == props::UNSPECIFIED) {
            clearValue(exclusive, node);
            node._type = props::LONG;
            node._local_val.long_val = 0L;
        }

        switch (node._type) {
            case props::ALIAS:
                {
            auto p = node._value.alias->target;
            exclusive.release();
            result = p->setLongValue(value);
            exclusive.acquire();
            break;
                }
            case props::BOOL:
                result = set_bool(exclusive, node, value == 0L ? false : true);
                break;
            case props::INT:
                result = set_int(exclusive, node, int(value));
                break;
            case props::LONG:
                result = set_long(exclusive, node, value);
                break;
            case props::FLOAT:
                result = set_float(exclusive, node, float(value));
                break;
            case props::DOUBLE:
                result = set_double(exclusive, node, double(value));
                break;
            case props::STRING:
            case props::UNSPECIFIED: {
                char buf[128];
                snprintf(buf, 128, "%ld", value);
                result = set_string(exclusive, node, buf);
                break;
            }
            case props::NONE:
            default:
                break;
        }

        if (getAttribute(exclusive, node, SGPropertyNode::TRACE_WRITE))
            trace_write(exclusive, node);
        return result;
    }

    static bool
    setFloatValue(SGPropertyLockExclusive& exclusive, SGPropertyNode& node, float value)
    {
        // Shortcut for common case
        if (node._attr == (SGPropertyNode::READ|SGPropertyNode::WRITE) && node._type == props::FLOAT)
            return set_float(exclusive, node, value);

        bool result = false;
        if (!getAttribute(exclusive, node, SGPropertyNode::WRITE)) return false;
        if (node._type == props::NONE || node._type == props::UNSPECIFIED) {
            clearValue(exclusive, node);
            node._type = props::FLOAT;
            node._local_val.float_val = 0;
        }

        switch (node._type) {
            case props::ALIAS:
                {
            auto p = node._value.alias->target;
            exclusive.release();
            result = p->setFloatValue(value);
            exclusive.acquire();
            break;
                }
            case props::BOOL:
                result = set_bool(exclusive, node, value == 0.0 ? false : true);
                break;
            case props::INT:
                result = set_int(exclusive, node, int(value));
                break;
            case props::LONG:
                result = set_long(exclusive, node, long(value));
                break;
            case props::FLOAT:
                result = set_float(exclusive, node, value);
                break;
            case props::DOUBLE:
                result = set_double(exclusive, node, double(value));
                break;
            case props::STRING:
            case props::UNSPECIFIED: {
                char buf[128];
                snprintf(buf, 128, "%f", value);
                result = set_string(exclusive, node, buf);
                break;
            }
            case props::NONE:
            default:
                break;
        }

        if (getAttribute(exclusive, node, SGPropertyNode::TRACE_WRITE))
            trace_write(exclusive, node);
        return result;
    }

    static bool
    setDoubleValue(SGPropertyLockExclusive& exclusive, SGPropertyNode& node, double value)
    {
        // Shortcut for common case
        if (node._attr == (SGPropertyNode::READ|SGPropertyNode::WRITE) && node._type == props::DOUBLE)
            return set_double(exclusive, node, value);

        bool result = false;
        if (!getAttribute(exclusive, node, SGPropertyNode::WRITE)) return false;
        if (node._type == props::NONE || node._type == props::UNSPECIFIED) {
            clearValue(exclusive, node);
            node._local_val.double_val = value;
            node._type = props::DOUBLE;
        }

        switch (node._type) {
            case props::ALIAS:
                {
            auto p = node._value.alias->target;
            exclusive.release();
            result = p->setDoubleValue(value);
            exclusive.acquire();
            break;
                }
            case props::BOOL:
                result = set_bool(exclusive, node, value == 0.0L ? false : true);
                break;
            case props::INT:
                result = set_int(exclusive, node, int(value));
                break;
            case props::LONG:
                result = set_long(exclusive, node, long(value));
                break;
            case props::FLOAT:
                result = set_float(exclusive, node, float(value));
                break;
            case props::DOUBLE:
                result = set_double(exclusive, node, value);
                break;
            case props::STRING:
            case props::UNSPECIFIED: {
                char buf[128];
                snprintf(buf, 128, "%f", value);
                result = set_string(exclusive, node, buf);
                break;
            }
            case props::NONE:
            default:
                break;
        }

        if (getAttribute(exclusive, node, SGPropertyNode::TRACE_WRITE))
            trace_write(exclusive, node);
        return result;
    }

    static bool
    setStringValue(SGPropertyLockExclusive& exclusive, SGPropertyNode& node, const char * value)
    {
        // Shortcut for common case
        if (node._attr == (SGPropertyNode::READ|SGPropertyNode::WRITE) && node._type == props::STRING)
            return set_string(exclusive, node, value);

        bool result = false;
        if (!getAttribute(exclusive, node, SGPropertyNode::WRITE)) return false;
        if (node._type == props::NONE || node._type == props::UNSPECIFIED) {
            clearValue(exclusive, node);
            node._type = props::STRING;
        }

        switch (node._type) {
            case props::ALIAS:
                {
            auto p = node._value.alias->target;
            exclusive.release();
            result = p->setStringValue(value);
            exclusive.acquire();
            break;
                }
            case props::BOOL:
                result = set_bool(exclusive, node, (strings_equal(value, "true")
		                   || atoi(value)) ? true : false);
                break;
            case props::INT:
                result = set_int(exclusive, node, atoi(value));
                break;
            case props::LONG:
                result = set_long(exclusive, node, strtol(value, 0, 0));
                break;
            case props::FLOAT:
                result = set_float(exclusive, node, atof(value));
                break;
            case props::DOUBLE:
                result = set_double(exclusive, node, strtod(value, 0));
                break;
            case props::STRING:
            case props::UNSPECIFIED:
                result = set_string(exclusive, node, value);
                break;
            case props::EXTENDED:
            {
                stringstream sstr(value);
                static_cast<SGRawExtended*>(node._value.val)->readFrom(sstr);
            }
            break;
            case props::NONE:
            default:
                break;
        }

        if (getAttribute(exclusive, node, SGPropertyNode::TRACE_WRITE))
            trace_write(exclusive, node);
        return result;
    }

    static bool
    setStringValue(SGPropertyLockExclusive& exclusive, SGPropertyNode& node, const std::string& value)
    {
        return setStringValue(exclusive, node, value.c_str());
    }

    static props::Type
    getType(SGPropertyLock& lock, const SGPropertyNode& node)
    {
        if (node._type == props::ALIAS) {
            auto n = node._value.alias->target;
            lock.release();
            props::Type ret = n->getType();
            lock.acquire();
            return ret;
        }
        else if (node._type == props::EXTENDED)
            return node._value.val->getType();
        return node._type;
    }

    static bool hasValue(SGPropertyLock& lock, const SGPropertyNode& node)
    {
        return node._type != simgear::props::NONE;
    }

    static void
    fireValueChanged (SGPropertyLockExclusive& exclusive, SGPropertyNode& self, SGPropertyNode * node)
    {
        forEachListener(
                exclusive,
                &self,
                self._listeners,
                [&](SGPropertyChangeListener* listener) { listener->valueChanged(node);}
                );
        if (s_property_change_parent_listeners || (self._attr & SGPropertyNode::VALUE_CHANGED_UP))
        {
            SGPropertyNode* parent = self._parent;
            if (parent)
            {
                exclusive.release();
                {
                    SGPropertyLockExclusive parent_exclusive(*parent);
                    fireValueChanged(parent_exclusive, *parent, node);
                }
                exclusive.acquire();
            }
        }
    }

    static void
    fireChildAdded (SGPropertyLockExclusive& exclusive, SGPropertyNode& self, SGPropertyNode* parent, SGPropertyNode* child)
    {
        forEachListener(
                exclusive,
                &self,
                self._listeners,
                [&](SGPropertyChangeListener* listener) { listener->childAdded(parent, child);}
                );
        SGPropertyNode* p = self._parent;
        if (p) {
            exclusive.release();
            {
                SGPropertyLockExclusive p_exclusive(*p);
                fireChildAdded(p_exclusive, *p, parent, child);
            }
            exclusive.acquire();
        }
    }

    static void
    fireChildRemoved (SGPropertyLockExclusive& exclusive, SGPropertyNode& self, SGPropertyNode* parent, SGPropertyNode* child)
    {
        forEachListener(
                exclusive,
                &self,
                self._listeners,
                [&](SGPropertyChangeListener* listener) { listener->childRemoved(parent, child); }
              );
        SGPropertyNode* p = self._parent;
        if (p) {
            exclusive.release();
            {
                SGPropertyLockExclusive p_exclusive(*p);
                fireChildRemoved(p_exclusive, *p, parent, child);
            }
            exclusive.acquire();
        }
    }

    static PropertyList
    getChildren(SGPropertyLock& lock, const SGPropertyNode& node, const std::string& name)
    {
        PropertyList children;
        size_t max = node._children.size();

        for (size_t i = 0; i < max; i++)
            if (node._children[i]->getNameString() == name)
                children.push_back(node._children[i]);

        sort(children.begin(), children.end(), CompareIndices());
        return children;
    }
};


template<typename SplitItr>
SGPropertyNode*
find_node_aux(SGPropertyNode * current, SplitItr& itr, bool create, int last_index)
{
  typedef typename SplitItr::value_type Range;
  // Run off the end of the list
  if (current == 0) {
    return 0;
  }

  // Success! This is the one we want.
  if (itr.eof())
    return current;
  Range token = *itr;
  // Empty name at this point is empty, not root.
  if (token.empty())
    return find_node_aux(current, ++itr, create, last_index);
  Range name = parse_name(current, token);
  if (equals(name, "."))
    return find_node_aux(current, ++itr, create, last_index);
  if (equals(name, "..")) {
    SGPropertyNode* parent = current->getParent();
    if (!parent) {
        SG_LOG(SG_GENERAL, SG_ALERT, "attempt to move past root with '..' node " << current->getNameString());
        return nullptr;
    }
    return find_node_aux(parent, ++itr, create, last_index);
  }
  int index = -1;
  if (last_index >= 0) {
    // If we are at the last token and last_index is valid, use
    // last_index as the index value
    bool lastTok = true;
    while (!(++itr).eof()) {
      if (!itr->empty()) {
        lastTok = false;
        break;
      }
    }
    if (lastTok)
      index = last_index;
  } else {
    ++itr;
  }

  if (index < 0) {
    index = 0;
    if (name.end() != token.end()) {
      if (*name.end() == '[') {
        typename Range::iterator i = name.end() + 1, end = token.end();
        for (;i != end; ++i) {
          if (isdigit(*i)) {
            index = (index * 10) + (*i - '0');
          } else {
            break;
          }
        }
        if (i == token.end() || *i != ']')
          throw std::runtime_error("unterminated index (looking for ']')");
      } else {
          throw std::runtime_error(std::string{"illegal characters in token: "} + std::string(name.begin(), name.end()));
      }
    }
  }
  return find_node_aux(
          SGPropertyNodeImpl::getChildImpl(*current, name.begin(), name.end(), index, create),
          itr,
          create,
          last_index
          );
}

// Internal function for parsing property paths. last_index provides
// and index value for the last node name token, if supplied.
#if PROPS_STANDALONE
static SGPropertyNode *
find_node (SGPropertyNode * current,
     const vector<PathComponent> &components,
     int position,
     bool create)
{
  // Run off the end of the list
  if (current == 0) {
    return 0;
  }

  // Success! This is the one we want.
  else if (position >= (int)components.size()) {
    return (current->getAttribute(SGPropertyNode::REMOVED) ? 0 : current);
  }

  // Empty component means root.
  else if (components[position].name == "") {
    return find_node(current->getRootNode(), components, position + 1, create);
  }

  // . means current directory
  else if (components[position].name == ".") {
    return find_node(current, components, position + 1, create);
  }

  // .. means parent directory
  else if (components[position].name == "..") {
    SGPropertyNode * parent = current->getParent();
    if (parent == 0)
      throw std::runtime_error("Attempt to move past root with '..'");
    else
      return find_node(parent, components, position + 1, create);
  }

  // Otherwise, a child name
  else {
    SGPropertyNode * child =
      current->getChild(components[position].name.c_str(),
      components[position].index,
      create);
    return find_node(child, components, position + 1, create);
  }
}
#else
template <typename Range>
SGPropertyNode *find_node(SGPropertyNode *current, const Range &path,
                          bool create, int last_index = -1) {
  using namespace boost;
  auto itr = make_split_iterator(path, first_finder("/", is_equal()));
  if (*path.begin() == '/')
    return find_node_aux(current->getRootNode(), itr, create, last_index);
  else
    return find_node_aux(current, itr, create, last_index);
}
#endif

////////////////////////////////////////////////////////////////////////
// Private methods from SGPropertyNode (may be inlined for speed).
////////////////////////////////////////////////////////////////////////

void
SGPropertyNode::clearValue ()
{
  SGPropertyLockExclusive exclusive(*this);
  SGPropertyNodeImpl::clearValue(exclusive, *this);
}

////////////////////////////////////////////////////////////////////////
// Public methods from SGPropertyNode.
////////////////////////////////////////////////////////////////////////

/**
 * Last used attribute
 * Update as needed when enum Attribute is changed
 */
const int SGPropertyNode::LAST_USED_ATTRIBUTE = VALUE_CHANGED_DOWN;

/**
 * Mutex to protect access to nodeOrigins.
 * This is kept on the heap and never deleted so that it remains usable even
 * after static globals are destructed.
 */
static std::shared_mutex& nodeOriginsLock = *new std::shared_mutex;

typedef std::map<const SGPropertyNode*, SGSourceLocation> NodeOriginMap;
/**
 * Origin source locations of certain nodes for debug output.
 * This is stored separately from SGPropertyNode to save memory since only a
 * small fraction of them use it.
 * It is a pointer that is only set while it contains entries. This is to
 * prevent the map being destroyed before the last SGPropertyNode objects have
 * been.
 */
static NodeOriginMap* nodeOrigins;

/**
 * Default constructor: always creates a root node.
 */
SGPropertyNode::SGPropertyNode ()
  : _index(0),
    _parent(nullptr),
    _type(props::NONE),
    _tied(false),
    _attr(READ|WRITE),
    _listeners(0)
{
  _local_val.string_val = 0;
  _value.val = 0;
  if (0) std::cerr << __FILE__ << ":" << __LINE__ << ":"
        << " SGPropertyNode()"
        << " this=" << this
        << " _name=" << _name
        << " SGReferenced::count(this)=" << SGReferenced::count(this)
        << " SGReferenced::shared(this)=" << SGReferenced::shared(this)
        << "\n";
 if (this == (SGPropertyNode*) 0x7fffffffd6a0) {
    std::cerr << __FILE__ << ":" << __LINE__ << ": ***\n";
 }
}


/**
 * Copy constructor.
 */
SGPropertyNode::SGPropertyNode (const SGPropertyNode &node)
  :
    //SGWeakReferenced(node),
    SGReferenced(node),
    _index(node._index),
    _name(node._name),
    _parent(nullptr),			// don't copy the parent
    _type(node._type),
    _tied(node._tied),
    _attr(node._attr),
    _listeners(0)		// CHECK!!
{
    setLocation(node.getLocation());

    SGPropertyLockShared shared(node);
    SGPropertyLockExclusive exclusive(*this);
    _local_val.string_val = 0;
    _value.val = 0;
    if (0) std::cerr << __FILE__ << ":" << __LINE__ << ":"
          << " SGPropertyNode()"
          << " this=" << this
          << " SGReferenced::count(this)=" << SGReferenced::count(this)
          << " SGReferenced::shared(this)=" << SGReferenced::shared(this)
          << "\n";
    if (_type == props::NONE)
        return;
    if (_type == props::ALIAS) {
        auto aliasTargetNode = node._value.alias->target;
        _value.alias = new AliasData(aliasTargetNode);
        _tied = false;
        if (node._value.alias->listener) {
      auto l = new AliasChangeListener{this};
      _value.alias->listener = l;
      exclusive.release();
      node._value.alias->target->addChangeListener(l);
        }
        return;
    }
    if (_tied || _type == props::EXTENDED) {
        _value.val = node._value.val->clone();
        return;
    }
    switch (_type) {
    case props::BOOL:
        SGPropertyNodeImpl::set_bool(exclusive, *this, SGPropertyNodeImpl::get_bool(shared, node));
        break;
    case props::INT:
        SGPropertyNodeImpl::set_int(exclusive, *this, SGPropertyNodeImpl::get_int(shared, node));
        break;
    case props::LONG:
        SGPropertyNodeImpl::set_long(exclusive, *this, SGPropertyNodeImpl::get_long(shared, node));
        break;
    case props::FLOAT:
        SGPropertyNodeImpl::set_float(exclusive, *this, SGPropertyNodeImpl::get_float(shared, node));
        break;
    case props::DOUBLE:
        SGPropertyNodeImpl::set_double(exclusive, *this, SGPropertyNodeImpl::get_double(shared, node));
        break;
    case props::STRING:
    case props::UNSPECIFIED:
        SGPropertyNodeImpl::set_string(exclusive, *this, SGPropertyNodeImpl::get_string(shared, node));
        break;
    default:
        break;
    }
}


/**
 * Convenience constructor.
 */
template<typename Itr>
SGPropertyNode::SGPropertyNode (Itr begin, Itr end,
				int index,
				SGPropertyNode* parent)
  : _index(index),
    _name(begin, end),
    _parent(parent),
    _type(props::NONE),
    _tied(false),
    _attr(READ|WRITE),
    _listeners(0)
{
  _local_val.string_val = 0;
  _value.val = 0;
  if (!validateName(_name))
      throw std::invalid_argument(std::string{"plain name expected instead of '"} + _name + '\'');
  if (0) std::cerr << __FILE__ << ":" << __LINE__ << ":"
        << " SGPropertyNode()"
        << " this=" << this
        << " _name=" << _name
        << " SGReferenced::count(this)=" << SGReferenced::count(this)
        << " SGReferenced::shared(this)=" << SGReferenced::shared(this)
        << "\n";
}

SGPropertyNode::SGPropertyNode( const std::string& name,
                                int index,
                                SGPropertyNode* parent)
  : _index(index),
    _name(name),
    _parent(parent),
    _type(props::NONE),
    _tied(false),
    _attr(READ|WRITE),
    // REVIEW: Memory Leak - 662 bytes in 32 blocks are indirectly lost
    _listeners(0)
{
  _local_val.string_val = 0;
  _value.val = 0;
  if (!validateName(name))
      throw std::invalid_argument(std::string{"plain name expected instead of '"} + _name + '\'');
}

/**
 * Destructor.
 */
SGPropertyNode::~SGPropertyNode ()
{
  // clean up this from nodeOrigins map
  setLocation(SGSourceLocation());

  for (unsigned i = 0; i < _children.size(); ++i)
    _children[i]->_parent = nullptr;
  clearValue();

  if (_listeners) {
    vector<SGPropertyChangeListener*>::iterator it;
    for (it = _listeners->_items.begin(); it != _listeners->_items.end(); ++it)
      (*it)->unregister_property(this);
    delete _listeners;
  }
}


/**
 * Alias to another node.
 */
bool SGPropertyNode::alias(SGPropertyNode* target, bool withListener)
{
  SGPropertyLockExclusive exclusive(*this);
  // check for correct/common use first
  if (target && (_type != props::ALIAS) && (!_tied))
  {
    /* loop protection: check alias chain; must not contain self */
    for (auto p = target; p; ) {
      if (p == this) return false;
      SGPropertyLockShared target_shared(*p);
      p = (p->_type == props::ALIAS) ? p->_value.alias->target.get() : nullptr;
    }

    SGPropertyNodeImpl::clearValue(exclusive, *this);
    _value.alias = new AliasData{target};
    _type = props::ALIAS;
    if (withListener) {
      auto l = new AliasChangeListener(this);
      _value.alias->listener = l;
      SGPropertyNodeImpl::setAttribute(exclusive, *this, LISTENER_SAFE, true);
      exclusive.release();
      target->addChangeListener(l);
    }
    return true;
  }

  // error handling cases
  if (!target)
  {
    exclusive.release(); // before calling getPath()
    SG_LOG(SG_GENERAL, SG_ALERT,
           "Failed to set alias " << getPath() << ". "
           "The target property does not exist.");
  }
  else
  if (_type == props::ALIAS)
  {
    const auto existingTarget = _value.alias->target;
    if (existingTarget == target)
      return true; // ok, identical alias requested

    exclusive.release(); // before calling getPath()
    SG_LOG(SG_GENERAL, SG_ALERT, "alias(): " << getPath() << " is already pointing to " << existingTarget->getPath() << " so it cannot alias '" << target->getPath() << ". Use unalias() first.");
  }
  else
  if (_tied)
  {
    exclusive.release(); // before calling getPath()
    SG_LOG(SG_GENERAL, SG_ALERT, "alias(): " << getPath() <<
        " is a tied property. It cannot alias " << target->getPath() << ".");
  }

  return false;
}


/**
 * Alias to another node by path.
 */
bool SGPropertyNode::alias(const char* path, bool withListener)
{
  return alias(getNode(path, true), withListener);
}

bool SGPropertyNode::alias(const std::string& path, bool withListener)
{
  return alias(getNode(path, true), withListener);
}

/**
 * Remove an alias.
 */
bool
SGPropertyNode::unalias ()
{
  SGPropertyLockExclusive exclusive(*this);
  if (_type != props::ALIAS)
    return false;
  // we always clean this flag when un-aliasing; since the property
  // cannot be tied anyway
  SGPropertyNodeImpl::setAttribute(exclusive, *this, LISTENER_SAFE, false);
  SGPropertyNodeImpl::clearValue(exclusive, *this);
  return true;
}

bool SGPropertyNode::isAlias () const
{
    SGPropertyLockShared shared(*this);
    return _type == simgear::props::ALIAS;
}

/**
 * Get the target of an alias.
 */
SGPropertyNode *
SGPropertyNode::getAliasTarget ()
{
  SGPropertyLockShared shared(*this);
  return (_type == props::ALIAS ? _value.alias->target.get() : nullptr);
}


const SGPropertyNode *
SGPropertyNode::getAliasTarget () const
{
  SGPropertyLockShared shared(*this);
  return (_type == props::ALIAS ? _value.alias->target.get() : nullptr);
}

/**
 * create a non-const child by name after the last node with the same name.
 */
SGPropertyNode *
SGPropertyNode::addChild(const char * name, int min_index, bool append)
{
  SGPropertyLockExclusive exclusive(*this);
  int pos = append
          ? std::max(find_last_child(exclusive, name, _children) + 1, min_index)
          : first_unused_index(exclusive, name, _children, min_index);

  SGPropertyNode_ptr node;
  // REVIEW: Memory Leak - 152 bytes in 1 blocks are definitely lost
  node = new SGPropertyNode(name, name + strlen(name), pos, this);
  SGPropertyNodeImpl::appendNode(exclusive, *this, node);
  SGPropertyNodeImpl::fireChildAdded(exclusive, *this, this /*parent*/, node);
  return node;
}

SGPropertyNode*
SGPropertyNode::addChild(const std::string& name, int min_index, bool append)
{
    return addChild(name.c_str(), min_index, append);
}

SGPropertyNode_ptr SGPropertyNode::addChild(SGPropertyNode_ptr node, const std::string& name,
        int min_index, bool append)
{
  SGPropertyLockExclusive exclusive(*this);
  int pos = append
          ? std::max(find_last_child(exclusive, name.c_str(), _children) + 1, min_index)
          : first_unused_index(exclusive, name.c_str(), _children, min_index);
  node->_name = name;
  node->_parent = this;
  node->_index = pos;
  SGPropertyNodeImpl::appendNode(exclusive, *this, node);
  SGPropertyNodeImpl::fireChildAdded(exclusive, *this, this /*parent*/, node);
  return node;
}

/**
 * Create multiple children with unused indices
 */
simgear::PropertyList
SGPropertyNode::addChildren( const std::string& name,
                             size_t count,
                             int min_index,
                             bool append )
{
  SGPropertyLockExclusive exclusive(*this);
  simgear::PropertyList nodes;
  std::set<int> used_indices;

  if( !append )
  {
    // First grab all used indices. This saves us of testing every index if it
    // is used for every element to be created
    for( size_t i = 0; i < nodes.size(); i++ )
    {
      const SGPropertyNode* node = nodes[i];

      if( node->getNameString() == name && node->getIndex() >= min_index )
        used_indices.insert(node->getIndex());
    }
  }
  else
  {
    // If we don't want to fill the holes just find last node
    min_index = std::max(find_last_child(exclusive, name.c_str(), _children) + 1, min_index);
  }

  for( int index = min_index;
            index < std::numeric_limits<int>::max() && nodes.size() < count;
          ++index )
  {
    if( used_indices.find(index) == used_indices.end() )
    {
      SGPropertyNode_ptr node;
      node = new SGPropertyNode(name, index, this);
      SGPropertyNodeImpl::appendNode(exclusive, *this, node);
      SGPropertyNodeImpl::fireChildAdded(exclusive, *this, this /*parent*/, node);
      nodes.push_back(node);
    }
  }

  return nodes;
}

/**
 * Get a non-const child by position.
 */
SGPropertyNode *
SGPropertyNode::getChild (int position)
{
  SGPropertyLockShared shared(*this);
  if (position >= 0 && position < nChildren())
    return _children[position];
  else
    return nullptr;
}


/**
 * Get a const child by position.
 */
const SGPropertyNode *
SGPropertyNode::getChild (int position) const
{
  SGPropertyLockShared shared(*this);
  if (position >= 0 && position < nChildren())
    return _children[position];
  else
    return 0;
}

bool SGPropertyNode::hasValue() const
{
    SGPropertyLockShared shared(*this);
    return SGPropertyNodeImpl::hasValue(shared, *this);
}

const std::string& SGPropertyNode::getNameString () const
{
    SGPropertyLockShared shared(*this);
    return _name;
}
int SGPropertyNode::getIndex () const
{
    SGPropertyLockShared shared(*this);
    return _index;
}


SGPropertyNode* SGPropertyNode::getParent()
{
    SGPropertyLockShared shared(*this);
    return _parent/*.lock()*/;
}

const SGPropertyNode* SGPropertyNode::getParent() const
{
    return _parent/*.lock()*/;
}


unsigned int SGPropertyNode::getPosition() const
{
    SGPropertyNode* parent = _parent/*.lock()*/;
    if (!parent) return 0;
    SGPropertyLockShared parent_shared(*parent);
    auto it = std::find(parent->_children.begin(), parent->_children.end(), this);
    assert(it != parent->_children.end());
    return std::distance(parent->_children.begin(), it);
}

int SGPropertyNode::nChildren () const
{
    SGPropertyLockShared shared(*this);
    return (int)_children.size();
}

bool SGPropertyNode::hasChild (const char * name, int index) const
{
    return getChild(name, index) != nullptr;
}

bool SGPropertyNode::hasChild (const std::string& name, int index) const
{
    return getChild(name, index) != nullptr;
}

/**
 * Get a non-const child by name and index, creating if necessary.
 */

SGPropertyNode *
SGPropertyNode::getChild (const char * name, int index, bool create)
{
  if (create)
  {
    SGPropertyLockExclusive exclusive(*this);
    return SGPropertyNodeImpl::getChildImplCreate(exclusive, *this, name, name + strlen(name), index);
  }
  else
  {
    SGPropertyLockShared shared(*this);
    return SGPropertyNodeImpl::getChildImpl(shared, *this, name, name + strlen(name), index);
  }
}

SGPropertyNode *
SGPropertyNode::getChild (const std::string& name, int index, bool create)
{
#if PROPS_STANDALONE
  const char *n = name.c_str();
  int pos = find_child(n, n + strlen(n), index, _children);
  if (pos >= 0) {
    return _children[pos];
  }
  else if (create) {
    // REVIEW: Memory Leak - 12,862 (11,856 direct, 1,006 indirect) bytes in 78 blocks are definitely lost
    SGPropertyNode* node = new SGPropertyNode(name, index, this);
    // REVIEW: Memory Leak - 104,647 (8 direct, 104,639 indirect) bytes in 1 blocks are definitely lost
    SGPropertyNodeImpl::appendNode(*this, node);
    fireChildAdded(node);
    return node;
  }
  else {
    return nullptr;
  }
#else
  if (create)
  {
    SGPropertyLockExclusive exclusive(*this);
    SGPropertyNode* node = SGPropertyNodeImpl::getExistingChild(exclusive, *this, /*name.begin(), name.end()*/ name.c_str(), name.c_str() + name.size(), index);
    if (node) {
      return node;
    } else {
      // REVIEW: Memory Leak - 12,862 (11,856 direct, 1,006 indirect) bytes in 78 blocks are definitely lost
      node = new SGPropertyNode(name, index, this);
      // REVIEW: Memory Leak - 104,647 (8 direct, 104,639 indirect) bytes in 1 blocks are definitely lost
      SGPropertyNodeImpl::appendNode(exclusive, *this, node);
      SGPropertyNodeImpl::fireChildAdded(exclusive, *this, this /*parent*/, node);
      return node;
    }
  }
  else
  {
    SGPropertyLockShared shared(*this);
    SGPropertyNode* node = SGPropertyNodeImpl::getExistingChild(shared, *this, name.c_str(), name.c_str() + name.size(), index);
    return node;
  }
#endif
}

/**
 * Get a const child by name and index.
 */
const SGPropertyNode *
SGPropertyNode::getChild (const char * name, int index) const
{
  SGPropertyLockShared shared(*this);
  int pos = find_child(shared, name, name + strlen(name), index, _children);
  if (pos >= 0)
    return _children[pos];
  else
    return nullptr;
}

const SGPropertyNode * SGPropertyNode::getChild (const std::string& name, int index) const
{
    return getChild(name.c_str(), index);
}

PropertyList
SGPropertyNode::getChildren(const char * name) const
{
    SGPropertyLockShared shared(*this);
    return SGPropertyNodeImpl::getChildren(shared, *this, name);
}

simgear::PropertyList SGPropertyNode::getChildren (const std::string& name) const
{
    return getChildren(name.c_str());
}

//------------------------------------------------------------------------------
bool SGPropertyNode::removeChild(SGPropertyNode* node)
{
  SGPropertyLockExclusive exclusive(*this);
  SGPropertyLockExclusive exclusive_node(*node);

  if (node->_parent != this) {
    assert(0);
    return false;
  }
  auto it = std::find(_children.begin(), _children.end(), node);
  if (it == _children.end()) {
    assert(0);
    return false;
  }
  SGPropertyNodeImpl::setAttribute(exclusive_node, *node, REMOVED, true);
  SGPropertyNodeImpl::clearValue(exclusive_node, *node);
  exclusive_node.release();
  SGPropertyNodeImpl::fireChildRemoved(exclusive, *this, this /*parent*/, node);

  // Need to find again because fireChildRemoved() will have temporarily
  // released our exclusive lock.
  it = std::find(_children.begin(), _children.end(), node);
  _children.erase(it);

  // fixme: should probably set node->_parent to null here. this was not done
  // in previous (non-locking) props code.
  return true;
}

//------------------------------------------------------------------------------
SGPropertyNode_ptr SGPropertyNode::removeChild(int pos)
{
  SGPropertyNode_ptr child;
  {
    SGPropertyLockShared shared(*this);
    if (pos < 0 || pos >= (int)_children.size())
      return SGPropertyNode_ptr();
    child = _children[pos];
  }
  removeChild(child);
  return child;
}

/**
 * Remove a child node
 */
SGPropertyNode_ptr
SGPropertyNode::removeChild(const char * name, int index)
{
  SGPropertyNode_ptr ret;
  int pos;
  {
    SGPropertyLockShared shared(*this);
    pos = find_child(shared, name, name + strlen(name), index, _children);
  }
  if (pos >= 0)
    ret = removeChild(pos);
  return ret;
}

SGPropertyNode_ptr SGPropertyNode::removeChild(const std::string& name, int index)
{
    return removeChild(name.c_str(), index);
}


/**
  * Remove all children with the specified name.
  */
PropertyList
SGPropertyNode::removeChildren(const std::string& name)
{
  PropertyList children;
  {
    SGPropertyLockShared shared(*this);

    for (int pos = static_cast<int>(_children.size() - 1); pos >= 0; pos--) {
      if (_children[pos]->getNameString() == name) {
        children.push_back(_children[pos]);
      }
    }
    sort(children.begin(), children.end(), CompareIndices());
  }

  for (SGPropertyNode_ptr child: children) {
    removeChild(child);
  }
  return children;
}

void
SGPropertyNode::removeAllChildren()
{
  // Inefficient but simple and hopefully safe.
  PropertyList  children;
  {
    SGPropertyLockShared shared(*this);
    children = _children;
  }
  for (SGPropertyNode* child: children) {
    removeChild(child);
  }
}

std::string
SGPropertyNode::getDisplayName (bool simplify) const
{
  std::string display_name;
  {
    SGPropertyLockShared shared(*this);
    display_name = _name;
  }
  if (_index != 0 || !simplify) {
    stringstream sstr;
    sstr << '[' << _index << ']';
    display_name += sstr.str();
  }
  return display_name;
}

std::string
SGPropertyNode::getPath (bool simplify) const
{
  std::vector<SGConstPropertyNode_ptr> nodes;
  const SGPropertyNode* node = this;
  for(;;) {
    SGPropertyNode* parent;
    {
        SGPropertyLockShared shared(*node);
        parent = node->_parent;
    }
    if (!parent) break;
    nodes.push_back(node);
    node = parent;
  }

  std::string result;
  for (auto it = nodes.rbegin(); it != nodes.rend(); ++it) {
    result += '/';
    result += (*it)->getDisplayName(simplify);
  }
  return result;
}

SGSourceLocation SGPropertyNode::getLocation() const
{
    std::shared_lock rlock(nodeOriginsLock);
    if (!nodeOrigins)
        return SGSourceLocation();
    auto it = nodeOrigins->find(this);
    if (it != nodeOrigins->end())
        return (*it).second;
    else
        return SGSourceLocation();
}

void SGPropertyNode::setLocation(const SGSourceLocation& location)
{
    std::unique_lock lock(nodeOriginsLock);
    if (location.isValid()) {
        if (!nodeOrigins)
            nodeOrigins = new NodeOriginMap;
        (*nodeOrigins)[this] = location;
    } else if (nodeOrigins) {
        nodeOrigins->erase(this);
        if (nodeOrigins->empty()) {
            delete nodeOrigins;
            nodeOrigins = nullptr;
        }
    }
}

bool SGPropertyNode::getAttribute (Attribute attr) const
{
    SGPropertyLockShared shared(*this);
    return SGPropertyNodeImpl::getAttribute(shared, *this, attr);
}

void SGPropertyNode::setAttribute (Attribute attr, bool state)
{
    SGPropertyLockExclusive exclusive(*this);
    SGPropertyNodeImpl::setAttribute(exclusive, *this, attr, state);
}

int SGPropertyNode::getAttributes() const
{
    SGPropertyLockShared shared(*this);
    return _attr;
}

void SGPropertyNode::setAttributes(int attr)
{
    SGPropertyLockExclusive exclusive(*this);
    SGPropertyNodeImpl::setAttributes(exclusive, *this, attr);
}

props::Type
SGPropertyNode::getType() const
{
  SGPropertyLockShared shared(*this);
  return SGPropertyNodeImpl::getType(shared, *this);
}


bool
SGPropertyNode::getBoolValue(bool defaultValue) const
{
  SGPropertyLockShared shared(*this);
  return SGPropertyNodeImpl::getBoolValue(shared, *this, defaultValue);
}

int
SGPropertyNode::getIntValue(int defaultValue) const
{
    SGPropertyLockShared shared(*this);
    return SGPropertyNodeImpl::getIntValue(shared, *this, defaultValue);
}

long
SGPropertyNode::getLongValue(long defaultValue) const
{
    SGPropertyLockShared shared(*this);
    return SGPropertyNodeImpl::getLongValue(shared, *this, defaultValue);
}

float
SGPropertyNode::getFloatValue(float defaultValue) const
{
    SGPropertyLockShared shared(*this);
    return SGPropertyNodeImpl::getFloatValue(shared, *this, defaultValue);
}

double
SGPropertyNode::getDoubleValue(double defaultValue) const
{
    SGPropertyLockShared shared(*this);
    return SGPropertyNodeImpl::getDoubleValue(shared, *this, defaultValue);
}


std::string
SGPropertyNode::getStringValue() const
{
    SGPropertyLockShared shared(*this);
    return SGPropertyNodeImpl::getStringValue(shared, *this);
}

bool
SGPropertyNode::setUnspecifiedValue (const std::string &value)
{
  SGPropertyLockExclusive exclusive(*this);
  bool result = false;
  if (!SGPropertyNodeImpl::getAttribute(exclusive, *this, WRITE)) return false;
  if (_type == props::NONE) {
    SGPropertyNodeImpl::clearValue(exclusive, *this);
    _type = props::UNSPECIFIED;
  }
  props::Type type = _type;
  if (type == props::EXTENDED)
      type = _value.val->getType();
  switch (type) {
  case props::ALIAS:
    {
      auto p = _value.alias->target;
      exclusive.release();
      result = p->setUnspecifiedValue(value);
      exclusive.acquire();
      break;
    }
  case props::BOOL:
    result = SGPropertyNodeImpl::set_bool(exclusive, *this, value == "true"
		       || atoi(value.c_str()) ? true : false);
    break;
  case props::INT:
    result = SGPropertyNodeImpl::set_int(exclusive, *this, atoi(value.c_str()));
    break;
  case props::LONG:
    result = SGPropertyNodeImpl::set_long(exclusive, *this, strtol(value.c_str(), 0, 0));
    break;
  case props::FLOAT:
    result = SGPropertyNodeImpl::set_float(exclusive, *this, atof(value.c_str()));
    break;
  case props::DOUBLE:
    result = SGPropertyNodeImpl::set_double(exclusive, *this, strtod(value.c_str(), 0));
    break;
  case props::STRING:
  case props::UNSPECIFIED:
    result = SGPropertyNodeImpl::set_string(exclusive, *this, value.c_str());
    break;
#if !PROPS_STANDALONE
  case props::VEC3D:
      exclusive.release();
      result = static_cast<SGRawValue<SGVec3d>*>(_value.val)->setValue(parseString<SGVec3d>(value));
      exclusive.acquire();
      break;
  case props::VEC4D:
      exclusive.release();
      result = static_cast<SGRawValue<SGVec4d>*>(_value.val)->setValue(parseString<SGVec4d>(value));
      exclusive.acquire();
      break;
#endif
  case props::NONE:
  default:
    break;
  }

  if (SGPropertyNodeImpl::getAttribute(exclusive, *this, TRACE_WRITE))
    SGPropertyNodeImpl::trace_write(exclusive, *this);
  return result;
}

bool SGPropertyNode::setBoolValue(bool value)
{
    SGPropertyLockExclusive exclusive(*this);
    return SGPropertyNodeImpl::setBoolValue(exclusive, *this, value);
}

bool SGPropertyNode::setIntValue(int value)
{
    SGPropertyLockExclusive exclusive(*this);
    return SGPropertyNodeImpl::setIntValue(exclusive, *this, value);
}

bool SGPropertyNode::setLongValue(long value)
{
    SGPropertyLockExclusive exclusive(*this);
    return SGPropertyNodeImpl::setLongValue(exclusive, *this, value);
}

bool SGPropertyNode::setFloatValue(float value)
{
    SGPropertyLockExclusive exclusive(*this);
    return SGPropertyNodeImpl::setFloatValue(exclusive, *this, value);
}

bool SGPropertyNode::setDoubleValue(double value)
{
    SGPropertyLockExclusive exclusive(*this);
    return SGPropertyNodeImpl::setDoubleValue(exclusive, *this, value);
}

bool SGPropertyNode::setStringValue(const char* value)
{
    SGPropertyLockExclusive exclusive(*this);
    return SGPropertyNodeImpl::setStringValue(exclusive, *this, value);
}

bool SGPropertyNode::setStringValue(const std::string& value)
{
    SGPropertyLockExclusive exclusive(*this);
    return SGPropertyNodeImpl::setStringValue(exclusive, *this, value);
}


//------------------------------------------------------------------------------
#if !PROPS_STANDALONE
bool SGPropertyNode::interpolate( const std::string& type,
                                  const SGPropertyNode& target,
                                  double duration,
                                  const std::string& easing )
{

  if( !_interpolation_mgr )
  {
    SG_LOG(SG_GENERAL, SG_WARN, "No property interpolator available");

    // no interpolation possible -> set to target immediately
    setUnspecifiedValue( target.getStringValue() );
    return false;
  }

  return _interpolation_mgr->interpolate(this, type, target, duration, easing);
}

//------------------------------------------------------------------------------
bool SGPropertyNode::interpolate( const std::string& type,
                                  const simgear::PropertyList& values,
                                  const double_list& deltas,
                                  const std::string& easing )
{
  if( !_interpolation_mgr )
  {
    SG_LOG(SG_GENERAL, SG_WARN, "No property interpolator available");

    // no interpolation possible -> set to last value immediately
    if( !values.empty() )
      setUnspecifiedValue(values.back()->getStringValue());
    return false;
  }

  return _interpolation_mgr->interpolate(this, type, values, deltas, easing);
}

//------------------------------------------------------------------------------
void SGPropertyNode::setInterpolationMgr(simgear::PropertyInterpolationMgr* mgr)
{
  _interpolation_mgr = mgr;
}

//------------------------------------------------------------------------------
simgear::PropertyInterpolationMgr* SGPropertyNode::getInterpolationMgr()
{
  return _interpolation_mgr;
}

simgear::PropertyInterpolationMgr* SGPropertyNode::_interpolation_mgr = 0;
#endif

//------------------------------------------------------------------------------
std::ostream& SGPropertyNode::printOn(std::ostream& stream) const
{
    SGPropertyLockShared shared(*this);
    if (!SGPropertyNodeImpl::getAttribute(shared, *this, READ))
        return stream;
    switch (_type) {
    case props::ALIAS:
        {
        auto p = _value.alias->target;
        shared.release();
        return p->printOn(stream);
        }
    case props::BOOL:
        stream << (SGPropertyNodeImpl::get_bool(shared, *this) ? "true" : "false");
        break;
    case props::INT:
        stream << SGPropertyNodeImpl::get_int(shared, *this);
        break;
    case props::LONG:
        stream << SGPropertyNodeImpl::get_long(shared, *this);
        break;
    case props::FLOAT:
        stream << SGPropertyNodeImpl::get_float(shared, *this);
        break;
    case props::DOUBLE:
        stream << SGPropertyNodeImpl::get_double(shared, *this);
        break;
    case props::STRING:
    case props::UNSPECIFIED:
        stream << SGPropertyNodeImpl::get_string(shared, *this);
        break;
    case props::EXTENDED:
        static_cast<SGRawExtended*>(_value.val)->printOn(stream);
        break;
    case props::NONE:
        break;
    default: // avoid compiler warning
        break;
    }
    return stream;
}

bool SGPropertyNode::isTied () const
{
    return _tied;
}

template<>
bool SGPropertyNode::tie(const SGRawValue<const char *> &rawValue, bool useDefault)
{
    SGPropertyLockExclusive exclusive(*this);
    if (_type == props::ALIAS || _tied)
        return false;

    useDefault = useDefault && SGPropertyNodeImpl::hasValue(exclusive, *this);
    std::string old_val;
    if (useDefault)
        old_val = SGPropertyNodeImpl::getStringValue(exclusive, *this);
    SGPropertyNodeImpl::clearValue(exclusive, *this);
    _type = props::STRING;
    _tied = true;
    _value.val = rawValue.clone();

    if (useDefault) {
        int save_attributes = SGPropertyNodeImpl::getAttributes(exclusive, *this);
        SGPropertyNodeImpl::setAttribute(exclusive, *this, WRITE, true );
        SGPropertyNodeImpl::setStringValue(exclusive, *this, old_val.c_str());
        SGPropertyNodeImpl::setAttributes(exclusive, *this, save_attributes);
    }

    return true;
}

bool
SGPropertyNode::untie()
{
  SGPropertyLockExclusive exclusive(*this);
  if (!_tied)
    return false;

  switch (_type) {
  case props::BOOL: {
    bool val = SGPropertyNodeImpl::getBoolValue(exclusive, *this);
    SGPropertyNodeImpl::clearValue(exclusive, *this);
    _type = props::BOOL;
    _local_val.bool_val = val;
    break;
  }
  case props::INT: {
    int val = SGPropertyNodeImpl::getIntValue(exclusive, *this);
    SGPropertyNodeImpl::SGPropertyNodeImpl::clearValue(exclusive, *this);
    _type = props::INT;
    _local_val.int_val = val;
    break;
  }
  case props::LONG: {
    long val = SGPropertyNodeImpl::SGPropertyNodeImpl::getLongValue(exclusive, *this);
    SGPropertyNodeImpl::clearValue(exclusive, *this);
    _type = props::LONG;
    _local_val.long_val = val;
    break;
  }
  case props::FLOAT: {
    float val = SGPropertyNodeImpl::getFloatValue(exclusive, *this);
    SGPropertyNodeImpl::clearValue(exclusive, *this);
    _type = props::FLOAT;
    _local_val.float_val = val;
    break;
  }
  case props::DOUBLE: {
    double val = SGPropertyNodeImpl::getDoubleValue(exclusive, *this);
    SGPropertyNodeImpl::clearValue(exclusive, *this);
    _type = props::DOUBLE;
    _local_val.double_val = val;
    break;
  }
  case props::STRING:
  case props::UNSPECIFIED: {
    std::string val = SGPropertyNodeImpl::getStringValue(exclusive, *this);
    SGPropertyNodeImpl::clearValue(exclusive, *this);
    _type = props::STRING;
    if (_local_val.string_val != nullptr)
        delete[] _local_val.string_val;
    _local_val.string_val = copy_string(val.c_str());
    break;
  }
  case props::EXTENDED: {
    SGRawExtended* val = static_cast<SGRawExtended*>(_value.val);
    _value.val = 0;             // Prevent clearValue() from deleting
    SGPropertyNodeImpl::clearValue(exclusive, *this);
    _type = props::EXTENDED;
    _value.val = val->makeContainer();
    delete val;
    break;
  }
  case props::NONE:
  default:
    break;
  }

  _tied = false;
  return true;
}

SGPropertyNode *
SGPropertyNode::getRootNode ()
{
  SGPropertyLockShared shared(*this);
  SGPropertyNode* parent = _parent/*.lock()*/;
  shared.release();
  return (parent) ? parent->getRootNode() : this;
}

const SGPropertyNode *
SGPropertyNode::getRootNode () const
{
  return const_cast<SGPropertyNode*>(this)->getRootNode();
}

SGPropertyNode *
SGPropertyNode::getNode (const char * relative_path, bool create)
{
#if PROPS_STANDALONE
  vector<PathComponent> components;
  parse_path(relative_path, components);
  return find_node(this, components, 0, create);

#else
  return find_node(
        this,
        boost::make_iterator_range(relative_path, relative_path + strlen(relative_path)),
        create
        );
#endif
}

SGPropertyNode *
SGPropertyNode::getNode (const char * relative_path, int index, bool create)
{
#if PROPS_STANDALONE
  vector<PathComponent> components;
  parse_path(relative_path, components);
  if (components.size() > 0)
    components.back().index = index;
  return find_node(this, components, 0, create);

#else
  return find_node(
        this,
        boost::make_iterator_range(relative_path, relative_path + strlen(relative_path)),
        create,
        index
        );
#endif
}

SGPropertyNode * SGPropertyNode::getNode (const std::string& relative_path, int index, bool create)
{
    return getNode(relative_path.c_str(), index, create);
}


const SGPropertyNode *
SGPropertyNode::getNode (const char * relative_path) const
{
  return const_cast<SGPropertyNode*>(this)->getNode(relative_path, false);
}

const SGPropertyNode * SGPropertyNode::getNode (const std::string& relative_path) const
{
    return getNode(relative_path.c_str());
}

const SGPropertyNode *
SGPropertyNode::getNode (const char * relative_path, int index) const
{
  return const_cast<SGPropertyNode*>(this)->getNode(relative_path, index, false);
}

const SGPropertyNode * SGPropertyNode::getNode (const std::string& relative_path, int index) const
{
    return getNode(relative_path.c_str(), index);
}

////////////////////////////////////////////////////////////////////////
// Convenience methods using relative paths.
////////////////////////////////////////////////////////////////////////


/**
 * Test whether another node has a value attached.
 */
bool
SGPropertyNode::hasValue (const char * relative_path) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node) ? node->hasValue() : false;
}

bool SGPropertyNode::hasValue (const std::string& relative_path) const
{
    return hasValue(relative_path.c_str());
}

/**
 * Get the value type for another node.
 */
props::Type
SGPropertyNode::getType (const char * relative_path) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node) ? node->getType() : props::UNSPECIFIED;
}

simgear::props::Type SGPropertyNode::getType (const std::string& relative_path) const
{
  return getType(relative_path.c_str());
}

/**
 * Get a bool value for another node.
 */
bool
SGPropertyNode::getBoolValue (const char * relative_path, bool defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node) ? node->getBoolValue() : defaultValue;
}

bool SGPropertyNode::getBoolValue (const std::string& relative_path, bool defaultValue) const
{
    return getBoolValue(relative_path.c_str(), defaultValue);
}

/**
 * Get an int value for another node.
 */
int
SGPropertyNode::getIntValue (const char * relative_path, int defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node) ? node->getIntValue() : defaultValue;
}

int SGPropertyNode::getIntValue (const std::string& relative_path, int defaultValue) const
{
    return getIntValue(relative_path.c_str(), defaultValue);
}

/**
 * Get a long value for another node.
 */
long
SGPropertyNode::getLongValue (const char * relative_path,
			      long defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node) ? node->getLongValue() : defaultValue;
}

long SGPropertyNode::getLongValue (const std::string& relative_path, long defaultValue) const
{
    return getLongValue(relative_path.c_str(), defaultValue);
}

/**
 * Get a float value for another node.
 */
float
SGPropertyNode::getFloatValue (const char * relative_path,
			       float defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node) ? node->getFloatValue() : defaultValue;
}

float SGPropertyNode::getFloatValue (const std::string& relative_path, float defaultValue) const
{
    return getFloatValue(relative_path.c_str(), defaultValue);
}

/**
 * Get a double value for another node.
 */
double
SGPropertyNode::getDoubleValue (const char * relative_path,
				double defaultValue) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node) ? node->getDoubleValue() : defaultValue;
}

double SGPropertyNode::getDoubleValue (const std::string& relative_path, double defaultValue) const
{
    return getDoubleValue(relative_path.c_str(), defaultValue);
}

/**
 * Get a string value for another node.
 */
std::string
SGPropertyNode::getStringValue (const char * relative_path,
				const char * defaultValue) const
{
    // TODO Check defaultValue != null
  const SGPropertyNode * node = getNode(relative_path);
  return (node) ? node->getStringValue() : defaultValue;
}

std::string SGPropertyNode::getStringValue (const std::string& relative_path, const char * defaultValue) const
{
    // TODO Check defaultValue != null
    return getStringValue(relative_path.c_str(), defaultValue);
}

/**
 * Set a bool value for another node.
 */
bool
SGPropertyNode::setBoolValue (const char * relative_path, bool value)
{
  return getNode(relative_path, true)->setBoolValue(value);
}

bool SGPropertyNode::setBoolValue (const std::string& relative_path, bool value)
{
    return setBoolValue(relative_path.c_str(), value);
}

/**
 * Set an int value for another node.
 */
bool
SGPropertyNode::setIntValue (const char * relative_path, int value)
{
  return getNode(relative_path, true)->setIntValue(value);
}

bool SGPropertyNode::setIntValue (const std::string& relative_path, int value)
{
    return setIntValue(relative_path.c_str(), value);
}

/**
 * Set a long value for another node.
 */
bool
SGPropertyNode::setLongValue (const char * relative_path, long value)
{
  return getNode(relative_path, true)->setLongValue(value);
}

bool SGPropertyNode::setLongValue (const std::string& relative_path, long value)
{
    return setLongValue(relative_path.c_str(), value);
}

/**
 * Set a float value for another node.
 */
bool
SGPropertyNode::setFloatValue (const char * relative_path, float value)
{
  return getNode(relative_path, true)->setFloatValue(value);
}

bool SGPropertyNode::setFloatValue (const std::string& relative_path, float value)
{
    return setFloatValue(relative_path.c_str(), value);
}

/**
 * Set a double value for another node.
 */
bool
SGPropertyNode::setDoubleValue (const char * relative_path, double value)
{
  return getNode(relative_path, true)->setDoubleValue(value);
}

bool SGPropertyNode::setDoubleValue (const std::string& relative_path, double value)
{
    return setDoubleValue(relative_path.c_str(), value);
}

/**
 * Set a string value for another node.
 */
bool
SGPropertyNode::setStringValue (const char * relative_path, const char * value)
{
  return getNode(relative_path, true)->setStringValue(value);
}

bool SGPropertyNode::setStringValue(const char * relative_path, const std::string& value)
{
    return setStringValue(relative_path, value.c_str());
}

bool SGPropertyNode::setStringValue (const std::string& relative_path, const char * value)
{
    return setStringValue(relative_path.c_str(), value);
}

bool SGPropertyNode::setStringValue (const std::string& relative_path, const std::string& value)
{
    return setStringValue(relative_path.c_str(), value.c_str());
}

/**
 * Set an unknown value for another node.
 */
bool
SGPropertyNode::setUnspecifiedValue (const char * relative_path, const char * value)
{
  return getNode(relative_path, true)->setUnspecifiedValue(value);
}


/**
 * Test whether another node is tied.
 */
bool
SGPropertyNode::isTied (const char * relative_path) const
{
  const SGPropertyNode * node = getNode(relative_path);
  return (node) ? node->isTied() : false;
}

bool SGPropertyNode::isTied (const std::string& relative_path) const
{
    return isTied(relative_path.c_str());
}

/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie(
        const char* relative_path,
		const SGRawValue<bool> &rawValue,
		bool useDefault
        )
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}

bool SGPropertyNode::tie (const std::string& relative_path, const SGRawValue<bool> &rawValue, bool useDefault)
{
    return tie(relative_path.c_str(), rawValue, useDefault);
}

/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie(
        const char * relative_path,
		const SGRawValue<int> &rawValue,
		bool useDefault
        )
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}

bool SGPropertyNode::tie (const std::string& relative_path, const SGRawValue<int> &rawValue, bool useDefault)
{
    return tie(relative_path.c_str(), rawValue, useDefault);
}

/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie(
        const char * relative_path,
		const SGRawValue<long> &rawValue,
		bool useDefault
        )
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}

bool SGPropertyNode::tie (const std::string& relative_path, const SGRawValue<long> &rawValue, bool useDefault)
{
    return tie(relative_path.c_str(), rawValue, useDefault);
}

/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie(
        const char* relative_path,
		const SGRawValue<float> &rawValue,
		bool useDefault
        )
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}

bool SGPropertyNode::tie (const std::string& relative_path, const SGRawValue<float> &rawValue, bool useDefault)
{
    return tie(relative_path.c_str(), rawValue, useDefault);
}

/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie(
        const char* relative_path,
		const SGRawValue<double> &rawValue,
		bool useDefault
        )
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}

bool SGPropertyNode::tie (const std::string& relative_path, const SGRawValue<double> &rawValue, bool useDefault)
{
    return tie(relative_path.c_str(), rawValue, useDefault);
}

/**
 * Tie a node reached by a relative path, creating it if necessary.
 */
bool
SGPropertyNode::tie(
        const char * relative_path,
		const SGRawValue<const char *> &rawValue,
		bool useDefault
        )
{
  return getNode(relative_path, true)->tie(rawValue, useDefault);
}

bool SGPropertyNode::tie (const std::string& relative_path, const SGRawValue<const char*> &rawValue, bool useDefault)
{
    return tie(relative_path.c_str(), rawValue, useDefault);
}

/**
 * Attempt to untie another node reached by a relative path.
 */
bool
SGPropertyNode::untie(const char * relative_path)
{
  SGPropertyNode* node = getNode(relative_path);
  return (node) ? node->untie() : false;
}

bool SGPropertyNode::untie (const std::string& relative_path)
{
    return untie(relative_path.c_str());
}

void
SGPropertyNode::addChangeListener(SGPropertyChangeListener* listener, bool initial)
{
  SGPropertyLockExclusive exclusive(*this);
  if (_listeners == 0)
    // REVIEW: Memory Leak - 32 bytes in 1 blocks are indirectly lost
    _listeners = new SGPropertyNodeListeners;

  /* If there's a nullptr entry (a listener that was unregistered), we
  overwrite it. This ensures that listeners that routinely unregister+register
  themselves don't make _listeners->_items grow unnecessarily. Otherwise simply
  append. */
  auto it = std::find(_listeners->_items.begin(), _listeners->_items.end(),
        (SGPropertyChangeListener*) nullptr
        );
  if (it == _listeners->_items.end()) {
    // REVIEW: Memory Leak - 8 bytes in 1 blocks are indirectly lost
    _listeners->_items.push_back(listener);
  }
  else {
    *it = listener;
  }
  listener->register_property(this);
  if (initial) {
    // REVIEW: Memory Leak - 24,928 bytes in 164 blocks are indirectly lost
    exclusive.release();
    listener->valueChanged(this);
  }
}

void
SGPropertyNode::removeChangeListener (SGPropertyChangeListener * listener)
{
  SGPropertyLockExclusive exclusive(*this);
  if (_listeners == 0)
    return;
  /* We use a std::unique_lock rather than a std::lock_guard because we may
  need to unlock early. */
  vector<SGPropertyChangeListener*>::iterator it =
    find(_listeners->_items.begin(), _listeners->_items.end(), listener);
  if (it != _listeners->_items.end()) {
    assert(_listeners->_num_iterators >= 0);
    if (_listeners->_num_iterators) {
      /* _listeners._items is currently being iterated further up the stack in
      this thread by one or more nested invocations of forEachListener(), so
      we must be careful to not break these iterators. So we must not delete
      _listeners. And we must not erase this entry from the vector because that
      could cause the next listener to be missed out.

      So we simply set this entry to nullptr (nullptr items are skipped by
      forEachListener()). When all invocations of forEachListener() have
      finished iterating and it is safe to modify things again, it will clean
      up any nullptr entries in _listeners->_items. */
      *it = nullptr;
      listener->unregister_property(this);
    }
    else {
      _listeners->_items.erase(it);
      listener->unregister_property(this);
      if (_listeners->_items.empty()) {
        delete _listeners;
        _listeners = nullptr;
      }
    }
  }
}

void
SGPropertyNode::fireValueChanged()
{
  SGPropertyLockExclusive exclusive(*this);
  SGPropertyNodeImpl::fireValueChanged(exclusive, *this, this);
}

void
SGPropertyNode::fireChildAdded (SGPropertyNode * child)
{
  SGPropertyLockExclusive exclusive(*this);
  SGPropertyNodeImpl::fireChildAdded(exclusive, *this, this /*parent*/, child);
}

void
SGPropertyNode::fireCreatedRecursive(bool fire_self)
{
  if( fire_self )
  {
    SGPropertyNode* parent = _parent/*.lock()*/;
    if (parent) {
      parent->fireChildAdded(this);
    }
    SGPropertyLockExclusive exclusive(*this);
    if( _children.empty() && SGPropertyNodeImpl::getType(exclusive, *this) != simgear::props::NONE )
      return SGPropertyNodeImpl::fireValueChanged(exclusive, *this, this);
  }

  simgear::PropertyList children;
  {
    SGPropertyLockShared shared(*this);
    children = _children;
  }
  for(size_t i = 0; i < children.size(); ++i)
    children[i]->fireCreatedRecursive(true);
}

void
SGPropertyNode::fireChildRemoved (SGPropertyNode * child)
{
  SGPropertyLockExclusive exclusive(*this);
  SGPropertyNodeImpl::fireChildRemoved(exclusive, *this, this /*parent*/, child);
}

void
SGPropertyNode::fireChildrenRemovedRecursive()
{
  simgear::PropertyList children;
  {
    SGPropertyLockShared shared(*this);
    children = _children;
  }
  for (SGPropertyNode* child: children)
  {
    fireChildRemoved(child);
    child->fireChildrenRemovedRecursive();
  }
}

int SGPropertyNode::nListeners() const
{
  SGPropertyLockShared shared(*this);
  if (!_listeners) return 0;
  int   n = 0;
  for (auto listener: _listeners->_items) {
    if (listener)   n += 1;
  }
  return n;
}

#if 0
//------------------------------------------------------------------------------
SGPropertyNode_ptr
SGPropertyNode::eraseChild(simgear::PropertyList::iterator child)
{
  ProtectScope lock(this, true /*write*/);
  SGPropertyNode_ptr node = *child;
  node->setAttribute(REMOVED, true);
  node->clearValue();
  fireChildRemoved(node);

  _children.erase(child);
  return node;
}
#endif

////////////////////////////////////////////////////////////////////////
// Implementation of SGPropertyChangeListener.
////////////////////////////////////////////////////////////////////////

SGPropertyChangeListener::SGPropertyChangeListener(bool recursive)
{
    SG_UNUSED(recursive); // for the moment, all listeners are recursive
}

SGPropertyChangeListener::~SGPropertyChangeListener ()
{
  for (int i = static_cast<int>(_properties.size() - 1); i >= 0; i--)
    _properties[i]->removeChangeListener(this);
}

void
SGPropertyChangeListener::valueChanged (SGPropertyNode * node)
{
  // NO-OP
}

void
SGPropertyChangeListener::childAdded (SGPropertyNode * node,
				      SGPropertyNode * child)
{
  // NO-OP
}

void
SGPropertyChangeListener::childRemoved (SGPropertyNode * parent,
					SGPropertyNode * child)
{
  // NO-OP
}

void
SGPropertyChangeListener::register_property (SGPropertyNode * node)
{
  // REVIEW: Memory Leak - 1,544 bytes in 193 blocks are indirectly lost
  _properties.push_back(node);
}

void
SGPropertyChangeListener::unregister_property (SGPropertyNode * node)
{
  vector<SGPropertyNode *>::iterator it =
    find(_properties.begin(), _properties.end(), node);
  if (it != _properties.end())
    _properties.erase(it);
}

////////////////////////////////////////////////////////////////////////
// Implementation of AliasChangeListener.
////////////////////////////////////////////////////////////////////////

void AliasChangeListener::valueChanged(SGPropertyNode* node)
{
  SG_UNUSED(node);
  _destination->fireValueChanged();
}


#if !PROPS_STANDALONE
template<>
std::ostream& SGRawBase<SGVec3d>::printOn(std::ostream& stream) const
{
    const SGVec3d vec
        = static_cast<const SGRawValue<SGVec3d>*>(this)->getValue();
    for (int i = 0; i < 3; ++i) {
        stream << vec[i];
        if (i < 2)
            stream << ' ';
    }
    return stream;
}

namespace simgear
{
template<>
std::istream& readFrom<SGVec3d>(std::istream& stream, SGVec3d& result)
{
    for (int i = 0; i < 3; ++i) {
        stream >> result[i];
    }
    return stream;
}
}
template<>
std::ostream& SGRawBase<SGVec4d>::printOn(std::ostream& stream) const
{
    const SGVec4d vec
        = static_cast<const SGRawValue<SGVec4d>*>(this)->getValue();
    for (int i = 0; i < 4; ++i) {
        stream << vec[i];
        if (i < 3)
            stream << ' ';
    }
    return stream;
}
#endif

namespace simgear
{
#if !PROPS_STANDALONE
    template<>
    std::istream& readFrom<SGVec4d>(std::istream& stream, SGVec4d& result)
    {
        for (int i = 0; i < 4; ++i) {
            stream >> result[i];
        }
        return stream;
    }
#endif

    namespace
    {
        #if 0
        bool compareNodeValue(const SGPropertyNode& lhs, const SGPropertyNode& rhs)
        {
            if (&lhs == &rhs) return true;
            /* Need to find whether this is above or below <to> in the property tree so
            we can take out locks in a safe order. */
            SGPropertyLockShared    shared_lhs;
            SGPropertyLockShared    shared_rhs;
            int d = ancestor_compare(&lhs, &rhs);
            if (d >= 0) {
                // <lhs> is parent/grandparent/... of <rhs>.
                shared_lhs.assign(lhs);
                shared_rhs.assign(rhs);
            }
            else {
                shared_rhs.assign(rhs);
                shared_lhs.assign(lhs);
            }
            return compareNodeValue(shared_lhs, shared_rhs, lhs, rhs);
        }
        #endif

        bool compareNodeValue(
                SGPropertyLockShared& shared_lhs,
                SGPropertyLockShared& shared_rhs,
                const SGPropertyNode& lhs,
                const SGPropertyNode& rhs
                )
        {
            props::Type ltype = lhs.getType();
            props::Type rtype = rhs.getType();
            if (ltype != rtype)
                return false;
            switch (ltype) {
            case props::NONE:
                return true;
            case props::ALIAS:
                return false;           // XXX Should we look in aliases?
            case props::BOOL:
                return lhs.getValue<bool>() == rhs.getValue<bool>();
            case props::INT:
                return lhs.getValue<int>() == rhs.getValue<int>();
            case props::LONG:
                return lhs.getValue<long>() == rhs.getValue<long>();
            case props::FLOAT:
                return lhs.getValue<float>() == rhs.getValue<float>();
            case props::DOUBLE:
                return lhs.getValue<double>() == rhs.getValue<double>();
            case props::STRING:
            case props::UNSPECIFIED:
                return lhs.getStringValue() == rhs.getStringValue();
#if !PROPS_STANDALONE
            case props::VEC3D:
                return lhs.getValue<SGVec3d>() == rhs.getValue<SGVec3d>();
            case props::VEC4D:
                return lhs.getValue<SGVec4d>() == rhs.getValue<SGVec4d>();
#endif
            default:
                return false;
            }
        }
    }
}

/* Returns +1 if a is parent of b, +2 if a is grandparent of b, etc. Otherwise
returns 0. We also return 0 if a==b. */
static int ancestor_distance(SGConstPropertyNode_ptr a, SGConstPropertyNode_ptr b)
{
    int distance = 0;
    for(;;) {
        b = b->getParent();
        if (!b) return 0;
        distance += 1;
        if (a == b) return distance;
    }
}

/* Returns +1/+2/+3/... if a is parent/grandparent/... of b. Returns
-1/-2/-3/... if b is parent/grandparent/... of a. */
static int ancestor_compare(SGConstPropertyNode_ptr a, SGConstPropertyNode_ptr b)
{
    int distance;
    distance = ancestor_distance(a, b);
    if (distance > 0) return distance;
    distance = ancestor_distance(b, a);
    if (distance > 0) return -distance;
    return 0;
}

void SGPropertyNode::copy(SGPropertyNode *to) const
{
    if (nChildren())
    {
        for (int i = 0; i < nChildren(); i++) {
            const SGPropertyNode *child = getChild(i);
            if (!child) break;  // We don't lock child, so getChild() could return 0;
            SGPropertyNode* to_child = to->getChild(child->getNameString());
            if (!to_child)
                to_child = to->addChild(child->getNameString());
            if (child->nChildren())
            {
                child->copy(to_child);
            }
            else
            {
                to_child->setValue(child->getStringValue().c_str());
            }
        }
    }
    else
        to->setValue(getStringValue().c_str());
}

SGPropertyNode * SGPropertyNode::getNode (const std::string& relative_path, bool create)
{
    return getNode(relative_path.c_str(), create);
}


bool SGPropertyNode::compare(const SGPropertyNode& lhs,
                             const SGPropertyNode& rhs)
{
    if (&lhs == &rhs)
        return true;
    /* Need to find whether this is above or below <to> in the property tree so
    we can take out locks in a safe order. */
    SGPropertyLockShared    shared_lhs;
    SGPropertyLockShared    shared_rhs;
    int d = ancestor_compare(&lhs, &rhs);
    if (d >= 0) {
        // <lhs> is parent/grandparent/... of <rhs>.
        shared_lhs.assign(lhs);
        shared_rhs.assign(rhs);
    }
    else {
        shared_rhs.assign(rhs);
        shared_lhs.assign(lhs);
    }
    int lhsChildren = lhs._children.size();
    int rhsChildren = rhs._children.size();
    if (lhsChildren != rhsChildren)
        return false;

    if (lhsChildren == 0)
        return compareNodeValue(shared_lhs, shared_rhs, lhs, rhs);

    for (size_t i = 0; i < lhs._children.size(); ++i) {
        const SGPropertyNode* lchild = lhs._children[i];
        const SGPropertyNode* rchild = rhs._children[i];
        // I'm guessing that the nodes will usually be in the same
        // order.
        if (lchild->getIndex() != rchild->getIndex()
                || lchild->getNameString() != rchild->getNameString()
                )
        {
            /* Search for matching child in rhs. */
            rchild = nullptr;
            for (PropertyList::const_iterator itr = rhs._children.begin();
                    itr != rhs._children.end();
                    ++itr)
            {
                if (lchild->getIndex() == (*itr)->getIndex()
                        && lchild->getNameString() == (*itr)->getNameString()
                        )
                {
                    rchild = *itr;
                    break;
                }
            }
            if (!rchild)
                return false;
        }
        if (!compare(*lchild, *rchild))
            return false;
    }
    return true;
}

struct PropertyPlaceLess {
    typedef bool result_type;
    bool operator()(SGPropertyNode_ptr lhs, SGPropertyNode_ptr rhs) const
    {
        int comp = lhs->getNameString().compare(rhs->getNameString());
        if (comp == 0)
            return lhs->getIndex() < rhs->getIndex();
        else
            return comp < 0;
    }
};


template<typename T>
bool SGPropertyNode::tie(const SGRawValue<T> &rawValue, bool useDefault)
{
    SGPropertyLockExclusive exclusive(*this);
    using namespace simgear::props;
    if (_type == ALIAS || _tied)
        return false;

    useDefault = useDefault && SGPropertyNodeImpl::hasValue(exclusive, *this);
    T old_val = SGRawValue<T>::DefaultValue();
    if (useDefault)
        old_val = getValue<T>(exclusive);
    SGPropertyNodeImpl::clearValue(exclusive, *this);
    if (PropertyTraits<T>::Internal)
        _type = PropertyTraits<T>::type_tag;
    else
        _type = EXTENDED;
    _tied = true;
    _value.val = rawValue.clone();
    if (useDefault) {
        int save_attributes = _attr;
        SGPropertyNodeImpl::setAttribute(exclusive, *this, WRITE, true);
        setValue(exclusive, old_val);
        _attr = save_attributes;
    }
    return true;

}


template<typename T>
T SGPropertyNode::getValue(
        SGPropertyLock& lock,
        typename std::enable_if<!simgear::props::PropertyTraits<T>::Internal>::type* dummy
        ) const
{
    using namespace simgear::props;
    if (_attr == (READ|WRITE)
            && _type == EXTENDED
            && _value.val->getType() == PropertyTraits<T>::type_tag)
    {
        return static_cast<SGRawValue<T>*>(_value.val)->getValue();
    }
    if (SGPropertyNodeImpl::getAttribute(lock, *this, TRACE_READ))
        SGPropertyNodeImpl::trace_read(lock, *this);
    if (!SGPropertyNodeImpl::getAttribute(lock, *this, READ))
        return SGRawValue<T>::DefaultValue();
    switch (_type) {
        case EXTENDED:
            if (_value.val->getType() == PropertyTraits<T>::type_tag)
                return static_cast<SGRawValue<T>*>(_value.val)->getValue();
            break;
        case STRING:
        case UNSPECIFIED:
            return simgear::parseString<T>(SGPropertyNodeImpl::make_string(lock, *this));
        default: // avoid compiler warning
            break;
    }
    return SGRawValue<T>::DefaultValue();
}

template<typename T>
T SGPropertyNode::getValue(
        typename std::enable_if<!simgear::props::PropertyTraits<T>::Internal>::type* dummy
        ) const
{
    SGPropertyLockShared shared(*this);
    return getValue<T>(shared, dummy);
}


template<typename T>
inline T
SGPropertyNode::getValue(typename std::enable_if<simgear::props::PropertyTraits<T>::Internal>::type* dummy) const
{
    return ::getValue<T>(this);
}

template<typename T, typename T_get /* = T */> // TODO use C++11 or traits
std::vector<T> SGPropertyNode::getChildValues(const std::string& name) const
{
    SGPropertyLockShared shared(*this);
    const simgear::PropertyList& props = SGPropertyNodeImpl::getChildren(shared, *this, name);
    std::vector<T> values( props.size() );

    for( size_t i = 0; i < props.size(); ++i )
        values[i] = props[i]->getValue<T_get>();

    return values;
}

template<typename T>
inline std::vector<T>
SGPropertyNode::getChildValues(const std::string& name) const
{
    return getChildValues<T, T>(name);
}


template<>
bool SGPropertyNode::setValue(SGPropertyLockExclusive& exclusive, const bool& val, void* dummy)
{
    return SGPropertyNodeImpl::setBoolValue(exclusive, *this, val);
}

template<>
bool SGPropertyNode::setValue(SGPropertyLockExclusive& exclusive, const int& val, void* dummy)
{
    return SGPropertyNodeImpl::setIntValue(exclusive, *this, val);
}

template<>
bool SGPropertyNode::setValue(SGPropertyLockExclusive& exclusive, const long& val, void* dummy)
{
    return SGPropertyNodeImpl::setLongValue(exclusive, *this, val);
}

template<>
bool SGPropertyNode::setValue(SGPropertyLockExclusive& exclusive, const float& val, void* dummy)
{
    return SGPropertyNodeImpl::setFloatValue(exclusive, *this, val);
}

template<>
bool SGPropertyNode::setValue(SGPropertyLockExclusive& exclusive, const double& val, void* dummy)
{
    return SGPropertyNodeImpl::setDoubleValue(exclusive, *this, val);
}


template<typename T>
bool SGPropertyNode::setValue(
        SGPropertyLockExclusive& exclusive,
        const T& val,
        typename std::enable_if<!simgear::props::PropertyTraits<T>::Internal>::type* dummy
        )
{
    using namespace simgear::props;
    if (_attr == (READ|WRITE) && _type == EXTENDED
        && _value.val->getType() == PropertyTraits<T>::type_tag) {
        static_cast<SGRawValue<T>*>(_value.val)->setValue(val);
        return true;
    }
    if (SGPropertyNodeImpl::getAttribute(exclusive, *this, WRITE)
        && (
            (
                _type == EXTENDED
                && _value.val->getType() == PropertyTraits<T>::type_tag
            )
            || _type == NONE
            || _type == UNSPECIFIED
        ))
    {
        if (_type == NONE || _type == UNSPECIFIED) {
            SGPropertyNodeImpl::clearValue(exclusive, *this);
            _type = EXTENDED;
            _value.val = new SGRawValueContainer<T>(val);
        } else {
            static_cast<SGRawValue<T>*>(_value.val)->setValue(val);
        }
        if (SGPropertyNodeImpl::getAttribute(exclusive, *this, TRACE_WRITE))
            SGPropertyNodeImpl::trace_write(exclusive, *this);
        return true;
    }
    return false;
}

template<typename T>
bool SGPropertyNode::setValue(
        const T& val,
        typename std::enable_if<!simgear::props::PropertyTraits<T>::Internal>::type* dummy
        )
{
    SGPropertyLockExclusive exclusive(*this);
    return setValue(exclusive, val, dummy);
}

template<typename T>
inline bool SGPropertyNode::setValue(
        const T& val,
        typename std::enable_if<simgear::props::PropertyTraits<T>::Internal>::type* dummy
        )
{
    return ::setValue(this, val);
}


// Explicit specializations and instantiations.
//

template bool SGPropertyNode::getValue<bool>(void*) const;
template int SGPropertyNode::getValue<int>(void*) const;
template long SGPropertyNode::getValue<long>(void*) const;
template float SGPropertyNode::getValue<float>(void*) const;
template double SGPropertyNode::getValue<double>(void*) const;
template std::string SGPropertyNode::getValue<std::string>(void*) const;
template SGVec3<double> SGPropertyNode::getValue<SGVec3<double>>(void*) const;
template SGVec4<double> SGPropertyNode::getValue<SGVec4<double>>(void*) const;

template<>
bool SGPropertyNode::getValue<bool>(SGPropertyLock& lock, void* dummy) const
{
    return SGPropertyNodeImpl::getBoolValue(lock, *this);
}

template<>
int SGPropertyNode::getValue<int>(SGPropertyLock& lock, void* dummy) const
{
    return SGPropertyNodeImpl::getIntValue(lock, *this);
}

template<>
long SGPropertyNode::getValue<long>(SGPropertyLock& lock, void* dummy) const
{
    return SGPropertyNodeImpl::getLongValue(lock, *this);
}

template<>
float SGPropertyNode::getValue<float>(SGPropertyLock& lock, void* dummy) const
{
    return SGPropertyNodeImpl::getFloatValue(lock, *this);
}

template<>
double SGPropertyNode::getValue<double>(SGPropertyLock& lock, void* dummy) const
{
    return SGPropertyNodeImpl::getDoubleValue(lock, *this);
}

template<>
std::string SGPropertyNode::getValue<std::string>(SGPropertyLock& lock, void* dummy) const
{
    return SGPropertyNodeImpl::getStringValue(lock, *this);
}

template bool SGPropertyNode::setValue(const bool&, void*);
template bool SGPropertyNode::setValue(const int&, void*);
template bool SGPropertyNode::setValue(const long&, void*);
template bool SGPropertyNode::setValue(const float&, void*);
template bool SGPropertyNode::setValue(const double&, void*);
template bool SGPropertyNode::setValue(const char* const&, void*);
template bool SGPropertyNode::setValue(const SGVec3<double>&, void*);
template bool SGPropertyNode::setValue(const SGVec4<double>&, void*);

template std::vector<float> SGPropertyNode::getChildValues(const std::string& name) const;
template std::vector<float> SGPropertyNode::getChildValues<float, float>(const std::string& name) const;

template std::vector<unsigned char> SGPropertyNode::getChildValues<unsigned char, int>(const std::string& name) const;

template bool SGPropertyNode::tie(const SGRawValue<bool> &rawValue, bool useDefault);
template bool SGPropertyNode::tie(const SGRawValue<int> &rawValue, bool useDefault);
template bool SGPropertyNode::tie(const SGRawValue<long> &rawValue, bool useDefault);
template bool SGPropertyNode::tie(const SGRawValue<float> &rawValue, bool useDefault);
template bool SGPropertyNode::tie(const SGRawValue<double> &rawValue, bool useDefault);


#if !PROPS_STANDALONE
size_t hash_value(const SGPropertyNode& node)
{
    using namespace boost;
    SGPropertyLockShared shared(node);

    if (node._children.empty()) {
        switch (SGPropertyNodeImpl::getType(shared, node)) {
            case props::NONE:
                return 0;
            case props::BOOL:
                return boost::hash_value(SGPropertyNodeImpl::getBoolValue(shared, node));
            case props::INT:
                return boost::hash_value(SGPropertyNodeImpl::getIntValue(shared, node));
            case props::LONG:
                return boost::hash_value(SGPropertyNodeImpl::getLongValue(shared, node));
            case props::FLOAT:
                return boost::hash_value(SGPropertyNodeImpl::getFloatValue(shared, node));
            case props::DOUBLE:
                return boost::hash_value(SGPropertyNodeImpl::getDoubleValue(shared, node));
            case props::STRING:
            case props::UNSPECIFIED:
            {
                std::string val = SGPropertyNodeImpl::getStringValue(shared, node);
                return boost::hash_range(val.begin(), val.end());
            }
            case props::VEC3D:
            {
                const SGVec3d val = node.getValue<SGVec3d>(shared);
                return boost::hash_range(&val[0], &val[3]);
            }
            case props::VEC4D:
            {
                const SGVec4d val = node.getValue<SGVec4d>(shared);
                return hash_range(&val[0], &val[4]);
            }
            case props::ALIAS:      // XXX Should we look in aliases?
            default:
                return 0;
        }
    } else {
        size_t seed = 0;
        PropertyList children = node._children; //(node._children.begin(), node._children.end());
        sort(children.begin(), children.end(), PropertyPlaceLess());
        for (PropertyList::const_iterator itr = children.begin(),
                 end = children.end();
             itr != end;
             ++itr) {
            hash_combine(seed, (*itr)->_name);
            hash_combine(seed, (*itr)->_index);
            hash_combine(seed, hash_value(**itr));
        }
        return seed;
    }
}

#endif

// end of props.cxx

#endif
