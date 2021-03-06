#include "MainMenu.h"

#include <string>

#include "Engine.h"
#include "Colors.h"
#include "Renderer.h"
#include "MenuInputHandler.h"
#include "SaveHandler.h"
#include "Highscore.h"
#include "Manual.h"
#include "Popup.h"
#include "TextFormatting.h"
#include "Credits.h"
#include "Audio.h"
#include "GameTime.h"
#include "DungeonClimb.h"
#include "Actor.h"
#include "ActorPlayer.h"
#include "Utils.h"

using namespace std;

void MainMenu::draw(const MenuBrowser& browser) const {
  trace << "MainMenu::draw()..." << endl;

  Pos pos(MAP_W_HALF, 3);

  trace << "MainMenu: Calling clearWindow()" << endl;
  Renderer::clearScreen();

  Renderer::drawPopupBox(Rect(Pos(0, 0), Pos(SCREEN_W - 1, SCREEN_H - 1)));

//  trace << "MainMenu: Drawing random background letters" << endl;
//  const int NR_X_CELLS = Config::screenPixelW / Config::getCellW();
//  const int NR_Y_CELLS = Config::screenPixelH / Config::getCellH();
//  const int BG_BRIGHTNESS = Rnd::range(14, 17);
//  for(int y = 0; y < NR_Y_CELLS; y++) {
//    for(int x = 0; x < NR_X_CELLS; x++) {
//      char cha = ' ';
//      if(Rnd::coinToss()) {
//        cha = 'a' + Rnd::range(0, 25);
//      }
//      SDL_Color bgClr = clrBlack;
//      bgClr.r = BG_BRIGHTNESS / 2;
//      bgClr.g = BG_BRIGHTNESS / 2;
//      bgClr.b = BG_BRIGHTNESS;
//      Renderer::drawGlyph(cha, panel_screen, Pos(x, y), bgClr);
//    }
//  }

  if(Config::isTilesMode()) {
    trace << "MainMenu: Calling drawMainMenuLogo()" << endl;
    Renderer::drawMainMenuLogo(0);
    pos.y += 10;
  } else {
    vector<string> logo;
    if(Config::isTilesMode() == false) {
      logo.push_back("        ___  __                __  __                  ");
      logo.push_back("| |\\  | |   |  )  /\\      /\\  |  )/    /\\  |\\  |  /\\   ");
      logo.push_back("+ | \\ | +-- +--  ____    ____ +-- -   ____ | \\ | ____  ");
      logo.push_back("| |  \\| |   | \\ /    \\  /    \\| \\ \\__/    \\|  \\|/    \\ ");
      logo.push_back("               \\                 \\                      ");
    }
    const int LOGO_X_POS_LEFT = (MAP_W - logo.at(0).size()) / 2;
    for(const string & row : logo) {
      pos.x = LOGO_X_POS_LEFT;
      for(const char & glyph : row) {
        if(glyph != ' ') {
          SDL_Color clr = clrGreenLgt;
          clr.g += Rnd::range(-50, 100);
          clr.g = max(0, min(254, int(clr.g)));
          Renderer::drawGlyph(glyph, panel_screen, pos, clr);
        }
        pos.x++;
      }
      pos.y += 1;
    }
    pos.y += 3;
  }

  if(IS_DEBUG_MODE) {
    Renderer::drawText(
      "## DEBUG MODE ##", panel_screen, Pos(1, 1), clrYellow);
  }

  trace << "MainMenu: Drawing HPL quote" << endl;
  SDL_Color quoteClr = clrGray;
  quoteClr.r /= 7;
  quoteClr.g /= 7;
  quoteClr.b /= 7;

  vector<string> quoteLines;
  TextFormatting::lineToLines(quote, 28, quoteLines);
  Pos quotePos(15, pos.y - 1);
  for(string & quoteLine : quoteLines) {
    Renderer::drawTextCentered(quoteLine, panel_screen, quotePos, quoteClr);
    quotePos.y++;
  }

  trace << "MainMenu: Drawing main menu" << endl;
  SDL_Color clrActive     = clrNosfTealLgt;
  SDL_Color clrInactive   = clrNosfTealDrk;
  SDL_Color clrActiveBg   = clrBlack;
  SDL_Color clrInactiveBg = clrBlack;

  pos.x = MAP_W_HALF;

  const int BOX_Y0 = pos.y - 1;

  Renderer::drawTextCentered(
    "New journey", panel_screen, pos,
    browser.isPosAtKey('a') ? clrActive : clrInactive,
    browser.isPosAtKey('a') ? clrActiveBg : clrInactiveBg);
  pos.y++;

  Renderer::drawTextCentered(
    "Resurrect", panel_screen, pos,
    browser.isPosAtKey('b') ? clrActive : clrInactive,
    browser.isPosAtKey('b') ? clrActiveBg : clrInactiveBg);
  pos.y++;

  Renderer::drawTextCentered(
    "Manual", panel_screen, pos,
    browser.isPosAtKey('c') ? clrActive : clrInactive,
    browser.isPosAtKey('c') ? clrActiveBg : clrInactiveBg);
  pos.y++;

  Renderer::drawTextCentered(
    "Options", panel_screen, pos,
    browser.isPosAtKey('d') ? clrActive : clrInactive,
    browser.isPosAtKey('d') ? clrActiveBg : clrInactiveBg);
  pos.y++;

  Renderer::drawTextCentered(
    "Credits", panel_screen, pos,
    browser.isPosAtKey('e') ? clrActive : clrInactive,
    browser.isPosAtKey('e') ? clrActiveBg : clrInactiveBg);
  pos.y++;

  Renderer::drawTextCentered(
    "High scores", panel_screen, pos,
    browser.isPosAtKey('f') ? clrActive : clrInactive,
    browser.isPosAtKey('f') ? clrActiveBg : clrInactiveBg);
  pos.y++;

  Renderer::drawTextCentered(
    "Escape to reality", panel_screen, pos,
    browser.isPosAtKey('g') ? clrActive : clrInactive,
    browser.isPosAtKey('g') ? clrActiveBg : clrInactiveBg);
  pos.y++;

  if(IS_DEBUG_MODE) {
    Renderer::drawTextCentered(
      "DEBUG: RUN BOT", panel_screen, pos,
      browser.isPosAtKey('h') ? clrActive : clrInactive,
      browser.isPosAtKey('h') ? clrActiveBg : clrInactiveBg);
    pos.y++;
  }

  const int BOX_Y1      = pos.y;
  const int BOX_W_HALF  = 10;
  const int BOX_X0      = pos.x - BOX_W_HALF;
  const int BOX_X1      = pos.x + BOX_W_HALF;
  Renderer::drawPopupBox(Rect(Pos(BOX_X0, BOX_Y0), Pos(BOX_X1, BOX_Y1)),
                             panel_screen);

  Renderer::drawTextCentered(
    gameVersionStr + " - " + __DATE__ + " (c) 2011-2014 Martin Tornqvist",
    panel_screen, Pos(MAP_W_HALF, SCREEN_H - 1), clrWhite);

  Renderer::updateScreen();

  trace << "MainMenu::draw() [DONE]" << endl;
}

