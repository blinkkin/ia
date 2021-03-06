#include "DungeonMaster.h"

#include "Engine.h"
#include "Highscore.h"
#include "TextFormatting.h"
#include "Renderer.h"
#include "Query.h"
#include "ActorPlayer.h"
#include "CharacterLines.h"
#include "Log.h"
#include "SdlWrapper.h"

void DungeonMaster::initXpArray() {
  xpForLvl[0] = 0;
  xpForLvl[1] = 0;
  for(int lvl = 2; lvl <= PLAYER_MAX_CLVL; lvl++) {
    xpForLvl[lvl] = xpForLvl[lvl - 1] + (100 * lvl);
  }
}

int DungeonMaster::getMonsterTotXpWorth(const ActorData& d) const {
  //K regulates player XP rate, higher -> more XP per monster
  const double K          = 0.5;
  const double HP         = d.hp;
  const double SPEED      = double(d.speed);
  const double SPEED_MAX  = double(ActorSpeed::endOfActorSpeed);
  const double SHOCK      = double(d.monsterShockLevel);
  const double SHOCK_MAX  = double(MonsterShockLevel::endOfMonsterShockLevel);

  const double SPEED_FACTOR   = (1.0 + ((SPEED / SPEED_MAX) * 0.50));
  const double SHOCK_FACTOR   = (1.0 + ((SHOCK / SHOCK_MAX) * 0.75));
  const double UNIQUE_FACTOR  = d.isUnique ? 2.0 : 1.0;

  return ceil(K * HP * SPEED_FACTOR * SHOCK_FACTOR * UNIQUE_FACTOR);
}

void DungeonMaster::playerGainLvl() {
  if(eng.player->deadState == ActorDeadState::alive) {
    clvl++;

    eng.log->addMsg(
      "--- Welcome to level " + toString(clvl) + "! ---", clrGreen);

    eng.playerCreateCharacter->pickNewTrait(false);

    eng.player->restoreHp(999, false);
    eng.player->changeMaxHp(2, true);
    eng.player->changeMaxSpi(1, true);
  }
}

void DungeonMaster::playerGainXp(const int XP_GAINED) {
  if(eng.player->deadState == ActorDeadState::alive) {
    for(int i = 0; i < XP_GAINED; i++) {
      xp++;
      if(clvl < PLAYER_MAX_CLVL) {
        if(xp >= xpForLvl[clvl + 1]) {
          playerGainLvl();
        }
      }
    }
  }
}

int DungeonMaster::getXpToNextLvl() const {
  if(clvl == PLAYER_MAX_CLVL) {
    return -1;
  }
  return xpForLvl[clvl + 1] - xp;
}

void DungeonMaster::playerLoseXpPercent(const int PERCENT) {
  xp = (xp * (100 - PERCENT)) / 100;
}

void DungeonMaster::addSaveLines(vector<string>& lines) const {
  lines.push_back(toString(clvl));
  lines.push_back(toString(xp));
  lines.push_back(toString(timeStarted.year_));
  lines.push_back(toString(timeStarted.month_));
  lines.push_back(toString(timeStarted.day_));
  lines.push_back(toString(timeStarted.hour_));
  lines.push_back(toString(timeStarted.minute_));
  lines.push_back(toString(timeStarted.second_));
}

void DungeonMaster::setParamsFromSaveLines(vector<string>& lines) {
  clvl = toInt(lines.front());
  lines.erase(lines.begin());
  xp = toInt(lines.front());
  lines.erase(lines.begin());
  timeStarted.year_ = toInt(lines.front());
  lines.erase(lines.begin());
  timeStarted.month_ = toInt(lines.front());
  lines.erase(lines.begin());
  timeStarted.day_ = toInt(lines.front());
  lines.erase(lines.begin());
  timeStarted.hour_ = toInt(lines.front());
  lines.erase(lines.begin());
  timeStarted.minute_ = toInt(lines.front());
  lines.erase(lines.begin());
  timeStarted.second_ = toInt(lines.front());
  lines.erase(lines.begin());
}

