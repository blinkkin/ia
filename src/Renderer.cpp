#include "Renderer.h"

#include <vector>
#include <iostream>

#include <SDL_image.h>

#include "Engine.h"
#include "Item.h"
#include "CharacterLines.h"
#include "Marker.h"
#include "Map.h"
#include "Actor.h"
#include "ActorPlayer.h"
#include "ActorMonster.h"
#include "Log.h"
#include "Attack.h"
#include "FeatureWall.h"
#include "FeatureDoor.h"
#include "Inventory.h"
#include "Utils.h"
#include "CommonData.h"
#include "SdlWrapper.h"

using namespace std;

namespace Renderer {

//---------------------------------------------------------------------- GLOBAL
CellRenderData  renderArray_[MAP_W][MAP_H];
CellRenderData  renderArrayNoActors_[MAP_W][MAP_H];
SDL_Surface*    screenSurface_       = NULL;
SDL_Surface*    mainMenuLogoSurface_ = NULL;

//------------------------------------------------------------------------LOCAL
namespace {
bool tilePixelData_[400][400];
bool fontPixelData_[400][400];

Engine* eng = NULL;

bool isInited() {
  return eng != NULL && screenSurface_ != NULL;
}

void loadMainMenuLogo() {
  trace << "Renderer::loadMainMenuLogo()..." << endl;

  SDL_Surface* mainMenuLogoSurfaceTmp = IMG_Load(mainMenuLogoImgName.data());

  mainMenuLogoSurface_ = SDL_DisplayFormatAlpha(mainMenuLogoSurfaceTmp);

  SDL_FreeSurface(mainMenuLogoSurfaceTmp);

  trace << "Renderer::loadMainMenuLogo() [DONE]" << endl;
}

Uint32 getPixel(SDL_Surface* const surface,
                const int PIXEL_X, const int PIXEL_Y) {
  int bpp = surface->format->BytesPerPixel;
  /* Here p is the address to the pixel we want to retrieve */
  Uint8* p = (Uint8*)surface->pixels + PIXEL_Y * surface->pitch + PIXEL_X * bpp;

  switch(bpp) {
    case 1:   return *p;          break;
    case 2:   return *(Uint16*)p; break;
    case 3: {
      if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
        return p[0] << 16 | p[1] << 8 | p[2];
      }   else {
        return p[0] | p[1] << 8 | p[2] << 16;
      }
    } break;
    case 4:   return *(Uint32*)p; break;
    default:  return -1;          break;
  }
  return -1;
}

void putPixel(SDL_Surface* const surface,
              const int PIXEL_X, const int PIXEL_Y, Uint32 pixel) {
  int bpp = surface->format->BytesPerPixel;
  /* Here p is the address to the pixel we want to set */
  Uint8* p = (Uint8*)surface->pixels + PIXEL_Y * surface->pitch + PIXEL_X * bpp;

  switch(bpp) {
    case 1:   *p = pixel;             break;
    case 2:   *(Uint16*)p = pixel;    break;
    case 3: {
      if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
        p[0] = (pixel >> 16) & 0xff;
        p[1] = (pixel >> 8) & 0xff;
        p[2] = pixel & 0xff;
      } else {
        p[0] = pixel & 0xff;
        p[1] = (pixel >> 8) & 0xff;
        p[2] = (pixel >> 16) & 0xff;
      }
    } break;
    case 4:   *(Uint32*)p = pixel;    break;
    default:  {}                      break;
  }
}

void loadFont() {
  trace << "Renderer::loadFont()..." << endl;

  SDL_Surface* fontSurfaceTmp = IMG_Load(Config::getFontName().data());

  Uint32 imgClr = SDL_MapRGB(fontSurfaceTmp->format, 255, 255, 255);

  for(int y = 0; y < fontSurfaceTmp->h; y++) {
    for(int x = 0; x < fontSurfaceTmp->w; x++) {
      fontPixelData_[x][y] = getPixel(fontSurfaceTmp, x, y) == imgClr;
    }
  }

  SDL_FreeSurface(fontSurfaceTmp);

  trace << "Renderer::loadFont() [DONE]" << endl;
}

void loadTiles() {
  trace << "Renderer::loadTiles()..." << endl;

  SDL_Surface* tileSurfaceTmp = IMG_Load(tilesImgName.data());

  Uint32 imgClr = SDL_MapRGB(tileSurfaceTmp->format, 255, 255, 255);
  for(int y = 0; y < tileSurfaceTmp->h; y++) {
    for(int x = 0; x < tileSurfaceTmp->w; x++) {
      tilePixelData_[x][y] = getPixel(tileSurfaceTmp, x, y) == imgClr;
    }
  }

  SDL_FreeSurface(tileSurfaceTmp);

  trace << "Renderer::loadTiles() [DONE]" << endl;
}

