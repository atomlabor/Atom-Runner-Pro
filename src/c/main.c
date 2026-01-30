#include <pebble.h>

// ---------------------------------------------------------------------------------------
// Atom Runner Pro | atomlabor.de 2026
// ---------------------------------------------------------------------------------------
#define STORAGE_KEY_CONFIG 200
#define STORAGE_KEY_HISTORY 300

#define KEY_SESSION_ACTIVE 400
#define KEY_SESSION_START 401
#define KEY_SESSION_STEPS 402

#define MAX_HISTORY 10

#define KEY_CONF_ORANGE 2000
#define KEY_CONF_RED 2001
#define KEY_CONF_AGE 2002
#define KEY_CONF_STEP 2003
#define KEY_CONF_UNIT 2004
#define KEY_CONF_GREEN 2005

#if defined(PBL_PLATFORM_EMERY)
  #define SCREEN_W 200
  #define SCREEN_H 228
  #define SHOW_LEFT_BAR 1
  #define H_STATUS 26
  #define H_BOTTOM 115 
  #define SIDEBAR_W 14      
  #define TIME_Y 36      
  #define RECT_HR GRect(0, 41, 100, 32)
  #define RECT_DATA_PACE GRect(102, 18, 82, 28) 
  #define RECT_DATA_DIST GRect(102, 46, 82, 68)
  #define HR_CENTER GPoint(50, 57)
  #define HR_RADIUS 46
  #define FONT_DATA_BIG_KEY FONT_KEY_GOTHIC_18_BOLD
  #define FONT_DATA_SMALL_KEY FONT_KEY_GOTHIC_18
  #define FONT_DETAIL_HEADER FONT_KEY_GOTHIC_24_BOLD
  #define FONT_DETAIL_BODY FONT_KEY_GOTHIC_18
  #define FONT_SUMMARY FONT_KEY_GOTHIC_24_BOLD
  #define DETAIL_MARGIN_X 8
  #define DETAIL_MARGIN_Y 0

#elif defined(PBL_ROUND)
  #define SCREEN_W 180
  #define SCREEN_H 180
  #define SHOW_LEFT_BAR 0
  #define H_STATUS 180     
  #define H_BOTTOM 0       
  #define SIDEBAR_W 0
  

  #define TIME_Y 20        
  
  #define RECT_HR GRect(0, 60, 180, 30)
  
  #define RECT_DATA_PACE GRect(0, 92, 180, 24)    
  
  #define RECT_DATA_DIST GRect(20, 112, 140, 45)   
  
  #define HR_CENTER GPoint(90, 90)                
  #define HR_RADIUS 84
  #define FONT_DATA_BIG_KEY FONT_KEY_GOTHIC_24_BOLD
  #define FONT_DATA_SMALL_KEY FONT_KEY_GOTHIC_18_BOLD
  #define FONT_DETAIL_HEADER FONT_KEY_GOTHIC_18_BOLD
  #define FONT_DETAIL_BODY FONT_KEY_GOTHIC_14
  #define FONT_SUMMARY FONT_KEY_GOTHIC_18_BOLD
  #define DETAIL_MARGIN_X 10 
  #define DETAIL_MARGIN_Y 25 

#else
  #define SCREEN_W 144
  #define SCREEN_H 168
  #define SHOW_LEFT_BAR 1
  #define H_STATUS 20   
  #define H_BOTTOM 85
  #define SIDEBAR_W 8
  #define TIME_Y 28
  #define RECT_HR GRect(0, 29, 70, 26)           
  #define RECT_DATA_PACE GRect(74, 10, 62, 20)
  #define RECT_DATA_DIST GRect(74, 30, 62, 50)
  #define HR_CENTER GPoint(35, 42)        
  #define HR_RADIUS 33
  #define FONT_DATA_BIG_KEY FONT_KEY_GOTHIC_14_BOLD
  #define FONT_DATA_SMALL_KEY FONT_KEY_GOTHIC_14
  #define FONT_DETAIL_HEADER FONT_KEY_GOTHIC_18_BOLD
  #define FONT_DETAIL_BODY FONT_KEY_GOTHIC_14
  #define FONT_SUMMARY FONT_KEY_GOTHIC_18_BOLD
  #define DETAIL_MARGIN_X 6
  #define DETAIL_MARGIN_Y 0
#endif

#define COLOR_ACCENT PBL_IF_COLOR_ELSE(GColorIslamicGreen, GColorWhite)
#define COLOR_RECORDING PBL_IF_COLOR_ELSE(GColorRed, GColorBlack)

