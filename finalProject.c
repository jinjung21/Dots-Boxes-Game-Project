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

bool is_horizontal;
bool valid;
int turn; // 0 if red turn, 1 if blue turn
int score;

bool is_occupied[40] = {0};

void plot_pixel(int x, int y, short int line_color);
void draw_horizontal(int x0, int y, int x1, short int line_color);
void draw_vertical(int x, int y0, int y1, short int line_color);
void draw_line(short int x0, short int y0, short int x1, short int y1, short int color);
void swap(short int* e1, short int* e2);
void wait_for_vsync();
void clear_screen();

void plot_grid(); // Prints the game board.
void add_score();
void print_red_num(int num); // Print score of red team from 0~9
void print_blue_num(int num); // Print score of blue team from 0~9
void print_turn(int turn); // Print "RED turn" or "BLUE turn"
void apply_move(bool is_horizontal, int x, int y, short int color); // If valid, color that line with the respective color.
void fill_box(int x, int y, short int color); // Color in the box with the respective color.

volatile int pixel_buffer_start;

int main(void)
{
    volatile int* pixel_ctrl_ptr = (int*)0xFF203020;
    // declare other variables(not shown)
    // initialize location and direction of rectangles(not shown)

    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the
                                        // back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
    clear_screen(); // pixel_buffer_start points to the pixel buffer
    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer
    clear_screen(); // pixel_buffer_start points to the pixel buffer

    while (1) {
        /* Erase any boxes and lines that were drawn in the last iteration */
        clear_screen();

        plot_grid();

        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    }
}

void plot_pixel(int x, int y, short int line_color)
{
    *(short int*)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

void draw_horizontal(int x0, int y, int x1, short int line_color)
{
    for (int i = x0; i < x1; i++) {
        plot_pixel(i, y, line_color);
    }
}

void draw_vertical(int x, int y0, int y1, short int line_color)
{
    for (int i = y0; i < y1; i++) {
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
            plot_pixel(x, y, WHITE);
        }
    }
}

void plot_grid()
{ // Prints the game board.
    clear_screen();

    for (int i = 20; i < 220; i++) {
        plot_pixel(i, 20, BLACK);
        plot_pixel(i, 70, BLACK);
        plot_pixel(i, 120, BLACK);
        plot_pixel(i, 170, BLACK);
        plot_pixel(i, 220, BLACK);
        plot_pixel(20, i, BLACK);
        plot_pixel(70, i, BLACK);
        plot_pixel(120, i, BLACK);
        plot_pixel(170, i, BLACK);
        plot_pixel(220, i, BLACK);
    }
    draw_horizontal(269, 78, 271, BLACK);
    draw_horizontal(269, 79, 271, BLACK);
    draw_horizontal(269, 82, 271, BLACK);
    draw_horizontal(269, 83, 271, BLACK);
}

void add_score()
{
}

void print_red_num(int num)
{ // print score
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

void print_blue_num(int num)
{ // print score

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
    draw_horizontal(231, 175, 245, BLACK); // print "T"
    draw_vertical(238, 175, 204, BLACK);

    draw_vertical(252, 185, 204, BLACK); // print "U"
    draw_horizontal(252, 204, 266, BLACK);
    draw_vertical(266, 185, 204, BLACK);
	draw_vertical(252, 175, 178, BLACK);
	draw_vertical(266, 175, 178, BLACK);

    draw_vertical(273, 175, 204, BLACK); // print "R"
    draw_horizontal(273, 175, 287, BLACK);
    draw_vertical(287, 175, 189, BLACK);
    draw_horizontal(273, 189, 287, BLACK);
    draw_line(273, 189, 287, 204, BLACK);

    draw_vertical(294, 175, 204, BLACK); // print "N"
    draw_line(294, 175, 308, 204, BLACK);
    draw_vertical(308, 175, 204, BLACK);

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

void apply_move(bool is_horizontal, int x, int y, short int color) { //If valid, color that line with the respective color.
    int i = 0;
    if (is_horizontal) {
        if (x == 20 && y == 20) {
            i = 0;
        }
        else if (x == 70 && y == 20) {
            i = 1;
        }
        else if (x == 120 && y == 20) {
            i = 2;
        }
        else if (x == 170 && y == 20) {
            i = 3;
        }
        else if (x == 20 && y == 70) {
            i = 4;
        }
        else if (x == 70 && y == 70) {
            i = 5;
        }
        else if (x == 120 && y == 70) {
            i = 6;
        }
        else if (x == 170 && y == 70) {
            i = 7;
        }
        else if (x == 20 && y == 120) {
            i = 8;
        }
        else if (x == 70 && y == 120) {
            i = 9;
        }
        else if (x == 120 && y == 120) {
            i = 10;
        }
        else if (x == 170 && y == 120) {
            i = 11;
        }
        else if (x == 20 && y == 170) {
            i = 12;
        }
        else if (x == 70 && y == 170) {
            i = 13;
        }
        else if (x == 120 && y == 170) {
            i = 14;
        }
        else if (x == 170 && y == 170) {
            i = 15;
        }
        else if (x == 20 && y == 220) {
            i = 16;
        }
        else if (x == 70 && y == 220) {
            i = 17;
        }
        else if (x == 120 && y == 220) {
            i = 18;
        }
        else if (x == 170 && y == 220) {
            i = 19;
        }
    }
    else if (!is_horizontal) {
        if (x == 20 && y == 20) {
            i = 20;
        }
        else if (x == 20 && y == 70) {
            i = 21;
        }
        else if (x == 20 && y == 120) {
            i = 22;
        }
        else if (x == 20 && y == 170) {
            i = 23;
        }
        else if (x == 70 && y == 20) {
            i = 24;
        }
        else if (x == 70 && y == 70) {
            i = 25;
        }
        else if (x == 70 && y == 120) {
            i = 26;
        }
        else if (x == 770 && y == 170) {
            i = 27;
        }
        else if (x == 120 && y == 20) {
            i = 28;
        }
        else if (x == 120 && y == 70) {
            i = 29;
        }
        else if (x == 120 && y == 120) {
            i = 30;
        }
        else if (x == 120 && y == 170) {
            i = 31;
        }
        else if (x == 170 && y == 20) {
            i = 32;
        }
        else if (x == 170 && y == 70) {
            i = 33;
        }
        else if (x == 170 && y == 120) {
            i = 34;
        }
        else if (x == 170 && y == 170) {
            i = 35;
        }
        else if (x == 220 && y == 20) {
            i = 36;
        }
        else if (x == 220 && y == 70) {
            i = 37;
        }
        else if (x == 220 && y == 120) {
            i = 38;
        }
        else if (x == 220 && y == 170) {
            i = 39;
        }
    }

    if (!(is_occupied[i])) {
        if (is_horizontal) {
            draw_horizontal(x, y, x + 50, color);
            is_occupied[i] = true;
        } 
        else if (!is_horizontal) {
            draw_vertical(x, y, y + 50, color);
            is_occupied[i] = true;
        }
    }
}

void fill_box(int x, int y, short int color)
{ // Color in the box with the respective color.
    for (int i = x; i < x + 50; i++) {
        for (int j = y; j < y + 50; j++) {
            plot_pixel(i, j, color);
        }
    }
}
