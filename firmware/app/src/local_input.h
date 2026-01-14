#pragma once

#include <joybus/joybus.h>

typedef void (*local_input_cb_t)(struct joybus_gc_controller_input *input);

void local_input_init(local_input_cb_t callback);
void local_input_set_motor_state(bool enabled);
void local_input_process();