void putPixelsOnScreenForTile(const TileId tile, const Pos& pixelPos,
                              const SDL_Color& clr) {
  if(isInited()) {
    const int CLR_TO = SDL_MapRGB(screenSurface_->format, clr.r, clr.g, clr.b);

    SDL_LockSurface(screenSurface_);

    const int CELL_W = Config::getCellW();
    const int CELL_H = Config::getCellH();

    const Pos sheetPoss = eng->art->getTilePoss(tile);
    const int SHEET_X0  = sheetPoss.x * CELL_W;
    const int SHEET_Y0  = sheetPoss.y * CELL_H;
    const int SHEET_X1  = SHEET_X0 + CELL_W - 1;
    const int SHEET_Y1  = SHEET_Y0 + CELL_H - 1;
    const int SCREEN_X0 = pixelPos.x;
    const int SCREEN_Y0 = pixelPos.y;

    int screenX = SCREEN_X0;
    int screenY = SCREEN_Y0;

    for(int sheetY = SHEET_Y0; sheetY <= SHEET_Y1; sheetY++) {
      screenX = SCREEN_X0;
      for(int sheetX = SHEET_X0; sheetX <= SHEET_X1; sheetX++) {
        if(tilePixelData_[sheetX][sheetY]) {
          putPixel(screenSurface_, screenX, screenY, CLR_TO);
        }
        screenX++;
      }
      screenY++;
    }

    SDL_UnlockSurface(screenSurface_);
  }
}

void putPixelsOnScreenForGlyph(const char GLYPH, const Pos& pixelPos,
                               const SDL_Color& clr) {
  const int CLR_TO = SDL_MapRGB(screenSurface_->format, clr.r, clr.g, clr.b);

  SDL_LockSurface(screenSurface_);

  const int CELL_W = Config::getCellW();
  const int CELL_H = Config::getCellH();

  const int SCALE = 1;

  const int CELL_W_SHEET = CELL_W / SCALE;
  const int CELL_H_SHEET = CELL_H / SCALE;

  const Pos sheetPoss = eng->art->getGlyphPoss(GLYPH);
  const int SHEET_X0  = sheetPoss.x * CELL_W_SHEET;
  const int SHEET_Y0  = sheetPoss.y * CELL_H_SHEET;
  const int SHEET_X1  = SHEET_X0 + CELL_W_SHEET - 1;
  const int SHEET_Y1  = SHEET_Y0 + CELL_H_SHEET - 1;
  const int SCREEN_X0 = pixelPos.x;
  const int SCREEN_Y0 = pixelPos.y;

  int screenX = SCREEN_X0;
  int screenY = SCREEN_Y0;

  for(int sheetY = SHEET_Y0; sheetY <= SHEET_Y1; sheetY++) {
    screenX = SCREEN_X0;
    for(int sheetX = SHEET_X0; sheetX <= SHEET_X1; sheetX++) {
      if(fontPixelData_[sheetX][sheetY]) {
        for(int dy = 0; dy < SCALE; dy++) {
          for(int dx = 0; dx < SCALE; dx++) {
            putPixel(screenSurface_, screenX + dx, screenY + dy, CLR_TO);
          }
        }
      }
      screenX += SCALE;
    }
    screenY += SCALE;
  }

  SDL_UnlockSurface(screenSurface_);
}

void drawGlyphAtPixel(const char GLYPH, const Pos& pixelPos,
                      const SDL_Color& clr, const bool DRAW_BG_CLR,
                      const SDL_Color& bgClr = clrBlack) {
  if(DRAW_BG_CLR) {
    const Pos cellDims(Config::getCellW(), Config::getCellH());
    drawRectangleSolid(pixelPos, cellDims, bgClr);
  }

  putPixelsOnScreenForGlyph(GLYPH, pixelPos, clr);
}

Pos getPixelPosForCellInPanel(const PanelId panel, const Pos& pos) {
  const Pos cellDims(Config::getCellW(), Config::getCellH());

  switch(panel) {
    case panel_screen: {
      return Pos(pos.x * cellDims.x, pos.y * cellDims.y);
    } break;

    case panel_map: {
      return (pos * cellDims) + Pos(0, Config::getMapPixelOffsetH());
    } break;

    case panel_log: {
      return pos * cellDims;
    } break;

    case panel_char: {
      return (pos * cellDims) + Pos(0, Config::getCharLinesPixelOffsetH());
    } break;
  }
  return Pos();
}

int getLifebarLength(const Actor& actor) {
  const int ACTOR_HP = max(0, actor.getHp());
  const int ACTOR_HP_MAX = actor.getHpMax(true);
  if(ACTOR_HP < ACTOR_HP_MAX) {
    int HP_PERCENT = (ACTOR_HP * 100) / ACTOR_HP_MAX;
    return ((Config::getCellW() - 2) * HP_PERCENT) / 100;
  }
  return -1;
}

void drawLifeBar(const Pos& pos, const int LENGTH) {
  if(LENGTH >= 0) {
    const Pos cellDims(Config::getCellW(),  Config::getCellH());
    const int W_GREEN   = LENGTH;
    const int W_BAR_TOT = cellDims.x - 2;
    const int W_RED     = W_BAR_TOT - W_GREEN;
    const Pos pixelPos =
      getPixelPosForCellInPanel(panel_map, pos + Pos(0, 1)) - Pos(0, 2);
    const int X0_GREEN  = pixelPos.x + 1;
    const int X0_RED    = X0_GREEN + W_GREEN;

    if(W_GREEN > 0) {
      drawLineHor(Pos(X0_GREEN, pixelPos.y), W_GREEN, clrGreenLgt);
    }
    if(W_RED > 0) {
      drawLineHor(Pos(X0_RED, pixelPos.y), W_RED, clrRedLgt);
    }
  }
}

