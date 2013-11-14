// This file is part of openCaesar3.
//
// openCaesar3 is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// openCaesar3 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with openCaesar3.  If not, see <http://www.gnu.org/licenses/>.

#ifndef __OPENCAESAR3_TRAINEEWALKER_H_INCLUDED__
#define __OPENCAESAR3_TRAINEEWALKER_H_INCLUDED__

#include "walker.hpp"

class Propagator;

/** This walker goes to work */
class TraineeWalker : public Walker
{
public:
  static TraineeWalkerPtr create( PlayerCityPtr city, constants::walker::Type traineeType );

  void checkDestination(const TileOverlay::Type buildingType, Propagator& pathPropagator);
  void send2City();
  void setOriginBuilding(Building &building);
  void computeWalkerPath();

  virtual void _reachedPathway();

  void save( VariantMap& stream) const;
  void load( const VariantMap& stream);

protected:
  TraineeWalker( PlayerCityPtr city, constants::walker::Type traineeType);
  void init(constants::walker::Type traineeType);

private:
  BuildingPtr _originBuilding;
  BuildingPtr _destinationBuilding;
  int _maxDistance;

  std::list<TileOverlay::Type> _buildingNeed;  // list of buildings needing this trainee
  float _maxNeed;  // evaluates the need for that trainee
};

#endif