typedef struct { int zone_green, zone_orange, zone_red, step_len, unit_system; } AppSettings;
typedef struct { time_t timestamp; int dist; int dur; int hr; int steps; } HistoryItem; 
typedef struct { HistoryItem items[MAX_HISTORY]; int count; } HistoryData;
typedef struct { bool active; time_t start_time; int hr_sum, hr_count; int initial_steps; int resumed_steps_offset; } WorkoutState;

static AppSettings s_settings;
static WorkoutState s_state = { .active = false, .resumed_steps_offset = 0 };
static HistoryData s_history = { .count = 0 };

static Window *s_window, *s_history_window, *s_detail_window;
static MenuLayer *s_menu_layer;
static ScrollLayer *s_detail_scroll_layer;
static TextLayer *s_detail_text_layer;
static Layer *s_status_layer, *s_bottom_layer, *s_big_pace_layer, *s_summary_layer, *s_left_accent_layer;
static TextLayer *s_date_layer, *s_time_layer, *s_hr_layer, *s_pace_layer, *s_dist_time_layer, *s_big_pace_text, *s_summary_text;

static char s_t_buf[8], s_d_buf[16], s_p_buf[32], s_dt_buf[64], s_big_p_buf[16], s_hr_val_buf[8], s_sum_buf[256], s_batt_buf[8];
static char s_detail_buf[1600], s_advice_cadence[300], s_advice_hr[300], s_advice_recovery[150];
static int s_hr = 0, s_batt = 100;
static bool s_big_pace_active = false, s_showing_post_run = false;
static HistoryItem s_selected_run;

#ifndef PBL_HEALTH
static int s_manual_steps = 0;
static int32_t s_avg_mag = 1000000; static int32_t s_prev_mag = 0; static uint64_t s_last_step_time = 0;
#endif

static void update_ui();
static void toggle_handler(ClickRecognizerRef rec, void *ctx);
static void up_handler(ClickRecognizerRef rec, void *ctx);
static void down_handler(ClickRecognizerRef rec, void *ctx);
static void battery_handler(BatteryChargeState state);
static void inbox_received_handler(DictionaryIterator *iter, void *context);
#ifdef PBL_HEALTH
static void health_handler(HealthEventType event, void *context);
#endif

static int get_health_steps_now() {
  #ifdef PBL_HEALTH
    return (int)health_service_sum_today(HealthMetricStepCount);
  #else
    return 0;
  #endif
}

