/*
Copyright (C) 2020
Sander Gieling
Inholland University of Applied Sciences at Alkmaar, the Netherlands

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation, 
Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

// Artwork bibliography
// --------------------
// Reticle taken from here:
// https://flyclipart.com/download-png#printable-crosshair-targets-clip-art-clipart-675613.png
// Blorp taken from here:
// https://opengameart.org/content/animated-top-down-survivor-player
// Desert floor tiling taken from here:
// https://www.flickr.com/photos/maleny_steve/8899498324/in/photostream/

#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h> // for IMG_Init and IMG_LoadTexture
#include <math.h> // for atan() function

#define SCREEN_WIDTH				1800
#define SCREEN_HEIGHT				1000
#define PI					3.14159265358979323846

// Move this much pixels every frame a move is detected:
#define PLAYER_MAX_SPEED			6.0f

// AFTER a movement-key is released, reduce the movement speed for 
// every consecutive frame by (1.0 - this amount):
#define PLAYER_DECELERATION			0.25f

// Make A Define That Will Set The Amount Of Bullets //
#define BULLET_COUNT				15
#define BULLET_SPEED				35

int moveY = 0;
int moveX = 0;

// A mouse structure holds mousepointer coords & a pointer texture:
typedef struct _mouse_ {
  int x;
  int y;
  SDL_Texture *txtr_reticle;
} mouse;

// Our Own Struct To Make Bullets //
typedef struct _bullet_ {
  int bulletCount;
  double x;
  double y;
  SDL_Texture *txtr_bullet;
  double angle;
} bullet;

typedef enum _playerstate_ {
  IDLE = 0,
  WALKING = 1,
} playerstate;

typedef enum _firestate_ {
  SAFETY_FIRST = 0,
  SHOOT_FIRST  = 1,
} firestate;

// Added: speed in both directions and rotation angle:
typedef struct _player_ {
  int x;
  int y;
  float speed_x;
  float speed_y;
  int up;
  int down;
  int left;
  int right;
  float angle;
  SDL_Texture *txtr_body;
  SDL_Texture *txtr_feet;
  int counterIdle; 
  int counterMoving;

  playerstate state;
  firestate gunstate;
} player;

typedef enum _keystate_ {
  UP = 0,
  DOWN = 1
} keystate;

void handle_key(SDL_KeyboardEvent *keyevent, keystate updown, player *tha_playa);
void makeBullet(player *tha_playa);
void travelBullet();

void createMuzzleFlash(player *tha_playa);

void showPlayerState(player *tha_playa);
// This function has changed because mouse movement was added:
void process_input(player *tha_playa, mouse *tha_mouse);
// This function has changed because mouse movement was added:
void update_player(player *tha_playa, mouse *tha_mouse);
void proper_shutdown(void);
SDL_Texture *load_texture(char *filename);

// This function has changed because texture rotation was added,
// which means drawing a texture centered on a coordinate is easier:
void blit(SDL_Texture *txtr, int x, int y, int center);
void blit_angled(SDL_Texture *txtr, int x, int y, float angle);

float get_angle(int x1, int y1, int x2, int y2, SDL_Texture *texture);

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;


// Make A Struct Array For Bullets //
struct _bullet_ bullets[BULLET_COUNT];
int bulletCounter = 0;

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  player blorp = {(SCREEN_WIDTH / 2), (SCREEN_HEIGHT / 2), 0.0f, 0.0f, UP, UP, UP, UP, 0.0, NULL, NULL, 0, 0, 0, 0};
	
  // New: Mouse is a type representing a struct containing x and y coords of mouse pointer:
  mouse mousepointer;
	
  unsigned int window_flags = 0;
  unsigned int renderer_flags = SDL_RENDERER_ACCELERATED;

  // All Security Checks //
  if (SDL_Init(SDL_INIT_VIDEO) < 0) exit(1);
  window = SDL_CreateWindow("Blorp is going to F U UP!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, window_flags);
  if (window == NULL) exit(1);	
  renderer = SDL_CreateRenderer(window, -1, renderer_flags);
  if (renderer == NULL) exit(1);	

  // Load All Textures //
  IMG_Init(IMG_INIT_PNG);
  mousepointer.txtr_reticle = load_texture("gfx/reticle.png");
  blorp.txtr_body = load_texture("gfx/idlebody/survivor-idle_handgun_0.png");
  blorp.txtr_feet = load_texture("gfx/idlefeet/survivor-idle_0.png");

  // No Visible Cursor For Us //
  SDL_ShowCursor(0);

  while (1) {
    SDL_SetRenderDrawColor(renderer, 120, 144, 156, 255);
    SDL_RenderClear(renderer);
		
    // # Sensor Reading #
    // Also takes the mouse movement into account:
    process_input(&blorp, &mousepointer);
	
    // If A Bullet Has Been Made Run The Muzzleflash //
    if (blorp.gunstate) createMuzzleFlash(&blorp);
    
    travelBullet();

    // # Applying Game Logic #
    // Also takes the mouse movement into account:
    update_player(&blorp, &mousepointer);

    // # Actuator Output Buffering #
    // Also takes texture rotation into account:
    showPlayerState(&blorp);
    blit_angled(blorp.txtr_feet, blorp.x, blorp.y, blorp.angle);
    blit_angled(blorp.txtr_body, blorp.x, blorp.y, blorp.angle);

    // New: Redraw mouse pointer centered on the mouse coordinates:
    blit(mousepointer.txtr_reticle, mousepointer.x, mousepointer.y, 1);
  
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_RenderPresent(renderer);
    SDL_Delay(16);
  }

  return 0;
}

void showPlayerState(player *tha_playa) {
  //FILE *fptr;
  //fptr = fopen("test.txt", "w");
  //fprintf(fptr, "Hello To The Edit File :D\n");
  //fclose(fptr);

  // Let's First Get The State Of The Player //
  playerstate state = tha_playa->state;

  switch(state) {
    case 0: // IDLE //
      if (tha_playa->counterIdle == 20) {
        tha_playa->counterIdle = 0;
      }

      char *texturesIdle[20] = {
	"gfx/idlebody/survivor-idle_handgun_0.png", "gfx/idlebody/survivor-idle_handgun_1.png",
	"gfx/idlebody/survivor-idle_handgun_2.png", "gfx/idlebody/survivor-idle_handgun_3.png",
	"gfx/idlebody/survivor-idle_handgun_4.png", "gfx/idlebody/survivor-idle_handgun_5.png",
	"gfx/idlebody/survivor-idle_handgun_6.png", "gfx/idlebody/survivor-idle_handgun_7.png",
	"gfx/idlebody/survivor-idle_handgun_8.png", "gfx/idlebody/survivor-idle_handgun_9.png",
	"gfx/idlebody/survivor-idle_handgun_10.png", "gfx/idlebody/survivor-idle_handgun_11.png",
	"gfx/idlebody/survivor-idle_handgun_12.png", "gfx/idlebody/survivor-idle_handgun_13.png",
	"gfx/idlebody/survivor-idle_handgun_14.png", "gfx/idlebody/survivor-idle_handgun_15.png",
	"gfx/idlebody/survivor-idle_handgun_16.png", "gfx/idlebody/survivor-idle_handgun_17.png",
	"gfx/idlebody/survivor-idle_handgun_18.png", "gfx/idlebody/survivor-idle_handgun_19.png"
      };

      tha_playa->txtr_body = load_texture(texturesIdle[tha_playa->counterIdle]);
      tha_playa->counterIdle = tha_playa->counterIdle + 1;
      break;
    case 1: // MOVING //
      if (tha_playa->counterMoving == 20) {
        tha_playa->counterMoving = 0;
      }

      char *texturesBody[20] = {
        "gfx/movebody/survivor-move_handgun_0.png", "gfx/movebody/survivor-move_handgun_1.png",
        "gfx/movebody/survivor-move_handgun_2.png", "gfx/movebody/survivor-move_handgun_3.png",
        "gfx/movebody/survivor-move_handgun_4.png", "gfx/movebody/survivor-move_handgun_5.png",
        "gfx/movebody/survivor-move_handgun_6.png", "gfx/movebody/survivor-move_handgun_7.png",
        "gfx/movebody/survivor-move_handgun_8.png", "gfx/movebody/survivor-move_handgun_9.png",
        "gfx/movebody/survivor-move_handgun_10.png", "gfx/movebody/survivor-move_handgun_11.png",
        "gfx/movebody/survivor-move_handgun_12.png", "gfx/movebody/survivor-move_handgun_13.png",
        "gfx/movebody/survivor-move_handgun_14.png", "gfx/movebody/survivor-move_handgun_15.png",
        "gfx/movebody/survivor-move_handgun_16.png", "gfx/movebody/survivor-move_handgun_17.png",
        "gfx/movebody/survivor-move_handgun_18.png", "gfx/movebody/survivor-move_handgun_19.png",
      };
    
      char *texturesFeet[20] = {
        "gfx/movefeet/survivor-walk_0.png", "gfx/movefeet/survivor-walk_1.png", 
        "gfx/movefeet/survivor-walk_2.png", "gfx/movefeet/survivor-walk_3.png", 
        "gfx/movefeet/survivor-walk_4.png", "gfx/movefeet/survivor-walk_5.png", 
        "gfx/movefeet/survivor-walk_6.png", "gfx/movefeet/survivor-walk_7.png", 
        "gfx/movefeet/survivor-walk_8.png", "gfx/movefeet/survivor-walk_9.png", 
        "gfx/movefeet/survivor-walk_10.png", "gfx/movefeet/survivor-walk_11.png", 
        "gfx/movefeet/survivor-walk_12.png", "gfx/movefeet/survivor-walk_13.png", 
        "gfx/movefeet/survivor-walk_14.png", "gfx/movefeet/survivor-walk_15.png", 
        "gfx/movefeet/survivor-walk_16.png", "gfx/movefeet/survivor-walk_17.png", 
        "gfx/movefeet/survivor-walk_18.png", "gfx/movefeet/survivor-walk_19.png", 
      };

      tha_playa->txtr_body = load_texture(texturesBody[tha_playa->counterMoving]);
      tha_playa->txtr_feet = load_texture(texturesFeet[tha_playa->counterMoving]);
      
      tha_playa->counterMoving = tha_playa->counterMoving + 1;
      break;
   }
}

// A New Function To Shoot That Bloody Pistol //
void makeBullet(player *tha_playa) {
  if (bulletCounter == BULLET_COUNT) bulletCounter = 0;
  bulletCounter++;
  
  // Make A bullet And Set All It's Values // 
  bullet bulletPointer;
  
  bulletPointer.bulletCount = bulletCounter;
  bulletPointer.angle = ((tha_playa->angle / 180) * PI);
 
  double compX = (80 * cos(bulletPointer.angle)) - (56 * sin(bulletPointer.angle));
  double compY = (80 * sin(bulletPointer.angle)) + (56 * cos(bulletPointer.angle));

  bulletPointer.x = tha_playa->x + (int)compX;
  bulletPointer.y = tha_playa->y + (int)compY;
  bulletPointer.txtr_bullet = load_texture("gfx/bullet20x5.png");
  bulletPointer.angle = tha_playa->angle;

  // Insert The Bullet In The Bullet Array //
  bullets[bulletPointer.bulletCount] = bulletPointer;
  tha_playa->gunstate = 1;
}

void travelBullet() {
  for (int a = 0; a < BULLET_COUNT; a++) {
    struct _bullet_ shotBullet = bullets[a];

    shotBullet.x += cos((shotBullet.angle/180)*PI) * BULLET_SPEED;
    shotBullet.y += sin((shotBullet.angle/180)*PI) * BULLET_SPEED;

    bullets[shotBullet.bulletCount] = shotBullet;

    blit_angled(shotBullet.txtr_bullet, (int)shotBullet.x, (int)shotBullet.y, (float)shotBullet.angle);
  }
}

///////////////////////////////////////////////////////////////////////////////////
// The Function createMuzzleFlash Is For Generating And Editing The Muzzle Flash //
void createMuzzleFlash(player *tha_playa) {
  // Calculate Angle //
  double angle = ((tha_playa->angle / 180) * PI);
  
  // Calculate Location Off Muzzle Flash //
  double muzzleX = (130 * cos(angle)) - (56 * sin(angle)) + tha_playa->x;
  double muzzleY = (130 * sin(angle)) + (56 * cos(angle)) + tha_playa->y;

  // Load The Image For Muzzle Flash //
  SDL_Texture *muzzleFlash = load_texture("gfx/shoot/muzzle_flash_01.png");

  // Remove Black Background From Texture //
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
  SDL_SetTextureBlendMode(muzzleFlash, SDL_BLENDMODE_ADD);
  SDL_SetTextureColorMod(muzzleFlash, 255, 255, 255);
  SDL_SetTextureAlphaMod(muzzleFlash, 255);

  // Load The Texture Into The Screen //
  blit_angled(muzzleFlash, (int)muzzleX, (int)muzzleY, tha_playa->angle);
  tha_playa->gunstate = 0;

  for (int delay = 0; delay < 1000; delay++);
}
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

void handle_key(SDL_KeyboardEvent *keyevent, keystate updown, player *tha_playa) {
  if (keyevent->repeat == 0) {
    int pressedW, pressedS, pressedA, pressedD;

    pressedW = keyevent->keysym.scancode == SDL_SCANCODE_W; 
    pressedS = keyevent->keysym.scancode == SDL_SCANCODE_S;
    pressedA = keyevent->keysym.scancode == SDL_SCANCODE_A;
    pressedD = keyevent->keysym.scancode == SDL_SCANCODE_D;

    if (pressedW || pressedS || pressedA || pressedD) tha_playa->state = 1; // state 1 is WALKING //
    if (pressedW) tha_playa->up    = updown;      
    if (pressedS) tha_playa->down  = updown;		
    if (pressedA) tha_playa->left  = updown;
    if (pressedD) tha_playa->right = updown;		
  }
}

void process_input(player *tha_playa, mouse *tha_mouse) {	
  SDL_Event event;
	
  while (SDL_PollEvent(&event))	{		
    switch (event.type) {
      case SDL_QUIT:	
        proper_shutdown();
        exit(0);
	break;
      case SDL_MOUSEBUTTONDOWN:
	makeBullet(tha_playa);
	break;
      case SDL_KEYDOWN:	
	if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
	  proper_shutdown();					
	  exit(0);
	} else {
	  handle_key(&event.key, DOWN, tha_playa);
	}
	break;
      case SDL_KEYUP:
	handle_key(&event.key, UP, tha_playa);
	break;
      default:
	break;		
    }
  }

  SDL_GetMouseState(&tha_mouse->x, &tha_mouse->y);
}

void update_player(player *tha_playa, mouse *tha_mouse) {
  if (tha_playa->up) {
    moveY = 1;
    
    tha_playa->speed_y = (float)PLAYER_MAX_SPEED;
    tha_playa->y -= (int)PLAYER_MAX_SPEED;
  } 
  if (tha_playa->down){		
    moveY = 2;
    
    tha_playa->speed_y = (float)PLAYER_MAX_SPEED;
    tha_playa->y += (int)PLAYER_MAX_SPEED;	
  }
  if (tha_playa->left) {
    moveX = 1;	
    
    tha_playa->speed_x = (float)PLAYER_MAX_SPEED;
    tha_playa->x -= (int)PLAYER_MAX_SPEED;
  } 
  if (tha_playa->right) {
    moveX = 2;
    
    tha_playa->speed_x = (float)PLAYER_MAX_SPEED;
    tha_playa->x += (int)PLAYER_MAX_SPEED;	
  }

  // Make Sure It Slowly Walks Off (Y version) //
  if (tha_playa->speed_y <= 0) {
    moveY = 0;
  } 
  if (moveY != 0) {
    // Step 1: Get The Current Speed Of Blorp 		//
    float currentSpeed = (float)tha_playa->speed_y;

    // Step 2: Remove A Slight Bit Off That Speed 	//
    currentSpeed = (float)currentSpeed - PLAYER_DECELERATION;
    tha_playa->speed_y = (float)currentSpeed - PLAYER_DECELERATION;

    if (moveY == 1) {
      // Step 3: Set It To y Of Blorp 			//
      tha_playa->y = tha_playa->y - (int)currentSpeed;
    } else if (moveY == 2) {
      tha_playa->y = tha_playa->y + (int)currentSpeed;
    }
  }

  // Make Sure It Slowly Walks Off (X version) //
  if (tha_playa->speed_x <= 0) {
    moveX = 0;
  } 
  if (moveX != 0) {
    // Step 1: Get The Current Speed Of Blorp 		//
    float currentSpeed = (float)tha_playa->speed_x;

    // Step 2: Remove A Slight Bit Off That Speed 	//
    currentSpeed = (float)currentSpeed - PLAYER_DECELERATION;
    tha_playa->speed_x = (float)currentSpeed - PLAYER_DECELERATION;

    if (moveX == 1) {
      // Step 3: Set It To y Of Blorp 			//
      tha_playa->x = tha_playa->x - (int)currentSpeed;
    } else if (moveX == 2) {
      tha_playa->x = tha_playa->x + (int)currentSpeed;
    }
  }

  if (tha_playa->speed_x <= 0 && tha_playa->speed_y == 0) tha_playa->state = 0; // state 0 is for IDLE //

  tha_playa->angle = get_angle(tha_playa->x, tha_playa->y, tha_mouse->x, tha_mouse->y, tha_playa->txtr_body);
}

void proper_shutdown(void) {
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

SDL_Texture *load_texture(char *filename) {
  SDL_Texture *txtr;
  txtr = IMG_LoadTexture(renderer, filename);
  return txtr;
}

void blit(SDL_Texture *txtr, int x, int y, int center) {
  SDL_Rect dest;
  dest.x = x;
  dest.y = y;

  SDL_QueryTexture(txtr, NULL, NULL, &dest.w, &dest.h);

  // If center != 0, render texture with its center on (x,y), NOT
  // with its top-left corner...
  if (center) {
    dest.x -= dest.w / 2;
    dest.y -= dest.h / 2;
  }

  SDL_RenderCopy(renderer, txtr, NULL, &dest);
}

void blit_angled(SDL_Texture *txtr, int x, int y, float angle) {
  SDL_Rect dest;
  dest.x = x;
  dest.y = y;

  SDL_QueryTexture(txtr, NULL, NULL, &dest.w, &dest.h);

  // Textures that are rotated MUST ALWAYS be rendered with their
  // center at (x, y) to have a symmetrical center of rotation:
  dest.x -= (dest.w / 2);
  dest.y -= (dest.h / 2);

  // Look up what this function does. What do these rectangles
  // mean? Why is the source rectangle NULL? What are acceptable
  // values for the `angle' parameter?
  SDL_RenderCopyEx(renderer, txtr, NULL, &dest, angle, NULL, SDL_FLIP_NONE);
}

float get_angle(int x1, int y1, int x2, int y2, SDL_Texture *txtr) {
  // We Make Sure We Have Our Variables //
  double pythagoras, sinusRule, arcTangus = 0;
  int h = 0;

  // We Want To Know Where Blorp Is //
  SDL_QueryTexture(txtr, NULL, NULL, NULL, &h);

  // Just To Keep Everything A Bit Organized //
  double answerA, answerB;
  
  // We Use Pyhtagaros To Calculate A Diagonal Distance Between Blorp And Mouse //
  answerA = pow((x2 - x1), 2);
  answerB = pow((y2 - y1), 2);
  pythagoras = sqrt(answerA + answerB);

  // After That We Use pythagoras And The Sinus Rule To Calulate Our Degree //
  answerA = h / 3.75;
  answerB = sin(90) / pythagoras;
  sinusRule = asin(answerA * answerB);

  // Now We Calculte It's Opposite //
  answerA = y2 - y1;
  answerB = x2 - x1;
  arcTangus = atan2(answerA, answerB);

  // We Calculate All Our Results And Transform It Into Degrees //
  answerA = arcTangus - sinusRule;
  answerB = 180 / PI;
  return (float)(answerA * answerB);
}