GameEntryMode MainMenu::run(bool& quit, int& introMusChannel) {
  trace << "MainMenu::run()" << endl;

  quote = getHplQuote();

  MenuBrowser browser(IS_DEBUG_MODE ? 8 : 7, 0);

  introMusChannel = Audio::play(SfxId::musCthulhiana_Madness);

  draw(browser);

  while(true) {
    const MenuAction action = eng.menuInputHandler->getAction(browser);

    switch(action) {
      case menuAction_browsed: {
        draw(browser);
      } break;

      case menuAction_esc: {
        quit    = true;
        return gameEntry_new;
      } break;

      case menuAction_space:
      case menuAction_selectedWithShift: {} break;

      case menuAction_selected: {
        if(browser.isPosAtKey('a')) {return gameEntry_new;}
        if(browser.isPosAtKey('b')) {
          if(eng.saveHandler->isSaveAvailable()) {
            eng.saveHandler->load();
            eng.gameTime->insertActorInLoop(dynamic_cast<Actor*>(eng.player));
            eng.dungeonClimb->travelDown();
            return gameEntry_load;
          } else {
            eng.popup->showMsg("Starting a new character instead.", false,
                               "No save available");
            return gameEntry_new;
          }
        }
        if(browser.isPosAtKey('c')) {
          eng.manual->run();
          draw(browser);
        }
        if(browser.isPosAtKey('d')) {
          Config::runOptionsMenu(eng);
          draw(browser);
        }
        if(browser.isPosAtKey('e')) {
          eng.credits->run();
          draw(browser);
        }
        if(browser.isPosAtKey('f')) {
          eng.highScore->runHighScoreScreen();
          draw(browser);
        }
        if(browser.isPosAtKey('g')) {
          quit    = true;
          return gameEntry_new;
        }
        if(IS_DEBUG_MODE) {
          if(browser.isPosAtKey('h')) {
            Config::setBotPlaying();
            return gameEntry_new;
          }
        }
      } break;
    }
  }
  return gameEntry_new;
}