static void detail_window_load(Window *window) {
  window_set_background_color(window, GColorWhite);
  Layer *root = window_get_root_layer(window); GRect bounds = layer_get_bounds(root);
  
  s_detail_scroll_layer = scroll_layer_create(bounds); 
  scroll_layer_set_click_config_onto_window(s_detail_scroll_layer, window);
  scroll_layer_set_callbacks(s_detail_scroll_layer, (ScrollLayerCallbacks){.click_config_provider=NULL});

  char t_str[32]; struct tm *tm = localtime(&s_selected_run.timestamp); strftime(t_str, sizeof(t_str), "%d.%m.%Y %H:%M", tm);
  char *u = (s_settings.unit_system == 0) ? "km" : "mi";
  float f = (s_settings.unit_system == 0) ? 1.0 : 0.621371;
  float dist_conv = (s_selected_run.dist / 1000.0) * f;
  int pace_s = 0; if (s_selected_run.dist > 0) pace_s = (int)(s_selected_run.dur * 1000.0 / (s_selected_run.dist * (s_settings.unit_system == 0 ? 1.0 : 1.60934)));
  int cadence = 0; if (s_selected_run.dur > 0) cadence = (s_selected_run.steps * 60) / s_selected_run.dur;
  int d_i = (int)dist_conv; int d_f = (int)((dist_conv - d_i) * 100);

  if (cadence < 135) snprintf(s_advice_cadence, sizeof(s_advice_cadence), "WALKING DETECTED (<135spm): Low impact. Ideal for active recovery.");
  else if (cadence < 155) snprintf(s_advice_cadence, sizeof(s_advice_cadence), "EFFICIENCY ALERT: Low run cadence. High impact risk. Try shorter steps.");
  else if (cadence < 170) snprintf(s_advice_cadence, sizeof(s_advice_cadence), "SOLID BASE: Recreational running rhythm. Good economy.");
  else if (cadence < 185) snprintf(s_advice_cadence, sizeof(s_advice_cadence), "OPTIMAL ECONOMY: Excellent frequency (170-185). Maximizes energy return.");
  else snprintf(s_advice_cadence, sizeof(s_advice_cadence), "ELITE TURNOVER: High frequency (>185). Watch for cardiac drift.");

  if (s_selected_run.hr > 0) {
    if (s_selected_run.hr < s_settings.zone_green) { snprintf(s_advice_hr, sizeof(s_advice_hr), "RECOVERY (Zone 1): Fat metabolism."); snprintf(s_advice_recovery, sizeof(s_advice_recovery), "Next: Anytime."); }
    else if (s_selected_run.hr < s_settings.zone_orange) { snprintf(s_advice_hr, sizeof(s_advice_hr), "AEROBIC (Zone 2-3): Builds base."); snprintf(s_advice_recovery, sizeof(s_advice_recovery), "Rest: 12-24h."); }
    else if (s_selected_run.hr < s_settings.zone_red) { snprintf(s_advice_hr, sizeof(s_advice_hr), "THRESHOLD (Zone 4): Hard effort."); snprintf(s_advice_recovery, sizeof(s_advice_recovery), "Rest: 24-48h."); }
    else { snprintf(s_advice_hr, sizeof(s_advice_hr), "VO2 PEAK (Zone 5): Max effort."); snprintf(s_advice_recovery, sizeof(s_advice_recovery), "Rest: >48h!"); }
  } else {

    if (cadence < 135) { snprintf(s_advice_hr, sizeof(s_advice_hr), "ESTIMATED ZONE 1 (Walking): Metabolic load likely very low."); snprintf(s_advice_recovery, sizeof(s_advice_recovery), "Next: Anytime."); }
    else if (cadence < 160) { snprintf(s_advice_hr, sizeof(s_advice_hr), "ESTIMATED ZONE 2 (Jogging): Based on cadence, likely aerobic."); snprintf(s_advice_recovery, sizeof(s_advice_recovery), "Rest: 12h."); }
    else if (cadence < 175) { snprintf(s_advice_hr, sizeof(s_advice_hr), "ESTIMATED ZONE 3 (Running): Moderate intensity estimated."); snprintf(s_advice_recovery, sizeof(s_advice_recovery), "Rest: 24h."); }
    else if (cadence < 190) { snprintf(s_advice_hr, sizeof(s_advice_hr), "ESTIMATED ZONE 4 (Tempo): High cadence suggests hard effort."); snprintf(s_advice_recovery, sizeof(s_advice_recovery), "Rest: 24-48h."); }
    else { snprintf(s_advice_hr, sizeof(s_advice_hr), "ESTIMATED ZONE 5 (Sprint): Elite turnover, maximal effort."); snprintf(s_advice_recovery, sizeof(s_advice_recovery), "Rest: >48h!"); }
  }

  snprintf(s_detail_buf, sizeof(s_detail_buf), 
    "ANALYSIS\n\n%s\n\nMETRICS\nTime: %02d:%02d:%02d\nDist: %d.%02d %s\nPace: %d:%02d /%s\nSteps: %d\n\nPHYSIOLOGY\nAvg HR: %d\nCadence: %d\n\nCOACH\n\nSTYLE:\n%s\n\nMETABOLISM:\n%s\n\nPLANNING:\n%s\n",
    t_str, s_selected_run.dur/3600, (s_selected_run.dur%3600)/60, s_selected_run.dur%60, d_i, d_f, u, pace_s/60, pace_s%60, u, s_selected_run.steps, s_selected_run.hr, cadence, s_advice_cadence, s_advice_hr, s_advice_recovery);

  s_detail_text_layer = text_layer_create(GRect(DETAIL_MARGIN_X, DETAIL_MARGIN_Y, bounds.size.w - (2*DETAIL_MARGIN_X), 2500));
  text_layer_set_text(s_detail_text_layer, s_detail_buf); 
  text_layer_set_font(s_detail_text_layer, fonts_get_system_font(FONT_DETAIL_BODY)); 
  #ifdef PBL_ROUND
    text_layer_set_text_alignment(s_detail_text_layer, GTextAlignmentCenter);
  #else
    text_layer_set_text_alignment(s_detail_text_layer, GTextAlignmentLeft);
  #endif
  text_layer_set_text_color(s_detail_text_layer, GColorBlack);
  text_layer_set_background_color(s_detail_text_layer, GColorClear);
  GSize max_size = text_layer_get_content_size(s_detail_text_layer); 
  text_layer_set_size(s_detail_text_layer, GSize(bounds.size.w - (2*DETAIL_MARGIN_X), max_size.h + 20));
  scroll_layer_set_content_size(s_detail_scroll_layer, GSize(bounds.size.w, max_size.h + 20 + DETAIL_MARGIN_Y));
  scroll_layer_add_child(s_detail_scroll_layer, text_layer_get_layer(s_detail_text_layer)); 
  layer_add_child(root, scroll_layer_get_layer(s_detail_scroll_layer));
}
static void detail_window_unload(Window *window) { text_layer_destroy(s_detail_text_layer); scroll_layer_destroy(s_detail_scroll_layer); }