void DungeonMaster::winGame() {
  eng.highScore->gameOver(true);

  string winMessage = "As I touch the crystal, there is a jolt of electricity. A surreal glow suddenly illuminates the area. ";
  winMessage += "I feel as if I have stirred something. I notice a dark figure observing me from the edge of the light. ";
  winMessage += "It is the shape of a human. The figure approaches me, but still no light falls on it as it enters. ";
  winMessage += "There is no doubt in my mind concerning the nature of this entity; It is the Faceless God who dwells in the depths of ";
  winMessage += "the earth, it is the Crawling Chaos - NYARLATHOTEP! ";
  winMessage += "I panic. Why is it I find myself here, stumbling around in darkness? ";
  winMessage += "The being beckons me to gaze into the stone. In the divine radiance I see visions beyond eternity, ";
  winMessage += "visions of unreal reality, visions of the brightest light of day and the darkest night of madness. ";
  winMessage += "The torrent of revelations does not seem unwelcome - I feel as if under a spell. There is only onward now. ";
  winMessage += "I demand to attain more - everything! So I make a pact with the Fiend......... ";
  winMessage += "I now harness the shadows that stride from world to world to sow death and madness. ";
  winMessage += "The destinies of all things on earth, living and dead, are mine. ";

  vector<string> winMessageLines;
  TextFormatting::lineToLines(winMessage, 68, winMessageLines);

  Renderer::coverPanel(panel_screen);
  Renderer::updateScreen();

  const int Y0 = 2;
  const unsigned int NR_OF_WIN_MESSAGE_LINES = winMessageLines.size();
  const int DELAY_BETWEEN_LINES = 40;
  SdlWrapper::sleep(DELAY_BETWEEN_LINES);
  for(unsigned int i = 0; i < NR_OF_WIN_MESSAGE_LINES; i++) {
    for(unsigned int ii = 0; ii <= i; ii++) {
      Renderer::drawTextCentered(winMessageLines.at(ii), panel_screen,
                                 Pos(MAP_W_HALF, Y0 + ii),
                                 clrMsgBad, clrBlack, true);
      if(i == ii && ii == NR_OF_WIN_MESSAGE_LINES - 1) {
        const string CMD_LABEL =
          "Space/Esc to record high-score and return to main menu";
        Renderer::drawTextCentered(
          CMD_LABEL, panel_screen,
          Pos(MAP_W_HALF, Y0 + NR_OF_WIN_MESSAGE_LINES + 2),
          clrWhite, clrBlack, true);
      }
    }
    Renderer::updateScreen();
    SdlWrapper::sleep(DELAY_BETWEEN_LINES);
  }

  eng.query->waitForEscOrSpace();
}

void DungeonMaster::onMonsterKilled(Actor& actor) {
  ActorData& d = actor.getData();

  d.nrOfKills += 1;

  if(d.hp >= 3) {
    if(eng.player->insanityObsessions[insanityObsession_sadism]) {
      eng.player->shock_ = max(0.0, eng.player->shock_ - 3.0);
    }
  }

  const int XP_WORTH_TOT  = getMonsterTotXpWorth(d);
  Monster* const monster  = dynamic_cast<Monster*>(&actor);
  const int XP_GAINED     = monster->hasGivenXpForSpotting_ ?
                            XP_WORTH_TOT / 2 : XP_WORTH_TOT;
  playerGainXp(XP_GAINED);
}

void DungeonMaster::onMonsterSpotted(Actor& actor) {
  Monster* const monster = dynamic_cast<Monster*>(&actor);
  if(monster->hasGivenXpForSpotting_ == false) {
    monster->hasGivenXpForSpotting_ = true;
    playerGainXp(getMonsterTotXpWorth(monster->getData()) / 2);
  }
}

void DungeonMaster::setTimeStartedToNow() {
  timeStarted = Utils::getCurrentTime();
}

