//------------------------------------------------------------------------------
// File : SkySingleton.hpp
//------------------------------------------------------------------------------
// SkyWorks : Copyright 2002 Mark J. Harris and
//						The University of North Carolina at Chapel Hill
//------------------------------------------------------------------------------
// Permission to use, copy, modify, distribute and sell this software and its 
// documentation for any purpose is hereby granted without fee, provided that 
// the above copyright notice appear in all copies and that both that copyright 
// notice and this permission notice appear in supporting documentation. 
// Binaries may be compiled with this software without any royalties or 
// restrictions. 
//
// The author(s) and The University of North Carolina at Chapel Hill make no 
// representations about the suitability of this software for any purpose. 
// It is provided "as is" without express or 
// implied warranty.
/**
 * @file SkySingleton.hpp
 * 
 * A generic singleton template wrapper to make classes into singletons
 */
#ifndef __SKYSINGLETON_HPP__
#define __SKYSINGLETON_HPP__

#include "SkyUtil.hpp"
#include <assert.h>
#include <stdlib.h>

//------------------------------------------------------------------------------
/**
 * @class SkySingleton
 * @brief A singleton template class.
 * 
 * Usage : Use this template container class to make any class into a
 *         singleton.  I usually do this:
 *
 * @code   
 * class MyClass       
 * {       
 * public:       
 *   // normal class stuff, but don't put ctor/dtor here.       
 *   int GetData() { return _someData; }       
 * protected:       
 *   // Make the ctor(s)/dtor protected, so this can only be       
 *   // instantiated as a singleton.  Note: singleton will still       
 *   // work on classes that do not follow this (public ctors)       
 *   // but violation of the singleton is possible then, since non-       
 *   // singleton versions of the class can be instantiated.       
 *   MyClass() : _someData(5) {}       
 *   MyClass(int arg) : _someData(arg) {} // etc...       
 *   // don't implement the copy constructor, because singletons       
 *   // shouldn't be copied!       
 *   MyClass(const MyClass& mc) {}       
 *   ~MyClass() {}       
 * private:       
 *   int _someData;       
 * };
 *
 * // now create a singleton of MyClass       
 * typedef SkySingleton<MyClass> MyClassSingleton;
 *
 * @endcode
 * Later, in your program code, you can instantiate the singleton and access
 * its members like so:
 *
 * @code   
 * void somefunc()        
 * {        
 *   // instantiate the MyClassSingleton        
 *   MyClassSingleton::Instantiate();        
 *   // could also call MyClassSingleton::Instantiate(10);        
 *   // since we have a constructor of that form in MyClass.
 *         
 *   // access the methods in MyClass:        
 *   int data1 = MyClassSingleton::InstancePtr()->GetData();        
 *   // or...        
 *   int data2 = MyClassSingleton::InstanceRef().GetData();
 *         
 *   // now destroy the singleton        
 *   MyClassSingleton::Destroy();        
 * }
 * @endcode
 */
template <class T>
class SkySingleton : protected T
{
public:
  
  //------------------------------------------------------------------------------
  // Function     	  : Instantiate
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
   * @fn Instantiate()
   * @brief Creates the singleton instance for class T.
   *
   * Assures (by assertion) that the instance will only be created once.
   * This works for default constructors.
   */ 
  static void Instantiate()
  {
    assert(!s_pInstance);
    s_pInstance = new SkySingleton();
  }
  
  //------------------------------------------------------------------------------
  // Function     	  : Destroy
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
   * @fn Destroy()           { SAFE_DELETE(s_pInstance);                 }
   * @brief Destructor, deletes the instance
   */ 
  static void Destroy()           { SAFE_DELETE(s_pInstance);                 }
  
  //------------------------------------------------------------------------------
  // Function     	  : InstancePtr
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
   * @fn InstancePtr()         { assert(s_pInstance); return s_pInstance;  }
   * @brief Returns a pointer to the instance
   */ 
  static T* InstancePtr()         { assert(s_pInstance); return s_pInstance;  }

  //------------------------------------------------------------------------------
  // Function     	  : InstanceRef
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
   * @fn InstanceRef()         { assert(s_pInstance); return *s_pInstance; }
   * @brief Returns a reference to the instance
   */ 
  static T& InstanceRef()         { assert(s_pInstance); return *s_pInstance; }