static void bg_black_draw_proc(Layer *l, GContext *ctx) { graphics_context_set_fill_color(ctx, GColorBlack); graphics_fill_rect(ctx, layer_get_bounds(l), 0, GCornerNone); }
#if SHOW_LEFT_BAR
static void accent_bar_draw_proc(Layer *l, GContext *ctx) { GRect b = layer_get_bounds(l); graphics_context_set_stroke_color(ctx, COLOR_ACCENT); for(int y=0; y < b.size.h; y++) for(int x=0; x < b.size.w; x++) if((x + y + (int)layer_get_frame(l).origin.y) % 2 == 0) graphics_draw_pixel(ctx, GPoint(x, y)); }
#endif

static void status_update_proc(Layer *l, GContext *ctx) { 
  GRect b = layer_get_bounds(l); 
  #ifdef PBL_ROUND
    graphics_context_set_fill_color(ctx, GColorWhite); graphics_fill_rect(ctx, b, 0, GCornerNone); 
    graphics_context_set_fill_color(ctx, GColorLightGray); graphics_fill_radial(ctx, GRect(HR_CENTER.x-HR_RADIUS, HR_CENTER.y-HR_RADIUS, HR_RADIUS*2, HR_RADIUS*2), GOvalScaleModeFitCircle, 8, 0, TRIG_MAX_ANGLE); 
    
    #ifdef PBL_COLOR
      GColor hrc = (s_hr >= s_settings.zone_red) ? GColorRed : (s_hr >= s_settings.zone_orange) ? GColorOrange : COLOR_ACCENT;
      if (s_hr == 0 && s_state.active) hrc = GColorLightGray;
      if (s_hr == 0 && !s_state.active) hrc = GColorBlack; 
      graphics_context_set_fill_color(ctx, hrc);
    #else
      graphics_context_set_fill_color(ctx, GColorBlack);
    #endif

    int angle = (s_hr > 0) ? (s_hr*TRIG_MAX_ANGLE)/210 : TRIG_MAX_ANGLE;
    graphics_fill_radial(ctx, GRect(HR_CENTER.x-HR_RADIUS, HR_CENTER.y-HR_RADIUS, HR_RADIUS*2, HR_RADIUS*2), GOvalScaleModeFitCircle, 8, 0, angle); 
    
    if(s_state.active) { graphics_context_set_fill_color(ctx, COLOR_RECORDING); graphics_fill_circle(ctx, GPoint(SCREEN_W / 2, 12), 4); }
    snprintf(s_batt_buf, sizeof(s_batt_buf), "%d%%", s_batt); graphics_context_set_text_color(ctx, GColorBlack); graphics_draw_text(ctx, s_batt_buf, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(SCREEN_W/2 - 20, 16, 40, 20), 0, GTextAlignmentCenter, NULL);
  #else
    graphics_context_set_fill_color(ctx, GColorWhite); graphics_fill_rect(ctx, b, 0, GCornerNone); 
    graphics_context_set_stroke_color(ctx, GColorBlack); graphics_draw_line(ctx, GPoint(0, b.size.h - 1), GPoint(b.size.w, b.size.h - 1)); 
    if(s_state.active) { graphics_context_set_fill_color(ctx, COLOR_RECORDING); graphics_fill_circle(ctx, GPoint(SCREEN_W / 2, b.size.h / 2), 4); } 
    snprintf(s_batt_buf, sizeof(s_batt_buf), "%d%%", s_batt); graphics_context_set_text_color(ctx, GColorBlack); graphics_draw_text(ctx, s_batt_buf, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(SCREEN_W-45, (b.size.h/2)-10, 40, 20), 0, GTextAlignmentRight, NULL); 
  #endif
}

