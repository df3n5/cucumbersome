#include "cog/src/cog.h"

#include <stdio.h>

#define State_start 1
#define State_running 2
#define State_endscreen_loading 3
#define State_endscreen_running 4
#define State_buyscreen_loading 5
#define State_buyscreen_running 6
#define State_end 7
#define State_finish 8

#define Event_test 1

// Constants
#define MaxPlots 10
#define PlotOutlineX -0.6
#define UITextXOffset -0.05
#define PlotW 0.1
#define PlotH 0.04
#define PlotHWell 0.14
#define PlotHOffsetWell 0.10
#define DOffsetX 1.25*E
#define ArrowOffsetX 2.5*E
#define E 0.161  // Golden ratio
#define GrowTime 10000  //ms
#define WaterTime 5000  //ms on how long between waterings
#define WaterBenefitTime 3000  //ms on how much water gives you
#define NLevels 3
#define RandRangeWaterTime 1000  // ms
#define RandRangeGrowTime 5000  // ms


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

/*
int32_t plot_amounts[] = {
    3,
    4,
    5
};
*/

typedef enum {
    Backwards,
    Frontwards
} direction;

typedef enum {
    Idle,
    Planted,
    Watered,
    Grown,
    Well,
    Seeds
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
    cog_text* action_text;
    int32_t water_left;
    int32_t seeds_left;
    int32_t well_water;  // # waters in the well
    int32_t seed_amount;  // # seeds can carry at once
    bool won;
    cog_text* water_text;
    cog_text* seeds_text;
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
    g.pos = 0;
    cog_debugf("nplots %d", g.nplots);
    g.max_pos = g.nplots+1;
    g.player_dir = Frontwards;
    for(int i = 0; i < MaxPlots; i++) {
        g.plot_states[i] = Idle;
        g.grow_timer[i] = 0;
        g.water_timer[i] = 0;
        g.grow_sprites[i] = 0;
    }
    g.plot_states[0] = Seeds;  // First thing is seeds
    g.plot_states[1] = Well;  // Second thing is a well.
    g.score = 0;
    g.level_timer = level_times[g.level];
    g.water_left = 0;
    g.seeds_left = 0;

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
        .pos=(cog_pos2) {
            .x=0.0, .y=-0.3
        },
        .dim=(cog_dim2) {
            .w=1.0, .h=0.7
        },
        .layer=2
    });

    // Plots
    for(int i=0;i<g.nplots;i++) {
        cog_sprite_id id = cog_sprite_add("../assets/images/plot2.png");
        cog_sprite_set(id, (cog_sprite) {
            .dim=(cog_dim2) {
                .w=PlotW, .h=PlotH
            },
            .pos=(cog_pos2) {
                .x=PlotOutlineX + (PlotW*2)*(i+2), .y=PlotH
            },
            .layer=3
        });
    }
    // Well
    cog_sprite_id well_id = cog_sprite_add("../assets/images/well.png");
    cog_sprite_set(well_id, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=0.15, .h=0.15
        },
        .pos=(cog_pos2) {
            .x=-0.39, .y=0.08
        },
        .layer=10
    });

    // Seeds 
    cog_sprite_id seeds_id = cog_sprite_add("../assets/images/seeds.png");
    cog_sprite_set(seeds_id, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=0.15, .h=0.15
        },
        .pos=(cog_pos2) {
            .x=-0.59, .y=0.08
        },
        .layer=10
    });



    // Plot outline
    cog_sprite_id plot_outline_id = cog_sprite_add("../assets/images/plot_outline.png");
    double w = 0.1;
    cog_sprite_set(plot_outline_id, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=w, .h=PlotHWell
        },
        .pos=(cog_pos2) {
            .x=PlotOutlineX, .y=PlotHOffsetWell
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
            .x=PlotOutlineX, .y=-E
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
        .pos = (cog_pos2) {.x=UITextXOffset, .y=-4.4*E},
        .col=(cog_color) {
            .r=0,.g=0,.b=0,.a=1
        },
        .layer=5
    });
    cog_text_set_str(id, "plant");
    g.action_text = cog_text_get(id);

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
        .pos = (cog_pos2) {.x=UITextXOffset, .y=-6.0*E},
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

    // Status of 2 resources

    cog_sprite_id water_id = cog_sprite_add("../assets/images/watering_can.png");
    cog_sprite_set(water_id, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=0.3*E, .h=0.3*E
        },
        .pos=(cog_pos2) {
            .x=player->pos.x-0.1*E, .y=player->pos.y-1.3*E
        },
        .layer=5
    });
    cog_sprite* water = cog_sprite_get(water_id);
    cog_text_id water_tid = cog_text_add();
    cog_text_set(water_tid, (cog_text) {
        .scale = (cog_dim2) {.w=0.001, .h=0.001},
        .dim = (cog_dim2) {.w=2.0, .h=0.003},
        .pos = (cog_pos2) {.x=player->pos.x+0.2*E, .y=water->pos.y-0.15*E},
        .col=(cog_color) {
            .r=0,.g=0,.b=0,.a=1
        },
        .layer=10
    });
    cog_text_set_str(water_tid, "0");
    g.water_text = cog_text_get(water_tid);

    cog_sprite_id seeds2_id = cog_sprite_add("../assets/images/seeds.png");
    cog_sprite_set(seeds2_id, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=0.5*E, .h=0.5*E
        },
        .pos=(cog_pos2) {
            .x=player->pos.x, .y=player->pos.y-2*E
        },
        .layer=5
    });

    cog_sprite* seeds = cog_sprite_get(seeds2_id);
    cog_text_id seeds_tid = cog_text_add();
    cog_text_set(seeds_tid, (cog_text) {
        .scale = (cog_dim2) {.w=0.001, .h=0.001},
        .dim = (cog_dim2) {.w=2.0, .h=0.003},
        .pos = (cog_pos2) {.x=seeds->pos.x+0.2*E, .y=seeds->pos.y-0.1*E},
        .col=(cog_color) {
            .r=0,.g=0,.b=0,.a=1
        },
        .layer=10
    });
    cog_text_set_str(seeds_tid, "0");
    g.seeds_text = cog_text_get(seeds_tid);

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
                    cog_sprite_id id = cog_sprite_add("../assets/images/plantedv2_1.png");
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


    // Change action text based on state of plot player is on
    g.action_text->col = (cog_color) {.r=0.0, .g=0, .b=0, .a=1};  // default colour
    //cog_debugf("pos : %d", g.plot_states[g.pos]);
    if(g.plot_states[g.pos] == Idle) {
        if(g.seeds_left > 0) {
            cog_text_set_str(g.action_text->id, "plant");
        } else {
            cog_text_set_str(g.action_text->id, "no seeds");
            g.action_text->col = (cog_color) {.r=1.0, .g=0, .b=0, .a=1};
        }
    } else if (g.plot_states[g.pos] == Watered || g.plot_states[g.pos] == Planted) {
        if(g.water_left > 0) {
            cog_text_set_str(g.action_text->id, "water");
        } else {
            cog_debugf("NO Water_left %d", g.water_left);
            cog_text_set_str(g.action_text->id, "no water");
            g.action_text->col = (cog_color) {.r=1.0, .g=0, .b=0, .a=1};
        }
    } else if (g.plot_states[g.pos] == Grown) {
        cog_text_set_str(g.action_text->id, "pick");
    } else if (g.plot_states[g.pos] == Well) {
        cog_text_set_str(g.action_text->id, "fill water");
    } else if (g.plot_states[g.pos] == Seeds) {
        cog_text_set_str(g.action_text->id, "get seeds");
    }

    // End game logic
    g.level_timer -= delta_millis;
    if(g.level_timer < 0) {
        cog_debugf("End level");
        return State_endscreen_loading;
    }

    cog_text_set_str(g.seeds_text->id, "%d", g.seeds_left);
    cog_text_set_str(g.water_text->id, "%d", g.water_left);

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
            if(g.plot_states[g.pos] == Well || g.plot_states[g.pos] == Seeds) {
                g.plot_outline->pos.y = PlotHOffsetWell;
                g.plot_outline->dim.h = PlotHWell;
            } else {
                g.plot_outline->pos.y = PlotH;
                g.plot_outline->dim.h = PlotH;
            }
            g.plot_outline->pos.x = new_x;
            g.player->pos.x = new_x;
            g.d_key->pos.x = new_x + DOffsetX;
            g.arrow->pos.x = new_x + ArrowOffsetX;
        }
        if(key == ' ') {
            cog_debugf("space pressed on %d in state %d", g.pos, g.plot_states[g.pos]);
            if(g.plot_states[g.pos] == Idle) {
                if(g.seeds_left > 0) {
                    g.seeds_left--;
                    g.plot_states[g.pos] = Planted;
                    g.grow_timer[g.pos] = GrowTime;
                    g.grow_timer[g.pos] -= cog_rand_int(0, RandRangeGrowTime);
                    cog_debugf("Grow time is now %d", g.grow_timer[g.pos]);

                    cog_sprite_id id = cog_sprite_add("../assets/images/plantedv2_0.png");
                    cog_sprite_set(id, (cog_sprite) {
                        .dim=(cog_dim2) {
                            .w=PlotW, .h=0.1
                        },
                        .pos=(cog_pos2) {
                            .x=PlotOutlineX + (PlotW*2)*g.pos, .y=0.1
                        },
                        .layer=6
                    });
                    g.grow_sprites[g.pos] = cog_sprite_get(id);
                }
            } else if(g.plot_states[g.pos] == Planted || g.plot_states[g.pos] == Watered) {
                if(g.water_left > 0) {
                    g.water_left--;
                    cog_debugf("water_left : %d", g.water_left);

                    // Only add new sprite if something has changed
                    if(g.plot_states[g.pos] == Planted) {
                        cog_sprite_id id = cog_sprite_add("../assets/images/plot_watered.png");
                        cog_sprite_set(id, (cog_sprite) {
                            .dim=(cog_dim2) {
                                .w=PlotW, .h=PlotH
                            },
                            .pos=(cog_pos2) {
                                .x=PlotOutlineX + (PlotW*2)*g.pos, .y=PlotH
                            },
                            .layer=5
                        });
                        g.watered_sprites[g.pos] = cog_sprite_get(id);

                        g.plot_states[g.pos] = Watered;
                        // Take some time off the clock
                        g.grow_timer[g.pos] -= WaterBenefitTime;
                        if(g.grow_timer[g.pos] < 0) g.grow_timer[g.pos] = 1; // Make it happen next check
                        g.water_timer[g.pos] = WaterTime;
                        g.water_timer[g.pos] -= cog_rand_int(0, RandRangeWaterTime); // Give some variety to this
                        cog_debugf("Water time is now %d", g.water_timer[g.pos]);
                    }

                }
            } else if(g.plot_states[g.pos] == Grown) {
                cog_sprite_remove(g.grown_sprites[g.pos]->id);
                g.plot_states[g.pos] = Idle;

                // Remove water if it exists
                if(g.water_timer > 0) {
                    cog_sprite_remove(g.watered_sprites[g.pos]->id);
                    g.water_timer[g.pos] = 0;
                }
                increment_score();
            } else if(g.plot_states[g.pos] == Well) {
                g.water_left = g.well_water;
            } else if(g.plot_states[g.pos] == Seeds) {
                g.seeds_left = g.seed_amount;
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
    g.won = false;
    if(g.score >= level_goals[g.level]) {
        g.level++; //Progress to next level
        g.won = true;
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
        if(g.won) {
            // TODO :Credits logic here
            return State_buyscreen_loading;
        } else {
            return State_start;
        }
    }
    return State_endscreen_running;
}

int32_t load_buyscreen(cog_state_info info) {
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
    cog_text_set_str(id, "Choose your   upgrade:");

    cog_sprite_id id1 = cog_sprite_add("../assets/images/1_key.png");
    cog_sprite_set(id1, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=0.5*E, .h=0.5*E
        },
        .pos=(cog_pos2) {
            .x=-2*E, .y=1*E
        },
        .layer=5
    });

    cog_sprite_id idplot = cog_sprite_add("../assets/images/plot2.png");
    cog_sprite_set(idplot, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=PlotW, .h=PlotH
        },
        .pos=(cog_pos2) {
            .x=1*E, .y=1*E
        },
        .layer=3
    });


    cog_sprite_id id2 = cog_sprite_add("../assets/images/2_key.png");
    cog_sprite_set(id2, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=0.5*E, .h=0.5*E
        },
        .pos=(cog_pos2) {
            .x=-2*E, .y=-1*E
        },
        .layer=5
    });

    cog_sprite_id idseeds = cog_sprite_add("../assets/images/seeds.png");
    cog_sprite_set(idseeds, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=0.8*E, .h=0.8*E
        },
        .pos=(cog_pos2) {
            .x=1*E, .y=-1*E
        },
        .layer=3
    });

    cog_sprite_id id3 = cog_sprite_add("../assets/images/3_key.png");
    cog_sprite_set(id3, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=0.5*E, .h=0.5*E
        },
        .pos=(cog_pos2) {
            .x=-2*E, .y=-3*E
        },
        .layer=5
    });

    cog_sprite_id idwater = cog_sprite_add("../assets/images/watering_can.png");
    cog_sprite_set(idwater, (cog_sprite) {
        .dim=(cog_dim2) {
            .w=0.5*E, .h=0.5*E
        },
        .pos=(cog_pos2) {
            .x=1*E, .y=-3*E
        },
        .layer=3
    });



    if(g.score >= level_goals[g.level]) {
        g.level++; //Progress to next level
    }

    return State_buyscreen_running;
}

