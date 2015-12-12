#include "cog/src/cog.h"

#define State_start 1
#define State_running 2
#define State_end 3
#define State_finish 4

#define Event_test 1

typedef struct {
    int32_t level;
    int32_t nplots;
} game;

static cog_state_fsm* fsm;
static game g;


int32_t load_level(cog_state_info info) {
    cog_debugf("loading_level...");
    g.level = 0;
    g.nplots = 3;


    cog_sprite_id bid = cog_sprite_add("../assets/images/game_back.png");
    cog_sprite_set(bid, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=1.0, .h=1.0
        },
        .rot=COG_PI/2
    });


    for(int i=0;i<g.nplots;i++) {
        cog_sprite_id id = cog_sprite_add("../assets/images/plot.png");
        double w = 0.1;
        cog_sprite_set(id, (cog_sprite) {
            .dim=(cog_dim2) {
                .w=w, .h=0.1
            },
            .pos=(cog_pos2) {
                .x=-0.4 + (w*2)*i, .y=0.1
            },
            .rot=COG_PI/2
        });
    }

    return State_running;
}

int32_t level_running(cog_state_info info) {
    //cog_debugf("running_level...");
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
             .window_h = 600,
             .fullscreen = false);
    fsm = cog_state_fsm_alloc();
    cog_state_fsm_add_transitions(fsm, transitions,
                                  (sizeof(transitions) /
                                   sizeof(*transitions)));
    cog_state_fsm_set_state(fsm, State_start);
    cog_start_main_loop(main_loop);
}
