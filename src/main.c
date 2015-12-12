#include "cog/src/cog.h"

#define State_start 1
#define State_running 2
#define State_end 3
#define State_finish 4

#define Event_test 1

static cog_state_fsm* fsm;

int32_t load_level(cog_state_info info) {
    cog_debugf("loading_level...");
    return State_running;
}

int32_t level_running(cog_state_info info) {
    cog_debugf("running_level...");
    return State_running;
}

cog_state_transition transitions[] = {
    {State_start, Event_test, &load_level},
    {State_running, Event_test, &level_running}
};

void main_loop(void) {
    cog_state_fsm_update(fsm);
}

int main(int argc, char* argv[]) {
    cog_init(.window_w = 800,
             .window_h = 400,
             .fullscreen = false);
    fsm = cog_state_fsm_alloc();
    cog_state_fsm_add_transitions(fsm, transitions,
                                  (sizeof(transitions) /
                                   sizeof(*transitions)));
    cog_state_fsm_set_state(fsm, State_start);
    cog_start_main_loop(main_loop);
}
