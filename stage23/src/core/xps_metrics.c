#include "xps_metrics.h"
#include "xps_timer.h"
#include <sys/resource.h>

xps_buffer_t *metrics_to_json(xps_metrics_t *metrics, float workers_cpu_percent[]);
float get_sys_cpu_percent();
void get_sys_mem_usage(u_long *total_mem_bytes, u_long *used_mem_bytes);

xps_metrics_t *xps_metrics_create(xps_core_t *core, xps_config_t *config) {
  assert(core != NULL);
  assert(config != NULL);

  xps_metrics_t *metrics = malloc(sizeof(xps_metrics_t));
  if (metrics == NULL) {
    logger(LOG_ERROR, "xps_metrics_create()", "malloc() faield for metrics");
    return NULL;
  }

  metrics->core = core;

  metrics->_last_worker_cpu_time_msec = 0;
  metrics->_last_worker_cpu_update_uptime_msec = 0;
  metrics->_res_time_sum = 0;
  metrics->_res_n = 0;

  metrics->server_name = config->server_name;
  metrics->pid = getpid();
  metrics->workers = config->workers;
  metrics->uptime_msec = 0;

  metrics->sys_cpu_usage_percent = 0;
  metrics->sys_ram_usage_bytes = 0;
  metrics->sys_ram_total_bytes = 0;

  metrics->worker_ram_usage_bytes = 0;
  metrics->worker_cpu_usage_percent = 0;

  metrics->conn_accept_error = 0;
  metrics->conn_current = 0;
  metrics->conn_accepted = 0;
  metrics->conn_error = 0;
  metrics->conn_timeout = 0;

  metrics->req_current = 0;
  metrics->req_file_serve = 0;
  metrics->req_redirect = 0;
  metrics->req_reverse_proxy = 0;
  metrics->req_total = 0;

  metrics->res_avg_res_time_msec = 0;
  metrics->res_peak_res_time_msec = 0;
  metrics->res_code_2xx = 0;
  metrics->res_code_3xx = 0;
  metrics->res_code_4xx = 0;
  metrics->res_code_5xx = 0;

  metrics->traffic_total_send_bytes = 0;
  metrics->traffic_total_recv_bytes = 0;

  logger(LOG_DEBUG, "xps_metrics_create()", "created metrics");

  return metrics;
}

void xps_metrics_destroy(xps_metrics_t *metrics) {
  assert(metrics != NULL);

  free(metrics);
  logger(LOG_DEBUG, "xps_metrics_destory()", "destroyed metrics");
}

xps_buffer_t *xps_metrics_get_json(xps_metrics_t *metrics) {
  assert(metrics != NULL);

  metrics->uptime_msec = metrics->core->curr_time_msec - metrics->core->init_time_msec;

  // System resource usage
  get_sys_mem_usage(&metrics->sys_ram_total_bytes, &metrics->sys_ram_usage_bytes);
  metrics->sys_cpu_usage_percent = get_sys_cpu_percent();

  // calculate cumulative metrics
  xps_metrics_t cumulative;
  memset(&cumulative, 0, sizeof(cumulative));

  cumulative.server_name = metrics->server_name;
  cumulative.pid = metrics->pid;
  cumulative.workers = metrics->workers;
  cumulative.sys_ram_total_bytes = metrics->sys_ram_total_bytes;
  cumulative.sys_ram_usage_bytes = metrics->sys_ram_usage_bytes;
  cumulative.sys_cpu_usage_percent = metrics->sys_cpu_usage_percent;

  float workers_cpu_percent[n_cores];

  // will be needing to remove this loop and keep it for the next stage
  for (int i = 0; i < n_cores; i++) {
    xps_metrics_t *curr = cores[i]->metrics;

    workers_cpu_percent[i] =
      curr->worker_cpu_usage_percent; // for stage22 manage this appropriately...(make it for a
                                      // single core)
    cumulative.worker_ram_usage_bytes += curr->worker_ram_usage_bytes;

    cumulative.conn_current += curr->conn_current;
    cumulative.conn_accepted += curr->conn_accepted;
    cumulative.conn_error += curr->conn_error;
    cumulative.conn_timeout += curr->conn_timeout;
    cumulative.conn_accept_error += curr->conn_accept_error;

    cumulative.req_current += curr->req_current;
    cumulative.req_total += curr->req_total;
    cumulative.req_file_serve += curr->req_file_serve;
    cumulative.req_reverse_proxy += curr->req_reverse_proxy;
    cumulative.req_redirect += curr->req_redirect;

    cumulative.res_avg_res_time_msec += curr->res_avg_res_time_msec;
    cumulative.res_peak_res_time_msec += curr->res_peak_res_time_msec;
    cumulative.res_code_2xx += curr->res_code_2xx;
    cumulative.res_code_3xx += curr->res_code_3xx;
    cumulative.res_code_4xx += curr->res_code_4xx;
    cumulative.res_code_5xx += curr->res_code_5xx;

    cumulative.traffic_total_send_bytes += curr->traffic_total_send_bytes;
    cumulative.traffic_total_recv_bytes += curr->traffic_total_recv_bytes;
  }

  return metrics_to_json(&cumulative, workers_cpu_percent);
}