static void bottom_update_proc(Layer *l, GContext *ctx) { 
  #ifndef PBL_ROUND
    GRect b = layer_get_bounds(l); graphics_context_set_fill_color(ctx, GColorBlack); graphics_fill_rect(ctx, b, 0, GCornerNone); 
    graphics_context_set_stroke_color(ctx, COLOR_ACCENT); for(int y=0; y<b.size.h; y++) for(int x=b.size.w-SIDEBAR_W; x<b.size.w; x++) if((x+y)%2 == 0) graphics_draw_pixel(ctx, GPoint(x,y)); 
    
    graphics_context_set_fill_color(ctx, GColorDarkGray); 
    graphics_fill_radial(ctx, GRect(HR_CENTER.x-HR_RADIUS, HR_CENTER.y-HR_RADIUS, HR_RADIUS*2, HR_RADIUS*2), GOvalScaleModeFitCircle, 12, 0, TRIG_MAX_ANGLE); 
    
    int angle = (s_hr > 0) ? (s_hr*TRIG_MAX_ANGLE)/210 : 0;

    #ifdef PBL_COLOR
      GColor hrc = (s_hr >= s_settings.zone_red) ? GColorRed : (s_hr >= s_settings.zone_orange ? GColorOrange : COLOR_ACCENT);
      if (s_hr == 0) hrc = GColorDarkGray;
      graphics_context_set_fill_color(ctx, hrc);
    #else
      graphics_context_set_fill_color(ctx, GColorWhite);
    #endif
    
    graphics_fill_radial(ctx, GRect(HR_CENTER.x-HR_RADIUS, HR_CENTER.y-HR_RADIUS, HR_RADIUS*2, HR_RADIUS*2), GOvalScaleModeFitCircle, 12, 0, angle); 
  #endif
}

#ifndef PBL_HEALTH
static void accel_data_handler(AccelData *data, uint32_t num_samples) {
  if (!s_state.active) return;
  bool step_detected = false;
  for (uint32_t i = 0; i < num_samples; i++) {
    int32_t mag = (data[i].x * data[i].x) + (data[i].y * data[i].y) + (data[i].z * data[i].z);
    s_avg_mag = ((s_avg_mag * 15) + mag) >> 4;
    int32_t threshold = s_avg_mag + (s_avg_mag >> 4); 
    if (mag > threshold && s_prev_mag <= threshold) { if (data[i].timestamp - s_last_step_time > 250) { s_manual_steps++; s_last_step_time = data[i].timestamp; step_detected = true; } }
    s_prev_mag = mag;
  }
  if(step_detected) update_ui();
}
#endif

static void get_metrics(int *steps, int *dist, int *pace_s) {
  #ifdef PBL_HEALTH
    int current_total = get_health_steps_now();
    *steps = (current_total - s_state.initial_steps) + s_state.resumed_steps_offset;
    if (*steps < 0) *steps = 0;
  #else
    *steps = s_manual_steps + s_state.resumed_steps_offset;
  #endif
  *dist = (*steps * s_settings.step_len) / 100;
  int elapsed = s_state.active ? (int)(time(NULL) - s_state.start_time) : 0;
  if (*dist > 5 && elapsed > 2) { float f = (s_settings.unit_system == 0) ? 1.0 : 1.60934; *pace_s = (int)(elapsed * 1000.0 / (*dist * f)); } else { *pace_s = 0; }
}

static void update_ui() {
  int steps, dist, pace_s; get_metrics(&steps, &dist, &pace_s);
  char *u = (s_settings.unit_system == 0) ? "km" : "mi";
  float d_raw = (s_settings.unit_system == 0) ? dist/1000.0 : (dist/1609.34);
  int d_i = (int)d_raw, d_f = (int)((d_raw - d_i) * 100); if (d_f < 0) d_f *= -1;
  
  if (s_state.active) { 
    snprintf(s_dt_buf, sizeof(s_dt_buf), "%d.%02d%s\n%02d:%02d\n%d steps", d_i, d_f, u, (int)(time(NULL)-s_state.start_time)/60, (int)(time(NULL)-s_state.start_time)%60, steps); 
    snprintf(s_p_buf, sizeof(s_p_buf), "%d:%02d /%s", pace_s/60, pace_s%60, u); 
  } else { 
    #ifdef PBL_ROUND
      snprintf(s_p_buf, sizeof(s_p_buf), "READY");
      snprintf(s_dt_buf, sizeof(s_dt_buf), "PRESS SELECT\nTO START"); 
    #else
      snprintf(s_dt_buf, sizeof(s_dt_buf), "0.00%s\n00:00\n0 steps", u); 
      snprintf(s_p_buf, sizeof(s_p_buf), "READY"); 
    #endif
  }
  
  #ifdef PBL_ROUND
    text_layer_set_text_alignment(s_pace_layer, GTextAlignmentCenter);
  #endif
  text_layer_set_text(s_dist_time_layer, s_dt_buf); 
  text_layer_set_text(s_pace_layer, s_p_buf);
  if (s_big_pace_active) { if (pace_s > 0) snprintf(s_big_p_buf, sizeof(s_big_p_buf), "%d:%02d", pace_s/60, pace_s%60); else snprintf(s_big_p_buf, sizeof(s_big_p_buf), "-:--"); text_layer_set_text(s_big_pace_text, s_big_p_buf); }
}

