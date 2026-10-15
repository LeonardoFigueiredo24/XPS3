/* Generated: D3D12 command list call tracer for debugging Dozen on the Xbox (no debug layer there). */
#include <stdint.h>
#include <string.h>
#include "util/log.h"

void dzn_trace_wrap(void *obj, unsigned slots);
void dzn_trace_dump(unsigned last);

#define DZN_TRACE_SLOTS 86
#define DZN_TRACE_RING 1024

struct dzn_trace_entry { uint64_t method, a1, a2; };
struct dzn_trace_entry dzn_trace_ring[DZN_TRACE_RING];
volatile uint32_t dzn_trace_pos;
void *dzn_trace_orig[DZN_TRACE_SLOTS];
static void *dzn_trace_proxy[DZN_TRACE_SLOTS];
static const void *dzn_trace_orig_vtbl;

static const char *const dzn_trace_names[DZN_TRACE_SLOTS] = {
   "QueryInterface",
   "AddRef",
   "Release",
   "GetPrivateData",
   "SetPrivateData",
   "SetPrivateDataInterface",
   "SetName",
   "GetDevice",
   "GetType",
   "Close",
   "Reset",
   "ClearState",
   "DrawInstanced",
   "DrawIndexedInstanced",
   "Dispatch",
   "CopyBufferRegion",
   "CopyTextureRegion",
   "CopyResource",
   "CopyTiles",
   "ResolveSubresource",
   "IASetPrimitiveTopology",
   "RSSetViewports",
   "RSSetScissorRects",
   "OMSetBlendFactor",
   "OMSetStencilRef",
   "SetPipelineState",
   "ResourceBarrier",
   "ExecuteBundle",
   "SetDescriptorHeaps",
   "SetComputeRootSignature",
   "SetGraphicsRootSignature",
   "SetComputeRootDescriptorTable",
   "SetGraphicsRootDescriptorTable",
   "SetComputeRoot32BitConstant",
   "SetGraphicsRoot32BitConstant",
   "SetComputeRoot32BitConstants",
   "SetGraphicsRoot32BitConstants",
   "SetComputeRootConstantBufferView",
   "SetGraphicsRootConstantBufferView",
   "SetComputeRootShaderResourceView",
   "SetGraphicsRootShaderResourceView",
   "SetComputeRootUnorderedAccessView",
   "SetGraphicsRootUnorderedAccessView",
   "IASetIndexBuffer",
   "IASetVertexBuffers",
   "SOSetTargets",
   "OMSetRenderTargets",
   "ClearDepthStencilView",
   "ClearRenderTargetView",
   "ClearUnorderedAccessViewUint",
   "ClearUnorderedAccessViewFloat",
   "DiscardResource",
   "BeginQuery",
   "EndQuery",
   "ResolveQueryData",
   "SetPredication",
   "SetMarker",
   "BeginEvent",
   "EndEvent",
   "ExecuteIndirect",
   "AtomicCopyBufferUINT",
   "AtomicCopyBufferUINT64",
   "OMSetDepthBounds",
   "SetSamplePositions",
   "ResolveSubresourceRegion",
   "SetViewInstanceMask",
   "WriteBufferImmediate",
   "SetProtectedResourceSession",
   "BeginRenderPass",
   "EndRenderPass",
   "InitializeMetaCommand",
   "ExecuteMetaCommand",
   "BuildRaytracingAccelerationStructure",
   "EmitRaytracingAccelerationStructurePostbuildInfo",
   "CopyRaytracingAccelerationStructure",
   "SetPipelineState1",
   "DispatchRays",
   "RSSetShadingRate",
   "RSSetShadingRateImage",
   "DispatchMesh",
   "Barrier",
   "OMSetFrontAndBackStencilRef",
   "RSSetDepthBias",
   "IASetIndexBufferStripCutValue",
   "SetProgram",
   "DispatchGraph"
};

