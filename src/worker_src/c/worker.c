#include <pebble_worker.h>

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Worker alive: %02d:%02d", tick_time->tm_hour, tick_time->tm_min);
}

static void worker_init() {
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  APP_LOG(APP_LOG_LEVEL_INFO, "Worker: STARTED");
}

static void worker_deinit() {
  tick_timer_service_unsubscribe();
  APP_LOG(APP_LOG_LEVEL_INFO, "Worker: STOPPED");
}

int main(void) {
  worker_init();
  worker_event_loop();
  worker_deinit();
  return 0;
}