void drawExclMarkAt(const Pos& pixelPos) {
  drawRectangleSolid(pixelPos,  Pos(3, 12),     clrBlack);
  drawLineVer(pixelPos +        Pos(1,  1), 6,  clrMagentaLgt);
  drawLineVer(pixelPos +        Pos(1,  9), 2,  clrMagentaLgt);
}

void drawPlayerShockExclMarks() {
  const double SHOCK  = eng->player->getPermShockTakenCurTurn();
  const int NR_EXCL   = SHOCK > 8 ? 3 : SHOCK > 3 ? 2 : SHOCK > 1 ? 1 : 0;

  if(NR_EXCL > 0) {
    const Pos& playerPos = eng->player->pos;
    const Pos pixelPosRight =
      getPixelPosForCellInPanel(panel_map, playerPos);

    for(int i = 0; i < NR_EXCL; i++) {
      drawExclMarkAt(pixelPosRight + Pos(i * 3, 0));
    }
  }
}

} //Namespace

//---------------------------------------------------------------------- GLOBAL
void init(Engine& engine) {
  trace << "Renderer::init()..." << endl;
  cleanup();

  eng = &engine;

  trace << "Renderer: Setting up rendering window" << endl;
  const string title = "IA " + gameVersionStr;
  SDL_WM_SetCaption(title.data(), NULL);

  const int W = Config::getScreenPixelW();
  const int H = Config::getScreenPixelH();
  if(Config::isFullscreen()) {
    screenSurface_ =
      SDL_SetVideoMode(W, H, SCREEN_BPP, SDL_SWSURFACE | SDL_FULLSCREEN);
  }
  if(Config::isFullscreen() == false || screenSurface_ == NULL) {
    screenSurface_ =
      SDL_SetVideoMode(W, H, SCREEN_BPP, SDL_SWSURFACE);
  }

  if(screenSurface_ == NULL) {
    trace << "[WARNING] Failed to create screen surface, ";
    trace << "in Renderer::init()" << endl;
  }

  loadFont();

  if(Config::isTilesMode()) {
    loadTiles();
    loadMainMenuLogo();
  }

  trace << "Renderer::init() [DONE]" << endl;
}

void cleanup() {
  trace << "Renderer::cleanup()..." << endl;

  eng = NULL;

  if(screenSurface_ != NULL) {
    SDL_FreeSurface(screenSurface_);
    screenSurface_ = NULL;
  }

  if(mainMenuLogoSurface_ != NULL) {
    SDL_FreeSurface(mainMenuLogoSurface_);
    mainMenuLogoSurface_ = NULL;
  }

  trace << "Renderer::cleanup() [DONE]" << endl;
}

void updateScreen() {
  if(isInited()) {SDL_Flip(screenSurface_);}
}

void clearScreen() {
  if(isInited()) {
    SDL_FillRect(screenSurface_, NULL,
                 SDL_MapRGB(screenSurface_->format, 0, 0, 0));
  }
}

void applySurface(const Pos& pixelPos, SDL_Surface* const src,
                  SDL_Rect* clip) {
  if(isInited()) {
    SDL_Rect offset;
    offset.x = pixelPos.x;
    offset.y = pixelPos.y;
    SDL_BlitSurface(src, clip, screenSurface_, &offset);
  }
}

void drawMainMenuLogo(const int Y_POS) {
  const Pos pos((Config::getScreenPixelW() - mainMenuLogoSurface_->w) / 2,
                Config::getCellH() * Y_POS);
  applySurface(pos, mainMenuLogoSurface_);
}

void drawMarker(const vector<Pos>& trail, const int EFFECTIVE_RANGE) {
  if(trail.size() > 2) {
    for(size_t i = 1; i < trail.size(); i++) {
      const Pos& pos = trail.at(i);
      coverCellInMap(pos);

      SDL_Color clr = clrGreenLgt;

      if(EFFECTIVE_RANGE != -1) {
        const int CHEB_DIST = Utils::chebyshevDist(trail.at(0), pos);
        if(CHEB_DIST > EFFECTIVE_RANGE) {clr = clrYellow;}
      }
      if(Config::isTilesMode()) {
        drawTile(tile_aimMarkerTrail, panel_map, pos, clr, clrBlack);
      } else {
        drawGlyph('*', panel_map, pos, clr, true, clrBlack);
      }
    }
  }

  const Pos& headPos = eng->marker->getPos();

  SDL_Color clr = clrGreenLgt;

  if(trail.size() > 2) {
    if(EFFECTIVE_RANGE != -1) {
      const int CHEB_DIST = Utils::chebyshevDist(trail.at(0), headPos);
      if(CHEB_DIST > EFFECTIVE_RANGE) {
        clr = clrYellow;
      }
    }
  }

  if(Config::isTilesMode()) {
    drawTile(tile_aimMarkerHead, panel_map, headPos, clr, clrBlack);
  } else {
    drawGlyph('X', panel_map, headPos, clr, true, clrBlack);
  }
}

