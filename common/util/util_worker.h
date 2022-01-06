#ifndef SMC_WORKER_H
#define SMC_WORKER_H

typedef struct {
  void (*fn)(void*, uint32_t);
  void* ptr_arg;
  uint32_t ui32_arg;
  uint32_t delay_ms;
  char* name;
} worker_job;

uint8_t get_work_count();
int add_work(worker_job);
void init_worker();

#endif
