/* D3D12 command list call tracer for debugging Dozen on the Xbox (no D3D12 debug layer there).
 * dzn_trace_macros.h routes every ID3D12GraphicsCommandList*_X() macro call through dzn_trace_note(). */
#include <stdint.h>
#include "util/log.h"

void dzn_trace_note(const void *list, const char *method);
void dzn_trace_dump(const void *list, unsigned last);

#define DZN_TRACE_RING 4096

struct dzn_trace_entry { const void *list; const char *method; };
static struct dzn_trace_entry dzn_trace_ring[DZN_TRACE_RING];
static uint32_t dzn_trace_pos;

void
dzn_trace_note(const void *list, const char *method)
{
   uint32_t i = __atomic_fetch_add(&dzn_trace_pos, 1, __ATOMIC_RELAXED) % DZN_TRACE_RING;
   dzn_trace_ring[i].list = list;
   dzn_trace_ring[i].method = method;
}

/* Print the last calls made on this command list (since its last Reset), oldest first. */
void
dzn_trace_dump(const void *list, unsigned last)
{
   uint32_t end = __atomic_load_n(&dzn_trace_pos, __ATOMIC_RELAXED);
   uint32_t begin = end > DZN_TRACE_RING ? end - DZN_TRACE_RING : 0;
   uint32_t start = begin;
   for (uint32_t i = end; i-- > begin;) {
      const struct dzn_trace_entry *e = &dzn_trace_ring[i % DZN_TRACE_RING];
      if (e->list == list && e->method && !__builtin_strcmp(e->method, "Reset")) {
         start = i;
         break;
      }
   }
   unsigned count = 0;
   for (uint32_t i = start; i != end; i++)
      count += dzn_trace_ring[i % DZN_TRACE_RING].list == list;
   mesa_loge("dzn: %u D3D12 calls on this command list since its Reset (showing last %u):", count, last);
   unsigned skip = count > last ? count - last : 0, n = 0;
   for (uint32_t i = start; i != end; i++) {
      const struct dzn_trace_entry *e = &dzn_trace_ring[i % DZN_TRACE_RING];
      if (e->list != list || n++ < skip)
         continue;
      mesa_loge("dzn:   #%u %s", n - 1, e->method);
   }
}