__asm__(".text\n"
"dzn_trace_common:\n"
"  movl $1, %r11d\n"
"  lock xaddl %r11d, dzn_trace_pos(%rip)\n"
"  andl $1023, %r11d\n"
"  imulq $24, %r11, %r11\n"
"  leaq dzn_trace_ring(%rip), %r10\n"
"  addq %r11, %r10\n"
"  movq %rax, (%r10)\n"
"  movq %rdx, 8(%r10)\n"
"  movq %r8, 16(%r10)\n"
"  leaq dzn_trace_orig(%rip), %r10\n"
"  jmp *(%r10,%rax,8)\n"
"dzn_trace_thunk_0:\n  movl $0, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_1:\n  movl $1, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_2:\n  movl $2, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_3:\n  movl $3, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_4:\n  movl $4, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_5:\n  movl $5, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_6:\n  movl $6, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_7:\n  movl $7, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_8:\n  movl $8, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_9:\n  movl $9, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_10:\n  movl $10, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_11:\n  movl $11, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_12:\n  movl $12, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_13:\n  movl $13, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_14:\n  movl $14, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_15:\n  movl $15, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_16:\n  movl $16, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_17:\n  movl $17, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_18:\n  movl $18, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_19:\n  movl $19, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_20:\n  movl $20, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_21:\n  movl $21, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_22:\n  movl $22, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_23:\n  movl $23, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_24:\n  movl $24, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_25:\n  movl $25, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_26:\n  movl $26, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_27:\n  movl $27, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_28:\n  movl $28, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_29:\n  movl $29, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_30:\n  movl $30, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_31:\n  movl $31, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_32:\n  movl $32, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_33:\n  movl $33, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_34:\n  movl $34, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_35:\n  movl $35, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_36:\n  movl $36, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_37:\n  movl $37, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_38:\n  movl $38, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_39:\n  movl $39, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_40:\n  movl $40, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_41:\n  movl $41, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_42:\n  movl $42, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_43:\n  movl $43, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_44:\n  movl $44, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_45:\n  movl $45, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_46:\n  movl $46, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_47:\n  movl $47, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_48:\n  movl $48, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_49:\n  movl $49, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_50:\n  movl $50, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_51:\n  movl $51, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_52:\n  movl $52, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_53:\n  movl $53, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_54:\n  movl $54, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_55:\n  movl $55, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_56:\n  movl $56, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_57:\n  movl $57, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_58:\n  movl $58, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_59:\n  movl $59, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_60:\n  movl $60, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_61:\n  movl $61, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_62:\n  movl $62, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_63:\n  movl $63, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_64:\n  movl $64, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_65:\n  movl $65, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_66:\n  movl $66, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_67:\n  movl $67, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_68:\n  movl $68, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_69:\n  movl $69, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_70:\n  movl $70, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_71:\n  movl $71, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_72:\n  movl $72, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_73:\n  movl $73, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_74:\n  movl $74, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_75:\n  movl $75, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_76:\n  movl $76, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_77:\n  movl $77, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_78:\n  movl $78, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_79:\n  movl $79, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_80:\n  movl $80, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_81:\n  movl $81, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_82:\n  movl $82, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_83:\n  movl $83, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_84:\n  movl $84, %eax\n  jmp dzn_trace_common\n"
"dzn_trace_thunk_85:\n  movl $85, %eax\n  jmp dzn_trace_common\n"
);

