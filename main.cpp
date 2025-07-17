#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH	640
#define SCREEN_HEIGHT	580

#define TEXT_POSITION_X 10
#define TEXT_POSITION_Y 14
#define GAP_X 4
#define GAP_Y 4
#define RECTANGLE_WIDTH 100
#define RECTANGLE_HEIGHT 36

#define PLAYER_X 0
#define PLAYER_Y SCREEN_HEIGHT-68

#define PLAYER_MIN_X 0
#define PLAYER_MAX_X SCREEN_WIDTH-24

#define MOVE_X 5
#define MOVE_Y 5

#define JUMP_VERTICAL 1
#define JUMP_HORIZONTAL 1

#define FALL_VERTICAL 1

#define LADDER_VERTICAL 2

#define SLOPE_VERTICAL 3

#define MAIN_PLATFORM_Y SCREEN_HEIGHT-36

#define MAX_PLATFORM_LEVEL 5
#define MAX_NUMBER_OF_PLATFORMS 6
#define MAX_PLATFORM_INFORMATION 3

#define MAX_LADDER_LEVEL 4
#define MAX_NUMBER_OF_LADDERS 2
#define MAX_LADDER_INFORMATION 2


struct Platform {
	int number_of_platform;
	int max_number_of_platforms;
	int x_left;
	int x_right;
	int current_y;
	bool is_flat;
	bool is_going_up;
	bool is_going_down;
	int width;
	int next_vertical_change_left;
	int next_vertical_change_right;
	int index = 0;
	int next_platform_y = 0;
};

void initialize_platform(Platform* current_platform, int horizontal, int vertical, int platforms[MAX_PLATFORM_LEVEL][MAX_NUMBER_OF_PLATFORMS][MAX_PLATFORM_INFORMATION]) {
	if (vertical == current_platform->next_platform_y - 32 && current_platform->next_platform_y != 0)
		current_platform->index += 1;
	
	int max_platform_number = 0;
	for (int i = MAX_NUMBER_OF_PLATFORMS - 1; i > -1; i--) {
		if (platforms[current_platform->index][i][0] != -1) {
			max_platform_number = i;
			break;
		}
	}
	current_platform->max_number_of_platforms = max_platform_number;

	if (platforms[current_platform->index][0][1] == platforms[current_platform->index][max_platform_number][1]) {
		current_platform->is_flat = true;
		current_platform->is_going_down = false;
		current_platform->is_going_up = false;
	}
	else {
		current_platform->is_flat = false;
		if (platforms[current_platform->index][0][1] - platforms[current_platform->index][max_platform_number][1] < 0) {
			current_platform->is_going_down = true;
			current_platform->is_going_up = false;
		}
		else if (platforms[current_platform->index][0][1] - platforms[current_platform->index][max_platform_number][1] > 0) {
			current_platform->is_going_down = false;
			current_platform->is_going_up = true;
		}
	}

	int current_number = 0;
	for (int i = 0; i <= max_platform_number; i++)
	{
		if (horizontal >= platforms[current_platform->index][i][0]-24 && horizontal <= platforms[current_platform->index][i][2]+18) {
			current_number = i;
			break;
		}
	}

	current_platform->number_of_platform = current_number;

	if (current_platform->is_flat) {
		current_platform->x_left = platforms[current_platform->index][current_number][0];
		current_platform->x_right = platforms[current_platform->index][current_number][2];
	}
	else {
		current_platform->x_left = platforms[current_platform->index][0][0];
		current_platform->x_right = platforms[current_platform->index][max_platform_number][2];
	}

	current_platform->current_y = vertical;


	if (current_platform->is_flat) {
		current_platform->width = platforms[current_platform->index][current_number][2] - platforms[current_platform->index][current_number][0];
	}
	else {
		current_platform->width = platforms[current_platform->index][max_platform_number][2] - platforms[current_platform->index][0][0];
	}


	if (current_platform->is_flat) {
		current_platform->next_vertical_change_left = -1;
		current_platform->next_vertical_change_right = -1;
	}
	else {
		current_platform->next_vertical_change_left = platforms[current_platform->index][current_number][0];
		current_platform->next_vertical_change_right = platforms[current_platform->index][current_number][2];
	}

	int max_next_number = 0;
	for (int i = MAX_NUMBER_OF_PLATFORMS - 1; i > -1; i--) {
		if (platforms[current_platform->index + 1][i][0] != -1) {
			max_next_number = i;
			break;
		}
	}
	int next_y_left = platforms[current_platform->index + 1][0][1];
	int next_y_right = platforms[current_platform->index + 1][max_next_number][1];
	if (next_y_left > next_y_right) {
		current_platform->next_platform_y = next_y_left;
	}
	else {
		current_platform->next_platform_y = next_y_right;
	}
}