void xps_metrics_set(xps_core_t *core, xps_metric_type_t type, long val) {
  assert(core != NULL);
  assert(val > 0);

  switch (type) {
    case M_CONN_ACCEPT:
      core->metrics->conn_current += val;
      core->metrics->conn_accepted += val;
      break;
    case M_CONN_CLOSE:
      core->metrics->conn_current -= val;
      assert(core->metrics->conn_current >= 0);
      break;
    case M_CONN_ACCEPT_ERROR:
      core->metrics->conn_accept_error += val;
      break;
    case M_CONN_ERROR:
      core->metrics->conn_error += val;
      break;
    case M_CONN_TIMEOUT:
      core->metrics->conn_timeout += val;
      break;
    case M_REQ_CREATE:
      core->metrics->req_total += val;
      core->metrics->req_current += val;
      break;
    case M_REQ_DESTROY:
      core->metrics->req_current -= val;
      assert(core->metrics->req_current >= 0);
      break;
    case M_REQ_FILE_SERVE:
      core->metrics->req_file_serve += val;
    case M_REQ_REVERSE_PROXY:
      core->metrics->req_reverse_proxy += val;
      break;
    case M_REQ_REDIRECT:
      core->metrics->req_redirect += val;
      break;
    case M_RES_TIME:
      core->metrics->_res_time_sum += val;
      core->metrics->_res_n += 1;
      core->metrics->res_avg_res_time_msec = core->metrics->_res_time_sum / core->metrics->_res_n;
      if (core->metrics->res_peak_res_time_msec < val)
        core->metrics->res_peak_res_time_msec = val;
      break;
    case M_RES_2XX:
      core->metrics->res_code_2xx += val;
      break;
    case M_RES_3XX:
      core->metrics->res_code_3xx += val;
      break;
    case M_RES_4XX:
      core->metrics->res_code_4xx += val;
      break;
    case M_RES_5XX:
      core->metrics->res_code_5xx += val;
      break;
    case M_TRAFFIC_SEND_BYTES:
      core->metrics->traffic_total_send_bytes += val;
      break;
    case M_TRAFFIC_RECV_BYTES:
      core->metrics->traffic_total_recv_bytes += val;
      break;
    default:
      logger(LOG_ERROR, "xps_set_metric()", "invalid metric type");
  }
}

