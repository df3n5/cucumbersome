#include "cog/src/cog.h"

#include <stdio.h>

#define State_start 1
#define State_running 2
#define State_endscreen_loading 3
#define State_endscreen_running 4
#define State_end 5
#define State_finish 6

#define Event_test 1

// Constants
#define MaxPlots 10
#define PlotOutlineX -0.4
#define PlotW 0.1
#define DOffsetX 1.25*E
#define ArrowOffsetX 2.5*E
#define E 0.161  // Golden ratio
#define GrowTime 3000  //ms
#define WaterTime 10000  //ms on how long between waterings
#define WaterBenefitTime 10000  //ms on how much water gives you
#define NLevels 3

int32_t level_times[] = {
    10000,
    20000,
    30000
};

int32_t level_goals[] = {
    2,
    3,
    4
};

int32_t plot_amounts[] = {
    3,
    4,
    5
};

typedef enum {
    Backwards,
    Frontwards
} direction;

typedef enum {
    Idle,
    Planted,
    Watered,
    Grown
} plot_state;

typedef struct {
    int32_t level;
    int32_t level_timer;
    int32_t nplots;
    cog_rect* sky;
    cog_sprite* player;
    cog_sprite* plot_outline;
    cog_sprite* d_key;
    cog_sprite* arrow;
    cog_sprite* sun;
    int32_t pos;
    int32_t max_pos;
    direction player_dir;
    int32_t score;
    cog_text* score_text;
    //Plots
    plot_state plot_states[MaxPlots];
    int32_t grow_timer[MaxPlots];
    int32_t water_timer[MaxPlots];
    cog_sprite* grow_sprites[MaxPlots];
    cog_sprite* grown_sprites[MaxPlots];
    cog_sprite* watered_sprites[MaxPlots];
} game;

static cog_state_fsm* fsm;
static game g;

int32_t load_level(cog_state_info info) {
    cog_clear();
    cog_debugf("loading_level...");
    // Init game
    // TODO : Change based on level_no
    g.nplots = plot_amounts[g.level];
    g.pos = 0;
    g.max_pos = plot_amounts[g.level]-1;
    g.player_dir = Frontwards;
    for(int i = 0; i < MaxPlots; i++) {
        g.plot_states[i] = Idle;
        g.grow_timer[i] = 0;
        g.water_timer[i] = 0;
        g.grow_sprites[i] = 0;
    }
    g.score = 0;
    g.level_timer = level_times[g.level];

    cog_rect_id rid = cog_rect_add();
    cog_rect_set(rid, (cog_rect) {
        .dim=(cog_dim2) {
            .w=1.0, .h=1.0
        },
        .col=(cog_color) {
            .r=9,.g=9,.b=1,.a=1
        },
        .layer=1
    });
    g.sky = cog_rect_get(rid);

    cog_sprite_id sid = cog_sprite_add("../assets/images/sun.png");
    cog_sprite_set(sid, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=E, .h=E
        },
        .pos=(cog_pos2) {
            .x=0, .y=0
        },
        .layer=2
    });
    g.sun = cog_sprite_get(sid);

    cog_sprite_id bid = cog_sprite_add("../assets/images/game_back.png");
    cog_sprite_set(bid, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=1.0, .h=1.0
        },
        .layer=2
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
            .layer=3
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
        .layer=10
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
        .layer=4
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
        .layer=5
    });

    cog_text_id id = cog_text_add();
    cog_text_set(id, (cog_text) {
        .scale = (cog_dim2) {.w=0.004, .h=0.004},
        .dim = (cog_dim2) {.w=2.0, .h=0.003},
        .pos = (cog_pos2) {.x=1.25*E, .y=-4.4*E},
        .col=(cog_color) {
            .r=0,.g=0,.b=0,.a=1
        },
        .layer=5
    });
    cog_text_set_str(id, "plant");

    // Score UI
    cog_sprite_id cucumber_id = cog_sprite_add("../assets/images/cucumber.png");
    cog_sprite_set(cucumber_id, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=0.1, .h=0.1
        },
        .pos=(cog_pos2) {
            .x=-2.5*E, .y=-5.5*E
        },
        .layer=5
    });
    cog_text_id tid = cog_text_add();
    cog_text_set(tid, (cog_text) {
        .scale = (cog_dim2) {.w=0.004, .h=0.004},
        .dim = (cog_dim2) {.w=2.0, .h=0.003},
        .pos = (cog_pos2) {.x=1.25*E, .y=-6.0*E},
        .col=(cog_color) {
            .r=0,.g=0,.b=0,.a=1
        },
        .layer=5
    });
    cog_text_set_str(tid, "0");
    g.score_text = cog_text_get(tid);

    cog_sprite* player = cog_sprite_get(pid);

    cog_sprite_id d_id = cog_sprite_add("../assets/images/d_key.png");
    cog_sprite_set(d_id, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=0.5*E, .h=0.5*E
        },
        .pos=(cog_pos2) {
            .x=player->pos.x + DOffsetX, .y=player->pos.y
        },
        .layer=5
    });

    cog_sprite_id r_id = cog_sprite_add("../assets/images/right_arrow.png");
    cog_sprite_set(r_id, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=0.5*E, .h=0.5*E
        },
        .pos=(cog_pos2) {
            .x=player->pos.x + ArrowOffsetX, .y=player->pos.y
        },
        .layer=5
    });

    g.plot_outline = cog_sprite_get(plot_outline_id);
    g.player = cog_sprite_get(pid);
    g.d_key = cog_sprite_get(d_id);
    g.arrow = cog_sprite_get(r_id);

    return State_running;
}