bool should_reinitialize_platform(Platform* current_platform, int horizontal, int vertical) {
	bool result = false;

	if (horizontal < current_platform->x_left ||
		horizontal > current_platform->x_right ||
		vertical != current_platform->current_y)
		result = true;

	return result;
}

void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32*)p = color;
};

void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for (int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	};
};

void DrawString(SDL_Surface* screen, int x, int y, const char* text,
	SDL_Surface* charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while (*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
	};
};

void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x;
	dest.y = y;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};

void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k,
	Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for (i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};

void stage_one(SDL_Surface* screen, SDL_Surface* main_p, SDL_Surface* medium_p, SDL_Surface* small_p,
	           SDL_Surface* top_p, SDL_Surface* final_p, SDL_Surface* drabina,
	           int platforms[MAX_PLATFORM_LEVEL][MAX_NUMBER_OF_PLATFORMS][MAX_PLATFORM_INFORMATION],
	           int ladders[MAX_LADDER_LEVEL][MAX_NUMBER_OF_LADDERS][MAX_LADDER_INFORMATION]) {
	
	DrawSurface(screen, main_p, platforms[0][0][0], platforms[0][0][1]);
	DrawSurface(screen, medium_p, platforms[1][0][0], platforms[1][0][1]);
	DrawSurface(screen, small_p, platforms[1][1][0], platforms[1][1][1]);
	DrawSurface(screen, small_p, platforms[2][0][0], platforms[2][0][1]);
	DrawSurface(screen, medium_p, platforms[2][1][0], platforms[2][1][1]);
	DrawSurface(screen, top_p, platforms[3][0][0], platforms[3][0][1]);
	DrawSurface(screen, final_p, platforms[4][0][0], platforms[4][0][1]);
	
	for (int platform = 0; platform < MAX_LADDER_LEVEL; platform++)
		for (int ladder = 0; ladder < MAX_NUMBER_OF_LADDERS; ladder++)
			if (ladders[platform][ladder][0] != -1)
				DrawSurface(screen, drabina, ladders[platform][ladder][0], ladders[platform][ladder][1] );
}

void stage_two(SDL_Surface* screen, SDL_Surface* main_p, SDL_Surface* medium_p, SDL_Surface* small_p,
	           SDL_Surface* top_p, SDL_Surface* final_p, SDL_Surface* drabina,
			   int platforms[MAX_PLATFORM_LEVEL][MAX_NUMBER_OF_PLATFORMS][MAX_PLATFORM_INFORMATION],
			   int ladders[MAX_LADDER_LEVEL][MAX_NUMBER_OF_LADDERS][MAX_LADDER_INFORMATION]) {
	DrawSurface(screen, main_p, platforms[0][0][0], platforms[0][0][1]);
	DrawSurface(screen,	small_p, platforms[1][0][0], platforms[1][0][1]);
	DrawSurface(screen, final_p, platforms[1][1][0], platforms[1][1][1]);
	DrawSurface(screen, small_p, platforms[1][2][0], platforms[1][2][1]);
	DrawSurface(screen, final_p, platforms[2][0][0], platforms[2][0][1]);
	DrawSurface(screen, small_p, platforms[2][1][0], platforms[2][1][1]);
	DrawSurface(screen, final_p, platforms[2][2][0], platforms[2][2][1]);
	DrawSurface(screen, top_p, platforms[3][0][0], platforms[3][0][1]);
	DrawSurface(screen, final_p, platforms[4][0][0], platforms[4][0][1]);

	for (int platform = 0; platform < MAX_LADDER_LEVEL; platform++)
		for (int ladder = 0; ladder < MAX_NUMBER_OF_LADDERS; ladder++)
			if (ladders[platform][ladder][0] != -1)
				DrawSurface(screen, drabina, ladders[platform][ladder][0], ladders[platform][ladder][1]);
}

void stage_three(SDL_Surface* screen, SDL_Surface* main_p, SDL_Surface* medium_p, SDL_Surface* small_p,
	             SDL_Surface* top_p, SDL_Surface* final_p, SDL_Surface* drabina,
	             int platforms[MAX_PLATFORM_LEVEL][MAX_NUMBER_OF_PLATFORMS][MAX_PLATFORM_INFORMATION],
				 int ladders[MAX_LADDER_LEVEL][MAX_NUMBER_OF_LADDERS][MAX_LADDER_INFORMATION]) {
	
	DrawSurface(screen, main_p, platforms[0][0][0], platforms[0][0][1]);

	
	for (int platform = 1; platform < 3; platform++) {
		for (int draw_platform = 0; draw_platform < 6; draw_platform++) {
			if (platforms[platform][draw_platform][0] != -1) {
				DrawSurface(screen, small_p, platforms[platform][draw_platform][0], platforms[platform][draw_platform][1]);
			}
		}
	}

	
	DrawSurface(screen, top_p, platforms[3][0][0], platforms[3][0][1]);
	DrawSurface(screen, final_p, platforms[4][0][0], platforms[4][0][1]);

	for (int platform = 0; platform < MAX_LADDER_LEVEL; platform++)
		for (int ladder = 0; ladder < MAX_NUMBER_OF_LADDERS; ladder++)
			if (ladders[platform][ladder][0] != -1)
				DrawSurface(screen, drabina, ladders[platform][ladder][0], ladders[platform][ladder][1]);
}