void drawBlastAnimAtField(const Pos& centerPos, const int RADIUS,
                          bool forbiddenCells[MAP_W][MAP_H],
                          const SDL_Color& colorInner,
                          const SDL_Color& colorOuter) {
  trace << "Renderer::drawBlastAnimAtField()..." << endl;
  if(isInited()) {
    drawMapAndInterface();

    bool isAnyBlastRendered = false;

    Pos pos;

    for(
      pos.y = max(1, centerPos.y - RADIUS);
      pos.y <= min(MAP_H - 2, centerPos.y + RADIUS);
      pos.y++) {
      for(
        pos.x = max(1, centerPos.x - RADIUS);
        pos.x <= min(MAP_W - 2, centerPos.x + RADIUS);
        pos.x++) {
        if(forbiddenCells[pos.x][pos.y] == false) {
          const bool IS_OUTER = pos.x == centerPos.x - RADIUS ||
                                pos.x == centerPos.x + RADIUS ||
                                pos.y == centerPos.y - RADIUS ||
                                pos.y == centerPos.y + RADIUS;
          const SDL_Color color = IS_OUTER ? colorOuter : colorInner;
          if(Config::isTilesMode()) {
            drawTile(tile_blast1, panel_map, pos, color, clrBlack);
          } else {
            drawGlyph('*', panel_map, pos, color, true, clrBlack);
          }
          isAnyBlastRendered = true;
        }
      }
    }
    updateScreen();
    if(isAnyBlastRendered) {SdlWrapper::sleep(Config::getDelayExplosion() / 2);}

    for(
      pos.y = max(1, centerPos.y - RADIUS);
      pos.y <= min(MAP_H - 2, centerPos.y + RADIUS);
      pos.y++) {
      for(
        pos.x = max(1, centerPos.x - RADIUS);
        pos.x <= min(MAP_W - 2, centerPos.x + RADIUS);
        pos.x++) {
        if(forbiddenCells[pos.x][pos.y] == false) {
          const bool IS_OUTER = pos.x == centerPos.x - RADIUS ||
                                pos.x == centerPos.x + RADIUS ||
                                pos.y == centerPos.y - RADIUS ||
                                pos.y == centerPos.y + RADIUS;
          const SDL_Color color = IS_OUTER ? colorOuter : colorInner;
          if(Config::isTilesMode()) {
            drawTile(tile_blast2, panel_map, pos, color, clrBlack);
          } else {
            drawGlyph('*', panel_map, pos, color, true, clrBlack);
          }
        }
      }
    }
    updateScreen();
    if(isAnyBlastRendered) {SdlWrapper::sleep(Config::getDelayExplosion() / 2);}
    drawMapAndInterface();
  }
  trace << "Renderer::drawBlastAnimAtField() [DONE]" << endl;
}

void drawBlastAnimAtPositions(const vector<Pos>& positions,
                              const SDL_Color& color) {
  trace << "Renderer::drawBlastAnimAtPositions()..." << endl;
  if(isInited()) {
    drawMapAndInterface();

    for(unsigned int i = 0; i < positions.size(); i++) {
      const Pos& pos = positions.at(i);
      if(Config::isTilesMode()) {
        drawTile(tile_blast1, panel_map, pos, color, clrBlack);
      } else {
        drawGlyph('*', panel_map, pos, color, true, clrBlack);
      }
    }
    updateScreen();
    SdlWrapper::sleep(Config::getDelayExplosion() / 2);

    for(unsigned int i = 0; i < positions.size(); i++) {
      const Pos& pos = positions.at(i);
      if(Config::isTilesMode()) {
        drawTile(tile_blast2, panel_map, pos, color, clrBlack);
      } else {
        drawGlyph('*', panel_map, pos, color, true, clrBlack);
      }
    }
    updateScreen();
    SdlWrapper::sleep(Config::getDelayExplosion() / 2);
    drawMapAndInterface();
  }
  trace << "Renderer::drawBlastAnimAtPositions() [DONE]" << endl;
}

void drawBlastAnimAtPositionsWithPlayerVision(
  const vector<Pos>& positions, const SDL_Color& clr) {

  vector<Pos> positionsWithVision;
  for(unsigned int i = 0; i < positions.size(); i++) {
    const Pos& pos = positions.at(i);
    if(eng->map->cells[pos.x][pos.y].isSeenByPlayer) {
      positionsWithVision.push_back(pos);
    }
  }

  Renderer::drawBlastAnimAtPositions(positionsWithVision, clr);
}

void drawTile(const TileId tile, const PanelId panel, const Pos& pos,
              const SDL_Color& clr, const SDL_Color& bgClr) {
  const Pos pixelPos = getPixelPosForCellInPanel(panel, pos);
  const Pos cellDims(Config::getCellW(), Config::getCellH());
  drawRectangleSolid(pixelPos, cellDims, bgClr);
  putPixelsOnScreenForTile(tile, pixelPos, clr);
}

