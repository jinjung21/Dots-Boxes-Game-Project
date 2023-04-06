#define SDRAM_BASE            0xC0000000
#define FPGA_ONCHIP_BASE      0xC8000000
#define FPGA_CHAR_BASE        0xC9000000

/* Cyclone V FPGA devices */
#define LEDR_BASE             0xFF200000
#define HEX3_HEX0_BASE        0xFF200020
#define HEX5_HEX4_BASE        0xFF200030
#define SW_BASE               0xFF200040
#define KEY_BASE              0xFF200050
#define TIMER_BASE            0xFF202000
#define PIXEL_BUF_CTRL_BASE   0xFF203020
#define CHAR_BUF_CTRL_BASE    0xFF203030

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

/* Screen size. */
#define RESOLUTION_X 320
#define RESOLUTION_Y 240

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

bool is_horizontal;
bool valid;
int score;

void plot_pixel(int x, int y, short int line_color);
void draw_horizontal(int x0, int y, int x1, short int line_color);
void draw_vertical(int x, int y0, int y1, short int line_color);
void wait_for_vsync();
void clear_screen();

void plot_grid(int x, int y, short int line_color); //Prints the game board and the current score for both players to the screen.
void add_score();
void print_num(); //Print number 0~16
void print_turn(); //Print "RED turn" or "BLUE turn"
bool is_valid(bool is_horizontal, int x, int y); //True if the selected line is unoccupied.
void apply_move(bool is_horizontal, int x, int y); //If valid, color that line with the respective color.
void fill_box(); //Color in the box with the respective color.

volatile int pixel_buffer_start; 

int main(void)
{
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
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

    while (1)
    {
        /* Erase any boxes and lines that were drawn in the last iteration */
        clear_screen();

        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    }
}

void plot_pixel(int x, int y, short int line_color)
{
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

void draw_horizontal(int x0, int y, int x1, short int line_color) {
    for (int i = x0; i < x1; i++) {
        plot_pixel(i, y, line_color);
    }
}

void draw_vertical(int x, int y0, int y1, short int line_color) {
    for (int i = y0; i < y1; i++) {
        plot_pixel(x, i, line_color);
    }
}

void wait_for_vsync() {
    volatile int* pixel_ctrl_ptr = (int*) 0xFF203020;
    int status;
    *pixel_ctrl_ptr = 1;
    status = *(pixel_ctrl_ptr + 3);
    while((status & 0x01) != 0) {
        status = *(pixel_ctrl_ptr + 3);
    }
    return;
}

void clear_screen() {
    int x, y;
    short int black = 0x0000;

    for (x = 0; x < 320; ++x) {
        for (y = 0; y < 240; ++y) {
            plot_pixel(x, y, black);
        }
    }
}

void plot_grid(int x, int y, short int line_color); //Prints the game board and the current score for both players to the screen.
void add_score();
void print_num(); //Print number 0~16
void print_turn(); //Print "RED turn" or "BLUE turn"
bool is_valid(bool is_horizontal, int x, int y); //True if the selected line is unoccupied.
void apply_move(bool is_horizontal, int x, int y); //If valid, color that line with the respective color.
void fill_box(); //Color in the box with the respective color.
