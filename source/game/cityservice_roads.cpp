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

#include "cityservice_roads.hpp"
#include "city.hpp"
#include "gamedate.hpp"
#include "path_finding.hpp"
#include "tilemap.hpp"
#include "building/house.hpp"
#include "house_level.hpp"
#include "road.hpp"
#include "building/constants.hpp"

using namespace constants;

class CityServiceRoads::Impl
{
public:
  PlayerCityPtr city;
  int maxDistance;
  int defaultIncreasePaved;
  int defaultDecreasePaved;

  DateTime lastTimeUpdate;
  ScopedPtr< Propagator > propagator;

  void updateRoadsAround( BuildingPtr building );
};

CityServicePtr CityServiceRoads::create(PlayerCityPtr city)
{
  CityServiceRoads* ret = new CityServiceRoads( city );

  return CityServicePtr( ret );
}

CityServiceRoads::CityServiceRoads(PlayerCityPtr city )
: CityService( "roads" ), _d( new Impl )
{
  _d->city = city;
  _d->maxDistance = 10;
  _d->defaultIncreasePaved = 4;
  _d->defaultDecreasePaved = -1;
  _d->lastTimeUpdate = GameDate::current();
  _d->propagator.reset( new Propagator( city ) );
}

void CityServiceRoads::update( const unsigned int time )
{
  if( _d->lastTimeUpdate.getMonth() == GameDate::current().getMonth() )
    return;

  _d->lastTimeUpdate = GameDate::current();

  std::vector< TileOverlay::Type > btypes;
  btypes.push_back( building::senate );

  CityHelper helper( _d->city );

  BuildingList buildings;
  foreach( TileOverlay::Type type, btypes )
  {
    BuildingList tmp = helper.find<Building>( type );
    buildings.insert( buildings.end(), tmp.begin(), tmp.end() );
  }

  HouseList houses = helper.find<House>( building::house );
  foreach( HousePtr house, houses )
  {
    if( house->getSpec().getLevel() >= House::bigMansion )
    {
      buildings.push_back( house.as<Building>() );
    }
  }

  foreach( BuildingPtr building, buildings )
  {
    _d->updateRoadsAround( building );
  }

  if( _d->lastTimeUpdate.getMonth() % 3 == 1 )
  {
    RoadList roads = helper.find<Road>( construction::road );
    foreach( RoadPtr road, roads )
    {
      road->appendPaved( _d->defaultDecreasePaved );
    }
  }
}

CityServiceRoads::~CityServiceRoads()
{

}

void CityServiceRoads::Impl::updateRoadsAround(BuildingPtr building)
{
  propagator->init( building.as<Construction>() );
  Propagator::PathWayList pathWayList = propagator->getWays( maxDistance );

  foreach( Pathway& current, pathWayList )
  {
    const TilesArray& tiles = current.getAllTiles();
    for( TilesArray::const_iterator it=tiles.begin(); it != tiles.end(); it++ )
    {
      RoadPtr road = (*it)->getOverlay().as<Road>();
      if( road.isValid() )
      {
        road->appendPaved( defaultIncreasePaved );
      }
    }
  }
}
