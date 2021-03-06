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

#include "marble_quarry.hpp"
#include "game/resourcegroup.hpp"
#include "gfx/tilemap.hpp"
#include "gfx/tile.hpp"
#include "city/city.hpp"
#include "core/gettext.hpp"
#include "constants.hpp"
#include "objects_factory.hpp"

using namespace gfx;

REGISTER_CLASS_IN_OVERLAYFACTORY(object::quarry, MarbleQuarry)

MarbleQuarry::MarbleQuarry()
  : Factory(good::none, good::marble, object::quarry, Size(2,2) )
{
  _animation().load( ResourceGroup::commerce, 44, 10);
  _animation().setDelay( 4 );
  _fgPictures().resize(2);

  _setClearAnimationOnStop( false );
}

void MarbleQuarry::timeStep( const unsigned long time )
{
  Factory::timeStep( time );
}

bool MarbleQuarry::canBuild( const city::AreaInfo& areaInfo ) const
{
  bool is_constructible = Construction::canBuild( areaInfo );

  Tilemap& tilemap = areaInfo.city->tilemap();
  TilesArray perimetr = tilemap.rect( areaInfo.pos + TilePos(-1, -1), size() + Size(2, 2), Tilemap::CheckCorners);

  bool near_mountain = !perimetr.select( Tile::tlRock ).empty();  // tells if the factory is next to a mountain

  const_cast< MarbleQuarry* >( this )->_setError( near_mountain ? "" : _("##build_near_mountain_only##") );

  return (is_constructible && near_mountain);
}
