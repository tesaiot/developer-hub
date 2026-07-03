/* QWA309 native: pot -> RGB matrix mixer. 3 pots (R/G/B) drive the DFR0522. */
#include "app_interface.h"
#include "pot_rgb_mixer_ui.h"
void example_main(lv_obj_t *parent) { (void)parent; pot_rgb_mixer_ui_create(); }