int getIndex(int vertical) {
	int current_y = vertical + 36;
	int index = (MAIN_PLATFORM_Y - current_y) / 126;
	return index;
}

bool fall_check(Platform* current_platform, int horizontal) {
	bool fall = true;

	if (horizontal >= current_platform->x_left - 18 &&	horizontal <= current_platform->x_right) {
		fall = false;
	}

	return fall;
}

void copy_array_platform(int platforms[MAX_PLATFORM_LEVEL][MAX_NUMBER_OF_PLATFORMS][MAX_PLATFORM_INFORMATION],
	                     int platforms_stage[MAX_PLATFORM_LEVEL][MAX_NUMBER_OF_PLATFORMS][MAX_PLATFORM_INFORMATION]) {
	for (int i = 0; i < MAX_PLATFORM_LEVEL; i++) {
		for (int j = 0; j < MAX_NUMBER_OF_PLATFORMS; j++) {
			for (int k = 0; k < MAX_PLATFORM_INFORMATION; k++) {
				platforms[i][j][k] = platforms_stage[i][j][k];
			}
		}
	}
}

void copy_array_ladder(int ladders[MAX_LADDER_LEVEL][MAX_NUMBER_OF_LADDERS][MAX_LADDER_INFORMATION],
	                   int ladders_stage[MAX_LADDER_LEVEL][MAX_NUMBER_OF_LADDERS][MAX_LADDER_INFORMATION]) {
	for (int i = 0; i < MAX_LADDER_LEVEL; i++) {
		for (int j = 0; j < MAX_NUMBER_OF_LADDERS; j++) {
			for (int k = 0; k < MAX_LADDER_INFORMATION; k++) {
				ladders[i][j][k] = ladders_stage[i][j][k];
			}
		}
	}
}

void mario_left(SDL_Renderer* renderer, int horizontal, int vertical, SDL_Surface* mario_l, SDL_Texture* texture_run) {
	SDL_Rect srcrect = { 0, 0, 24, 32 };
	SDL_Rect dstrect = { horizontal, vertical, 24, 32 };
	SDL_RenderCopy(renderer, texture_run, &srcrect, &dstrect);
	SDL_RenderPresent(renderer);
}

void run_right(SDL_Renderer* renderer, int horizontal, int vertical, SDL_Surface* run_r, SDL_Texture* texture_run_right) {
	int ticks = SDL_GetTicks();
	int sprite = (ticks / 100) % 2;
	SDL_Rect srcrect = { sprite * 30, 0, 30, 32 };
	SDL_Rect dstrect = { horizontal, vertical, 30, 32 };
	SDL_RenderCopy(renderer, texture_run_right, &srcrect, &dstrect);
	SDL_RenderPresent(renderer);
}

void run_left(SDL_Renderer* renderer, int horizontal, int vertical, SDL_Surface* run_l, SDL_Texture *texture_run_left) {
	Uint32 ticks = SDL_GetTicks();
	Uint32 sprite = (ticks / 100) % 2;
	SDL_Rect srcrect = {sprite * 30, 0, 30, 32 };
	SDL_Rect dstrect = { horizontal, vertical, 30, 32 };
	SDL_RenderCopy(renderer, texture_run_left, &srcrect, &dstrect);
	SDL_RenderPresent(renderer);
}

void jump_right(SDL_Renderer* renderer, int horizontal, int vertical, SDL_Surface* jump_r, SDL_Texture* texture_jump_right) {
	SDL_Rect srcrect = { 0, 0, 32, 30 };
	SDL_Rect dstrect = { horizontal, vertical, 32, 30 };
	SDL_RenderCopy(renderer, texture_jump_right, &srcrect, &dstrect);
	SDL_RenderPresent(renderer);
}

void jump_left(SDL_Renderer* renderer, int horizontal, int vertical, SDL_Surface *jump_l, SDL_Texture* texture_jump_left) {
	SDL_Rect srcrect = { 0, 0, 32, 30 };
	SDL_Rect dstrect = { horizontal, vertical, 32, 30 };
	SDL_RenderCopy(renderer, texture_jump_left, &srcrect, &dstrect);
	SDL_RenderPresent(renderer);
}

