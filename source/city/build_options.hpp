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
// Copyright 2012-2014 Dalerank, dalerankn8@gmail.com

#ifndef __CAESARIA_BUILD_OPTIONS_H_INCLUDED__
#define __CAESARIA_BUILD_OPTIONS_H_INCLUDED__

#include "core/referencecounted.hpp"
#include "core/scopedptr.hpp"
#include "core/variant.hpp"
#include "vfs/path.hpp"
#include "objects/constants.hpp"

namespace city
{

namespace development
{

/**
 * @brief The city build options class
 */
class Options : public ReferenceCounted
{
public:
  Options();
  virtual ~Options();

  void setBuildingAvailable( const object::Type type, bool mayBuild );
  unsigned int getBuildingsQuote( const object::Type type ) const;

  void setAvailable(bool av);

  bool isBuildingAvailable( const object::Type type ) const;

  void clear();
  void load( const VariantMap& options );
  VariantMap save() const;

  Options& operator=(const Options& a);

  void toggleBuildingAvailable( const object::Type type );
  bool isCheckDesirability() const;
  unsigned int maximumForts() const;

private:
  class Impl;
  ScopedPtr< Impl > _d;
};

}//end namespace development

}//end namespace city
#endif //__CAESARIA_BUILD_OPTIONS_H_INCLUDED__
