/*******************************************************************
 *
 * main.c - LVGL simulator for GNU/Linux
 *
 * Based on the original file from the repository
 *
 * @note eventually this file won't contain a main function and will
 * become a library supporting all major operating systems
 *
 * To see how each driver is initialized check the
 * 'src/lib/display_backends' directory
 *
 * - Clean up
 * - Support for multiple backends at once
 *   2025 EDGEMTech Ltd.
 *
 * Author: EDGEMTech Ltd, Erik Tagirov (erik.tagirov@edgemtech.ch)
 *
 ******************************************************************/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"

#include "src/lib/driver_backends.h"
#include "src/lib/simulator_util.h"
#include "src/lib/simulator_settings.h"

// whatever works man, cmake is kinda confusing
#define SMC_DESKTOP
#include "menu/alarm.cpp"
#include "menu/menu.h"
#include "menu/menu.cpp"
#include "menu/lvgl_homescreen.h"
#include "menu/lvgl_homescreen.cpp"

#include "ui.h"

static char * selected_backend;

/* Internal functions */
static void configure_simulator(int argc, char ** argv);

/* Global simulator settings, defined in lv_linux_backend.c */
extern simulator_settings_t settings;

static void configure_simulator(int argc, char ** argv)
{
    driver_backends_register();
    settings.fullscreen    = true;
    settings.window_width  = 320;
    settings.window_height = 240;
}

int main(int argc, char ** argv)
{

    configure_simulator(argc, argv);

    lv_init();

    if(driver_backends_init_backend(selected_backend) == -1) {
        die("Failed to initialize display backend");
    }
    if(driver_backends_init_backend("EVDEV") == -1) {
        die("Failed to initialize evdev");
    }

    test_menu();

    while(1) {
        smc_internal_loop();
        lv_timer_handler();
    }

    return 0;
}
