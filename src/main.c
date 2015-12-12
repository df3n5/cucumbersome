#include "cog/src/cog.h"

#define State_start 1
#define State_running 2
#define State_end 3
#define State_finish 4

#define Event_test 1

// Constants
#define PlotOutlineX -0.4
#define PlotW 0.1
#define E 0.161  // Golden ratio

typedef enum {
    Backwards,
    Frontwards
} direction;

typedef struct {
    int32_t level;
    int32_t nplots;
    cog_sprite* player;
    cog_sprite* plot_outline;
    int32_t pos;
    int32_t max_pos;
    direction player_dir;
} game;

static cog_state_fsm* fsm;
static game g;

int32_t load_level(cog_state_info info) {
    cog_debugf("loading_level...");
    g.level = 0;
    // TODO : Change based on level_no
    g.nplots = 3;
    g.pos = 0;
    g.max_pos = 2;
    g.player_dir = Frontwards;


    cog_sprite_id bid = cog_sprite_add("../assets/images/game_back.png");
    cog_sprite_set(bid, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=1.0, .h=1.0
        },
        .rot=COG_PI/2
    });


    for(int i=0;i<g.nplots;i++) {
        cog_sprite_id id = cog_sprite_add("../assets/images/plot.png");
        cog_sprite_set(id, (cog_sprite) {
            .dim=(cog_dim2) {
                .w=PlotW, .h=0.1
            },
            .pos=(cog_pos2) {
                .x=PlotOutlineX + (PlotW*2)*i, .y=0.1
            },
            .rot=COG_PI/2
        });
    }

    cog_sprite_id plot_outline_id = cog_sprite_add("../assets/images/plot_outline.png");
    double w = 0.1;
    cog_sprite_set(plot_outline_id, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=w, .h=0.1
        },
        .pos=(cog_pos2) {
            .x=PlotOutlineX, .y=0.1
        },
        .rot=COG_PI/2
    });


    cog_sprite_id pid = cog_sprite_add("../assets/images/player.png");
    cog_sprite_set(pid, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=0.1, .h=0.1
        },
        .pos=(cog_pos2) {
            .x=-0.4, .y=-E
        },
        .rot=0
    });

    g.plot_outline = cog_sprite_get(plot_outline_id);
    g.player = cog_sprite_get(pid);

    return State_running;
}

int32_t level_running(cog_state_info info) {
    //cog_debugf("running_level...");
    return State_running;
}

int32_t level_running_keypress(cog_state_info info) {
    if(cog_input_key_pressed()) {
        uint32_t key = cog_input_key_code_pressed();
        if(key == 'd') {
            if(g.player_dir == Frontwards) {
                g.pos++;
            } else {
                g.pos--;
            }
            if(g.pos > g.max_pos) {
                //g.pos = g.max_pos;
                g.pos = 0;
                //g.player_dir = Backwards;
            }
            /*if(g.pos <= 0) {*/
                /*g.pos = 0;*/
                /*g.player_dir = Frontwards;*/
            /*}*/
            cog_debugf("d pressed, pos is %d %d %d", g.pos, g.max_pos, g.pos == g.max_pos);
            double new_x = PlotOutlineX + ((PlotW * 2) * g.pos);
            g.plot_outline->pos.x = new_x;
            g.player->pos.x = new_x;
        }
        if(key == ' ') {
            cog_debugf("space pressed");
        }
    }

    return State_running;
}


cog_state_transition transitions[] = {
    {State_start, Event_test, &load_level},
    {State_running, Event_test, &level_running},
    {State_running, COG_E_KEYDOWN, &level_running_keypress}
};

void main_loop(void) {
/*
        if(cog_input_key_pressed()) {
            uint32_t key = cog_input_key_code_pressed();
            cog_debugf("key is %d", key);
        }
*/
    cog_state_fsm_update(fsm);
}

int main(int argc, char* argv[]) {
    cog_init(.window_w = 800,
             .window_h = 800,
             .fullscreen = false);
    fsm = cog_state_fsm_alloc();
    cog_state_fsm_add_transitions(fsm, transitions,
                                  (sizeof(transitions) /
                                   sizeof(*transitions)));
    cog_state_fsm_set_state(fsm, State_start);
    //cog_start_main_loop(main_loop);

    while(!cog_hasquit()) {
        cog_loopstep();
        main_loop();
    }
}
