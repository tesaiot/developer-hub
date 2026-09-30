/**
 * @file    pages.h
 * @brief   Forward declarations for all widget demo sub-pages
 *
 * Each page_xxx_create() builds its demo inside the given parent.
 * The parent is a full-screen container provided by the grid menu.
 */

#ifndef PAGES_H
#define PAGES_H

#include "pse84_common.h"

void page_hello_create(lv_obj_t *parent);
void page_button_create(lv_obj_t *parent);
void page_arc_create(lv_obj_t *parent);
void page_checkbox_create(lv_obj_t *parent);
void page_switch_create(lv_obj_t *parent);
void page_progress_create(lv_obj_t *parent);
void page_spinner_create(lv_obj_t *parent);
void page_dropdown_create(lv_obj_t *parent);
void page_textarea_create(lv_obj_t *parent);
void page_slider_create(lv_obj_t *parent);
void page_table_create(lv_obj_t *parent);

#endif /* PAGES_H */
