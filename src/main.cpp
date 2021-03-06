#include <SDL.h>

#include "Engine.h"

#include "MainMenu.h"
#include "Renderer.h"
#include "InventoryHandler.h"
#include "PlayerBonuses.h"
#include "PlayerCreateCharacter.h"
#include "ActorPlayer.h"
#include "MapGen.h"
#include "PopulateMonsters.h"
#include "DungeonClimb.h"
#include "Popup.h"
#include "CharacterLines.h"
#include "Log.h"
#include "Query.h"
#include "Highscore.h"
#include "Postmortem.h"
#include "DungeonMaster.h"
#include "DebugModeStatPrinter.h"
#include "Audio.h"
#include "Bot.h"
#include "Input.h"
#include "SdlWrapper.h"

#ifdef _WIN32
#undef main
#endif
int main(int argc, char* argv[]) {
  trace << "main()..." << endl;

  (void)argc;
  (void)argv;

  Engine eng;
  SdlWrapper::init();
  Config::init();
  Input::init();
  Renderer::init(eng);
  Audio::init();

  bool quitGame = false;
  while(quitGame == false) {
    eng.initGame();

    int introMusChannel = -1;
    const GameEntryMode gameEntryType =
      eng.mainMenu->run(quitGame, introMusChannel);

    if(quitGame == false) {
      eng.quitToMainMenu_ = false;

      if(gameEntryType == gameEntry_new) {
        if(Config::isBotPlaying()) {
          eng.playerBonHandler->setAllTraitsToPicked();
          eng.bot->init();
        }
        eng.playerCreateCharacter->createCharacter();
        eng.player->spawnStartItems();

        eng.gameTime->insertActorInLoop(eng.player);

        if(Config::isIntroLevelSkipped() == false) {
          //If intro level is used, build forest.
          Renderer::coverPanel(panel_screen);
          Renderer::updateScreen();
          MapGenIntroForest(eng).run();
        } else {
          //Else build first dungeon level
          eng.dungeonClimb->travelDown();
        }
        eng.dungeonMaster->setTimeStartedToNow();
        const TimeData& t = eng.dungeonMaster->getTimeStarted();
        trace << "Game started on: " << t.getTimeStr(time_minute, true) << endl;
      }

      Audio::fadeOutChannel(introMusChannel);

      eng.player->updateFov();
      Renderer::drawMapAndInterface();

      if(gameEntryType == gameEntry_new) {
        if(Config::isIntroLevelSkipped() == 0) {
          string introMessage = "I stand on a cobbled forest path, ahead lies a shunned and decrepit old church building. ";
          introMessage += "From years of investigation and discreet inquiries, I know this to be the access point to the abhorred ";
          introMessage += "\"Cult of Starry Wisdom\". ";
          introMessage += "I will enter these sprawling catacombs and rob them of treasures and knowledge. ";
          introMessage += "The ultimate prize is an artifact of non-human origin called \"The shining Trapezohedron\" ";
          introMessage += "- a window to all secrets of the universe.";
          eng.popup->showMsg(introMessage, true, "The story so far...");
        }
      }

      //========== M A I N   L O O P ==========
      while(eng.quitToMainMenu_ == false) {
        if(eng.player->deadState == ActorDeadState::alive) {

          Actor* const actor = eng.gameTime->getCurrentActor();

          //Properties running on the actor's turn are not immediately applied
          //on the actor, but instead placed in a buffer. This is to ensure
          //that e.g. a property set to last one turn actually covers one turn
          //(and not applied after the actor acts, and ends before the actor's
          //next turn)
          //The contents of the buffer are moved to the applied properties here
          actor->getPropHandler().applyActorTurnPropBuffer();

          actor->updateColor();

          if(actor->getPropHandler().allowAct()) {
            actor->onActorTurn();
          } else {
            if(actor == eng.player) {
              SdlWrapper::sleep(DELAY_PLAYER_UNABLE_TO_ACT);
            }
            eng.gameTime->actorDidAct();
          }
        } else {
          //Player is dead, run postmortem, then return to main menu
          dynamic_cast<Player*>(eng.player)->waitTurnsLeft = -1;
          eng.log->addMsg(
            "=== I AM DEAD === (press space/esc to view postmortem information)",
            clrMsgBad);
          Renderer::drawMapAndInterface();
          eng.log->clearLog();
          eng.query->waitForEscOrSpace();
          eng.highScore->gameOver(false);
          eng.postmortem->run(&quitGame);
          eng.quitToMainMenu_ = true;
        }
      }
    }
    eng.cleanupGame();
  }
  Audio::cleanup();
  Renderer::cleanup();
  Input::cleanup();
  SdlWrapper::cleanup();

  trace << "main() [DONE]" << endl;

  return 0;
}