void mario_ladder(SDL_Renderer* renderer, int horizontal, int vertical, SDL_Surface* m_ladder, SDL_Texture* texture_m_ladder) {
	Uint32 ticks = SDL_GetTicks();
	Uint32 sprite = (ticks / 100) % 2;
	SDL_Rect srcrect = { sprite*26, 0, 26, 32 };
	SDL_Rect dstrect = { horizontal, vertical, 26, 32 };
	SDL_RenderCopy(renderer, texture_m_ladder, &srcrect, &dstrect);
	SDL_RenderPresent(renderer);
}

void donkey_kong(SDL_Renderer* renderer, SDL_Surface* d_kong, SDL_Texture* texture_d_kong, int current_level) {
	Uint32 ticks = SDL_GetTicks();
	Uint32 sprite = (ticks / 800) % 4;
	SDL_Rect srcrect = { sprite * 86, 0, 86, 64 };
	if (current_level == 1 || current_level == 2) {
		SDL_Rect dstrect = { 80, 102, 86, 64 };
		SDL_RenderCopy(renderer, texture_d_kong, &srcrect, &dstrect);
	}
	else if (current_level == 3) {
		SDL_Rect dstrect = { 80, 220, 86, 64 };
		SDL_RenderCopy(renderer, texture_d_kong, &srcrect, &dstrect);
	}
}

void stage_end(SDL_Surface* screen, int czerwony, int niebieski, char text[128], double worldTime, SDL_Surface* charset, int current_level) {
	if (current_level == 1 || current_level == 2) {
		DrawRectangle(screen, 210, 225, 200, 120, czerwony, niebieski);
		sprintf(text, "Stage end", worldTime);
		DrawString(screen, 273, 250, text, charset);
		sprintf(text, "[C] - Continue", worldTime);
		DrawString(screen, 255, 285, text, charset);
		sprintf(text, "[ESC] - Exit", worldTime);
		DrawString(screen, 260, 300, text, charset);
	}
	else {
		DrawRectangle(screen, 210, 225, 200, 120, czerwony, niebieski);
		sprintf(text, "Game Completed", worldTime);
		DrawString(screen, 253, 250, text, charset);
		sprintf(text, "[R] - Play Again", worldTime);
		DrawString(screen, 245, 285, text, charset);
		sprintf(text, "[ESC] - Exit", worldTime);
		DrawString(screen, 260, 300, text, charset);
	}
}

void points(SDL_Surface* screen, int czerwony, int niebieski, char text[128], double worldTime, SDL_Surface* charset) {
	DrawRectangle(screen, 530, 4, 100, 62, czerwony, niebieski);
	sprintf(text, "Wykonane", worldTime);
	DrawString(screen, 535, 14, text, charset);
	sprintf(text, "podpunkty:", worldTime);
	DrawString(screen, 535, 25, text, charset);
	sprintf(text, "1 - 4,", worldTime);
	DrawString(screen, 535, 40, text, charset);
	sprintf(text, "A, B, E, H", worldTime);
	DrawString(screen, 535, 51, text, charset);
}

#ifdef __cplusplus
extern "C"
#endif

