#include "cog/src/cog.h"

#define State_start 1
#define State_running 2
#define State_end 3
#define State_finish 4

#define Event_test 1

// Constants
#define MaxPlots 10
#define PlotOutlineX -0.4
#define PlotW 0.1
#define DOffsetX 1.25*E
#define ArrowOffsetX 2.5*E
#define E 0.161  // Golden ratio
#define GrowTime 10000  //ms

typedef enum {
    Backwards,
    Frontwards
} direction;

typedef enum {
    Idle,
    Planted,
    Watered
} plot_state;

typedef struct {
    int32_t level;
    int32_t nplots;
    cog_sprite* player;
    cog_sprite* plot_outline;
    cog_sprite* d_key;
    cog_sprite* arrow;
    int32_t pos;
    int32_t max_pos;
    direction player_dir;
    plot_state plot_states[MaxPlots];
    int32_t grow_timer[MaxPlots];
    cog_sprite* grow_sprites[MaxPlots];
} game;

static cog_state_fsm* fsm;
static game g;

int32_t load_level(cog_state_info info) {
    cog_debugf("loading_level...");
    // Init game
    g.level = 0;
    // TODO : Change based on level_no
    g.nplots = 3;
    g.pos = 0;
    g.max_pos = 2;
    g.player_dir = Frontwards;
    for(int i = 0; i < MaxPlots; i++) {
        g.plot_states[i] = Idle;
        g.grow_timer[i] = 0;
        g.grow_sprites[i] = 0;
    }


    cog_sprite_id bid = cog_sprite_add("../assets/images/game_back.png");
    cog_sprite_set(bid, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=1.0, .h=1.0
        },
    });


    // Plots
    for(int i=0;i<g.nplots;i++) {
        cog_sprite_id id = cog_sprite_add("../assets/images/plot.png");
        cog_sprite_set(id, (cog_sprite) {
            .dim=(cog_dim2) {
                .w=PlotW, .h=0.1
            },
            .pos=(cog_pos2) {
                .x=PlotOutlineX + (PlotW*2)*i, .y=0.1
            },
        });
    }

    // Plot outline
    cog_sprite_id plot_outline_id = cog_sprite_add("../assets/images/plot_outline.png");
    double w = 0.1;
    cog_sprite_set(plot_outline_id, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=w, .h=0.1
        },
        .pos=(cog_pos2) {
            .x=PlotOutlineX, .y=0.1
        },
        .layer=12
    });

    // Player
    cog_sprite_id pid = cog_sprite_add("../assets/images/player.png");
    cog_sprite_set(pid, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=0.1, .h=0.1
        },
        .pos=(cog_pos2) {
            .x=-0.4, .y=-E
        },
    });

    // UI
    cog_sprite_id spacebar_id = cog_sprite_add("../assets/images/spacebar.png");
    cog_sprite_set(spacebar_id, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=0.3, .h=0.1
        },
        .pos=(cog_pos2) {
            .x=-2.5*E, .y=-4*E
        },
    });

    cog_text_id id = cog_text_add();
    cog_text_set(id, (cog_text) {
        .scale = (cog_dim2) {.w=0.004, .h=0.004},
        .dim = (cog_dim2) {.w=2.0, .h=0.003},
        .pos = (cog_pos2) {.x=1.25*E, .y=-4.4*E},
        .col=(cog_color) {
            .r=0,.g=0,.b=0,.a=1
        }
    });
    cog_text_set_str(id, "plant");

    cog_sprite* player = cog_sprite_get(pid);

    cog_sprite_id d_id = cog_sprite_add("../assets/images/d_key.png");
    cog_sprite_set(d_id, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=0.5*E, .h=0.5*E
        },
        .pos=(cog_pos2) {
            .x=player->pos.x + DOffsetX, .y=player->pos.y
        },
    });

    cog_sprite_id r_id = cog_sprite_add("../assets/images/right_arrow.png");
    cog_sprite_set(r_id, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=0.5*E, .h=0.5*E
        },
        .pos=(cog_pos2) {
            .x=player->pos.x + ArrowOffsetX, .y=player->pos.y
        },
    });

    g.plot_outline = cog_sprite_get(plot_outline_id);
    g.player = cog_sprite_get(pid);
    g.d_key = cog_sprite_get(d_id);
    g.arrow = cog_sprite_get(r_id);

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
            g.d_key->pos.x = new_x + DOffsetX;
            g.arrow->pos.x = new_x + ArrowOffsetX;
        }
        if(key == ' ') {
            cog_debugf("space pressed");
            if(g.plot_states[g.pos] == Idle) {
                g.plot_states[g.pos] = Planted;
                g.grow_timer[g.pos] = GrowTime;

                cog_sprite_id id = cog_sprite_add("../assets/images/planted_0.png");
                cog_sprite_set(id, (cog_sprite) {
                    .dim=(cog_dim2) {
                        .w=PlotW, .h=0.1
                    },
                    .pos=(cog_pos2) {
                        .x=PlotOutlineX + (PlotW*2)*g.pos, .y=0.1
                    },
                    .layer=10
                });
            } else if(g.plot_states[g.pos] == Planted) {
                g.plot_states[g.pos] = Watered;
                cog_sprite_id id = cog_sprite_add("../assets/images/plot_watered.png");
                cog_sprite_set(id, (cog_sprite) {
                    .dim=(cog_dim2) {
                        .w=PlotW, .h=0.1
                    },
                    .pos=(cog_pos2) {
                        .x=PlotOutlineX + (PlotW*2)*g.pos, .y=0.1
                    },
                });
            }
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
