#pragma once

#include <joybus/target/gc_controller.h>

void input_source_wavebird_init(void);
void input_source_wavebird_process(void);
void input_source_wavebird_handle_button_press(void);
struct joybus_gc_controller *input_source_wavebird_get_controller(void);