void drawGlyph(const char GLYPH, const PanelId panel, const Pos& pos,
               const SDL_Color& clr, const bool DRAW_BG_CLR,
               const SDL_Color& bgClr) {
  const Pos pixelPos = getPixelPosForCellInPanel(panel, pos);
  drawGlyphAtPixel(GLYPH, pixelPos, clr, DRAW_BG_CLR, bgClr);
}

void drawText(const string& str, const PanelId panel, const Pos& pos,
              const SDL_Color& clr, const SDL_Color& bgClr) {
  Pos pixelPos = getPixelPosForCellInPanel(panel, pos);

  if(pixelPos.y < 0 || pixelPos.y >= Config::getScreenPixelH()) {
    return;
  }

  const Pos cellDims(Config::getCellW(), Config::getCellH());
  const int LEN = str.size();
  drawRectangleSolid(pixelPos, Pos(cellDims.x * LEN, cellDims.y), bgClr);

  for(int i = 0; i < LEN; i++) {
    if(pixelPos.x < 0 || pixelPos.x >= Config::getScreenPixelW()) {
      return;
    }
    drawGlyphAtPixel(str.at(i), pixelPos, clr, false);
    pixelPos.x += cellDims.x;
  }
}

int drawTextCentered(const string& str, const PanelId panel,
                     const Pos& pos, const SDL_Color& clr,
                     const SDL_Color& bgClr,
                     const bool IS_PIXEL_POS_ADJ_ALLOWED) {
  const int LEN         = str.size();
  const int LEN_HALF    = LEN / 2;
  const int X_POS_LEFT  = pos.x - LEN_HALF;

  const Pos cellDims(Config::getCellW(), Config::getCellH());

  Pos pixelPos = getPixelPosForCellInPanel(panel, Pos(X_POS_LEFT, pos.y));

  if(IS_PIXEL_POS_ADJ_ALLOWED) {
    const int PIXEL_X_ADJ = LEN_HALF * 2 == LEN ? cellDims.x / 2 : 0;
    pixelPos += Pos(PIXEL_X_ADJ, 0);
  }

  const int W_TOT_PIXEL = LEN * cellDims.x;

  SDL_Rect sdlRect = {
    (Sint16)pixelPos.x, (Sint16)pixelPos.y,
    (Uint16)W_TOT_PIXEL, (Uint16)cellDims.y
  };
  SDL_FillRect(screenSurface_, &sdlRect, SDL_MapRGB(screenSurface_->format,
               bgClr.r, bgClr.g, bgClr.b));

  for(int i = 0; i < LEN; i++) {
    if(pixelPos.x < 0 || pixelPos.x >= Config::getScreenPixelW()) {
      return X_POS_LEFT;
    }
    drawGlyphAtPixel(str.at(i), pixelPos, clr, false, bgClr);
    pixelPos.x += cellDims.x;
  }
  return X_POS_LEFT;
}

void coverPanel(const PanelId panel) {
  const int SCREEN_PIXEL_W = Config::getScreenPixelW();

  switch(panel) {
    case panel_char: {
      const Pos pixelPos = getPixelPosForCellInPanel(panel, Pos(0, 0));
      coverAreaPixel(pixelPos,
                     Pos(SCREEN_PIXEL_W, Config::getCharLinesPixelH()));
    } break;

    case panel_log: {
      coverAreaPixel(Pos(0, 0), Pos(SCREEN_PIXEL_W, Config::getLogPixelH()));
    } break;

    case panel_map: {
      const Pos pixelPos = getPixelPosForCellInPanel(panel, Pos(0, 0));
      coverAreaPixel(pixelPos, Pos(SCREEN_PIXEL_W, Config::getMapPixelH()));
    } break;

    case panel_screen: {clearScreen();} break;
  }
}

void coverArea(const PanelId panel, const Pos& pos, const Pos& dims) {
  const Pos pixelPos = getPixelPosForCellInPanel(panel, pos);
  const Pos cellDims(Config::getCellW(), Config::getCellH());
  coverAreaPixel(pixelPos, dims * cellDims);
}

void coverAreaPixel(const Pos& pixelPos, const Pos& pixelDims) {
  drawRectangleSolid(pixelPos, pixelDims, clrBlack);
}

void coverCellInMap(const Pos& pos) {
  const Pos cellDims(Config::getCellW(), Config::getCellH());
  Pos pixelPos = getPixelPosForCellInPanel(panel_map, pos);
  coverAreaPixel(pixelPos, cellDims);
}

void drawLineHor(const Pos& pixelPos, const int W,
                 const SDL_Color& clr) {
  const int SCALE = 1;
  const Pos offset(0, 1 - SCALE);
  drawRectangleSolid(pixelPos + offset, Pos(W, 2 * SCALE), clr);
}

void drawLineVer(const Pos& pixelPos, const int H,
                 const SDL_Color& clr) {
  drawRectangleSolid(pixelPos, Pos(1, H), clr);
}

void drawRectangleSolid(const Pos& pixelPos, const Pos& pixelDims,
                        const SDL_Color& clr) {
  if(isInited()) {
    SDL_Rect sdlRect = {(Sint16)pixelPos.x, (Sint16)pixelPos.y,
                        (Uint16)pixelDims.x, (Uint16)pixelDims.y
                       };
    SDL_FillRect(screenSurface_, &sdlRect,
                 SDL_MapRGB(screenSurface_->format, clr.r, clr.g, clr.b));
  }
}

