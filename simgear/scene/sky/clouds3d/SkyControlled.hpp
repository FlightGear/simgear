//------------------------------------------------------------------------------
// File : SkyControlled.hpp
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
 * @file SkyControlled.hpp
 * 
 * Interface definition for controlled objects.
 */
#ifndef __SKYCONTROLLED_HPP__
#define __SKYCONTROLLED_HPP__

//#include "BHXController.hpp"

template<typename ControlStateType> class SkyController;


//------------------------------------------------------------------------------
/**
 * @class SkyControlled
 * @brief A base class defining an interface for controlled objects.
 * 
 * This class abstracts the control of objects into a simple interface that a
 * class may inherit that allows it to be controlled by SkyController objects.
 * A class simply inherits from SkyControlled, which forces the class to implement
 * the method UpdateStateFromControls().  This method usually uses the SkyController
 * object passed to SetController() to query the input state via 
 * SkyController::GetControlState().  
 *
 * @see SkyController
 */
template<typename ControlStateType>
class SkyControlled
{
public:
  //! Constructor.
  SkyControlled()          { _pController = NULL; }
  //! Destructor
  virtual ~SkyControlled() { _pController = NULL; }

  //! Sets the controller which will control this controlled object.
  void SetController(SkyController<ControlStateType> *pController) { _pController = pController; }
  
  //! Updates the state of the controlled object based on the controls (received from the controller).
  virtual SKYRESULT UpdateStateFromControls(SKYTIME timeStep) = 0;

protected:
  SkyController<ControlStateType>  *_pController;
  ControlStateType                  _controlState;
};

#endif //__SKYCONTROLLED_HPP__