int main(int argc, char** argv) {

	int t1, t2, quit, frames, rc, horizontal, vertical, base, fall_goal, current_level;
	
	double delta, worldTime, fpsTimer, fps, distance, etiSpeed;

	bool jumping = false;
	bool jumping_left = false;
	bool jumping_right = false;
	bool going_up = true;
	bool end = false;
	bool fall = false;
	bool change_level = true;
	bool is_ladder = false;
	
	current_level = 1;

	struct Platform current_platform;

	SDL_Event event;
	SDL_Surface* screen, * charset;
	SDL_Surface* mario_right;
	SDL_Surface* main_platform;
	SDL_Surface* medium_platform;
	SDL_Surface* small_platform;
	SDL_Surface* top_platform;
	SDL_Surface* final_platform;
	SDL_Surface* drabina;
	SDL_Surface* barrels;
	SDL_Texture* scrtex;
	SDL_Window* window;
	SDL_Renderer* renderer;

	int platform_stage_one[MAX_PLATFORM_LEVEL][MAX_NUMBER_OF_PLATFORMS][MAX_PLATFORM_INFORMATION] = 
		{ {{0, 544, 640}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}},
		  {{0, 418, 460}, {560, 418, 640}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}},
		  {{0, 292, 80}, {180, 292, 640}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}},
		  {{0, 166, 540}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}},
	   	  {{230, 40, 410}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}} };

	int ladder_stage_one[MAX_LADDER_LEVEL][MAX_NUMBER_OF_LADDERS][MAX_LADDER_INFORMATION] = 
		{ {{180, 454}, {-1, -1}},
		  {{30, 328}, {580, 328}},
		  {{480, 202}, {-1, -1}},
		  {{230, 76}, {373, 76}} };

	int platform_stage_two[MAX_PLATFORM_LEVEL][MAX_NUMBER_OF_PLATFORMS][MAX_PLATFORM_INFORMATION] = 
		{ {{0, 544, 640}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}},
		  {{80, 418, 160}, {230, 418, 410}, {480, 418, 560}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}},
		  {{30, 292, 210}, {280, 292, 360}, {430, 292, 610}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}},
		  {{0, 166, 540}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}},
		  {{230, 40, 410}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}} };

	int ladder_stage_two[MAX_LADDER_LEVEL][MAX_NUMBER_OF_LADDERS][MAX_LADDER_INFORMATION] = 
		{ {{299, 454}, {-1, -1}},
		  {{102, 328}, {502, 328}},
		  {{302, 202}, {-1, -1}},
		  {{230, 76}, {373, 76}} };

	int platform_stage_three[MAX_PLATFORM_LEVEL][MAX_NUMBER_OF_PLATFORMS][MAX_PLATFORM_INFORMATION] = 
		{ {{0, 544, 640}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}},
		  {{0, 456, 80}, {80, 459, 160}, {160, 462, 240}, {240, 465, 320}, {320, 468, 400}, {400, 471, 480}},
		  {{160, 375, 240}, {240, 372, 320}, {320, 369, 400}, {400, 366, 480}, {480, 363, 560}, {560, 360, 640}},
		  {{0, 284, 540}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}},
		  {{230, 158, 410}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}} };

	int ladder_stage_three[MAX_LADDER_LEVEL][MAX_NUMBER_OF_LADDERS][MAX_LADDER_INFORMATION] =
		{ {{-1, -1}, {-1, -1}},
		  {{-1, -1}, {-1, -1}},
		  {{-1, -1}, {-1, -1}},
		  {{230, 194}, {373, 194}} };

	int platforms[MAX_PLATFORM_LEVEL][MAX_NUMBER_OF_PLATFORMS][MAX_PLATFORM_INFORMATION];
	int ladders[MAX_LADDER_LEVEL][MAX_NUMBER_OF_LADDERS][MAX_LADDER_INFORMATION];

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
	}

	rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0,
		&window, &renderer);
	if (rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 1;
	};

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_SetWindowTitle(window, "King Donkey");

	screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
		0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		SCREEN_WIDTH, SCREEN_HEIGHT);

	charset = SDL_LoadBMP("Images/cs8x8.bmp");
	if (charset == NULL) {
		printf("SDL_LoadBMP(Images/cs8x8.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	SDL_SetColorKey(charset, true, 0x000000);

	mario_right = SDL_LoadBMP("Images/mario.bmp");
	if (mario_right == NULL) {
		printf("SDL_LoadBMP(Images/mario.bmp) error: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	};

	main_platform = SDL_LoadBMP("Images/main_platform.bmp");
	if (main_platform == NULL) {
		printf("SDL_LoadBMP(Images/main_platform.bmp) error: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	};

	medium_platform = SDL_LoadBMP("Images/medium_platform.bmp");
	if (medium_platform == NULL) {
		printf("SDL_LoadBMP(Images/medium_platform.bmp) error: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	};

	small_platform = SDL_LoadBMP("Images/small_platform.bmp");
	if (small_platform == NULL) {
		printf("SDL_LoadBMP(Images/small_platform.bmp) error: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	};

	top_platform = SDL_LoadBMP("Images/top_platform.bmp");
	if (top_platform == NULL) {
		printf("SDL_LoadBMP(Images/top_platform.bmp) error: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	};

	final_platform = SDL_LoadBMP("Images/final_platform.bmp");
	if (final_platform == NULL) {
		printf("SDL_LoadBMP(Images/final_platform.bmp) error: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	};

	drabina = SDL_LoadBMP("Images/drabina.bmp");
	if (drabina == NULL) {
		printf("SDL_LoadBMP(Images/drabina.bmp) error: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	};

	barrels = SDL_LoadBMP("Images/barrel_top.bmp");
	if (barrels == NULL) {
		printf("SDL_LoadBMP(Images/barrel_top.bmp) error: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	};

	char text[128];
	int czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	int czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	int niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);

	frames = 0;
	fpsTimer = 0;
	fps = 0;
	quit = 0;
	
	distance = 0;
	etiSpeed = 1;
	
	base = 0;
	
	const Uint8* keys;

	SDL_Surface* mario_l = SDL_LoadBMP("Images/mario_left.bmp");
	SDL_Texture* texture_run = SDL_CreateTextureFromSurface(renderer, mario_l);

	SDL_Surface* run_r = SDL_LoadBMP("Images/run_right.bmp");
	SDL_Texture* texture_run_right = SDL_CreateTextureFromSurface(renderer, run_r);

	SDL_Surface* run_l = SDL_LoadBMP("Images/run_left.bmp");
	SDL_Texture* texture_run_left = SDL_CreateTextureFromSurface(renderer, run_l);

	SDL_Surface* jump_r = SDL_LoadBMP("Images/jump_right.bmp");
	SDL_Texture* texture_jump_right = SDL_CreateTextureFromSurface(renderer, jump_r);

	SDL_Surface* jump_l = SDL_LoadBMP("Images/jump_left.bmp");
	SDL_Texture* texture_jump_left = SDL_CreateTextureFromSurface(renderer, jump_l);

	SDL_Surface* m_ladder = SDL_LoadBMP("Images/m_ladder.bmp");
	SDL_Texture* texture_m_ladder = SDL_CreateTextureFromSurface(renderer, m_ladder);

	SDL_Surface* d_kong = SDL_LoadBMP("Images/donkey_kong_animation.bmp");
	SDL_Texture* texture_d_kong = SDL_CreateTextureFromSurface(renderer, d_kong);

	while (!quit) {
		if (change_level == true) {
			t1 = SDL_GetTicks();
			worldTime = 0;
			horizontal = PLAYER_X;
			vertical = PLAYER_Y;
			if (current_level == 1) {
				current_platform.index = 0;
				copy_array_platform(platforms, platform_stage_one);
				copy_array_ladder(ladders, ladder_stage_one);
			}
			if (current_level == 2) {
				current_platform.index = 0;
				copy_array_platform(platforms, platform_stage_two);
				copy_array_ladder(ladders, ladder_stage_two);
			}
			if (current_level == 3) {
				current_platform.index = 0;
				copy_array_platform(platforms, platform_stage_three);
				copy_array_ladder(ladders, ladder_stage_three);
			}
			initialize_platform(&current_platform, horizontal, vertical, platforms);
			change_level = false;
			end = false;
		}

		t2 = SDL_GetTicks();
		delta = (t2 - t1) * 0.001;
		t1 = t2;

		

		SDL_FillRect(screen, NULL, czarny);

		if (current_level == 1) {
			stage_one(screen, main_platform, medium_platform, small_platform, top_platform, final_platform, drabina, platforms, ladders);
		}
		else if (current_level == 2) {
			stage_two(screen, main_platform, medium_platform, small_platform, top_platform, final_platform, drabina, platforms, ladders);
		}
		else if (current_level == 3) {
			stage_three(screen, main_platform, medium_platform, small_platform, top_platform, final_platform, drabina, platforms, ladders);
		}

		
		DrawRectangle(screen, GAP_X, GAP_Y, RECTANGLE_WIDTH, RECTANGLE_HEIGHT, czerwony, niebieski);
		sprintf(text, "Czas = %.1lf", worldTime);
		DrawString(screen, TEXT_POSITION_X, TEXT_POSITION_Y, text, charset);

		fpsTimer += delta;
		if (fpsTimer > 0.5) {
			fps = frames * 2;
			frames = 0;
			fpsTimer -= 0.5;
		};

		points(screen, czerwony, niebieski, text, worldTime, charset);

		keys = SDL_GetKeyboardState(NULL);
		if (current_platform.index == 4) {
			stage_end(screen, czerwony, niebieski, text, worldTime, charset, current_level);
			if (current_level == 1 || current_level == 2) {
				if (keys[SDL_SCANCODE_C]) {
					current_level += 1;
					change_level = true;
				}
			}
			else if (current_level == 3) {
				if (keys[SDL_SCANCODE_R]) {
					current_level = 1;
					change_level = true;
				}
			}
			
			end = true;
		}

		if (!end) {
			worldTime += delta;
		}
		if (!keys[SDL_SCANCODE_UP] && !keys[SDL_SCANCODE_DOWN] && !keys[SDL_SCANCODE_LEFT] && !keys[SDL_SCANCODE_RIGHT]) {
			DrawSurface(screen, mario_right, horizontal, vertical);
		}

		if (current_level == 1 || current_level == 2) {
			DrawSurface(screen, barrels, 0, 80);
		}
		else {
			DrawSurface(screen, barrels, 0, 198);
		}

		SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
		
		SDL_RenderCopy(renderer, scrtex, NULL, NULL);
		donkey_kong(renderer, d_kong, texture_d_kong, current_level);
		if (!keys[SDL_SCANCODE_UP] && !keys[SDL_SCANCODE_DOWN] && !keys[SDL_SCANCODE_LEFT] && !keys[SDL_SCANCODE_RIGHT]) {
			SDL_RenderPresent(renderer);
		}
		
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
					case SDLK_1:
						current_level = 1;
						change_level = true;
						break;
					case SDLK_2:
						current_level = 2;
						change_level = true;
						break;
					case SDLK_3:
						current_level = 3;
						change_level = true;
						break;
					case SDLK_n:
						change_level = true;
						break;
					case SDLK_LEFT:
						horizontal -= MOVE_X;
						is_ladder = false;
						if (horizontal < PLAYER_MIN_X) {
							horizontal = PLAYER_MIN_X;
						}
						if (horizontal <= current_platform.next_vertical_change_left && current_platform.is_going_down) {
							vertical -= SLOPE_VERTICAL;
							current_platform.number_of_platform -= 1;
							current_platform.next_vertical_change_left = platforms[current_platform.index][current_platform.number_of_platform][0];
							current_platform.next_vertical_change_right = platforms[current_platform.index][current_platform.number_of_platform][2];

						}
						if (horizontal <= current_platform.next_vertical_change_left && current_platform.is_going_up) {
							vertical += SLOPE_VERTICAL;
							current_platform.number_of_platform -= 1;
							current_platform.next_vertical_change_left = platforms[current_platform.index][current_platform.number_of_platform][0];
							current_platform.next_vertical_change_right = platforms[current_platform.index][current_platform.number_of_platform][2];
						}

						for (int ladder = 0; ladder < MAX_NUMBER_OF_LADDERS; ladder++) {
							if (ladders[current_platform.index][ladder][0] == -1)
								continue;
							if (horizontal >= ladders[current_platform.index][ladder][0]-10 && horizontal <= ladders[current_platform.index][ladder][0] + 18+10) {
								is_ladder = true;
								if (vertical < ladders[current_platform.index][ladder][1] + 90 - 36 && vertical > ladders[current_platform.index][ladder][1] - 131) {
									if (horizontal <= ladders[current_platform.index][ladder][0]) {
										horizontal = ladders[current_platform.index][ladder][0];
									}
								}
								break;
							}
						}

						if (!is_ladder) {
							jumping_left = true;
							base = vertical;
						}

						fall = fall_check(&current_platform, horizontal);
						if (fall && current_platform.index >= 1) {
							fall_goal = platforms[current_platform.index - 1][0][1] - 36;
						}

						break;

					case SDLK_RIGHT:
						is_ladder = false;
						horizontal += MOVE_X;
						
						if (horizontal > PLAYER_MAX_X) {
							horizontal = PLAYER_MAX_X;
						}
						if (horizontal + 24 >= current_platform.next_vertical_change_right && current_platform.is_going_down) {
							vertical += SLOPE_VERTICAL;
							current_platform.number_of_platform += 1;
							current_platform.next_vertical_change_left = platforms[current_platform.index][current_platform.number_of_platform][0];
							current_platform.next_vertical_change_right = platforms[current_platform.index][current_platform.number_of_platform][2];
						}
						if (horizontal + 24 >= current_platform.next_vertical_change_right && current_platform.is_going_up) {
							vertical -= SLOPE_VERTICAL;
							current_platform.number_of_platform += 1;
							current_platform.next_vertical_change_left = platforms[current_platform.index][current_platform.number_of_platform][0];
							current_platform.next_vertical_change_right = platforms[current_platform.index][current_platform.number_of_platform][2];
						}

						for (int ladder = 0; ladder < MAX_NUMBER_OF_LADDERS; ladder++) {
							if (ladders[current_platform.index][ladder][0] == -1)
								continue;
							if (horizontal >= ladders[current_platform.index][ladder][0] - 10 && horizontal <= ladders[current_platform.index][ladder][0] + 18 + 10) {
								is_ladder = true;
								if (vertical < ladders[current_platform.index][ladder][1] + 90 - 36 && vertical > ladders[current_platform.index][ladder][1] - 131) {
									if (horizontal >= ladders[current_platform.index][ladder][0] + 18) {
										horizontal = ladders[current_platform.index][ladder][0] + 18;
									}
								}
								break;
							}
						}

						if (!is_ladder) {
							jumping_right = true;
							base = vertical;
						}

						fall = fall_check(&current_platform, horizontal);
						if (fall && current_platform.index >= 1) {
							fall_goal = platforms[current_platform.index - 1][0][1] - 36;
						}

						break;
					case SDLK_ESCAPE:
						quit = 1;
						break;

					case SDLK_UP:
						is_ladder = false;
						for (int ladder = 0; ladder < MAX_NUMBER_OF_LADDERS; ladder++) {
							if (ladders[current_platform.index][ladder][0] == -1)
								continue;
							if (horizontal >= ladders[current_platform.index][ladder][0] && horizontal <= ladders[current_platform.index][ladder][0] + 18) {
								is_ladder = true;
								jumping = false;
								vertical -= LADDER_VERTICAL;
								if (vertical <= current_platform.next_platform_y - 32){
									vertical = current_platform.next_platform_y - 32;
									is_ladder = false;
									if (should_reinitialize_platform(&current_platform, horizontal, vertical))
										initialize_platform(&current_platform, horizontal, vertical, platforms);
								}
								break;
							}
						}
						if (!is_ladder) {
							jumping = true;
							base = vertical;
						}
						break;


					case SDLK_DOWN:
						for (int ladder = 0; ladder < MAX_NUMBER_OF_LADDERS; ladder++) {
							if (ladders[current_platform.index][ladder][0] == -1)
								continue;
							if (horizontal >= ladders[current_platform.index][ladder][0] && horizontal <= ladders[current_platform.index][ladder][0] + 18) {
								vertical += LADDER_VERTICAL;
								if (vertical >= current_platform.current_y){
									vertical = current_platform.current_y;
									is_ladder = false;
									if (should_reinitialize_platform(&current_platform, horizontal, vertical))
										initialize_platform(&current_platform, horizontal, vertical, platforms);
								}
								break;
							}
						}
						break;
				};
				break;

			case SDL_KEYUP:
				switch (event.key.keysym.sym) {
				case SDLK_LEFT:
					jumping_left = false;
					break;

				case SDLK_RIGHT:
					jumping_right = false;
					break;
				};
				break;
			};
		};

		if (keys[SDL_SCANCODE_RIGHT]) {
			run_right(renderer, horizontal, vertical, run_r, texture_run_right);
		}
		if (keys[SDL_SCANCODE_LEFT]) {
			run_left(renderer, horizontal, vertical, run_r, texture_run_left);
		}
		if (keys[SDL_SCANCODE_UP]) {
			mario_ladder(renderer, horizontal, vertical, m_ladder, texture_m_ladder);
		}
		if (keys[SDL_SCANCODE_DOWN]) {
			mario_ladder(renderer, horizontal, vertical, m_ladder, texture_m_ladder);
		}

		if (jumping && going_up) {
			jump_right(renderer, horizontal, vertical, jump_r, texture_jump_right);
			vertical -= JUMP_VERTICAL;

			if (jumping_left) {
				jump_left(renderer, horizontal, vertical, jump_l, texture_jump_left);
				horizontal -= JUMP_HORIZONTAL;
			}

			if (jumping_right) {
				jump_right(renderer, horizontal, vertical, jump_r, texture_jump_right);
				horizontal += JUMP_HORIZONTAL;
			}

			if ((vertical <= base - 54 && (current_level == 1 || current_level == 2)) ||
				(vertical <= base - 100 && (current_level == 3))) {
				going_up = false;
			}
		}
		else if (jumping && !going_up){
			jump_right(renderer, horizontal, vertical, jump_r, texture_jump_right);
			vertical += JUMP_VERTICAL;

			if (jumping_left) {
				jump_left(renderer, horizontal, vertical, jump_l, texture_jump_left);
				horizontal -= JUMP_HORIZONTAL;
			}

			if (jumping_right) {
				jump_right(renderer, horizontal, vertical, jump_r, texture_jump_right);
				horizontal += JUMP_HORIZONTAL;
			}

			if ((vertical >= base) || (vertical >= current_platform.next_platform_y - 32 && current_level == 3 && (jumping_right || jumping_left))) {
				going_up = true;
				jumping = false;
				if (jumping_right) {
					jumping_right = false;
					if (should_reinitialize_platform(&current_platform, horizontal, vertical))
						initialize_platform(&current_platform, horizontal, vertical, platforms);	
				}

				if (jumping_left) {
					jumping_left = false;
					if (should_reinitialize_platform(&current_platform, horizontal, vertical))
						initialize_platform(&current_platform, horizontal, vertical, platforms);
				}
			}
		}

		if (fall) {
			vertical += FALL_VERTICAL;
			if (vertical >= fall_goal) {
				vertical = fall_goal;
				fall = false;
				current_platform.index -= 1;
				initialize_platform(&current_platform, horizontal, vertical, platforms);
			}
		}

		frames++;
	};

	SDL_FreeSurface(charset);
	SDL_FreeSurface(screen);
	SDL_DestroyTexture(scrtex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
	return 0;
};