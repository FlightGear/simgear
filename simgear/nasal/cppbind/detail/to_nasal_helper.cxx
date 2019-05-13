// Conversion functions to convert C++ types to Nasal types
//
// Copyright (C) 2012  Thomas Geymayer <tomgey@gmail.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#include "to_nasal_helper.hxx"
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>

#include <simgear/math/SGMath.hxx>
#include <simgear/misc/sg_path.hxx>

#include <boost/function.hpp>

#include <simgear/threads/SGThread.hxx>
#include <mutex>
#include <condition_variable>
#include <atomic>


namespace nasal
{
  //----------------------------------------------------------------------------
  naRef to_nasal_helper(naContext c, const std::string& str)
  {
    naRef ret = naNewString(c);
    naStr_fromdata(ret, str.c_str(), static_cast<int>(str.size()));
    return ret;
  }

  //----------------------------------------------------------------------------
  naRef to_nasal_helper(naContext c, const char* str)
  {
    return to_nasal_helper(c, std::string(str));
  }

  //----------------------------------------------------------------------------
  naRef to_nasal_helper(naContext c, const Hash& hash)
  {
    return hash.get_naRef();
  }

  //----------------------------------------------------------------------------
  naRef to_nasal_helper(naContext c, const naRef& ref)
  {
    return ref;
  }

  //------------------------------------------------------------------------------
  naRef to_nasal_helper(naContext c, const SGGeod& pos)
  {
    nasal::Hash hash(c);
    hash.set("lat", pos.getLatitudeDeg());
    hash.set("lon", pos.getLongitudeDeg());
    hash.set("elevation", pos.getElevationM());
    return hash.get_naRef();
  }

  //----------------------------------------------------------------------------
  naRef to_nasal_helper(naContext c, const SGPath& path)
  {
    return to_nasal_helper(c, path.utf8Str());
  }

  //----------------------------------------------------------------------------
  naRef to_nasal_helper(naContext c, naCFunction func)
  {
    return naNewFunc(c, naNewCCode(c, func));
  }

  //----------------------------------------------------------------------------
  static naRef free_function_invoker( naContext c,
                                      naRef me,
                                      int argc,
                                      naRef* args,
                                      void* user_data )
  {
    free_function_t* func = static_cast<free_function_t*>(user_data);
    assert(func);

    try
    {
      return (*func)(nasal::CallContext(c, me, argc, args));
    }
    catch(const std::exception& ex)
    {
      naRuntimeError(c, "Fatal error in Nasal call: %s", ex.what());
    }
    catch(...)
    {
      naRuntimeError(c, "Unknown exception in Nasal call.");
    }

    return naNil();
  }

  //----------------------------------------------------------------------------
  static void free_function_destroy(void* func)
  {
    delete static_cast<free_function_t*>(func);
  }

  //----------------------------------------------------------------------------
  naRef to_nasal_helper(naContext c, const free_function_t& func)
  {
    return naNewFunc
    (
      c,
      naNewCCodeUD
      (
        c,
        &free_function_invoker,
        new free_function_t(func),
        &free_function_destroy
      )
    );
  }

  template <class T>   class FastStack
  {
  public:
      T* st;
      int allocationSize;
      int lastIndex;
      //std::mutex mutex_;

  public:
      FastStack(int stackSize);
      ~FastStack();

      inline void resize(int newSize);
      inline void push(T x);
      inline void pop();
      inline void clear();
      inline void iterate(int(*process)(naRef v));
      inline size_t size() {
          return lastIndex + 1;
      }
      T top()
      {
          //std::unique_lock<std::mutex> lck(mutex_);
          return st[lastIndex];
      }
      void push_if_not_present(naRef r);
  };

  template <class T>
  FastStack<T>::FastStack(int stackSize)
  {
      st = NULL;
      this->allocationSize = stackSize;
      st = (T*)malloc(stackSize * sizeof(naRef));
      lastIndex = -1;
  }
  template <class T>
  FastStack<T>::~FastStack()
  {
      delete[] st;
  }

  template <class T>
  void FastStack<T>::clear()
  {
      lastIndex = -1;
  }

  template <class T>
  void FastStack<T>::push_if_not_present(naRef r) {
      /*for (int i = 0; i <= lastIndex; i++)
          if (st[i] == r)
              return;*/
      push(r);
  }
  template <class T>
  void FastStack<T>::iterate(int(*process)(naRef v))
  {
      for (int i = 0; i <= lastIndex; i++)
          if (process(st[i]))
              break;
  }

  template <class T>
  void FastStack<T>::pop()
  {
      --lastIndex;
  }

  template <class T>
  void FastStack<T>::push(T x)
  {
      if (++lastIndex >= allocationSize)
          resize(allocationSize * 2);
      st[lastIndex] = x;
  }

  template <class T>
  void FastStack<T>::resize(int newSize)
  {
      //std::unique_lock<std::mutex> lck(mutex_);
      T* new_st = (T*)realloc(st, newSize * sizeof(naRef));
      if (new_st)
      {
          st = new_st;
          allocationSize = newSize;
          SG_LOG(SG_NASAL, SG_WARN, "Increased tc stack to " << allocationSize);
      }
      else
          throw "Failed to grow tc stack";
  } 
  FastStack < naRef> t_stack(40);
  extern"C" {


      int __stack_hwm = 0;
      void na_t_stack_push(naRef v) {
          t_stack.push(v);

          if (t_stack.size() > __stack_hwm)
              __stack_hwm = t_stack.size();
      }
      extern int na_t_stack_count() {
          return t_stack.size();
      }
      extern naRef na_t_stack_pop()
      {
          naRef v = t_stack.top();
          t_stack.pop();
          return v;
      }
  }
} // namespace nasal