extern char dzn_trace_thunk_0[];
extern char dzn_trace_thunk_1[];
extern char dzn_trace_thunk_2[];
extern char dzn_trace_thunk_3[];
extern char dzn_trace_thunk_4[];
extern char dzn_trace_thunk_5[];
extern char dzn_trace_thunk_6[];
extern char dzn_trace_thunk_7[];
extern char dzn_trace_thunk_8[];
extern char dzn_trace_thunk_9[];
extern char dzn_trace_thunk_10[];
extern char dzn_trace_thunk_11[];
extern char dzn_trace_thunk_12[];
extern char dzn_trace_thunk_13[];
extern char dzn_trace_thunk_14[];
extern char dzn_trace_thunk_15[];
extern char dzn_trace_thunk_16[];
extern char dzn_trace_thunk_17[];
extern char dzn_trace_thunk_18[];
extern char dzn_trace_thunk_19[];
extern char dzn_trace_thunk_20[];
extern char dzn_trace_thunk_21[];
extern char dzn_trace_thunk_22[];
extern char dzn_trace_thunk_23[];
extern char dzn_trace_thunk_24[];
extern char dzn_trace_thunk_25[];
extern char dzn_trace_thunk_26[];
extern char dzn_trace_thunk_27[];
extern char dzn_trace_thunk_28[];
extern char dzn_trace_thunk_29[];
extern char dzn_trace_thunk_30[];
extern char dzn_trace_thunk_31[];
extern char dzn_trace_thunk_32[];
extern char dzn_trace_thunk_33[];
extern char dzn_trace_thunk_34[];
extern char dzn_trace_thunk_35[];
extern char dzn_trace_thunk_36[];
extern char dzn_trace_thunk_37[];
extern char dzn_trace_thunk_38[];
extern char dzn_trace_thunk_39[];
extern char dzn_trace_thunk_40[];
extern char dzn_trace_thunk_41[];
extern char dzn_trace_thunk_42[];
extern char dzn_trace_thunk_43[];
extern char dzn_trace_thunk_44[];
extern char dzn_trace_thunk_45[];
extern char dzn_trace_thunk_46[];
extern char dzn_trace_thunk_47[];
extern char dzn_trace_thunk_48[];
extern char dzn_trace_thunk_49[];
extern char dzn_trace_thunk_50[];
extern char dzn_trace_thunk_51[];
extern char dzn_trace_thunk_52[];
extern char dzn_trace_thunk_53[];
extern char dzn_trace_thunk_54[];
extern char dzn_trace_thunk_55[];
extern char dzn_trace_thunk_56[];
extern char dzn_trace_thunk_57[];
extern char dzn_trace_thunk_58[];
extern char dzn_trace_thunk_59[];
extern char dzn_trace_thunk_60[];
extern char dzn_trace_thunk_61[];
extern char dzn_trace_thunk_62[];
extern char dzn_trace_thunk_63[];
extern char dzn_trace_thunk_64[];
extern char dzn_trace_thunk_65[];
extern char dzn_trace_thunk_66[];
extern char dzn_trace_thunk_67[];
extern char dzn_trace_thunk_68[];
extern char dzn_trace_thunk_69[];
extern char dzn_trace_thunk_70[];
extern char dzn_trace_thunk_71[];
extern char dzn_trace_thunk_72[];
extern char dzn_trace_thunk_73[];
extern char dzn_trace_thunk_74[];
extern char dzn_trace_thunk_75[];
extern char dzn_trace_thunk_76[];
extern char dzn_trace_thunk_77[];
extern char dzn_trace_thunk_78[];
extern char dzn_trace_thunk_79[];
extern char dzn_trace_thunk_80[];
extern char dzn_trace_thunk_81[];
extern char dzn_trace_thunk_82[];
extern char dzn_trace_thunk_83[];
extern char dzn_trace_thunk_84[];
extern char dzn_trace_thunk_85[];
static void *const dzn_trace_thunks[DZN_TRACE_SLOTS] = {dzn_trace_thunk_0, dzn_trace_thunk_1, dzn_trace_thunk_2, dzn_trace_thunk_3, dzn_trace_thunk_4, dzn_trace_thunk_5, dzn_trace_thunk_6, dzn_trace_thunk_7, dzn_trace_thunk_8, dzn_trace_thunk_9, dzn_trace_thunk_10, dzn_trace_thunk_11, dzn_trace_thunk_12, dzn_trace_thunk_13, dzn_trace_thunk_14, dzn_trace_thunk_15, dzn_trace_thunk_16, dzn_trace_thunk_17, dzn_trace_thunk_18, dzn_trace_thunk_19, dzn_trace_thunk_20, dzn_trace_thunk_21, dzn_trace_thunk_22, dzn_trace_thunk_23, dzn_trace_thunk_24, dzn_trace_thunk_25, dzn_trace_thunk_26, dzn_trace_thunk_27, dzn_trace_thunk_28, dzn_trace_thunk_29, dzn_trace_thunk_30, dzn_trace_thunk_31, dzn_trace_thunk_32, dzn_trace_thunk_33, dzn_trace_thunk_34, dzn_trace_thunk_35, dzn_trace_thunk_36, dzn_trace_thunk_37, dzn_trace_thunk_38, dzn_trace_thunk_39, dzn_trace_thunk_40, dzn_trace_thunk_41, dzn_trace_thunk_42, dzn_trace_thunk_43, dzn_trace_thunk_44, dzn_trace_thunk_45, dzn_trace_thunk_46, dzn_trace_thunk_47, dzn_trace_thunk_48, dzn_trace_thunk_49, dzn_trace_thunk_50, dzn_trace_thunk_51, dzn_trace_thunk_52, dzn_trace_thunk_53, dzn_trace_thunk_54, dzn_trace_thunk_55, dzn_trace_thunk_56, dzn_trace_thunk_57, dzn_trace_thunk_58, dzn_trace_thunk_59, dzn_trace_thunk_60, dzn_trace_thunk_61, dzn_trace_thunk_62, dzn_trace_thunk_63, dzn_trace_thunk_64, dzn_trace_thunk_65, dzn_trace_thunk_66, dzn_trace_thunk_67, dzn_trace_thunk_68, dzn_trace_thunk_69, dzn_trace_thunk_70, dzn_trace_thunk_71, dzn_trace_thunk_72, dzn_trace_thunk_73, dzn_trace_thunk_74, dzn_trace_thunk_75, dzn_trace_thunk_76, dzn_trace_thunk_77, dzn_trace_thunk_78, dzn_trace_thunk_79, dzn_trace_thunk_80, dzn_trace_thunk_81, dzn_trace_thunk_82, dzn_trace_thunk_83, dzn_trace_thunk_84, dzn_trace_thunk_85};

