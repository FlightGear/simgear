//------------------------------------------------------------------------------
// File : SkyController.hpp
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
 * @file SkyController.hpp
 * 
 * Abstract base class for game object controllers.
 */
#ifndef __SKYCONTROLLER_HPP__
#define __SKYCONTROLLER_HPP__

#include "SkyUtil.hpp"


//------------------------------------------------------------------------------
/**
 * @class SkyController
 * @brief A class that defines an interface for translating general control input into game object control.
 * 
 * This class abstracts game object control from specific control input, such 
 * as via user interface devices or via artificial intelligence.  Subclasses of
 * this class implement the method GetControlState() so that objects controlled
 * by an instance of a SkyController can query the control state that determines 
 * their actions.  The object need not know whether it is controlled by a human 
 * or the computer since either controller provides it the same interface.
 *
 * @see SkyControlled
 */
template <typename ControlStateType>
class SkyController
{
public:
  //! Constructor.
  SkyController() {}
  //! Destructor.
  virtual ~SkyController() {}
  
  //! Fills out the control state structure passed in with the current state of controls.
  virtual SKYRESULT GetControlState(ControlStateType &controlState) = 0;
};

#endif //__SKYCONTROLLER_HPP__