  //------------------------------------------------------------------------------
  // Function     	  :  static void Instantiate
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
   * @fn  static void Instantiate(const A& a)
   * @brief Instantiates class of type T that have constructors with an argument
   *
   * This might be a source of confusion.  These templatized
   * functions are used to instantiate classes of type T that
   * have constructors with arguments.  For n arguments, you
   * to add a function below with n arguments.  Note, these will
   * only be created if they are used, since they are templates.
   * I've added 4 below, for 1-4 arguments.  If you get a
   * compilation error, add one for the number of arguments you
   * need.  Also need a SkySingleton protected constructor with
   * the same number of arguments.
   */ 
  template<class A>
  static void Instantiate(const A& a)
  {
    assert(!s_pInstance);
    s_pInstance = new SkySingleton(a);
  }

  //------------------------------------------------------------------------------
  // Function     	  : Instantiate
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
   * @fn Instantiate(const A& a, const B& b)
   * @brief Instantiates class of type T that have constructors with 2 args
   */ 
  template<class A, class B>
  static void Instantiate(const A& a, const B& b)
  {
    assert(!s_pInstance);
    s_pInstance = new SkySingleton(a, b);
  }

  //------------------------------------------------------------------------------
  // Function     	  : Instantiate
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
   * @fn Instantiate(const A& a, const B& b, const C& c)
   * @brief Instantiates class of type T that have constructors with 3 args
   */ 
  template<class A, class B, class C>
  static void Instantiate(const A& a, const B& b, const C& c)
  {
    assert(!s_pInstance);
    s_pInstance = new SkySingleton(a, b, c);
  }

  //------------------------------------------------------------------------------
  // Function     	  : Instantiate
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
   * @fn Instantiate(const A& a, const B& b, const C& c, const D& d)
   * @brief Instantiates class of type T that have constructors with 4 args
   */ 
  template<class A, class B, class C, class D> 
  static void Instantiate(const A& a, const B& b, const C& c, const D& d)
  {
    assert(!s_pInstance);
    s_pInstance = new SkySingleton(a, b, c, d);
  }
  
protected:
  // although the instance is of type SkySingleton<T>, the Instance***() funcs 
  // above implicitly cast it to type T.
  static SkySingleton* s_pInstance;
  
private:

  //------------------------------------------------------------------------------
  // Function     	  : SkySingleton
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
   * @fn SkySingleton()
   * @brief Hidden so that the singleton can only be instantiated via public static Instantiate function.
   */ 
  SkySingleton() : T() {}
  
  //------------------------------------------------------------------------------
  // Function     	  : Singleton
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
   * @fn Singleton(const A& a)
   * @brief Used by the templatized public Instantiate() functions.
   */ 
  template<class A> 
  SkySingleton(const A& a) : T(a) {}

  //------------------------------------------------------------------------------
  // Function     	  : Singleton
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
   * @fn Singleton(const A& a, const B& b)
   * @brief Used by the templatized public Instantiate() functions.
   */ 
  template<class A, class B> 
  SkySingleton(const A& a, const B& b) : T(a, b) {}

  //------------------------------------------------------------------------------
  // Function     	  : Singleton
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
   * @fn Singleton(const A& a, const B& b, const C& c)
   * @brief Used by the templatized public Instantiate() functions.
   */ 
  template<class A, class B, class C> 
  SkySingleton(const A& a, const B& b, const C& c) : T(a, b, c) {}

  //------------------------------------------------------------------------------
  // Function     	  : Singleton
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
   * @fn Singleton(const A& a, const B& b, const C &c, const D& d)
   * @brief Used by the templatized public Instantiate() functions.
   */ 
  template<class A, class B, class C, class D> 
  SkySingleton(const A& a, const B& b, const C &c, const D& d) : T(a, b, c, d) {}

  //------------------------------------------------------------------------------
  // Function     	  : SkySingleton
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
   * @fn SkySingleton(const SkySingleton&)
   * @brief Hidden because you can't copy a singleton!
   */ 
  SkySingleton(const SkySingleton&) {}  // hide the copy ctor: singletons can'

  //------------------------------------------------------------------------------
  // Function     	  : ~SkySingleton
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
   * @fn ~SkySingleton()
   * @brief Destructor, hidden, destroy via the public static Destroy() method.
   */ 
  ~SkySingleton()                  {}  // hide the dtor: 
};

// declare the static instance pointer
template<class T> SkySingleton<T>* SkySingleton<T>::s_pInstance = NULL;

#endif //__SKYSINGLETON_HPP__