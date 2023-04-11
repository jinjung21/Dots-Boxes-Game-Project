#define SDRAM_BASE 0xC0000000
#define FPGA_ONCHIP_BASE 0xC8000000
#define FPGA_CHAR_BASE 0xC9000000

/* Cyclone V FPGA devices */
#define LEDR_BASE 0xFF200000
#define HEX3_HEX0_BASE 0xFF200020
#define HEX5_HEX4_BASE 0xFF200030
#define SW_BASE 0xFF200040
#define KEY_BASE 0xFF200050
#define TIMER_BASE 0xFF202000
#define PIXEL_BUF_CTRL_BASE 0xFF203020
#define CHAR_BUF_CTRL_BASE 0xFF203030

/* VGA colors */
#define WHITE 0xFFFF
#define YELLOW 0xFFE0
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define GREY 0xC618
#define PINK 0xFC18
#define ORANGE 0xFC00
#define BLACK 0x0000

/* Screen size. */
#define RESOLUTION_X 320
#define RESOLUTION_Y 240

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool is_horizontal;
bool valid;
int* turn; // 0 if red turn, 1 if blue turn
int score_red;
int score_blue;
short int color;
int x;
int y;
int delX;
int delY;
char byte1, byte2, byte3;
bool game_start = false;

short int grid_horizontal[5][4] = {
    {WHITE, WHITE, WHITE, WHITE},
    {WHITE, WHITE, WHITE, WHITE},
    {WHITE, WHITE, WHITE, WHITE},
    {WHITE, WHITE, WHITE, WHITE},
    {WHITE, WHITE, WHITE, WHITE}
};
short int grid_vertical[4][5] = {
    {WHITE, WHITE, WHITE, WHITE, WHITE},
    {WHITE, WHITE, WHITE, WHITE, WHITE},
    {WHITE, WHITE, WHITE, WHITE, WHITE},
    {WHITE, WHITE, WHITE, WHITE, WHITE},
};

short int grid0[48][48] = {BLACK};
short int grid1[48][48] = {BLACK};
short int grid2[48][48] = {BLACK};
short int grid3[48][48] = {BLACK};
short int grid4[48][48] = {BLACK};
short int grid5[48][48] = {BLACK};
short int grid6[48][48] = {BLACK};
short int grid7[48][48] = {BLACK};
short int grid8[48][48] = {BLACK};
short int grid9[48][48] = {BLACK};
short int grid10[48][48] = {BLACK};
short int grid11[48][48] = {BLACK};
short int grid12[48][48] = {BLACK};
short int grid13[48][48] = {BLACK};
short int grid14[48][48] = {BLACK};
short int grid15[48][48] = {BLACK};


void plot_pixel(int x, int y, short int line_color);
short int get_pixel(int x, int y);
void draw_horizontal(int x0, int y, int x1, short int line_color);
void draw_vertical(int x, int y0, int y1, short int line_color);
void draw_line(short int x0, short int y0, short int x1, short int y1, short int color);
void swap(short int* e1, short int* e2);
void wait_for_vsync();
void clear_screen();

void plot_grid(); // Prints the game board.
// void add_score(bool is_horizontal, int x, int y, short int color, int i);
void print_red_num(int num); // Print score of red team from 0~9
void print_blue_num(int num); // Print score of blue team from 0~9
void print_turn(int turn); // Print "RED turn" or "BLUE turn"
void apply_move(bool is_horizontal, int x, int y, short int color, int* turn); // If valid, color that line with the respective color.
void fill_box(int x, int y, short int color); // Color in the box with the respective color.
void display_line(bool is_horizontal, int x, int y, short int color); // Display line preview 

void move_line(char dir);
void end_game();
void draw_title();
void update_grid(bool is_horizontal, int x, int y, short int color);
void write_string(int x, int y, char str[]);
void write_char(int x, int y, char character);
void clear_all_text();
void reset_game();
void add_red_score();
void add_blue_score();

void disable_A9_interrupts (void);
void set_A9_IRQ_stack (void);
void config_GIC(void);
void config_PS2();
void enable_A9_interrupts(void);
void PS2_ISR(void);
void config_interrupt(int, int);

volatile int pixel_buffer_start;

typedef enum {
	title,
    gamestart,
	winner
} gamestate;

gamestate state;

