#include <3ds.h>
#include <citro2d.h>
#include "c2d/base.h"
#include "c2d/sprite.h"
#include "c2d/spritesheet.h"
#include "c2d/text.h"
#include "sys/unistd.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h> 

/*
 * Zhennyak (Adrian):
 * Siendo honestos, hay varias cosas que cambie respecto al codigo original (refiriendome a la logica del juego, que normalmente no hace falta tocar)
 * para hacerlo un poco mas eficiente y legible no es perfecto, hay partes del codigo que requieren una mayor reescritura y reestructuracion, pero cumple su proposito
 * a poder ser, separar la logica del juego de lo que es la renderizacion del mismo, porque se vuelve demasiado confuso
*/

#define BOTTOM_WIDTH 320
#define BOTTOM_HEIGHT 240
const int SLOT_WIDTH = BOTTOM_WIDTH / 10;
const int SLOT_HEIGHT = BOTTOM_HEIGHT / 7; // Contando la interfaz

//colors, yep, I'm using game maker names
#define c_black   0xFF000000
#define c_marron  0xFF000080
#define c_green   0xFF008000
#define c_olive   0xFF008080
#define c_navy    0xFF800000
#define c_purple  0xFF800080
#define c_teal    0xFF808000
#define c_gray    0xFF808080
#define c_silver  0xFFC0C0C0
#define c_red     0xFF0000FF
#define c_lime    0xFF00FF00
#define c_yellow  0xFF00FFFF
#define c_orange  0xFF007FFF
#define c_blue    0xFFFF0000
#define c_fuchsia 0xFFFF00FF
#define c_aqua    0xFFFFFF00
#define c_white   0xFFFFFFFF

//variables
int rmax;//for rng
int rmin;

int money = 500; //user can see this DEFAULT: 500
int level = 1;
int xp = 0;
float xpneed = 10;
int score = 0;

int screen = 1;
int selectorpos = 0; 
int selx = 0;
int sely = 0;
int selmode = 0; //pointer/buttons mode
int previr1x = 0;
int previr1y = 0;

float bar; //float variable to use decimals for healthbars

//"objects"
int tower[10][6][8];

int enemypointer = 0;
int enemycount = 0;
int enemyspawncooldown = 1400;

int fireball[300][3]; //maxcount, values
int fireballpointer = 0;
int maxfireballs = 299;
int fireballrot = 0;
int fireballcount = 0; //just for debug

// Citro2D
C2D_SpriteSheet spriteSheet;
C2D_ImageTint enemiesTint[7];

int min(int a, int b) {
    return (a < b) ? a : b;
}

void create_fireball(int _x,int _y,int _damage)
{
    if(fireballpointer != maxfireballs)
    {
        fireballpointer++;
    }
    else
    {
        fireballpointer = 0;
    }
        fireball[fireballpointer][0] = _x;
        fireball[fireballpointer][1] = _y;
        fireball[fireballpointer][2] = _damage;
    fireballcount++; //then I'll delete this line (maybe)
}
void destroy_fireball(int index)
{
    fireball[index][0] = 0;
    fireball[index][1] = 0;
    fireball[index][2] = 0;
    fireballcount--; //this too
}
void heal_tower(int _x,int _y,int _c)
{
    if(tower[_x][_y][0] + _c < tower[_x][_y][6]) //si no pasa el límite
    {
        tower[_x][_y][0] += _c; //curar
    }
    else
    {
        tower[_x][_y][0] = tower[_x][_y][6]; //vida = vidamax
    }
}