static void toggle_handler(ClickRecognizerRef rec, void *ctx) {
  if (s_showing_post_run) { s_showing_post_run = false; layer_set_hidden(s_summary_layer, true); return; }
  
  if (s_state.active) {
    #ifndef PBL_HEALTH
      accel_data_service_unsubscribe();
    #endif
    app_worker_kill();
    persist_delete(KEY_SESSION_ACTIVE);

    int s, d, p; get_metrics(&s, &d, &p); int dur = (int)(time(NULL)-s_state.start_time); int avg_hr = (s_state.hr_count > 0) ? (s_state.hr_sum / s_state.hr_count) : s_hr;
    for (int i = MAX_HISTORY-1; i > 0; i--) { s_history.items[i] = s_history.items[i-1]; } 
    s_history.items[0] = (HistoryItem){.timestamp = time(NULL), .dist = d, .dur = dur, .hr = avg_hr, .steps = s}; 
    if (s_history.count < MAX_HISTORY) s_history.count++;
    persist_write_data(STORAGE_KEY_HISTORY, &s_history, sizeof(s_history));

    snprintf(s_sum_buf, sizeof(s_sum_buf), "RUN SAVED\n\nDist: %d.%02dkm\nTime: %02d:%02d\nAvg HR: %d\nSteps: %d\n\n[SELECT]", d/1000, (d%1000)/10, dur/60, dur%60, avg_hr, s);
    text_layer_set_text(s_summary_text, s_sum_buf); 
    s_state.active = false; s_showing_post_run = true; layer_set_hidden(s_summary_layer, false); vibes_double_pulse();
  } else {
    s_state.active = true; s_state.start_time = time(NULL); s_state.hr_sum = 0; s_state.hr_count = 0; s_state.resumed_steps_offset = 0;
    app_worker_launch(); 
    persist_write_bool(KEY_SESSION_ACTIVE, true);
    persist_write_int(KEY_SESSION_START, (int)s_state.start_time);
    #ifdef PBL_HEALTH
      s_state.initial_steps = get_health_steps_now();
      persist_write_int(KEY_SESSION_STEPS, s_state.initial_steps);
    #else
      s_manual_steps = 0; accel_data_service_subscribe(25, accel_data_handler); accel_service_set_sampling_rate(ACCEL_SAMPLING_25HZ);
    #endif
    vibes_short_pulse();
  }
  update_ui(); layer_mark_dirty(s_status_layer);
}

