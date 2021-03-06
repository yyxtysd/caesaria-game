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
// Copyright 2012-2013 Gregoire Athanase, gathanase@gmail.com
// Copyright 2012-2015 Dalerank, dalerankn8@gmail.com

#include "level.hpp"
#include <GameEvents>
#include <GameGfx>
#include <GameCity>
#include <GameGui>
#include <GameCore>
#include <GameLogger>

#include "game/resourcegroup.hpp"
#include "objects/objects_factory.hpp"
#include "layers/constants.hpp"
#include "game/alarm_event_holder.hpp"
#include "game/game.hpp"
#include "game/funds.hpp"
#include "game/gamedate.hpp"
#include "world/empire.hpp"
#include "game/settings.hpp"
#include "game/patrolpointeventhandler.hpp"
#include "sound/engine.hpp"
#include "world/romechastenerarmy.hpp"
#include "layers/layer.hpp"
#include "scripting/core.hpp"
#include "game/debug_handler.hpp"
#include "game/hotkey_manager.hpp"
#include "steam.hpp"

using namespace gui;
using namespace events;
using namespace gfx;
using namespace city;

namespace scene
{
typedef SmartList<EventHandler> EventHandlers;
const int topMenuHeight = 24;

class Level::Impl
{
public:
  EventHandlers eventHandlers;
  gui::TopMenu* topMenu;
  Engine* engine;
  DebugHandler dhandler;
  CityRenderer renderer;
  Game* game; // current game
  AlarmEventHolder alarmsHolder;
  std::string mapToLoad;
  TilePos selectedTilePos;
  citylayer::Type lastLayerId;
  bool simulationPaused;
  undo::UStack undoStack;

  int result;

public:
  void resolveSelectLayer(int type);
  void showTradeAdvisorWindow();
  void checkFailedMission(Level *lvl, bool forceFailed=false);
  void checkWinMission();
  void makeScreenShot();
  void layerChangedOut(int type);
  void setAlarmEnabled(bool enable);
  void resolveWarningMessage( std::string );
  void saveCameraPos(Point p);
  void handleDirectionChange( Direction direction );
  void initRender();
  void initMainUI();
  void installHandlers( Base* scene);
  void initSound();
  void initTabletUI(Level* scene);
  void resolveUndoChange(bool enable);