void tower_action(int _x,int _y) //contador llega a 0
{
    switch(tower[_x][_y][4])
    {
        case 0: //lanzador
        case 1: //lanzador 2
            create_fireball(_x * 32, _y,tower[_x][_y][7]);
        break;
        case 2: //aweonao de plata
            money += 25*tower[_x][_y][1];
        break;
        case 7: //maría, digo planta.
        heal_tower(_x,_y,15);
            switch(_x)
            {
                default:
                    heal_tower(_x+1,_y,15);
                    heal_tower(_x-1,_y,15);
                    break;
                case 0: //si está en la izquierda
                    heal_tower(_x+1,_y,15);
                break;

                case 10: //si está en la derecha
                    heal_tower(_x-1,_y,15);
                break;
            }
            switch (_y)
            {
                default:
                    heal_tower(_x,_y+1,15);
                    heal_tower(_x,_y-1,15);
                    break;
                case 0: //si está en la izquierda
                    heal_tower(_x,_y+1,15);
                break;

                case 6: //si está en la derecha
                    heal_tower(_x,_y-1,15);
                break;
            }

        break;
    }
}

//iniciar valores de las torres
static const int pvida[] =   {25,  250,  1, 150,  500, 5000,    1,  15};
static const int pprecio[] = {250, 900, 50, 500, 1500, 5000, 1000, 300};
// static const int ptex[] =    {0,   2,   1,  8,   9,   10,    7,     11};
static const int pcooldown[]={120, 30,  600, -1,  -1,  -1,   -1,    120};

// Se opto por eliminar color puesto que cada uno tiene una direccion a un tint, es mucho mas eficiente en cuanto a memoria y calidad!
typedef struct
{
    int id, y, hp, damage, knockback, resistance, explode, maxhp;
    float x, speed;
    C2D_ImageTint *tint;
} enemydata;

int maxenemies = 32; //CAMBIA AQUÍ EL MÁXIMO DE ENEMIGOS
enemydata enemies[32]; 

void create_enemy(int _tipo, int _y)
{
    if (enemycount != maxenemies)
    {
        while (enemies[enemypointer].id != -1)
        {
            enemypointer++;
            if (enemypointer >= maxenemies)
            {
                enemypointer = 0;
            }
        }
	enemydata *enemy = &enemies[enemypointer];
        enemy -> id = enemypointer;
        enemy -> y = _y; // RANDOM
        enemy -> x = BOTTOM_WIDTH;
        enemy -> damage = round(level * 1.2);
        enemy -> hp = min(round(2 * (level * 1.8)),9);//velocidad máxima = 9
        enemy -> knockback = -10;
        enemy -> explode = 0; //unused variable
        enemy -> speed = -1.3 * (level * 0.16);
        enemy -> resistance = 0;
	enemy -> tint = &enemiesTint[_tipo];

        switch (_tipo)
        {
            default:
            case 0:
                break;
            case 1:
                enemy -> damage <<= 2;
                break;
            case 2:
                enemy -> knockback = 0;
                enemy -> hp >>= 2;
                enemy -> speed *= 0.5;
                enemy -> damage = 0;
                break;
            case 3:
                enemy -> resistance = 1;
                break;
            case 4:
                enemy -> speed *= 1.5;
                break;
            case 5:
                enemy -> hp <<= 2;
                enemy -> speed *= 0.5;
                break;
            case 6:
                enemy -> hp >>= 3;
                enemy -> speed *= 3;
                break;
        }

        enemy -> maxhp = enemy -> hp;
        enemycount++;
    }
}

void destroy_enemy(int _id)
{
    enemies[_id].id = -1;
    enemies[_id].x = -1;
    enemies[_id].speed = -0;
    enemies[_id].hp = 0;
    enemies[_id].explode = 0;
    enemies[_id].resistance = 0;
    enemies[_id].damage = 0;
    enemies[_id].y = -1;
    enemies[_id].knockback = 0;
    enemycount--;
}
void create_tower(int _x, int _y, int t)
{
    tower[_x][_y][0] = pvida[t];
    tower[_x][_y][1] = 1; //nivel?
    tower[_x][_y][2] = pprecio[t];
    tower[_x][_y][3] = pcooldown[t];//cooldown
    tower[_x][_y][4] = t; //tipo
    tower[_x][_y][5] = tower[_x][_y][3]; //cooldown máximo
    tower[_x][_y][6] = pvida[t]; //vida máxima
    tower[_x][_y][7] = 1; //Propiedad extra
}
void destroy_tower(int _x, int _y)
{
    tower[_x][_y][0] = 0;
    tower[_x][_y][1] = 0;
    tower[_x][_y][2] = 0;
    tower[_x][_y][3] = 0;
    tower[_x][_y][4] = 0;
    tower[_x][_y][5] = 0;
    tower[_x][_y][6] = 0;
    tower[_x][_y][7] = 0;
}