static void up_handler(ClickRecognizerRef rec, void *ctx) { if (s_state.active) { s_big_pace_active = !s_big_pace_active; layer_set_hidden(s_big_pace_layer, !s_big_pace_active); update_ui(); vibes_short_pulse(); } }
static uint16_t get_rows(MenuLayer *ml, uint16_t s, void *c) { return s_history.count > 0 ? s_history.count : 1; }
static void draw_row(GContext *ctx, const Layer *l, MenuIndex *idx, void *c) { if (s_history.count == 0) { menu_cell_basic_draw(ctx, l, "No Runs", "Start training!", NULL); return; } HistoryItem *it = &s_history.items[idx->row]; static char t_b[32], s_b[64]; struct tm *tm = localtime(&it->timestamp); strftime(t_b, sizeof(t_b), "%d.%m. %H:%M", tm); snprintf(s_b, sizeof(s_b), "%d.%02dk | %dm | %d stp", it->dist/1000, (it->dist%1000)/10, it->dur/60, it->steps); menu_cell_basic_draw(ctx, l, t_b, s_b, NULL); }
static void select_history_handler(MenuLayer *ml, MenuIndex *idx, void *ctx) { if (s_history.count == 0) return; s_selected_run = s_history.items[idx->row]; s_detail_window = window_create(); window_set_window_handlers(s_detail_window, (WindowHandlers){ .load = detail_window_load, .unload = detail_window_unload }); window_stack_push(s_detail_window, true); }
static void history_window_load(Window *w) { s_menu_layer = menu_layer_create(layer_get_bounds(window_get_root_layer(w))); menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){ .get_num_rows = get_rows, .draw_row = draw_row, .select_click = select_history_handler }); menu_layer_set_click_config_onto_window(s_menu_layer, w); layer_add_child(window_get_root_layer(w), menu_layer_get_layer(s_menu_layer)); }
static void down_handler(ClickRecognizerRef rec, void *ctx) { if (!s_state.active) { s_history_window = window_create(); window_set_window_handlers(s_history_window, (WindowHandlers){ .load = history_window_load, .unload = window_destroy }); window_stack_push(s_history_window, true); } }
static void battery_handler(BatteryChargeState state) { s_batt = state.charge_percent; layer_mark_dirty(s_status_layer); }
static void click_config_provider(void *context) { window_single_click_subscribe(BUTTON_ID_SELECT, toggle_handler); window_single_click_subscribe(BUTTON_ID_UP, up_handler); window_single_click_subscribe(BUTTON_ID_DOWN, down_handler); }
static void tick_handler(struct tm *t, TimeUnits u) { clock_copy_time_string(s_t_buf, sizeof(s_t_buf)); text_layer_set_text(s_time_layer, s_t_buf); strftime(s_d_buf, sizeof(s_d_buf), "%a %d.%m.", t); text_layer_set_text(s_date_layer, s_d_buf); update_ui(); }
static void inbox_received_handler(DictionaryIterator *i, void *c) { Tuple *t; if((t=dict_find(i, MESSAGE_KEY_CONFIG_UNIT_SYSTEM))) s_settings.unit_system = atoi(t->value->cstring); if((t=dict_find(i, MESSAGE_KEY_CONFIG_STEP_LENGTH))) s_settings.step_len = atoi(t->value->cstring); persist_write_data(STORAGE_KEY_CONFIG, &s_settings, sizeof(s_settings)); update_ui(); }
#ifdef PBL_HEALTH
static void health_handler(HealthEventType e, void *c) { if (e == HealthEventHeartRateUpdate) { HealthValue v = health_service_peek_current_value(HealthMetricHeartRateRawBPM); if(v > 0) { s_hr = (int)v; if (s_state.active) { s_state.hr_sum += s_hr; s_state.hr_count++; } snprintf(s_hr_val_buf, sizeof(s_hr_val_buf), "%d", s_hr); text_layer_set_text(s_hr_layer, s_hr_val_buf); layer_mark_dirty(s_status_layer); } } }
#endif

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window); GFont main_f = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
  #ifdef PBL_PLATFORM_EMERY
    main_f = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOM_60));
  #endif
  
  s_status_layer = layer_create(GRect(0,0,SCREEN_W,H_STATUS)); 
  layer_set_update_proc(s_status_layer, status_update_proc); 
  layer_add_child(root, s_status_layer);
  
  #ifdef PBL_ROUND
  s_date_layer = text_layer_create(GRect(0,0,0,0)); 
  #else
  s_date_layer = text_layer_create(GRect(6, 2, 110, 20)); 
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14)); 
  text_layer_set_background_color(s_date_layer, GColorClear); 
  text_layer_set_text_color(s_date_layer, GColorBlack); 
  layer_add_child(s_status_layer, text_layer_get_layer(s_date_layer));
  #endif
  
  s_time_layer = text_layer_create(GRect(0, TIME_Y, SCREEN_W, 70)); 
  text_layer_set_font(s_time_layer, main_f); 
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter); 
  text_layer_set_background_color(s_time_layer, GColorClear); 
  layer_add_child(root, text_layer_get_layer(s_time_layer));
  
  #if SHOW_LEFT_BAR
    s_left_accent_layer = layer_create(GRect(0, H_STATUS + 1, SIDEBAR_W, SCREEN_H - H_STATUS - H_BOTTOM - 1)); 
    layer_set_update_proc(s_left_accent_layer, accent_bar_draw_proc); 
    layer_add_child(root, s_left_accent_layer);
  #endif
  
  #ifndef PBL_ROUND
  s_bottom_layer = layer_create(GRect(0, SCREEN_H-H_BOTTOM, SCREEN_W, H_BOTTOM)); 
  layer_set_update_proc(s_bottom_layer, bottom_update_proc); 
  layer_add_child(root, s_bottom_layer);
  #endif
  
  s_hr_layer = text_layer_create(RECT_HR); 
  text_layer_set_font(s_hr_layer, fonts_get_system_font(FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM)); 
  text_layer_set_text_alignment(s_hr_layer, GTextAlignmentCenter); 
  #ifdef PBL_ROUND
    text_layer_set_text_color(s_hr_layer, GColorBlack); 
    layer_add_child(root, text_layer_get_layer(s_hr_layer));
  #else
    text_layer_set_text_color(s_hr_layer, GColorWhite); 
    layer_add_child(s_bottom_layer, text_layer_get_layer(s_hr_layer));
  #endif
  text_layer_set_background_color(s_hr_layer, GColorClear); 

  s_pace_layer = text_layer_create(RECT_DATA_PACE); 
  text_layer_set_font(s_pace_layer, fonts_get_system_font(FONT_DATA_BIG_KEY)); 
  text_layer_set_background_color(s_pace_layer, GColorClear); 
  
  s_dist_time_layer = text_layer_create(RECT_DATA_DIST); 
  text_layer_set_font(s_dist_time_layer, fonts_get_system_font(FONT_DATA_SMALL_KEY)); 
  text_layer_set_background_color(s_dist_time_layer, GColorClear); 
  
  #ifdef PBL_ROUND
    text_layer_set_text_alignment(s_pace_layer, GTextAlignmentCenter);
    text_layer_set_text_color(s_pace_layer, GColorBlack);
    layer_add_child(root, text_layer_get_layer(s_pace_layer));
    
    text_layer_set_text_alignment(s_dist_time_layer, GTextAlignmentCenter);
    text_layer_set_text_color(s_dist_time_layer, GColorBlack);
    layer_add_child(root, text_layer_get_layer(s_dist_time_layer));
  #else
    text_layer_set_text_alignment(s_pace_layer, GTextAlignmentLeft);
    text_layer_set_text_color(s_pace_layer, GColorWhite);
    layer_add_child(s_bottom_layer, text_layer_get_layer(s_pace_layer));
    
    text_layer_set_text_alignment(s_dist_time_layer, GTextAlignmentLeft);
    text_layer_set_text_color(s_dist_time_layer, GColorLightGray);
    layer_add_child(s_bottom_layer, text_layer_get_layer(s_dist_time_layer));
  #endif
  
  s_big_pace_layer = layer_create(layer_get_bounds(root)); layer_set_update_proc(s_big_pace_layer, bg_black_draw_proc); layer_set_hidden(s_big_pace_layer, true); layer_add_child(root, s_big_pace_layer);
  s_big_pace_text = text_layer_create(GRect(0, (SCREEN_H/2)-40, SCREEN_W, 80)); text_layer_set_font(s_big_pace_text, main_f); text_layer_set_text_alignment(s_big_pace_text, GTextAlignmentCenter); text_layer_set_text_color(s_big_pace_text, GColorWhite); text_layer_set_background_color(s_big_pace_text, GColorClear); layer_add_child(s_big_pace_layer, text_layer_get_layer(s_big_pace_text));
  s_summary_layer = layer_create(layer_get_bounds(root)); layer_set_update_proc(s_summary_layer, bg_black_draw_proc); layer_set_hidden(s_summary_layer, true); layer_add_child(root, s_summary_layer);
  s_summary_text = text_layer_create(GRect(10, 20, SCREEN_W-20, SCREEN_H-40)); text_layer_set_font(s_summary_text, fonts_get_system_font(FONT_SUMMARY)); text_layer_set_text_alignment(s_summary_text, GTextAlignmentCenter); text_layer_set_text_color(s_summary_text, GColorWhite); text_layer_set_background_color(s_summary_text, GColorClear); layer_add_child(s_summary_layer, text_layer_get_layer(s_summary_text));
  
  time_t now = time(NULL); tick_handler(localtime(&now), MINUTE_UNIT);
}