int main(void)
{
    volatile int* pixel_ctrl_ptr = (int*)0xFF203020;

    *turn = 0;
    score_red = 0;
    score_blue = 0;
    is_horizontal = 1;
    x = 20;
    y = 20;
    delX = 0;
    delY = 0;
    color = RED;

    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the
                                        // back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
    clear_screen();
    draw_title();
    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer

    disable_A9_interrupts();
    set_A9_IRQ_stack(); 
    config_GIC();
    config_PS2(); 
    enable_A9_interrupts();
        
    draw_title();

    state = title;

    clear_all_text();
    write_string(30, 52, "Press [SPACE] to Begin");

    // instructions
    write_string(5, 26, "I N S T R U C T I O N S");
    write_string(9, 30, "W : Move Up");
    write_string(9, 33, "A : Move Left");
    write_string(9, 36, "S : Move Down");
    write_string(9, 39, "D : Move Right");
    write_string(9, 42, "[SHIFT] : Change Orientation");
    write_string(9, 45, "[ENTER] : Place Line");

    write_string(45, 30, "Player 1: RED    Player 2: BLUE");
    write_string(43, 33, "A player that completes the fourth");
    write_string(45, 35, "side of a box conquers the box.");
    write_string(45, 37, "The player with more boxes WIN!");

    while (1) {   
        switch (state) {
            case title:
                if (game_start) {
                    state = gamestart;
                }
                break;
            case gamestart:
                if (*turn == 0)
                    color = RED;
                else if (*turn == 1)
                    color = BLUE;

                clear_screen();
                clear_all_text();
                plot_grid();
                display_line(is_horizontal, x, y, color);
                print_red_num(score_red);
                print_blue_num(score_blue);
                print_turn(*turn);

                wait_for_vsync();
				pixel_buffer_start = *(pixel_ctrl_ptr + 1);
				break;   
            case winner:
 				clear_screen();
                clear_all_text();
				if (score_blue == 9) {
                    write_string(35, 30, "Winner : BLUE");
                } else if (score_red == 9) {
                    write_string(35, 30, "Winner : RED");
                }
                write_string(30, 52, "Press [SPACE] to Restart");

                reset_game();

                wait_for_vsync();
				pixel_buffer_start = *(pixel_ctrl_ptr + 1);
				break;                             
        }
    }
}