string MainMenu::getHplQuote() const {
  vector<string> quotes;
  quotes.resize(0);
  quotes.push_back("Happy is the tomb where no wizard hath lain and happy the town at night whose wizards are all ashes.");
  quotes.push_back("Our means of receiving impressions are absurdly few, and our notions of surrounding objects infinitely narrow. We see things only as we are constructed to see them, and can gain no idea of their absolute nature.");
  quotes.push_back("Disintegration is quite painless, I assure you.");
  quotes.push_back("I am writing this under an appreciable mental strain, since by tonight I shall be no more...");
  quotes.push_back("The end is near. I hear a noise at the door, as of some immense slippery body lumbering against it. It shall not find me...");
  quotes.push_back("Sometimes I believe that this less material life is our truer life, and that our vain presence on the terraqueous globe is itself the secondary or merely virtual phenomenon.");
  quotes.push_back("Life is a hideous thing, and from the background behind what we know of it peer daemoniacal hints of truth which make it sometimes a thousandfold more hideous.");
  quotes.push_back("Science, already oppressive with its shocking revelations, will perhaps be the ultimate exterminator of our human species, if separate species we be, for its reserve of unguessed horrors could never be borne by mortal brains if loosed upon the world....");
  quotes.push_back("Madness rides the star-wind... claws and teeth sharpened on centuries of corpses... dripping death astride a bacchanale of bats from nigh-black ruins of buried temples of Belial...");
  quotes.push_back("Memories and possibilities are ever more hideous than realities.");
  quotes.push_back("Yog-Sothoth knows the gate. Yog-Sothoth is the gate. Yog-Sothoth is the key and guardian of the gate. Past, present, future, all are one in Yog-Sothoth. He knows where the Old Ones broke through of old, and where They shall break through again.");
  quotes.push_back("Slowly but inexorably crawling upon my consciousness and rising above every other impression, came a dizzying fear of the unknown; not death, but some nameless, unheard-of thing inexpressibly more ghastly and abhorrent.");
  quotes.push_back("I felt that some horrible scene or object lurked beyond the silk-hung walls, and shrank from glancing through the arched, latticed windows that opened so bewilderingly on every hand.");
  quotes.push_back("There now ensued a series of incidents which transported me to the opposite extremes of ecstasy and horror; incidents which I tremble to recall and dare not seek to interpret...");
  quotes.push_back("From the new-flooded lands it flowed again, uncovering death and decay; and from its ancient and immemorial bed it trickled loathsomely, uncovering nighted secrets of the years when Time was young and the gods unborn.");
  quotes.push_back("The moon is dark, and the gods dance in the night; there is terror in the sky, for upon the moon hath sunk an eclipse foretold in no books of men or of earth's gods...");
  quotes.push_back("May the merciful gods, if indeed there be such, guard those hours when no power of the will can keep me from the chasm of sleep. With him who has come back out of the nethermost chambers of night, haggard and knowing, peace rests nevermore.");
  quotes.push_back("What I learned and saw in those hours of impious exploration can never be told, for want of symbols or suggestions in any language.");
  quotes.push_back("From even the greatest of horrors irony is seldom absent.");
  quotes.push_back("The most merciful thing in the world, I think, is the inability of the human mind to correlate all its contents.");
  quotes.push_back("In his house at R'lyeh dead Cthulhu waits dreaming.");
  quotes.push_back("Ph'nglui mglw'nafh Cthulhu R'lyeh wgah'nagl fhtagn");
  quotes.push_back("They worshipped, so they said, the Great Old Ones who lived ages before there were any men, and who came to the young world out of the sky...");
  quotes.push_back("That is not dead which can eternal lie, and with strange aeons even death may die.");
  quotes.push_back("I have looked upon all that the universe has to hold of horror, and even the skies of spring and the flowers of summer must ever afterward be poison to me. But I do not think my life will be long. I know too much, and the cult still lives.");
  quotes.push_back("Something terrible came to the hills and valleys on that meteor, and something terrible, though I know not in what proportion, still remains.");
  quotes.push_back("Man's respect for the imponderables varies according to his mental constitution and environment. Through certain modes of thought and training it can be elevated tremendously, yet there is always a limit.");
  quotes.push_back("As human beings, our only sensible scale of values is one based on lessening the agony of existence.");
  quotes.push_back("The oldest and strongest emotion of mankind is fear, and the oldest and strongest kind of fear is fear of the unknown.");
  quotes.push_back("I have seen the dark universe yawning, where the black planets roll without aim, where they roll in their horror unheeded, without knowledge, or lustre, or name.");
  quotes.push_back("Searchers after horror haunt strange, far places.");
  quotes.push_back("The sciences have hitherto harmed us little; but some day the piecing together of dissociated knowledge will open up such terrifying vistas of reality, that we shall either go mad from the revelation or flee from the deadly light into the peace and safety of a new dark age.");
  quotes.push_back("There are horrors beyond life's edge that we do not suspect, and once in a while man's evil prying calls them just within our range.");
  quotes.push_back("We live on a placid island of ignorance in the midst of black seas of infinity, and it was not meant that we should voyage far.");
  quotes.push_back("There are black zones of shadow close to our daily paths, and now and then some evil soul breaks a passage through. When that happens, the man who knows must strike before reckoning the consequences.");
  quotes.push_back("Non-Euclidean calculus and quantum physics are enough to stretch any brain; and when one mixes them with folklore, and tries to trace a strange background of multi-dimensional reality behind the ghoulish hints of Gothic tales and the wild whispers of the chimney-corner, one can hardly expect to be wholly free from mental tension.");
  quotes.push_back("I could not help feeling that they were evil things-- mountains of madness whose farther slopes looked out over some accursed ultimate abyss.");
  quotes.push_back("That seething, half luminous cloud background held ineffable suggestions of a vague, ethereal beyondness far more than terrestrially spatial; and gave appalling reminders of the utter remoteness, separateness, desolation, and aeon-long death of this untrodden and unfathomed austral world.");
  quotes.push_back("With five feeble senses we pretend to comprehend the boundlessly complex cosmos, yet other beings might not only see very differently, but might see and study whole worlds of matter, energy, and life which lie close at hand yet can never be detected with the senses we have.");
  quotes.push_back("It is absolutely necessary, for the peace and safety of mankind, that some of earth's dark, dead corners and unplumbed depths be left alone; lest sleeping abnormalities wake to resurgent life, and blasphemously surviving nightmares squirm and splash out of their black lairs to newer and wider conquests.");
  quotes.push_back("I felt myself on the edge of the world; peering over the rim into a fathomless chaos of eternal night.");
  quotes.push_back("And where Nyarlathotep went, rest vanished, for the small hours were rent with the screams of nightmare.");
  quotes.push_back("It was just a colour out of space - a frightful messenger from unformed realms of infinity beyond all Nature as we know it; from realms whose mere existence stuns the brain and numbs us with the black extra-cosmic gulfs it throws open before our frenzied eyes.");
  quotes.push_back("It lumbered slobberingly into sight and gropingly squeezed its gelatinous green immensity through the black doorway into the tainted outside air of that poison city of madness.");
  quotes.push_back("The Thing cannot be described - there is no language for such abysms of shrieking and immemorial lunacy, such eldritch contradictions of all matter, force, and cosmic order.");
  quotes.push_back("I could tell I was at the gateway of a region half-bewitched through the piling-up of unbroken time-accumulations; a region where old, strange things have had a chance to grow and linger because they have never been stirred up.");
  return "\"" + quotes.at(Rnd::range(0, quotes.size() - 1)) + "\"";
}