int main(int argc, char **argv) {
	romfsInit();
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
	consoleInit(GFX_TOP, NULL);
	srand(time(NULL));

	C3D_RenderTarget *screen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

	spriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");

	if (!spriteSheet) svcBreak(USERBREAK_PANIC);

	bool gameover = false;

	// Inicializar el hud
	struct {
		C2D_Sprite sprite;
		C2D_Text text;
	} hudItems[9];

	for(int i = 0; i < 9; i++) {
		C2D_SpriteFromSheet(&hudItems[i].sprite, spriteSheet, i + 2);
		C2D_SpriteSetPos(&hudItems[i].sprite, 32 * i, 32 * 6);
		if(i < 8) {
			char texto[6];
			sprintf(texto, "$%i", pprecio[i]);
			C2D_TextParse(&hudItems[i].text, C2D_TextBufNew(6), texto);
		}
	}

	C2D_Sprite selectorSprite;
	C2D_SpriteFromSheet(&selectorSprite, spriteSheet, 11);
	C2D_SpriteSetPos(&selectorSprite, 0.0f, 6 * 32.0f);

	//some data
	for (int i = 0; i < maxenemies; i++) {
		enemies[i].id = -1;
	}

	// Enemies Tint
	C2D_PlainImageTint(&enemiesTint[0], c_lime, 0.48f);
	C2D_PlainImageTint(&enemiesTint[1], c_red, 0.48f);
	C2D_PlainImageTint(&enemiesTint[2], c_blue, 0.48f);
	C2D_PlainImageTint(&enemiesTint[3], c_orange, 0.48f);
	C2D_PlainImageTint(&enemiesTint[4], c_purple, 0.48f);
	C2D_PlainImageTint(&enemiesTint[5], c_gray, 0.48f);
	C2D_PlainImageTint(&enemiesTint[6], c_yellow, 0.48f);

	C2D_Image gradient = C2D_SpriteSheetGetImage(spriteSheet, 13);

	// Touch controls
	touchPosition touch;
	bool hasTouch = false;

	// Dynamic text
	C2D_Text guiText;
	C2D_TextBuf guiBuf = C2D_TextBufNew(10);
	char textFormatBuffer[10];

	printf("Funciona\n");

	// Loop forever
	while(aptMainLoop()) {

		hidScanInput();
		u32 kDown = hidKeysDown();

		touch.px = 0;
		touch.py = 0;

		hidTouchRead(&touch);

	        if (kDown & KEY_START)  break;
		if(gameover) continue;

		uint cursorPosX = touch.px / SLOT_WIDTH;
		uint cursorPosY = touch.py / SLOT_HEIGHT;

		// Seleccionar el hud
		if(cursorPosY == 6 && cursorPosX < 9) {
			selectorpos = cursorPosX;
			C2D_SpriteSetPos(&selectorSprite, selectorpos * 32.0f, 6 * 32.0f);
		}


		if(touch.px != 0 && touch.py != 0 && ! hasTouch) {
			hasTouch = true;
			// Vender
			if(selectorpos == 8) {
				destroy_tower(cursorPosX, cursorPosY);
			}
			// Comprar
			else if(tower[cursorPosX][cursorPosY][0] == 0){
				if(money >= pprecio[selectorpos] && cursorPosY < 6)
				{
					money -= pprecio[selectorpos];
					create_tower(cursorPosX, cursorPosY, selectorpos);
		                }
			}
			else if (money >= tower[cursorPosX][cursorPosY][2] && tower[cursorPosX][cursorPosY][4] != 1 && tower[cursorPosX][cursorPosY][4] != 6) 
			//subir de nivel una torre
				{
					money -= tower[cursorPosX][cursorPosY][2]; //quitar plata
					tower[cursorPosX][cursorPosY][2] = round((tower[cursorPosX][cursorPosY][2]*1.5)/25)*25; //cambiar precio
					tower[cursorPosX][cursorPosY][0] = tower[cursorPosX][cursorPosY][6] << 1; //actualizar vida
					tower[cursorPosX][cursorPosY][6] = tower[cursorPosX][cursorPosY][0]; //actualizar vida máxima
					tower[cursorPosX][cursorPosY][1]++; //subir el contador de niveles
					switch(tower[cursorPosX][cursorPosY][4])//COOLDOWN!!1
					{
						case 0://lanzador 1
							if(tower[cursorPosX][cursorPosY][4] >= pcooldown[0]>>1)
							{
								tower[cursorPosX][cursorPosY][3]-=5;
							}
							else
							{
								tower[cursorPosX][cursorPosY][3] = pcooldown[0];
								tower[cursorPosX][cursorPosY][7]<<=1;
							}
							break;
						case 2://minero
							if(tower[cursorPosX][cursorPosY][4] >= pcooldown[2]>>2)
							{
								tower[cursorPosX][cursorPosY][4] -= 5;
							}
						case 7://planta
							if(tower[cursorPosX][cursorPosY][3] >= 10)
							{
								tower[cursorPosX][cursorPosY][3]-=5;
							}
							break;
					}
				}

		}
		else if(touch.px == 0 && touch.py == 0) {
			hasTouch = false;
		}

		if(enemyspawncooldown > 0)//enemy spawner
	        {
			enemyspawncooldown--;
	        }
		else
	        {
			score += level;

			int pos = rand() % 6;
			rmin = 0, rmax = min(level,12);
			int rtipo = rand() % (rmax - rmin + 1) + rmin;
			switch(rtipo)
	                {
				default:
					create_enemy(0,pos);
					break;
				case 2:
					create_enemy(5,pos);
					break;
				case 3:
					create_enemy(6,pos);
					break;
				case 5:
					create_enemy(2,pos);
					break;
				case 7:
					create_enemy(4,pos);
					break;
				case 10:
					create_enemy(3,pos);
					break;
				case 12:
					create_enemy(1,pos);
					break;
			}
			if(600/level >=  5)
	                {
				enemyspawncooldown = 600/level;
	                }
			else
	                {
				enemyspawncooldown = 5;
			}    
			
			if(xp < xpneed)
			{
				xp++;
			}
			else
			{
				xpneed = floor(xpneed*1.1);
				 xp = 0;
				level++;
			 }
		}

		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C2D_TargetClear(screen, C2D_Color32f(0.0f, 0.0f, 0.0f, 1.0f));
		C2D_SceneBegin(screen);
		// Background
		for(int i = 0; i < 5; i++) {
			for(int j = 0; j < 3; j++) {
				C2D_DrawImageAt(C2D_SpriteSheetGetImage(spriteSheet, 0),
						i * 64.0f,
						j * 64.0f,
						-1.0f,
						NULL,
						1.0f,
						1.0f);
			}
		}

		//rotation fo the fireball
		if(fireballrot < 26)
		{
			fireballrot++;
		}
		else
		{
			fireballrot = 0;
		}

		// draw fireballs
		for (int i = 0; i < maxfireballs; i++) {
			if (fireball[i][2] != 0) 
			{
				//we want them to move
				fireball[i][0] += 4;
				if(fireball[i][0] > BOTTOM_WIDTH)
				{
					destroy_fireball(i);
					continue;
				}
				C2D_DrawImageAtRotated(C2D_SpriteSheetGetImage(spriteSheet, 1),
						(float) fireball[i][0],
						fireball[i][1] * 32.0f + 16,
						0,
						fireballrot,
						NULL,
						1.0f,
						1.0f
						);
			}
		}


		//draw towers:
		for (int i = 0; i < 10; i++){
			for(int j = 0; j < 6; j++){
				if(tower[i][j][0] == 0) continue;

				C2D_DrawImageAt(
						C2D_SpriteSheetGetImage(spriteSheet, tower[i][j][4] + 2),
						i * 32,
						j * 32,
						0.0f,
						NULL,
						1,
						1
				);

				switch(tower[i][j][4])//uhh this is my way to do it!
				{
					case 0: //lanzador
					case 2: //minero
					case 7: //planta
						if(tower[i][j][3]>0) //cooldown logic
						{
							tower[i][j][3]--;
						}
						else
						{
							tower[i][j][3] = tower[i][j][5];
							tower_action(i,j);
						}
					default: //walls
						sprintf(textFormatBuffer, "lv%d", tower[i][j][1]);
						C2D_TextParse(&guiText, guiBuf, textFormatBuffer);
						C2D_TextOptimize(&guiText);
						C2D_DrawText(&guiText, C2D_WithColor, i * 32, j * 32, 1.0f, 0.4f, 0.4f, c_white);
						C2D_TextBufClear(guiBuf);

						sprintf(textFormatBuffer, "$%d", tower[i][j][2]);
						C2D_TextParse(&guiText, guiBuf, textFormatBuffer);
						C2D_TextOptimize(&guiText);
						C2D_DrawText(&guiText, C2D_WithColor, i * 32, j * 32 +8, 1.0f, 0.4f, 0.4f, c_white);
						C2D_TextBufClear(guiBuf);

						C2D_DrawImageAt(gradient, i * 32, j * 32, 1.0f, &enemiesTint[0], ((float) tower[i][j][0]/tower[i][j][6])*8.0f, 0.5f); //healthbar
						// C2D_DrawRectSolid(i * 32, j * 32, 1, ((float) tower[i][j][6] / tower[i][j][6]) * 32, 2, c_green);
						break;
					case 1: //lanzador 2
						if(tower[i][j][3]>0)
						{
							tower[i][j][3]--;
						}
						else
						{
							tower[i][j][3] = tower[i][j][5];
							tower_action(i,j);
						}
						C2D_DrawImageAt(gradient, i * 32, j * 32, 1.0f, &enemiesTint[0], ((float) tower[i][j][0]/tower[i][j][6])*8.0f, 0.5f); //healthbar
						break;
				}
			}
		}

		// HAY UN PROBLEMA POR ACA
		//draw enemies
		for (int i = 0; i < maxenemies; i++)
		{
			if (enemies[i].hp > 0) //is alive?
			{
				if (enemies[i].x < 1) //YOU LOSE!!?
				{
				    gameover = true;
				    printf("Gameover\n");
				    printf("Press start to exit\n");
				}
				C2D_DrawImageAt(C2D_SpriteSheetGetImage(spriteSheet, 12),
						enemies[i].x,
						enemies[i].y * 32.0f,
						0,
						enemies[i].tint,
						1,
						1
						);

				// GRRLIB_DrawTile(enemies[i].x, enemies[i].y << 6, gfx_images, 0, 1, 1, enemies[i].color, 4); // Imagen del enemigo
				enemies[i].x += enemies[i].speed;
				// printf("Enemy %i (%f, %i)\n", i, enemies[i].x, enemies[i].y);

				// Escaneo de fireballs para detectar colisión
				if(enemies[i].resistance == 0)
				{
					for (int j = 0; j < maxfireballs; j++)
					{
						if (enemies[i].y == fireball[j][1])  // Verifica si están en la misma coordenada Y
						{
							if (fireball[j][0] > enemies[i].x)  // Verifica si la fireball está delante del enemigo
							{
								enemies[i].hp -= fireball[j][2]; // Resta vida según el daño de la fireball
								destroy_fireball(j); //destruir la bola de fuego
								if (enemies[i].hp <= 0)  // Si la vida baja de 0, destruir enemigo
								{
									destroy_enemy(i);
									break; // Salir del loop de fireballs, ya que el enemigo ha sido eliminado
								}
							}
						}
					}
				}
				//Lógica de las torres
				int tx = floor(enemies[i].x / 32);
				int ty = enemies[i].y;

				if(tower[tx][ty][0] != 0) //si hay una torre en la posición del enemigo
				{
					if(tower[tx][ty][4] == 6) //si es el teléfono
					{
						destroy_enemy(i);
						destroy_tower(tx,ty);
					}
					else
					{
						tower[tx][ty][0] -= enemies[i].damage;
						if(tower[tx][ty][0] <= 0)
						{
							destroy_tower(tx,ty);
							continue; //ya que la torre se eliminó y no hay nada más que revisar, saltar al siguente loop
						}
						enemies[i].resistance = 0;
						enemies[i].x -= enemies[i].knockback;
					}
				}
			}
		}

		//interfaz
		bar = (xp/xpneed);
		C2D_DrawImageAt(gradient, 0.0f, 32 * 6.0f, 0.0f, &enemiesTint[2], BOTTOM_WIDTH / 2.0f, 8.0f);
	
		C2D_DrawImageAt(gradient, 0.0f, 32 * 7.0f, 0.0f, &enemiesTint[5], BOTTOM_WIDTH / 2.0f, 8.0f);
		C2D_DrawImageAt(gradient, 0.0f, 32 * 7.0f, 1.0f, &enemiesTint[0], bar * 80.0f, 8.0f);

		sprintf(textFormatBuffer, "$%d", money);
		C2D_TextParse(&guiText, guiBuf, textFormatBuffer);
		C2D_TextOptimize(&guiText);
		C2D_DrawText(&guiText, C2D_WithColor, 9 * 32.0f, 32.0f * 6 + 16, 1.0f, 0.3f, 0.4f, c_white);
		C2D_TextBufClear(guiBuf);
		
		sprintf(textFormatBuffer, "Lv: %d", level);
		C2D_TextParse(&guiText, guiBuf, textFormatBuffer);
		C2D_TextOptimize(&guiText);
		C2D_DrawText(&guiText, C2D_WithColor, 9 * 32.0f, 32.0f * 6, 1.0f, 0.3f, 0.4f, c_white);
		C2D_TextBufClear(guiBuf);

		//imágenes de la "tienda"
		for (int i = 0; i < 9; i++) {
			C2D_DrawSprite(&hudItems[i].sprite);
			if(hudItems[i].text.buf != NULL) {
				C2D_DrawText(&hudItems[i].text,
						C2D_WithColor,
						hudItems[i].sprite.params.pos.x,
						hudItems[i].sprite.params.pos.y + 32,
						1,
						0.35,
						0.5,
						c_white
					);
			}
		}

		C2D_DrawSprite(&selectorSprite);
		C3D_FrameEnd(0);
		usleep(10); // Si se quita esto literalmente se rompe la compatibilidad con consolas XD
	}
// if(screen == 2) //Lose screen :(
// { TODO NUNCA LO IMPLEMENTE XD
//     GRRLIB_DrawImg(0,0, gfx_grassbg, 0, 1, 1, c_gray);
//     GRRLIB_DrawImg(256,0, gfx_grassbg, 0, 1, 1, c_gray);
//     GRRLIB_DrawImg(512,0, gfx_grassbg, 0, 1, 1, c_gray);
//     GRRLIB_DrawImg(0,256, gfx_grassbg, 0, 1, 1, c_gray);
//     GRRLIB_DrawImg(256,256, gfx_grassbg, 0, 1, 1, c_gray);
//     GRRLIB_DrawImg(512,256, gfx_grassbg, 0, 1, 1, c_gray);
//
//     GRRLIB_Printf(0,0,font,c_red,1,"You lose :c");
//     GRRLIB_Printf(0,16,font,c_white,1,"Final level: %d",level);
//     GRRLIB_Printf(0,32,font,c_white,1,"Final score: %d",score);
//     GRRLIB_Printf(0,440,font,c_white,1,"Press home to exit");
//     GRRLIB_Render();  // Render the frame buffer to the TV
// }
	// Delete graphics
	C2D_SpriteSheetFree(spriteSheet);

	// Deinit libs
	C2D_Fini();
	C3D_Fini();
	gfxExit();
	romfsExit();
	return 0;
}