void plot_pixel(int x, int y, short int line_color)
{
    *(short int*)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

short int get_pixel(int x, int y) {
	return *(short int *)(*(int *)0xFF203020 + (y << 10) + (x << 1));
}

void draw_horizontal(int x0, int y, int x1, short int line_color)
{
	int i = 0;
    for (i = x0; i < x1; i++) {
        plot_pixel(i, y, line_color);
    }
}

void draw_vertical(int x, int y0, int y1, short int line_color)
{
	int i = 0;
    for (i = y0; i < y1; i++) {
        plot_pixel(x, i, line_color);
    }
}

void draw_line(short int x0, short int y0, short int x1, short int y1, short int color)
{
    bool is_steep = abs(y1 - y0) > abs(x1 - x0);

    if(is_steep){
        swap(&x0, &y0); 
        swap(&x1, &y1);
    }
    if(x0 > x1){
        swap(&x0, &x1);
        swap(&y0, &y1);
    }

    int deltax = x1 - x0;
    int deltay = abs(y1 - y0); 
    int error = -(deltax / 2);
    int y = y0; 
    
    int y_step;

    if(y0 < y1){
        y_step = 1;
    }else{
        y_step = -1;
    }

    int x;
    for(x = x0; x <= x1; x++){
        
        if(is_steep){
            plot_pixel(y,x, color);
        }
        else{
            plot_pixel(x,y, color);
        }
        
        error = error + deltay;
        if(error >= 0){
            y = y + y_step;
            error = error - deltax;
        }
    }
}

void swap(short int* e1, short int* e2)
{
    int temp = *e1;
    *e1 = *e2;
    *e2 = temp;
}

void wait_for_vsync()
{
    volatile int* pixel_ctrl_ptr = (int*)0xFF203020;
    int status;
    *pixel_ctrl_ptr = 1;
    status = *(pixel_ctrl_ptr + 3);
    while ((status & 0x01) != 0) {
        status = *(pixel_ctrl_ptr + 3);
    }
    return;
}

void clear_screen()
{
    int x, y;

    for (x = 0; x < 320; ++x) {
        for (y = 0; y < 240; ++y) {
            plot_pixel(x, y, BLACK);
        }
    }
}

void update_grid(bool is_horizontal, int x, int y, short int color) {
    int i = (x - 20) / 50;
    int j = (y - 20) / 50;

    if (is_horizontal) {
        grid_horizontal[j][i] = color;
    } else if (!is_horizontal) {
        grid_vertical[j][i] = color;
    }
}

void plot_grid() { 
	
    draw_horizontal(269,78,271,WHITE);
    draw_horizontal(269,79,271,WHITE);
    draw_horizontal(269,82,271,WHITE);
    draw_horizontal(269,83,271,WHITE);

    draw_horizontal(20, 20, 70, grid_horizontal[0][0]);
    draw_horizontal(70, 20, 120, grid_horizontal[0][1]);
    draw_horizontal(120, 20, 170, grid_horizontal[0][2]);
    draw_horizontal(170, 20, 220, grid_horizontal[0][3]);

    draw_horizontal(20, 70, 70, grid_horizontal[1][0]);
    draw_horizontal(70, 70, 120, grid_horizontal[1][1]);
    draw_horizontal(120, 70, 170, grid_horizontal[1][2]);
    draw_horizontal(170, 70, 220, grid_horizontal[1][3]);

    draw_horizontal(20, 120, 70, grid_horizontal[2][0]);
    draw_horizontal(70, 120, 120, grid_horizontal[2][1]);
    draw_horizontal(120, 120, 170, grid_horizontal[2][2]);
    draw_horizontal(170, 120, 220, grid_horizontal[2][3]);

    draw_horizontal(20, 170, 70, grid_horizontal[3][0]);
    draw_horizontal(70, 170, 120, grid_horizontal[3][1]);
    draw_horizontal(120, 170, 170, grid_horizontal[3][2]);
    draw_horizontal(170, 170, 220, grid_horizontal[3][3]);
    
    draw_horizontal(20, 220, 70, grid_horizontal[4][0]);
    draw_horizontal(70, 220, 120, grid_horizontal[4][1]);
    draw_horizontal(120, 220, 170, grid_horizontal[4][2]);
    draw_horizontal(170, 220, 220, grid_horizontal[4][3]);

    draw_vertical(20, 20, 70, grid_vertical[0][0]);
    draw_vertical(20, 70, 120, grid_vertical[1][0]);
    draw_vertical(20, 120, 170, grid_vertical[2][0]);
    draw_vertical(20, 170, 220, grid_vertical[3][0]);

    draw_vertical(70, 20, 70, grid_vertical[0][1]);
    draw_vertical(70, 70, 120, grid_vertical[1][1]);
    draw_vertical(70, 120, 170, grid_vertical[2][1]);
    draw_vertical(70, 170, 220, grid_vertical[3][1]);

    draw_vertical(120, 20, 70, grid_vertical[0][2]);
    draw_vertical(120, 70, 120, grid_vertical[1][2]);
    draw_vertical(120, 120, 170, grid_vertical[2][2]);
    draw_vertical(120, 170, 220, grid_vertical[3][2]);
    
    draw_vertical(170, 20, 70, grid_vertical[0][3]);
    draw_vertical(170, 70, 120, grid_vertical[1][3]);
    draw_vertical(170, 120, 170, grid_vertical[2][3]);
    draw_vertical(170, 170, 220, grid_vertical[3][3]);

    draw_vertical(220, 20, 70, grid_vertical[0][4]);
    draw_vertical(220, 70, 120, grid_vertical[1][4]);
    draw_vertical(220, 120, 170, grid_vertical[2][4]);
    draw_vertical(220, 170, 220, grid_vertical[3][4]);
	int i = 0;
    for (i = 21; i < 69; i++) {
        draw_horizontal(21, i, 69, grid0[0][0]);
        draw_horizontal(71, i, 119, grid1[0][0]);
        draw_horizontal(121, i, 169, grid2[0][0]);
        draw_horizontal(171, i, 219, grid3[0][0]);
    }
	int j = 0;
    for (j = 71; j < 119; j++) {
        draw_horizontal(21, j, 69, grid4[0][0]);
        draw_horizontal(71, j, 119, grid5[0][0]);
        draw_horizontal(121, j, 169, grid6[0][0]);
        draw_horizontal(171, j, 219, grid7[0][0]);
    }
	int k=0;
    for (k = 121; k < 169; k++) {
        draw_horizontal(21, k, 69, grid8[0][0]);
        draw_horizontal(71, k, 119, grid9[0][0]);
        draw_horizontal(121, k, 169, grid10[0][0]);
        draw_horizontal(171, k, 219, grid11[0][0]);
    }
	int m = 0;
    for (m = 171; m < 219; m++) {
        draw_horizontal(21, m, 69, grid12[0][0]);
        draw_horizontal(71, m, 119, grid13[0][0]);
        draw_horizontal(121, m, 169, grid14[0][0]);
        draw_horizontal(171, m, 219, grid15[0][0]);
    }
}

void draw_title(){
    int x, y;

    for (x = 0; x < 320; ++x) {
        for (y = 0; y < 240; ++y) {
            plot_pixel(x, y, BLACK);
        }
    }
}

void write_string(int x, int y, char str[]) {
	int i = 0;
	for (i = 0; i < strlen(str); i++) {
		write_char(x + i, y, str[i]);
	}
}

void write_char(int x, int y, char character) {
	volatile int charBuffer = 0xC9000000;
	*(char *)(charBuffer + (y << 7) + x) = character;
}

void clear_all_text() {
	int x,y;
	for (x = 0; x < 80; x++) {
		for (y = 0; y < 60; y++) {
			write_char(x, y, ' ');
		}
	}
}

void print_red_num(int num) { // print score
    if (num == 0) {
        draw_horizontal(230, 45, 265, RED);
        draw_horizontal(230, 116, 265, RED);
        draw_vertical(230, 45, 116, RED);
        draw_vertical(265, 45, 116, RED);
    } else if (num == 1) {
        draw_vertical(250, 45, 116, RED);
    } else if (num == 2) {
        draw_horizontal(230, 45, 265, RED);
        draw_vertical(265, 45, 80, RED);
        draw_horizontal(230, 80, 265, RED);
        draw_vertical(230, 80, 116, RED);
        draw_horizontal(230, 116, 265, RED);
    } else if (num == 3) {
        draw_horizontal(230, 45, 265, RED);
        draw_vertical(265, 45, 80, RED);
        draw_horizontal(230, 80, 265, RED);
        draw_vertical(265, 80, 116, RED);
        draw_horizontal(230, 116, 265, RED);
    } else if (num == 4) {
        draw_vertical(230, 45, 80, RED);
        draw_horizontal(230, 80, 265, RED);
        draw_vertical(265, 45, 116, RED);
    } else if (num == 5) {
        draw_horizontal(230, 45, 265, RED);
        draw_vertical(230, 45, 80, RED);
        draw_horizontal(230, 80, 265, RED);
        draw_vertical(265, 80, 116, RED);
        draw_horizontal(230, 116, 265, RED);
    } else if (num == 6) {
        draw_vertical(230, 45, 116, RED);
        draw_horizontal(230, 80, 265, RED);
        draw_vertical(265, 80, 116, RED);
        draw_horizontal(230, 116, 265, RED);
    } else if (num == 7) {
        draw_horizontal(230, 45, 265, RED);
        draw_vertical(265, 45, 116, RED);
    } else if (num == 8) {
        draw_horizontal(230, 45, 265, RED);
        draw_horizontal(230, 116, 265, RED);
        draw_vertical(230, 45, 116, RED);
        draw_vertical(265, 45, 116, RED);
        draw_horizontal(230, 80, 265, RED);
    } else if (num == 9) {
        draw_horizontal(230, 45, 265, RED);
        draw_vertical(230, 45, 80, RED);
        draw_horizontal(230, 80, 265, RED);
        draw_vertical(265, 45, 116, RED);
    }
}

void print_blue_num(int num) { // print score
    if (num == 0) {
        draw_horizontal(274, 45, 309, BLUE);
        draw_horizontal(274, 116, 309, BLUE);
        draw_vertical(274, 45, 116, BLUE);
        draw_vertical(309, 45, 116, BLUE);
    } else if (num == 1) {
        draw_vertical(294, 45, 116, BLUE);
    } else if (num == 2) {
        draw_horizontal(274, 45, 309, BLUE);
        draw_vertical(309, 45, 80, BLUE);
        draw_horizontal(274, 80, 309, BLUE);
        draw_vertical(274, 80, 116, BLUE);
        draw_horizontal(274, 116, 309, BLUE);
    } else if (num == 3) {
        draw_horizontal(274, 45, 309, BLUE);
        draw_vertical(309, 45, 80, BLUE);
        draw_horizontal(274, 80, 309, BLUE);
        draw_vertical(309, 80, 116, BLUE);
        draw_horizontal(274, 116, 309, BLUE);
    } else if (num == 4) {
        draw_vertical(274, 45, 80, BLUE);
        draw_horizontal(274, 80, 309, BLUE);
        draw_vertical(309, 45, 116, BLUE);
    } else if (num == 5) {
        draw_horizontal(274, 45, 309, BLUE);
        draw_vertical(274, 45, 80, BLUE);
        draw_horizontal(274, 80, 309, BLUE);
        draw_vertical(309, 80, 116, BLUE);
        draw_horizontal(274, 116, 309, BLUE);
    } else if (num == 6) {
        draw_vertical(274, 45, 116, BLUE);
        draw_horizontal(274, 80, 309, BLUE);
        draw_vertical(309, 80, 116, BLUE);
        draw_horizontal(274, 116, 309, BLUE);
    } else if (num == 7) {
        draw_horizontal(274, 45, 309, BLUE);
        draw_vertical(309, 45, 116, BLUE);
    } else if (num == 8) {
        draw_horizontal(274, 45, 309, BLUE);
        draw_horizontal(274, 116, 309, BLUE);
        draw_vertical(274, 45, 116, BLUE);
        draw_vertical(309, 45, 116, BLUE);
        draw_horizontal(274, 80, 309, BLUE);
    } else if (num == 9) {
        draw_horizontal(274, 45, 309, BLUE);
        draw_vertical(274, 45, 80, BLUE);
        draw_horizontal(274, 80, 309, BLUE);
        draw_vertical(309, 45, 116, BLUE);
    }
}

void print_turn(int turn){ // Print "RED turn" or "BLUE turn"
    //print "TURN"
    draw_horizontal(231, 175, 245, WHITE); // print "T"
    draw_vertical(238, 175, 204, WHITE);

    draw_vertical(252, 185, 204, WHITE); // print "U"
    draw_horizontal(252, 204, 266, WHITE);
    draw_vertical(266, 185, 204, WHITE);
	draw_vertical(252, 175, 178, WHITE);
	draw_vertical(266, 175, 178, WHITE);

    draw_vertical(273, 175, 204, WHITE); // print "R"
    draw_horizontal(273, 175, 287, WHITE);
    draw_vertical(287, 175, 189, WHITE);
    draw_horizontal(273, 189, 287, WHITE);
    draw_line(273, 189, 287, 204, WHITE);

    draw_vertical(294, 175, 204, WHITE); // print "N"
    draw_line(294, 175, 308, 204, WHITE);
    draw_vertical(308, 175, 204, WHITE);

    if (turn == 0){ // print "RED"
        draw_vertical(241, 135, 164, RED); // print "R"
        draw_horizontal(241, 135, 255, RED);
        draw_vertical(255, 135, 149, RED);
        draw_horizontal(241, 149, 255, RED);
        draw_line(241, 149, 255, 164, RED);

        draw_vertical(263, 135, 164, RED); // print "E"
        draw_horizontal(263, 135, 277, RED);
        draw_horizontal(263, 149, 277, RED);
        draw_horizontal(263, 164, 277, RED);

        draw_vertical(284, 135, 164, RED); // print "D"
        draw_horizontal(284, 135, 291, RED);
        draw_line(291, 135, 298, 142, RED);
        draw_vertical(298, 142, 157, RED);
        draw_line(291, 164, 298, 157, RED);
        draw_horizontal(284, 164, 291, RED);
    }
    else if (turn == 1){ // print "BLUE"
        draw_vertical(231, 135, 164, BLUE); // print "B"
        draw_vertical(245, 135, 164, BLUE);
        draw_horizontal(231, 135, 245, BLUE);
        draw_horizontal(231, 149, 245, BLUE);
        draw_horizontal(231, 164, 245, BLUE);

        draw_vertical(252, 135, 164, BLUE); // print "L"
        draw_horizontal(252, 164, 266, BLUE);

        draw_vertical(273, 135, 164, BLUE); // print "U"
        draw_horizontal(273, 164, 287, BLUE);
        draw_vertical(287, 135, 164, BLUE);

        draw_vertical(294, 135, 164, BLUE); // print "E"
        draw_horizontal(294, 135, 308, BLUE);
        draw_horizontal(294, 149, 308, BLUE);
        draw_horizontal(294, 164, 308, BLUE);
    }
}

void apply_move(bool is_horizontal, int x, int y, short int color, int* turn) { //If valid, color that line with the respective color.
    int i = (x - 20) / 50;
    int j = (y - 20) / 50;
    if (is_horizontal) {
        update_grid(is_horizontal, x, y, color);

        if (j == 0){
            if (grid_horizontal[j+1][i] != (short int)WHITE && grid_vertical[j][i] != (short int)WHITE && grid_vertical[j][i+1] != (short int)WHITE){ // square below
                fill_box(x, y, color);
                if (color == (short int)RED){
                    score_red++;
                }
                else if (color == (short int)BLUE){
                    score_blue++;
                }
            }
        }
        else if (j>0 && j<4){

            if (grid_horizontal[j-1][i] != (short int)WHITE && grid_vertical[j-1][i] != (short int)WHITE && grid_vertical[j-1][i+1] != (short int)WHITE){ // square above
                fill_box(x, y - 50, color);
                if (color == (short int)RED){
                    score_red++;
                }
                else if (color == (short int)BLUE){
                    score_blue++;
                }
            }
            if (grid_horizontal[j+1][i] != (short int)WHITE && grid_vertical[j][i] != (short int)WHITE && grid_vertical[j][i+1] != (short int)WHITE){ // square below
                fill_box(x, y, color);
                if (color == (short int)RED){
                    score_red++;
                }
                else if (color == (short int)BLUE){
                    score_blue++;
                }
            }
        }
        else if (j == 4){
            if (grid_horizontal[j-1][i] != (short int)WHITE && grid_vertical[j-1][i] != (short int)WHITE && grid_vertical[j-1][i+1] != (short int)WHITE){ // square above
                fill_box(x, y - 50, color);
                if (color == (short int)RED){
                    score_red++;
                }
                else if (color == (short int)BLUE){
                    score_blue++;
                }
            }
        }

        // if (grid_horizontal[j - 1][i] != (short int)WHITE && grid_vertical[j - 1][i] != (short int)WHITE && grid_vertical[j - 1][i + 1] != (short int)WHITE) {   // Square above
        //     fill_box(x, y - 50, color);
        //     if (color == (short int)RED) {
        //         score_red++;
        //     } else if (color == (short int)BLUE) {
        //         score_blue++;
        //     }
        // }
        // if (grid_horizontal[j + 1][i] != (short int)WHITE && grid_vertical[j][i] != (short int)WHITE && grid_vertical[j][i + 1] != (short int)WHITE) {   // Square below
        //     fill_box(x, y, color);
        //     if (color == (short int)RED) {
        //      score_red++;
        //     } else if (color == (short int)BLUE) {
        //      score_blue++;
        //     }
        // }        

        if (score_blue == 9 || score_red == 9) {
            state = winner;
        }
    } 
    else if (!is_horizontal) {
        update_grid(is_horizontal, x, y, color);

        if (i == 0){
            if (grid_vertical[j][i+1] != (short int)WHITE && grid_horizontal[j][i] != (short int)WHITE && grid_horizontal[j+1][i] != (short int)WHITE){
                fill_box(x, y, color);
                if (color == (short int)RED) {
                    score_red++;
                } else if (color == (short int)BLUE) {
                    score_blue++;
                }
            }
        }
        else if (i > 0 && i < 4){
            if (grid_vertical[j][i-1] != (short int)WHITE && grid_horizontal[j][i-1] != (short int)WHITE && grid_horizontal[j+1][i-1] != (short int)WHITE){
                fill_box(x - 50, y, color);
                if (color == (short int)RED) {
                    score_red++;
                } else if (color == (short int)BLUE) {
                    score_blue++;
                }
            }
            if (grid_vertical[j][i+1] != (short int)WHITE && grid_horizontal[j][i] != (short int)WHITE && grid_horizontal[j+1][i] != (short int)WHITE){
                fill_box(x, y, color);
                if (color == (short int)RED) {
                    score_red++;
                } else if (color == (short int)BLUE) {
                    score_blue++;
                }
            }
        }
        else if (i == 4){
            if (grid_vertical[j][i-1] != (short int)WHITE && grid_horizontal[j][i-1] != (short int)WHITE && grid_horizontal[j+1][i-1] != (short int)WHITE){
                fill_box(x - 50, y, color);
                if (color == (short int)RED) {
                    score_red++;
                } else if (color == (short int)BLUE) {
                    score_blue++;
                }
            }
        }

        // if (grid_vertical[j][i - 1] != (short int)WHITE && grid_horizontal[j][i - 1] != (short int)WHITE && grid_horizontal[j + 1][i - 1] != (short int)WHITE) {   // Square left
        //     fill_box(x - 50, y, color);
        //     if (color == (short int)RED) {
        //      score_red++;
        //     } else if (color == (short int)BLUE) {
        //      score_blue++;
        //     }
        // }
        // if (grid_vertical[j][i + 1] != (short int)WHITE && grid_horizontal[j][i] != (short int)WHITE && grid_horizontal[j + 1][i] != (short int)WHITE) {   // Square right
        //     fill_box(x, y, color);
        //     if (color == (short int)RED) {
        //      score_red++;
        //     } else if (color == (short int)BLUE) {
        //      score_blue++;
        //     }
        // }        

        if (score_blue == 9 || score_red == 9) {
            state = winner;
        }
    }
    *turn = *(turn) ^ 1; // xor
}

void fill_box(int x, int y, short int color) { // Color in the box with the respective color.
    if (x == 20 && y == 20) {
		int i,j;
        for (i = 0; i < 48; i++) {
            for (j = 0; j < 48;j++) {
                grid0[i][j] = color;
            }
        }
    } else if (x == 70 && y == 20) {
		int i,j;
        for (i = 0; i < 48; i++) {
            for (j = 0; j < 48;j++) {
                grid1[i][j] = color;
            }
        }
    } else if (x == 120 && y == 20) {
		int i,j;
        for (i = 0; i < 48; i++) {
            for (j = 0; j < 48;j++) {
                grid2[i][j] = color;
            }
        }
    } else if (x == 170 && y == 20) {
		int i,j;
        for (i = 0; i < 48; i++) {
            for (j = 0; j < 48;j++) {
                grid3[i][j] = color;
            }
        }
    } else if (x == 20 && y == 70) {
		int i,j;
        for (i = 0; i < 48; i++) {
            for (j = 0; j < 48;j++) {
                grid4[i][j] = color;
            }
        }
    } else if (x == 70 && y == 70) {
		int i,j;
        for (i = 0; i < 48; i++) {
            for (j = 0; j < 48;j++) {
                grid5[i][j] = color;
            }
        }
    } else if (x == 120 && y == 70) {
		int i,j;
        for (i = 0; i < 48; i++) {
            for (j = 0; j < 48;j++) {
                grid6[i][j] = color;
            }
        }
    } else if (x == 170 && y == 70) {
		int i,j;
        for (i = 0; i < 48; i++) {
            for (j = 0; j < 48;j++) {
                grid7[i][j] = color;
            }
        }
    } else if (x == 20 && y == 120) {
		int i,j;
        for (i = 0; i < 48; i++) {
            for (j = 0; j < 48;j++) {
                grid8[i][j] = color;
            }
        }
    } else if (x == 70 && y == 120) {
		int i,j;
        for (i = 0; i < 48; i++) {
            for (j = 0; j < 48;j++) {
                grid9[i][j] = color;
            }
        }
    } else if (x == 120 && y == 120) {
		int i,j;
        for (i = 0; i < 48; i++) {
            for (j = 0; j < 48;j++) {
                grid10[i][j] = color;
            }
        }
    } else if (x == 170 && y == 120) {
		int i,j;
        for (i = 0; i < 48; i++) {
            for (j = 0; j < 48;j++) {
                grid11[i][j] = color;
            }
        }
    } else if (x == 20 && y == 170) {
		int i,j;
        for (i = 0; i < 48; i++) {
            for (j = 0; j < 48;j++) {
                grid12[i][j] = color;
            }
        }
    } else if (x == 70 && y == 170) {
		int i,j;
        for (i = 0; i < 48; i++) {
            for (j = 0; j < 48;j++) {
                grid13[i][j] = color;
            }
        }
    } else if (x == 120 && y == 170) {
		int i,j;
        for (i = 0; i < 48; i++) {
            for (j = 0; j < 48;j++) {
                grid14[i][j] = color;
            }
        }
    } else if (x == 170 && y == 170) {
		int i,j;
        for (i = 0; i < 48; i++) {
            for (j = 0; j < 48;j++) {
                grid15[i][j] = color;
            }
        }
    }
}

void display_line(bool is_horizontal, int x, int y, short int color) {
    if (is_horizontal){
		int i = 0;
        for (i = x; i < x + 46; i = i + 8){
            draw_horizontal(i, y, i + 3, color);
        }
    }
    else if (!is_horizontal){
		int i =0;
        for (i = y; i < y + 46; i = i + 8){
            draw_vertical(x, i, i + 3, color);
        }
    }
}

void move_line(char dir) {
    switch (dir) {
        case 'w':
            if (!is_horizontal) {
		if (y == 20 && x == 220){
			delX = 0;
			delY = 0;
		}
		else if (y==20){
			delX = 50;
			delY = 0;
		}
                else if (y > 20) {
                    delX = 0;
                    delY = -50;
                }
            } else if (is_horizontal) {
		if (y == 20 && x == 170){
			delX = 0;
			delY = 0;
		}
		else if (y==20){
			delX = 50;
			delY = 0;
		}
                else if (y > 20) {
                    delX = 0;
                    delY = -50;
                }
            }
            x += delX;
            y += delY;        
            
            display_line(is_horizontal, x, y, color);
            break;

        case 'a':
            if (!is_horizontal) {
		if (x == 20 && y == 170){
			delX = 0;
			delY = 0;
		}
		else if (x == 20){
			delX = 0;
			delY = 50;
		}
                else if (x > 20) {
                    delX = -50;
                    delY = 0;
                }
            } else if (is_horizontal) {
		if (x == 20 && y == 220){
			delX = 0;
			delY = 0;
		}
		else if (x == 20){
			delX = 0;
			delY = 50;
		}
                else if (x > 20) {
                    delX = -50;
                    delY = 0;
                }
            }
            x += delX;
            y += delY;        
            
            display_line(is_horizontal, x, y, color);
            break;

        case 's':
            if (!is_horizontal) {
		if (y == 170 && x == 220){
			delX = 0;
			delY = 0;
		}
		else if (y == 170){
			delX = 50;
			delY = 0;
		}
                else if (y < 170) {
                    delX = 0;
                    delY = 50;
                }
            } else if (is_horizontal) {
		if (y == 220 && x == 170){
			delX = 0;
			delY = 0;
		}
		else if (y == 220){
			delX = 50;
			delY = 0;
		}
                else if (y < 220) {
                    delX = 0;
                    delY = 50;
                }
            }
            x += delX;
            y += delY;        
            
            display_line(is_horizontal, x, y, color);
            break;

        case 'd':
            if (!is_horizontal) {
		if (x == 220 && y == 170){
			delX = 0;
			delY = 0;
		}
		else if (x == 220){
			delX = 0;
			delY = 50;
		}
                else if (x < 220) {
                    delX = 50;
                    delY = 0;
                }
            } else if (is_horizontal) {
		if (x == 170 && y == 220){
			delX = 0;
			delY = 0;
		}
		else if (x == 170){
			delX = 0;
			delY = 50;
		}
                else if (x < 170) {
                    delX = 50;
                    delY = 0;
                }
            }
            x += delX;
            y += delY;        
            
            display_line(is_horizontal, x, y, color);
            break;

        default:
            break;
    }
}

void reset_game() {
	int a,b,c,d,i,j;
    for (a = 0; a < 5; a++) {
        for (b = 0; b < 4; b++) {
            grid_horizontal[a][b] = (short int)WHITE;
        }
    }
    for (c = 0; c < 4; c++) {
        for (d = 0; d < 5; d++) {
            grid_vertical[c][d] = (short int)WHITE;
        }
    }
    for (i = 0; i < 48; i++) {
        for (j = 0; j < 48; j++) {
            grid0[i][j] = (short int)BLACK;
            grid1[i][j] = (short int)BLACK;
            grid2[i][j] = (short int)BLACK;
            grid3[i][j] = (short int)BLACK;
            grid4[i][j] = (short int)BLACK;
            grid5[i][j] = (short int)BLACK;
            grid6[i][j] = (short int)BLACK;
            grid7[i][j] = (short int)BLACK;
            grid8[i][j] = (short int)BLACK;
            grid9[i][j] = (short int)BLACK;
            grid10[i][j] = (short int)BLACK;
            grid11[i][j] = (short int)BLACK;
            grid12[i][j] = (short int)BLACK;
            grid13[i][j] = (short int)BLACK;
            grid14[i][j] = (short int)BLACK;
            grid15[i][j] = (short int)BLACK;
        }
    }
    *turn = 0;
    is_horizontal = 1;
    x = 20;
    y = 20;
    delX = 0;
    delY = 0;
    color = RED;
}

void config_PS2() {
    volatile int * PS2_Control = (int *) 0xFF200104;
    *PS2_Control = 0x00000001; 
}

void __attribute__ ((interrupt)) __cs3_isr_irq (void) {
    // Read the ICCIAR from the CPU Interface in the GIC
    int interrupt_ID = *((int *) 0xFFFEC10C);
    if (interrupt_ID == 79) // check if interrupt is from the PS2
        PS2_ISR ();
    else
        while (1); // if unexpected, then stay here
    // Write to the End of Interrupt Register (ICCEOIR)
    *((int *) 0xFFFEC110) = interrupt_ID;
 }
void __attribute__ ((interrupt)) __cs3_reset (void) {
    while(1);
}
void __attribute__ ((interrupt)) __cs3_isr_undef (void) {
    while(1);  
}
void __attribute__ ((interrupt)) __cs3_isr_swi (void) {
    while(1);
}
void __attribute__ ((interrupt)) __cs3_isr_pabort (void) {
    while(1);
}
void __attribute__ ((interrupt)) __cs3_isr_dabort (void) {
    while(1);
}
void __attribute__ ((interrupt)) __cs3_isr_fiq (void) {
    while(1);
}

void disable_A9_interrupts(void) {
    int status = 0b11010011;
    asm("msr cpsr, %[ps]" : : [ps]"r"(status));
}

void set_A9_IRQ_stack(void) {
    int stack, mode;
    stack = 0xFFFFFFFF - 7; // top of A9 onchip memory, aligned to 8 bytes
    mode = 0b11010010;
    asm("msr cpsr, %[ps]" : : [ps] "r" (mode));
    asm("mov sp, %[ps]" : : [ps] "r" (stack));
    mode = 0b11010011;
    asm("msr cpsr, %[ps]" : : [ps] "r" (mode));
}

void enable_A9_interrupts(void) {
    int status = 0b01010011;
    asm("msr cpsr, %[ps]" : : [ps]"r"(status));
}

void config_GIC(void) {
    config_interrupt (79, 1); // configure the FPGA PS2 interrupt (79)
    // Set Interrupt Priority Mask Register (ICCPMR). Enable all priorities
    *((int *) 0xFFFEC104) = 0xFFFF;
    // Set the enable in the CPU Interface Control Register (ICCICR)
    *((int *) 0xFFFEC100) = 1;
    // Set the enable in the Distributor Control Register (ICDDCR)
    *((int *) 0xFFFED000) = 1;
}

void config_interrupt(int N, int CPU_target) {
    int reg_offset, index, value, address;
    /* Configure the Interrupt Set-Enable Registers (ICDISERn).
    * reg_offset = (integer_div(N / 32) * 4; value = 1 << (N mod 32) */
    reg_offset = (N >> 3) & 0xFFFFFFFC;
    index = N & 0x1F;
    value = 0x1 << index;
    address = 0xFFFED100 + reg_offset;
    /* Using the address and value, set the appropriate bit */
    *(int *)address |= value;
    /* Configure the Interrupt Processor Targets Register (ICDIPTRn)
    * reg_offset = integer_div(N / 4) * 4; index = N mod 4 */
    reg_offset = (N & 0xFFFFFFFC);
    index = N & 0x3;
    address = 0xFFFED800 + reg_offset + index;
    /* Using the address and value, write to (only) the appropriate byte */
    *(char *)address = (char) CPU_target;
}

void PS2_ISR(void) {
    volatile int * PS2_Control = (int *) 0xFF200104;
    *PS2_Control = 1;  // clear RI 
    
    volatile int * PS2_keyboard_ptr = (int *)0xFF200100;
    int PS2_data, RVALID;
    char dir;

    PS2_data = *(PS2_keyboard_ptr); // read the Data register in the PS/2 port
    RVALID = PS2_data & 0x8000; // extract the RVALID field
    if (RVALID){
        byte1 = byte2;
        byte2 = byte3;
        byte3 = PS2_data & 0xFF;


        if ((byte3 == 0x29) && (byte2 == 0xF0)) { // game start (space)
            game_start = true;
            state = game_start;
			score_red = 0;
			score_blue = 0;
        }

        if((byte3 == 0x12) && (byte2 == 0xF0)) { // change horizontal -> vertical (shift)
	    if (is_horizontal){
		if(y == 220){
		    y -= 50;
                    is_horizontal = is_horizontal ^ 1;
		}
		else{
		    is_horizontal = is_horizontal ^ 1;
                }
	    }
	    else if (!is_horizontal){
		if (x == 220){
		    x -= 50;
		    is_horizontal = is_horizontal ^ 1;
		}
		else{
		    is_horizontal = is_horizontal ^ 1;
		}
	    }
        }


        if((byte3 == 0x5A) && (byte2 == 0xF0)) { //draw line 
            if (is_horizontal) {
                if (get_pixel(x + 5, y) == (short int)WHITE)
                    apply_move(is_horizontal, x, y, color, turn);
            } else if (!is_horizontal) {
                if (get_pixel(x, y + 5) == (short int)WHITE)
                    apply_move(is_horizontal, x, y, color, turn);
            }
        }

        if ((byte3 == 0x1D) && (byte2 == 0xF0)) { //case W
            dir = 'w';
            move_line(dir);
        }
        else if ((byte3 == 0x1C) && (byte2 == 0xF0)) { //case A
            dir = 'a';
            move_line(dir);
        }
        else if ((byte3 == 0x1B) && (byte2 == 0xF0)) { //case S
            dir = 's';
            move_line(dir);
        }
        else if ((byte3 == 0x23) && (byte2 == 0xF0)) { //case D
            dir = 'd';
            move_line(dir);
        }
    }
    return;
}