void drawProjectiles(vector<Projectile*>& projectiles,
                     const bool SHOULD_DRAW_MAP_BEFORE) {

  if(SHOULD_DRAW_MAP_BEFORE) {drawMapAndInterface(false);}

  for(Projectile * p : projectiles) {
    if(p->isDoneRendering == false && p->isVisibleToPlayer) {
      coverCellInMap(p->pos);
      if(Config::isTilesMode()) {
        if(p->tile != tile_empty) {
          drawTile(p->tile, panel_map, p->pos, p->clr);
        }
      } else {
        if(p->glyph != -1) {
          drawGlyph(p->glyph, panel_map, p->pos, p->clr);
        }
      }
    }
  }

  updateScreen();
}

void drawPopupBox(const Rect& border, const PanelId panel,
                  const SDL_Color& clr) {
  const bool IS_TILES = Config::isTilesMode();

  //Vertical bars
  const int Y0_VERT = border.x0y0.y + 1;
  const int Y1_VERT = border.x1y1.y - 1;
  for(int y = Y0_VERT; y <= Y1_VERT; y++) {
    if(IS_TILES) {
      drawTile(tile_popupVerticalBar,
               panel, Pos(border.x0y0.x, y), clr, clrBlack);
      drawTile(tile_popupVerticalBar,
               panel, Pos(border.x1y1.x, y), clr, clrBlack);
    } else {
      drawGlyph('|',
                panel, Pos(border.x0y0.x, y), clr, true, clrBlack);
      drawGlyph('|',
                panel, Pos(border.x1y1.x, y), clr, true, clrBlack);
    }
  }

  //Horizontal bars
  const int X0_VERT = border.x0y0.x + 1;
  const int X1_VERT = border.x1y1.x - 1;
  for(int x = X0_VERT; x <= X1_VERT; x++) {
    if(IS_TILES) {
      drawTile(tile_popupHorizontalBar,
               panel, Pos(x, border.x0y0.y), clr, clrBlack);
      drawTile(tile_popupHorizontalBar,
               panel, Pos(x, border.x1y1.y), clr, clrBlack);
    } else {
      drawGlyph('-', panel, Pos(x, border.x0y0.y), clr, true, clrBlack);
      drawGlyph('-', panel, Pos(x, border.x1y1.y), clr, true, clrBlack);
    }
  }

  //Corners
  if(IS_TILES) {
    drawTile(tile_popupCornerTopLeft,
             panel, Pos(border.x0y0.x, border.x0y0.y), clr, clrBlack);
    drawTile(tile_popupCornerTopRight,
             panel, Pos(border.x1y1.x, border.x0y0.y), clr, clrBlack);
    drawTile(tile_popupCornerBottomLeft,
             panel, Pos(border.x0y0.x, border.x1y1.y), clr, clrBlack);
    drawTile(tile_popupCornerBottomRight,
             panel, Pos(border.x1y1.x, border.x1y1.y), clr, clrBlack);
  } else {
    drawGlyph(
      '+', panel, Pos(border.x0y0.x, border.x0y0.y), clr, true, clrBlack);
    drawGlyph(
      '+', panel, Pos(border.x1y1.x, border.x0y0.y), clr, true, clrBlack);
    drawGlyph(
      '+', panel, Pos(border.x0y0.x, border.x1y1.y), clr, true, clrBlack);
    drawGlyph(
      '+', panel, Pos(border.x1y1.x, border.x1y1.y), clr, true, clrBlack);
  }
}

void drawMapAndInterface(const bool SHOULD_UPDATE_SCREEN) {
  if(isInited()) {
    clearScreen();

    drawMap();

    CharacterLines::drawInfoLines(*eng);
    CharacterLines::drawLocationInfo(*eng);
    eng->log->drawLog(false);

    if(SHOULD_UPDATE_SCREEN) {updateScreen();}
  }
}