void increment_score() {
    g.score++;
    cog_text_set_str(g.score_text->id, "%d", g.score);
}

double lerp(double y0, double y1, double x) {
    double x0 = 0.0;
    double x1 = 1.0;
    return y0 + ((y1 - y0)*((x-x0)/(x1-x0)));
}

int32_t level_running(cog_state_info info) {
    //cog_debugf("running_level...");
    uint32_t delta_millis = cog_time_delta_millis();
    // State transitions for plants
    for(int i = 0;i<MaxPlots;i++) {
        if(g.plot_states[i] == Planted || g.plot_states[i] == Watered) {
            // Update grow timer to tell if it is ready to be harvested
            if(g.grow_timer[i] > 0) {
                g.grow_timer[i] -= delta_millis;
                if(g.grow_timer[i] <= 0) {
                    g.grow_timer[i] = 0;
                    g.plot_states[i] = Grown;
                    cog_sprite_remove(g.grow_sprites[i]->id);
                    cog_sprite_id id = cog_sprite_add("../assets/images/planted_1.png");
                    cog_sprite_set(id, (cog_sprite) {
                        .dim=(cog_dim2) {
                            .w=PlotW, .h=0.1
                        },
                        .pos=(cog_pos2) {
                            .x=PlotOutlineX + (PlotW*2)*i, .y=0.1
                        },
                        .layer=5
                    });
                    g.grown_sprites[i] = cog_sprite_get(id);
                }
            }

            // Update water timer to tell if it is ready to be watered again
            if(g.water_timer[i] > 0) {
                g.water_timer[i] -= delta_millis;
                
                if(g.water_timer[i] <= 0) {
                    cog_sprite_remove(g.watered_sprites[i]->id);
                    g.water_timer[i] = 0;
                    g.plot_states[i] = Planted;
                }
            }
        }
    }

    // End game logic
    g.level_timer -= delta_millis;
    if(g.level_timer < 0) {
        cog_debugf("End level");
        return State_endscreen_loading;
    }

    // get time as value between 0 and 1
    double t = 1.0 - (g.level_timer / (double)level_times[g.level]);

    // Move sun
    double ratio = 0.91;
    double angle = lerp((COG_PI*ratio), (0.09*COG_PI), t);
    g.sun->pos.x = 0.8 * cog_math_cosf(angle);
    g.sun->pos.y = 0.8 * cog_math_sinf(angle);

    // Lerp sky colour
    g.sky->col.r = lerp(0.968, 0.031, t);
    g.sky->col.g = lerp(0.984, 0.18, t);
    g.sky->col.b = lerp(1.0, 0.4196, t);
/*
    if(t<0.5) {
        t*=2.0;
        g.sky->col.r = lerp(0.968, 0.031, t);
        g.sky->col.g = lerp(0.984, 0.18, t);
        g.sky->col.b = lerp(1.0, 0.4196, t);
    } else {
        t = (t - 0.5) * 2.0;
        g.sky->col.r = lerp(0.031, 0.0, t);
        g.sky->col.g = lerp(0.18, 0.0, t);
        g.sky->col.b = lerp(0.4196, 0.0, t);
    }
*/

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
            //cog_debugf("d pressed, pos is %d %d %d", g.pos, g.max_pos, g.pos == g.max_pos);
            double new_x = PlotOutlineX + ((PlotW * 2) * g.pos);
            g.plot_outline->pos.x = new_x;
            g.player->pos.x = new_x;
            g.d_key->pos.x = new_x + DOffsetX;
            g.arrow->pos.x = new_x + ArrowOffsetX;
        }
        if(key == ' ') {
            //cog_debugf("space pressed");
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
                    .layer=5
                });
                g.grow_sprites[g.pos] = cog_sprite_get(id);
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
                    .layer=5
                });
                g.watered_sprites[g.pos] = cog_sprite_get(id);
                // Take some time off the clock
                g.grow_timer[g.pos] -= WaterBenefitTime;
                if(g.grow_timer[g.pos] < 0) g.grow_timer[g.pos] = 1; // Make it happen next check
                g.water_timer[g.pos] = WaterTime;
            } else if(g.plot_states[g.pos] == Grown) {
                cog_sprite_remove(g.grown_sprites[g.pos]->id);
                g.plot_states[g.pos] = Idle;

                // Remove water if it exists
                if(g.water_timer > 0) {
                    cog_sprite_remove(g.watered_sprites[g.pos]->id);
                    g.water_timer[g.pos] = 0;
                }
                increment_score();
            }
        }
    }

    return State_running;
}

