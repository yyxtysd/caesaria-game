// This file is part of CaesarIA.
//
// CaesarIA is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// CaesarIA is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with CaesarIA.  If not, see <http://www.gnu.org/licenses/>.
//
// Copyright 2012-2054 Dalerank, dalerankn8@gmail.com

#ifndef __CAESARIA_DEBUGTIMER_H_INCLUDED__
#define __CAESARIA_DEBUGTIMER_H_INCLUDED__

#include "core/smartptr.hpp"
#include "core/scopedptr.hpp"
#include "core/referencecounted.hpp"
#include "core/signals.hpp"

class DebugTimer
{
public:
  static unsigned int ticks();
  static void reset( const std::string& name );
  static unsigned int take(const std::string& name, bool reset=false);
  static unsigned int delta( const std::string& name, bool reset=false );

  static void check( const std::string& prefix, const std::string& name );

private:
  static DebugTimer& instance();
  DebugTimer();

  class Impl;
  ScopedPtr<Impl> _d;
};

#endif //__CAESARIA_DEBUGTIMER_H_INCLUDED__

