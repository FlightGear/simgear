// ----------------------------------------------------------------------------
//  Description      : Generic exceptions
// ----------------------------------------------------------------------------
//  (c) Copyright 1996 by iXiONmedia, all rights reserved.
// ----------------------------------------------------------------------------




#include "ixlib_i18n.hh"
#include <ixlib_exgen.hh>
#include <ixlib_numconv.hh>




using namespace ixion;




static char *(PlainText[]) = { 
  N_("Unable to evaluate expression"),
  N_("Function not yet implemented"),
  N_("General error"),
  N_("NULL pointer encountered"),
  N_("Invalid parameter"),
  N_("Index out of range"),
  N_("Buffer overrun"),
  N_("Buffer underrun"),
  N_("Item not found"),
  N_("Invalid operation"),
  N_("Dimension mismatch"),
  N_("Operation cancelled"),
  N_("Unable to operate on empty set"),
  N_("Unable to remove GC entry"),
  N_("Unable to protect non-freeable entry")
  };




// generic_exception ----------------------------------------------------------
char const *generic_exception::getText() const {
  return _(PlainText[Error]);
  }