int32_t buyscreen_running(cog_state_info info) {
    return State_buyscreen_running;
}

int32_t buyscreen_running_keypress(cog_state_info info) {
    uint32_t key = cog_input_key_code_pressed();
    cog_debugf("Key is %d", key);
    if(key == '1') {
        g.nplots++;
        return State_start;
    } else if(key == '2') {
        g.seed_amount++;
        return State_start;
    } else if(key == '3') {
        g.well_water++;
        return State_start;
    }
    return State_buyscreen_running;
}

cog_state_transition transitions[] = {
    {State_start, COG_E_DUMMY, &load_level},
    {State_running, COG_E_DUMMY, &level_running},
    {State_running, COG_E_KEYDOWN, &level_running_keypress},
    {State_endscreen_loading, COG_E_DUMMY, &load_endscreen},
    {State_endscreen_running, COG_E_DUMMY, &endscreen_running},
    {State_endscreen_running, COG_E_KEYDOWN, &endscreen_running_keypress},
    {State_buyscreen_loading, COG_E_DUMMY, &load_buyscreen},
    {State_buyscreen_running, COG_E_DUMMY, &buyscreen_running},
    {State_buyscreen_running, COG_E_KEYDOWN, &buyscreen_running_keypress},
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
    g.well_water = 1;
    g.seed_amount = 1;
    g.nplots = 2;

    cog_init(.window_w = 800,
             .window_h = 800,
             .fullscreen = false,
             .debug = true);
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