xps_buffer_t *metrics_to_json(xps_metrics_t *metrics, float workers_cpu_percent[]) {

  assert(metrics != NULL);

  xps_buffer_t *buff = xps_buffer_create(2000, 0, NULL);
  if (buff == NULL) {
    logger(LOG_ERROR, "metrics_to_json()", "xps_buffer_create() failed");
    return NULL;
  }

  // setup array of workers cpu percent values
  char workers_cpu_percent_str[n_cores * 20];
  memset(workers_cpu_percent_str, 0, sizeof(workers_cpu_percent_str));
  strncat(workers_cpu_percent_str, "[", sizeof(workers_cpu_percent_str));

  for (int i = 0; i < n_cores; i++) {
    char temp[20];
    snprintf(temp, strlen(temp), "%f%s", workers_cpu_percent[i], i == n_cores - 1 ? "" : ",");
    strncat(workers_cpu_percent_str, temp, sizeof(workers_cpu_percent_str));
  }
  strncat(workers_cpu_percent_str, "]", sizeof(workers_cpu_percent_str));

  snprintf(
    buff->data, buff->size,
    "{"
    "\"server_name\": \"%s\","
    "\"pid\": %u,"
    "\"workers\": %u,"
    "\"uptime_msec\": %lu,"

    "\"sys_cpu_usage_percent\": \"%f\","
    "\"sys_ram_usage_bytes\": %lu,"
    "\"sys_ram_total_bytes\": %lu,"

    "\"workers_cpu_usage_percent\": %s,"
    "\"workers_ram_usage_bytes\": %lu,"

    "\"conn_current\": %lu,"
    "\"conn_accepted\": %lu,"
    "\"conn_error\": %lu,"
    "\"conn_timeout\": %lu,"
    "\"conn_accepted_error\": %lu,"

    "\"req_current\": %lu,"
    "\"req_total\": %lu,"
    "\"req_file_serve\": %lu,"
    "\"req_reverse_proxy\": %lu,"
    "\"req_redirect\": %lu,"

    "\"res_avg_res_time_msec\": %lu,"
    "\"res_peak_res_time_msec\": %lu,"
    "\"res_code_2xx\": %lu,"
    "\"res_code_3xx\": %lu,"
    "\"res_code_4xx\": %lu,"
    "\"res_code_5xx\": %lu,"

    "\"traffic_total_send_bytes\": %lu,"
    "\"traffic_total_recv_bytes\": %lu"
    "}",
    metrics->server_name, metrics->pid, metrics->workers, metrics->uptime_msec,
    metrics->sys_cpu_usage_percent, metrics->sys_ram_usage_bytes, metrics->sys_ram_total_bytes,
    workers_cpu_percent_str, metrics->worker_ram_usage_bytes, metrics->conn_current,
    metrics->conn_accepted, metrics->conn_error, metrics->conn_timeout, metrics->conn_accept_error,
    metrics->req_current, metrics->req_total, metrics->req_file_serve, metrics->req_reverse_proxy,
    metrics->req_redirect, metrics->res_avg_res_time_msec, metrics->res_peak_res_time_msec,
    metrics->res_code_2xx, metrics->res_code_3xx, metrics->res_code_4xx, metrics->res_code_5xx,
    metrics->traffic_total_send_bytes, metrics->traffic_total_recv_bytes);

  buff->len = strlen(buff->data);

  return buff;
}

float get_sys_cpu_percent() {
  static unsigned long long idle_prev = 0;
  static unsigned long long total_prev = 0;

  FILE *fp;
  char buf[1024], cpu_label[5];
  unsigned long long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
  unsigned long long idle_diff, total_diff;
  float usage;

  // Open the /proc/stat file
  fp = fopen("/proc/stat", "r");
  if (fp == NULL) {
    logger(LOG_ERROR, "get_sys_cpu_percent()", "failed to open /proc/stat");
    perror("Error message");
    return 0.0;
  }

  // Read the first line from /proc/stat
  fgets(buf, sizeof(buf), fp);

  // Parse CPU usage data
  sscanf(buf, "%s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu", cpu_label, &user, &nice,
         &system, &idle, &iowait, &irq, &softirq, &steal, &guest, &guest_nice);

  // Calculate total CPU time
  unsigned long long total = user + nice + system + idle + iowait + irq + softirq + steal;

  // Calculate CPU idle time and total time
  idle_diff = idle - idle_prev;
  total_diff = total - total_prev;
  
  //calculate CPU usage percentage
  usage = 100.0 * (total_diff - idle_diff) / total_diff;

  //update previous values for next iteration
  idle_prev = idle;
  total_prev = total;

  //close the file
  fclose(fp);

  return usage;
}