static void init() {
  if (persist_exists(STORAGE_KEY_CONFIG)) persist_read_data(STORAGE_KEY_CONFIG, &s_settings, sizeof(s_settings));
  else { s_settings = (AppSettings){ .zone_green = 110, .zone_orange = 145, .zone_red = 165, .step_len = 75, .unit_system = 0 }; }
  if (persist_exists(STORAGE_KEY_HISTORY)) persist_read_data(STORAGE_KEY_HISTORY, &s_history, sizeof(s_history));
  s_window = window_create(); window_set_click_config_provider(s_window, click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers){.load=window_load, .unload=window_destroy}); 
  window_stack_push(s_window, true);
  
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler); battery_handler(battery_state_service_peek());
  app_message_register_inbox_received(inbox_received_handler); app_message_open(512, 512);
  #ifdef PBL_HEALTH
    health_service_events_subscribe(health_handler, NULL);
  #endif

  if (app_worker_is_running()) {
    if (persist_exists(KEY_SESSION_ACTIVE) && persist_read_bool(KEY_SESSION_ACTIVE)) {
      s_state.active = true;
      if (persist_exists(KEY_SESSION_START)) s_state.start_time = (time_t)persist_read_int(KEY_SESSION_START);
      #ifdef PBL_HEALTH
        if (persist_exists(KEY_SESSION_STEPS)) s_state.initial_steps = persist_read_int(KEY_SESSION_STEPS);
      #else
        accel_data_service_subscribe(25, accel_data_handler); accel_service_set_sampling_rate(ACCEL_SAMPLING_25HZ);
      #endif
    }
  }
}

int main() { init(); app_event_loop(); return 0; }