/* Route every call on this command list through a thunk that records (method, arg1, arg2) in a ring.
 * slots = vtable size of the most derived interface the object supports. */
void
dzn_trace_wrap(void *obj, unsigned slots)
{
   void ***pvtbl = (void ***)obj;
   if (*pvtbl == dzn_trace_proxy)
      return;
   if (!dzn_trace_orig_vtbl) {
      if (slots > DZN_TRACE_SLOTS)
         slots = DZN_TRACE_SLOTS;
      memcpy(dzn_trace_orig, *pvtbl, slots * sizeof(void *));
      for (unsigned i = 0; i < DZN_TRACE_SLOTS; i++)
         dzn_trace_proxy[i] = i < slots ? dzn_trace_thunks[i] : NULL;
      dzn_trace_orig_vtbl = *pvtbl;
      mesa_logi("dzn: tracing D3D12 command lists (%u methods)", slots);
   }
   if (*pvtbl == dzn_trace_orig_vtbl)
      *pvtbl = dzn_trace_proxy;
}

void
dzn_trace_dump(unsigned last)
{
   uint32_t end = dzn_trace_pos;
   if (last > DZN_TRACE_RING)
      last = DZN_TRACE_RING;
   if (last > end)
      last = end;
   mesa_loge("dzn: last %u D3D12 command list calls (oldest first, args = 1st/2nd after this):", last);
   for (uint32_t i = end - last; i != end; i++) {
      const struct dzn_trace_entry *e = &dzn_trace_ring[i % DZN_TRACE_RING];
      mesa_loge("dzn:   #%u %s(0x%llx, 0x%llx)", i, e->method < DZN_TRACE_SLOTS ? dzn_trace_names[e->method] : "?",
                (unsigned long long)e->a1, (unsigned long long)e->a2);
   }
}