void get_sys_mem_usage(u_long *total_mem_bytes, u_long *used_mem_bytes){
  *total_mem_bytes = 0;
  *used_mem_bytes = 0;

  FILE *file = fopen("/proc/meminfo", "r");
  if (!file) {
    logger(LOG_ERROR, "get_sys_mem_usage()", "failed to open /proc/meminfo");
    perror("Error message");
    return;
  }

  char line[200];
  u_long mem_total = 0, mem_free = 0, buffers = 0, cached = 0;
  while(fgets(line, sizeof(line), file)){
    if(strncmp(line, "MemTotal:", 9) == 0){
      const char *ptr = line + 9;
      // Skip non-digit characters
      while (*ptr < '0' || *ptr > '9')
        ptr++;
      // Convert digits to unsigned long
      mem_total = strtoul(ptr, NULL, 10);
    }else if(strncmp(line, "MemFree:", 8) == 0){
      const char *ptr = line + 7;
      // Skip non-digit characters
      while (*ptr < '0' || *ptr > '9')
        ptr++;
      // Convert digits to unsigned long
      mem_free = strtoul(ptr, NULL, 10);
    }else if(strncmp(line, "Buffers:", 8) == 0){
      const char *ptr = line + 8;
      // Skip non-digit characters
      while (*ptr < '0' || *ptr > '9')
        ptr++;
      // Convert digits to unsigned long
      buffers = strtoul(ptr, NULL, 10);
    }else if(strncmp(line, "Cached:", 7)){
      const char *ptr = line + 7;
      // Skip non-digit characters
      while (*ptr < '0' || *ptr > '9')
        ptr++;
      // Convert digits to unsigned long
      cached = strtoul(ptr, NULL, 10);
    }
  }

  fclose(file);
  *total_mem_bytes = mem_total * 1024;
  *used_mem_bytes = (mem_total - mem_free - buffers - cached) * 1024;

}

void xps_metrics_update_handler(void *ptr){
  assert(ptr != NULL);
  xps_core_t *core = ptr;
  xps_metrics_t *metrics = core->metrics;

  //Server resource usage
  struct rusage usage;
  if(getrusage(RUSAGE_THREAD, &usage) != 0){
    logger(LOG_ERROR, "xps_metrics_get_json()", "getrusage() failed");
    perror("Error message");
    metrics->worker_cpu_usage_percent = 0;
    metrics->worker_ram_usage_bytes = 0;
  }else{
    //Calculate worker CPU Usage percent
    u_long curr_worker_cpu_time_msec = timeval_to_msec(usage.ru_utime) + timeval_to_msec(usage.ru_stime);

    u_long diff_cpu_time = curr_worker_cpu_time_msec - metrics->_last_worker_cpu_time_msec;
    u_long uptime_msec = core->curr_time_msec - core->init_time_msec;
    u_long diff_uptime_msec = uptime_msec - metrics->_last_worker_cpu_update_uptime_msec;

    metrics->worker_cpu_usage_percent = ((float)diff_cpu_time * 100) / ((float)diff_uptime_msec);

    metrics->_last_worker_cpu_time_msec = curr_worker_cpu_time_msec;
    metrics->_last_worker_cpu_update_uptime_msec = uptime_msec;

    metrics->worker_ram_usage_bytes = usage.ru_maxrss * 1024;
  }

  xps_timer_update(core->metrics_update_timer, DEFAULT_METRICS_UPDATE_MSEC);

}