  std::string getScreenshotName();
};

Level::Level(Game& game, gfx::Engine& engine ) : _d( new Impl )
{
  _d->topMenu = NULL;
  _d->game = &game;
  _d->simulationPaused = false;
  _d->engine = &engine;
}

Level::~Level() {
  script::Core::execFunction( "OnLevelDestroyed" );
}

void Level::Impl::initRender()
{
  bool oldGraphics = KILLSWITCH(oldgfx) || !SETTINGS_STR(c3gfx).empty();
  renderer.initialize( game->city(), engine, game->gui(), oldGraphics );
  renderer.setViewport( engine->viewportSize() );
  renderer.camera()->setScrollSpeed( SETTINGS_VALUE( scrollSpeed ) );
}

void Level::Impl::initMainUI()
{
  PlayerCityPtr city = game->city();
  gui::Ui& ui = *game->gui();

  ui.clear();

  topMenu = &ui.add<TopMenu>(topMenuHeight, !city->getOption(PlayerCity::c3gameplay));

  //bool fitToHeidht = OSystem::isAndroid();
  //menu = Menu::create( ui.rootWidget(), -1, city, fitToHeidht );
  //menu->hide();

  WindowMessageStack::create( ui.rootWidget() );

  if( KILLSWITCH( rightMenu ) )
  {
    //rightPanel->setSide( MenuRigthPanel::rightSide );
    //menu->setSide( Menu::rightSide, rightPanel->lefttop() );
    //extMenu->setSide( Menu::rightSide, rightPanel->lefttop() );
  }
  else
  {
    //rightPanel->setSide( MenuRigthPanel::leftSide );
    //menu->setSide( Menu::leftSide, rightPanel->righttop() );
    //extMenu->setSide( Menu::leftSide, rightPanel->righttop() );
  }
}

void Level::Impl::installHandlers( Base* scene )
{
  scene->installEventHandler( PatrolPointEventHandler::create( *game, renderer ) );
}

void Level::Impl::initSound()
{
  auto sound = game->city()->statistic().services.find<city::AmbientSound>();

  if( sound.isValid() )
    sound->setCamera(renderer.camera());
}

void Level::Impl::initTabletUI( Level* scene )
{
  //specific tablet actions bar
  auto& tabletUi = game->gui()->add<tablet::ActionsBar>();
  tablet::ActionsHandler::assignTo( &tabletUi, scene );

  tabletUi.setVisible( SETTINGS_VALUE(showTabletMenu) );
}

void Level::Impl::resolveUndoChange(bool enable)
{
  VariantList vl; vl << enable;
  events::dispatch<events::ScriptFunc>("OnChangeSessionUndoState", vl);
}

void Level::initialize()
{
  PlayerCityPtr city = _d->game->city();

  _d->initRender();
  _d->initMainUI();
  _d->installHandlers( this );
  _d->initSound();
  _d->initTabletUI( this );
  _d->undoStack.init( city );

  //connect elements
  //city->tilemap().setFlag( Tilemap::fSvkGround, !KILLSWITCH(oldgfx) );
  CONNECT( city, onWarningMessage(),              _d.data(),         Impl::resolveWarningMessage )

  //CONNECT( _d->extMenu, onSelectOverlayType(),    _d.data(),         Impl::resolveSelectLayer )

  CONNECT( city, onDisasterEvent(),               &_d->alarmsHolder, AlarmEventHolder::add )
  CONNECT( &_d->alarmsHolder, onMoveToAlarm(),    _d->renderer.camera(), Camera::setCenter )
  CONNECT( &_d->alarmsHolder, onAlarmChange(),    _d.data(),       Impl::setAlarmEnabled )

  CONNECT( _d->renderer.camera(), onPositionChanged(), _d.data(),    Impl::saveCameraPos )
  CONNECT( _d->renderer.camera(), onDirectionChanged(), _d.data(),   Impl::handleDirectionChange )

  CONNECT( &_d->renderer, onLayerSwitch(), _d.data(),              Impl::layerChangedOut )
  CONNECT( &_d->undoStack, onUndoChange(),       _d.data(),       Impl::resolveUndoChange )

  _d->renderer.camera()->setCenter(city->cameraPos());

  _d->dhandler.insertTo(_d->game, _d->topMenu);
  _d->dhandler.setVisible(false);

  events::dispatch<events::ScriptFunc>("OnMissionStart");
}

std::string Level::nextFilename() const { return _d->mapToLoad; }

void Level::Impl::setAlarmEnabled(bool enable) {
  VariantList vl; vl << enable;
  events::dispatch<events::ScriptFunc>("OnChangeSessionAlarmState", vl);
}

void Level::Impl::resolveWarningMessage(std::string text)
{
  events::dispatch<WarningMessage>( text, WarningMessage::neitral );
}

void Level::Impl::saveCameraPos(Point p)
{
  Size scrSize = engine->screenSize();
  Tile* tile = renderer.camera()->at( Point( scrSize.width()/2, scrSize.height()/2 ), false );

  if( tile )
  {
    game->city()->setCameraPos( tile->pos() );
  }
}

void Level::Impl::handleDirectionChange(Direction direction)
{
  events::dispatch<WarningMessage>( _("##" + direction::Helper::instance().findName( direction ) + "##"), 1 );
}

std::string Level::Impl::getScreenshotName()
{
  DateTime time = DateTime::currenTime();
  vfs::Path filename = utils::format( 0xff, "oc3_[%04d_%02d_%02d_%02d_%02d_%02d].png",
                                      time.year(), time.month(), time.day(),
                                      time.hour(), time.minutes(), time.seconds() );
  vfs::Directory screenDir = SETTINGS_STR( screenshotDir );
  return (screenDir/filename).toString();
}

void Level::draw(Engine& engine)
{
  _d->renderer.render();

  _d->game->gui()->beforeDraw();
  _d->game->gui()->draw();
}

void Level::animate( unsigned int time )
{
  _d->renderer.animate( time );

  if( game::Date::isWeekChanged() )
  {
    _d->checkWinMission();
    _d->checkFailedMission( this );
  }

  if( game::Date::isMonthChanged() )
  {
    int autosaveInterval = SETTINGS_VALUE(autosaveInterval);
    if ((int)game::Date::current().month() % autosaveInterval == 0)
      events::dispatch<events::ScriptFunc>("OnSimulationCreateAutosave");
  }
}

void Level::afterFrame()
{
  if (game::Date::isDayChanged())
    events::dispatch<events::ScriptFunc>("OnGameDayChanged");
}

void Level::handleEvent( NEvent& event )
{
  //After MouseDown events are send to the same target till MouseUp
  Ui& gui = *_d->game->gui();

  if( event.EventType == sEventQuit )
  {
    events::dispatch<ScriptFunc>("OnRequestExitGame");
    return;
  }

  for( auto it=_d->eventHandlers.begin(); it != _d->eventHandlers.end(); )
  {
    (*it)->handleEvent( event );
    if( (*it)->finished() ) { it = _d->eventHandlers.erase( it ); }
    else{ ++it; }
  }

  bool eventResolved = gui.handleEvent( event );

  if( !eventResolved )
  {
    eventResolved = _tryExecHotkey( event );
  }

  if( !eventResolved )
  {
    _d->renderer.handleEvent( event );
    if( event.EventType == sEventMouse )
      _d->selectedTilePos = _d->renderer.screen2tilepos( event.mouse.pos() );
  }
}

void Level::Impl::makeScreenShot()
{
  std::string filename = getScreenshotName();
  Logger::debug( "Level: create screenshot " + filename );

  Engine::instance().createScreenshot( filename );
  events::dispatch<WarningMessage>( "Screenshot save to " + filename, WarningMessage::neitral );
}

void Level::Impl::layerChangedOut(int layer)
{
  if (layer > citylayer::simple && layer < citylayer::build)
  {
    lastLayerId = (citylayer::Type)layer;
    std::string layerName = citylayer::Helper::instance().findName((citylayer::Type)layer);

    VariantList vl; vl << Variant(layerName);
    events::dispatch<events::ScriptFunc>("OnRendererLayerChange", vl);
  }
  undoStack.finished();
}

void Level::Impl::checkFailedMission( Level* lvl, bool forceFailed )
{
  PlayerCityPtr pcity = game->city();

  const city::VictoryConditions& vc = pcity->victoryConditions();
  MilitaryPtr mil = pcity->statistic().services.find<Military>();
  InfoPtr info = pcity->statistic().services.find<Info>();

  if (mil.isValid() && info.isValid())
  {
    const city::Info::MaxParameters& params = info->maxParams();

    bool failedByDestroy = mil->value() > 0 && params[ Info::population ].value > 0 && !pcity->states().population;
    bool failedByTime = ( !vc.isSuccess() && game::Date::current() > vc.finishDate() );

    if( failedByDestroy || failedByTime || forceFailed )
    {
      game->pause();
      events::dispatch<ScriptFunc>("OnMissionLose");
      steamapi::missionLose(vc.name());
    }
  }
}

void Level::Impl::checkWinMission()
{
  auto city = game->city();
  auto& conditions = city->victoryConditions();

  int culture = city->culture();
  int prosperity = city->prosperity();
  int favour = city->states().favor;
  int peace = city->peace();
  int population = city->states().population;
  bool success = conditions.isSuccess(culture, prosperity, favour, peace, population);

  if (success)
  {
    events::dispatch<ScriptFunc>("OnMissionWin");
    steamapi::missionWin(conditions.name());
  }
}

bool Level::installEventHandler(EventHandlerPtr handler) { _d->eventHandlers.push_back( handler ); return true; }

void Level::setOption(const std::string& name, Variant value)
{
  if (name == "nextFile") {
    _d->mapToLoad = value.toString();
    bool isNextBriefing = vfs::Path( _d->mapToLoad ).isMyExtension( ".briefing" );
    _d->result = isNextBriefing ? Level::res_briefing : Level::res_load;
    stop();
  } else if (name == "advisor") {
    events::dispatch<ShowAdvisorWindow>(true, value.toEnum<Advisor>());
  } else if (name == "layer") {
    int type = citylayer::count;
    if (value.type() == Variant::String) {
      type = citylayer::Helper::instance().findType(value.toString());
    } else {
      type = citylayer::Type(value.toInt());
    }

    _d->renderer.setLayer(type);
  } else if (name == "buildMode") {
    int type = object::unknown;
    if (value.type() == Variant::String) {
      type = object::toType(value.toString());
    } else {
      type = value.toInt();
    }
    _d->renderer.setMode(BuildMode::create(object::Type(type)));
  } else if (name == "removeTool") {
    _d->renderer.setMode(DestroyMode::create());
  } else if (name == "editorMode") {
    int type = object::unknown;
    if (value.type() == Variant::String) {
      type = object::toType(value.toString());
    } else {
      type = value.toInt();
    }
    _d->renderer.setMode(EditorMode::create(object::Type(type)));
  } else if (name == "rotateRight") {
    _d->renderer.rotateRight();
  } else if (name == "rotateLeft") {
    _d->renderer.rotateLeft();
  } else if (name == "showEmpireMap") {
    events::dispatch<ShowEmpireMap>(true);
  } else if (name == "undo") {
    _d->undoStack.undo();
  } else if (name == "nextAlarm") {
    _d->alarmsHolder.next();
  }
}

Variant Level::getOption(const std::string& name)
{
  if (name == "layer") {
    return _d->renderer.layerType();
  } else if (name == "lastLayer") {
    return _d->lastLayerId;
  }

  return Variant();
}

void Level::setMode(int mode)
{
  switch(mode)
  {
  case res_quit:
    _d->result = Level::res_quit;
    stop();
  break;

  case res_restart:
    _d->result = Level::res_restart;
    stop();
  break;

  case res_menu:
    _d->result = Level::res_menu;
    stop();
  break;
  }
}

void Level::Impl::resolveSelectLayer( int type ){  renderer.setMode( LayerMode::create( type ) );}
void Level::Impl::showTradeAdvisorWindow(){ events::dispatch<ShowAdvisorWindow>(true, advisor::trading);  }
void Level::setCameraPos(TilePos pos, bool force) {  _d->renderer.camera()->setCenter( pos, force ); }
void Level::switch2layer(int layer) { _d->renderer.setLayer( layer ); }
Camera* Level::camera() const { return _d->renderer.camera(); }
undo::UStack&Level::undoStack() { return _d->undoStack; }
int  Level::result() const {  return _d->result; }

bool Level::_tryExecHotkey(NEvent &event)
{
  bool handled = false;
  if( event.EventType == sEventKeyboard && !event.keyboard.pressed)
  {
    handled = game::HotkeyManager::instance().execute(event.keyboard.key, event.keyboard.control, event.keyboard.shift, event.keyboard.alt);
    if (handled)
      return true;

    if( !event.keyboard.shift )
    {
      handled = true;
      switch (event.keyboard.key)
      {
      case KEY_KEY_E:
      {
        TilePos center = _d->renderer.camera()->center();
        TileRect trect( center-config::tilemap.unitLocation(), center+config::tilemap.unitLocation());
        TilePos currect = _d->game->city()->getBorderInfo(PlayerCity::roadEntry).epos();
        PlayerCity::TileType rcenter = trect.contain(currect)
                                          ? PlayerCity::roadExit
                                          : PlayerCity::roadEntry;
        _d->renderer.camera()->setCenter( _d->game->city()->getBorderInfo( rcenter ).epos(), false );
      }
      break;

      default:
        handled = false;
      break;
      }

      if (handled)
        return handled;
    }

    switch(event.keyboard.key)
    {
    case KEY_KEY_P:
    {
      _d->simulationPaused =  !_d->simulationPaused;
      events::dispatch<Pause>(_d->simulationPaused ? Pause::pause : Pause::play);
      handled = true;
    }
    break;

    case KEY_F1: case KEY_F2:
    case KEY_F3: case KEY_F4:
    {
      if( event.keyboard.control )
      {
        unsigned int index = event.keyboard.key - KEY_KEY_1;
        development::Options bopts;
        bopts = _d->game->city()->buildOptions();
        if( event.keyboard.shift )
        {
          TilePos camPos = _d->renderer.camera()->center();
          _d->game->city()->activePoints().set( index, camPos );
          _d->game->city()->setBuildOptions( bopts );
        }
        else
        {
          TilePos camPos = _d->game->city()->activePoints().get( index );
          _d->renderer.camera()->setCenter( camPos );
        }

        handled = true;
      }
    }
    break;

    case KEY_SNAPSHOT:
        _d->makeScreenShot();
      handled = true;
    break;

    case KEY_ESCAPE:
    {
      Widget::Widgets children = _d->game->gui()->rootWidget()->children();
      for (auto it : children)
      {
        bool handled = it->onEvent(event);
        if (handled)
            break;
      }
    }
    break;

    default:
    break;
    }
  }

  return handled;
}

}//end namespace scene