void drawMap() {
  if(isInited()) {
    CellRenderData* curDrw = NULL;
    CellRenderData tmpDrw;

    const bool IS_TILES = Config::isTilesMode();

    //---------------- INSERT STATIC FEATURES AND BLOOD INTO ARRAY
    for(int y = 0; y < MAP_H; y++) {
      for(int x = 0; x < MAP_W; x++) {

        if(eng->map->cells[x][y].isSeenByPlayer) {

          curDrw = &renderArray_[x][y];
          curDrw->clear();

          const FeatureStatic* const f = eng->map->cells[x][y].featureStatic;

          TileId  goreTile  = tile_empty;
          char    goreGlyph = ' ';
          if(f->canHaveGore()) {
            goreTile  = f->getGoreTile();
            goreGlyph = f->getGoreGlyph();
          }
          if(goreTile == tile_empty) {
            curDrw->tile  = f->getTile();
            curDrw->glyph = f->getGlyph();
            const SDL_Color& featureClr   = f->getClr();
            const SDL_Color& featureClrBg = f->getClrBg();
            curDrw->clr = f->hasBlood() ? clrRedLgt : featureClr;
            if(Utils::isClrEq(featureClrBg, clrBlack) == false) {
              curDrw->clrBg = featureClrBg;
            }
          } else {
            curDrw->tile  = goreTile;
            curDrw->glyph = goreGlyph;
            curDrw->clr   = clrRed;
          }
          if(eng->map->cells[x][y].isLight) {
            if(f->canMoveCmn()) {
              curDrw->isMarkedAsLit = true;
            }
          }
        }
      }
    }

    int xPos, yPos;
    //---------------- INSERT DEAD ACTORS INTO ARRAY
    for(Actor * actor : eng->gameTime->actors_) {
      xPos = actor->pos.x;
      yPos = actor->pos.y;
      if(
        actor->deadState == ActorDeadState::corpse &&
        actor->getData().glyph != ' ' &&
        actor->getData().tile != tile_empty &&
        eng->map->cells[xPos][yPos].isSeenByPlayer) {
        curDrw = &renderArray_[xPos][yPos];
        curDrw->clr   = clrRed;
        curDrw->tile  = actor->getTile();
        curDrw->glyph = actor->getGlyph();
      }
    }

    for(int y = 0; y < MAP_H; y++) {
      for(int x = 0; x < MAP_W; x++) {
        curDrw = &renderArray_[x][y];
        if(eng->map->cells[x][y].isSeenByPlayer) {
          //---------------- INSERT ITEMS INTO ARRAY
          const Item* const item = eng->map->cells[x][y].item;
          if(item != NULL) {
            curDrw->clr   = item->getClr();
            curDrw->tile  = item->getTile();
            curDrw->glyph = item->getGlyph();
          }

          //COPY ARRAY TO PLAYER MEMORY (BEFORE LIVING ACTORS AND MOBILE FEATURES)
          renderArrayNoActors_[x][y] = renderArray_[x][y];

          //COLOR CELLS MARKED AS LIT YELLOW
          if(curDrw->isMarkedAsLit) {
            curDrw->clr = clrYellow;
          }
        }
      }
    }

    //---------------- INSERT MOBILE FEATURES INTO ARRAY
    for(FeatureMob * mob : eng->gameTime->featureMobs_) {
      xPos = mob->getX();
      yPos = mob->getY();
      const TileId  mobTile   = mob->getTile();
      const char    mobGlyph  = mob->getGlyph();
      if(
        mobTile != tile_empty && mobGlyph != ' ' &&
        eng->map->cells[xPos][yPos].isSeenByPlayer) {
        curDrw = &renderArray_[xPos][yPos];
        curDrw->clr = mob->getClr();
        curDrw->tile  = mobTile;
        curDrw->glyph = mobGlyph;
      }
    }

    //---------------- INSERT LIVING ACTORS INTO ARRAY
    for(Actor * actor : eng->gameTime->actors_) {
      if(actor != eng->player) {
        xPos = actor->pos.x;
        yPos = actor->pos.y;

        if(actor->deadState == ActorDeadState::alive) {

          curDrw = &renderArray_[xPos][yPos];

          const Monster* const monster = dynamic_cast<const Monster*>(actor);

          if(eng->player->isSeeingActor(*actor, NULL)) {

            if(
              actor->getTile()  != tile_empty &&
              actor->getGlyph() != ' ') {

              curDrw->clr   = actor->getClr();
              curDrw->tile  = actor->getTile();
              curDrw->glyph = actor->getGlyph();

              curDrw->lifebarLength = getLifebarLength(*actor);
              curDrw->isLivingActorSeenHere = true;
              curDrw->isFadeEffectAllowed = false;

              if(monster->leader == eng->player) {
                // TODO reimplement allied indicator
              } else {
                if(monster->awareOfPlayerCounter_ <= 0) {
                  curDrw->clrBg = clrBlue;
                }
              }
            }
          } else {
            if(monster->playerAwareOfMeCounter_ > 0) {
              curDrw->isAwareOfMonsterHere  = true;
            }
          }
        }
      }
    }

    //---------------- DRAW THE GRID
    for(int y = 0; y < MAP_H; y++) {
      for(int x = 0; x < MAP_W; x++) {

        tmpDrw = renderArray_[x][y];

        if(eng->map->cells[x][y].isSeenByPlayer) {
          if(tmpDrw.isFadeEffectAllowed) {
            const int DIST_FROM_PLAYER =
              Utils::chebyshevDist(eng->player->pos, Pos(x, y));
            if(DIST_FROM_PLAYER > 1) {
              const double DIST_FADE_DIV =
                min(2.0, 1.0 + (double(DIST_FROM_PLAYER - 1) * 0.33));
              tmpDrw.clr.r /= DIST_FADE_DIV;
              tmpDrw.clr.g /= DIST_FADE_DIV;
              tmpDrw.clr.b /= DIST_FADE_DIV;
            }
          }
        } else if(eng->map->cells[x][y].isExplored) {
          bool isAwareOfMonsterHere = tmpDrw.isAwareOfMonsterHere;
          renderArray_[x][y] = eng->map->cells[x][y].playerVisualMemory;
          tmpDrw = renderArray_[x][y];
          tmpDrw.isAwareOfMonsterHere = isAwareOfMonsterHere;

          tmpDrw.clr.r   /= 4; tmpDrw.clr.g   /= 4; tmpDrw.clr.b   /= 4;
          tmpDrw.clrBg.r /= 4; tmpDrw.clrBg.g /= 4; tmpDrw.clrBg.b /= 4;
        }

        if(IS_TILES) {
          //Walls are given perspective.
          //If the tile to be set is a (top) wall tile, check the tile beneath it.
          //If the tile beneath is not a front or top wall tile, and that cell is
          //explored, change the current tile to wall front
          if(
            tmpDrw.isLivingActorSeenHere == false &&
            tmpDrw.isAwareOfMonsterHere  == false) {
            const TileId tileSeen = renderArrayNoActors_[x][y].tile;
            const TileId tileMem  = eng->map->cells[x][y].playerVisualMemory.tile;
            const bool IS_TILE_WALL =
              eng->map->cells[x][y].isSeenByPlayer ?
              Wall::isTileAnyWallTop(tileSeen) :
              Wall::isTileAnyWallTop(tileMem);
            if(IS_TILE_WALL) {
              const Feature* const f = eng->map->cells[x][y].featureStatic;
              const FeatureId featureId = f->getId();
              bool isHiddenDoor = false;
              if(featureId == feature_door) {
                isHiddenDoor = dynamic_cast<const Door*>(f)->isSecret();
              }
              if(
                y < MAP_H - 1 &&
                (featureId == feature_stoneWall || isHiddenDoor)) {
                if(eng->map->cells[x][y + 1].isExplored) {
                  const bool IS_CELL_BELOW_SEEN =
                    eng->map->cells[x][y + 1].isSeenByPlayer;

                  const TileId tileBelowSeen =
                    renderArrayNoActors_[x][y + 1].tile;

                  const TileId tileBelowMem =
                    eng->map->cells[x][y + 1].playerVisualMemory.tile;

                  const bool TILE_BELOW_IS_WALL_FRONT =
                    IS_CELL_BELOW_SEEN ? Wall::isTileAnyWallFront(tileBelowSeen) :
                    Wall::isTileAnyWallFront(tileBelowMem);

                  const bool TILE_BELOW_IS_WALL_TOP =
                    IS_CELL_BELOW_SEEN ? Wall::isTileAnyWallTop(tileBelowSeen) :
                    Wall::isTileAnyWallTop(tileBelowMem);

                  bool tileBelowIsRevealedDoor =
                    IS_CELL_BELOW_SEEN ? Door::isTileAnyDoor(tileBelowSeen) :
                    Door::isTileAnyDoor(tileBelowMem);

                  if(
                    TILE_BELOW_IS_WALL_FRONT  ||
                    TILE_BELOW_IS_WALL_TOP    ||
                    tileBelowIsRevealedDoor) {
                    if(featureId == feature_stoneWall) {
                      const Wall* const wall = dynamic_cast<const Wall*>(f);
                      tmpDrw.tile = wall->getTopWallTile();
                    }
                  } else if(featureId == feature_stoneWall) {
                    const Wall* const wall = dynamic_cast<const Wall*>(f);
                    tmpDrw.tile = wall->getFrontWallTile();
                  } else if(isHiddenDoor) {
                    tmpDrw.tile = Config::isTilesWallSymbolFullSquare() ?
                                  tile_wallTop :
                                  tile_wallFront;
                  }
                }
              }
            }
          }
        }

        Pos pos(x, y);

        if(tmpDrw.isAwareOfMonsterHere) {
          drawGlyph('!', panel_map, pos, clrBlack, true, clrNosfTealDrk);
        } else if(tmpDrw.tile != tile_empty && tmpDrw.glyph != ' ') {
          if(IS_TILES) {
            drawTile(tmpDrw.tile, panel_map, pos, tmpDrw.clr, tmpDrw.clrBg);
          } else {
            drawGlyph(tmpDrw.glyph, panel_map, pos, tmpDrw.clr, true,
                      tmpDrw.clrBg);
          }

          if(tmpDrw.lifebarLength != -1) {
            drawLifeBar(pos, tmpDrw.lifebarLength);
          }
        }

        if(eng->map->cells[x][y].isExplored == false) {
          renderArray_[x][y].clear();
        }
      }
    }

    //---------------- DRAW PLAYER CHARACTER
    bool isRangedWpn = false;
    const Pos& pos = eng->player->pos;
    Item* item = eng->player->getInv().getItemInSlot(slot_wielded);
    if(item != NULL) {
      isRangedWpn = item->getData().isRangedWeapon;
    }
    if(IS_TILES) {
      const TileId tile = isRangedWpn ? tile_playerFirearm : tile_playerMelee;
      drawTile(tile, panel_map, pos, eng->player->getClr(), clrBlack);
    } else {
      drawGlyph('@', panel_map, pos, eng->player->getClr(), true, clrBlack);
    }
    const int LIFE_BAR_LENGTH = getLifebarLength(*eng->player);
    if(LIFE_BAR_LENGTH != -1) {
      drawLifeBar(pos, LIFE_BAR_LENGTH);
    }
    drawPlayerShockExclMarks();
  }
}
} //Render