int32_t load_endscreen(cog_state_info info) {
    cog_clear();

    cog_sprite_id bid = cog_sprite_add("../assets/images/end_back.png");
    cog_sprite_set(bid, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=1.0, .h=1.0
        },
        .layer=2
    });

    cog_text_id id = cog_text_add();
    cog_text_set(id, (cog_text) {
        .scale = (cog_dim2) {.w=0.004, .h=0.004},
        .dim = (cog_dim2) {.w=1.5, .h=0.003},
        .pos = (cog_pos2) {.x=-1.0 + E, .y=4.4*E},
        .col=(cog_color) {
            .r=0,.g=0,.b=0,.a=1
        },
        .layer=5
    });
    char* retry_text[] = {"retry", "continue to"};
    int level = g.level;
    int retry_index = 0;
    if(g.score >= level_goals[g.level]) {
        retry_index = 1;
        level += 1;
    }

    cog_text_set_str(id, "Let's call it a  night.\n\n\nHarvested: %d\n\nGoal: %d\n\n\nPress enter to%s day %d", g.score, level_goals[g.level], retry_text[retry_index], level + 1);

    if(g.score >= level_goals[g.level]) {
        g.level++; //Progress to next level
    }

    return State_endscreen_running;
}

int32_t endscreen_running(cog_state_info info) {
    return State_endscreen_running;
}

int32_t endscreen_running_keypress(cog_state_info info) {
    uint32_t key = cog_input_key_code_pressed();
    cog_debugf("Key is %d", key);
    if(key == 13) {
        return State_start;
    }
    return State_endscreen_running;
}

cog_state_transition transitions[] = {
    {State_start, Event_test, &load_level},
    {State_running, Event_test, &level_running},
    {State_running, COG_E_KEYDOWN, &level_running_keypress},
    {State_endscreen_loading, Event_test, &load_endscreen},
    {State_endscreen_running, Event_test, &endscreen_running},
    {State_endscreen_running, COG_E_KEYDOWN, &endscreen_running_keypress},
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
    g.level = 0;
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
