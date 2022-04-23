#ifndef __PIM_RUNTIME_H_
#define __PIM_RUNTIME_H_

#include <iostream>
#include "pim_config.h"

// PIM Preprocessor
bool isSuitableOps(PIM_OP op);
uint8_t* MapMemory(uint8_t *pmemAddr_, uint8_t *data, size_t len);
PIMKernel GetMicrokernelCode(PIM_OP op, PIM_OP_ATTRS op_attrs);

// PIM Memory Manager
bool AllocMem(uint8_t* pim_mem);
void FreeMem(uint8_t* pim_mem);
size_t ReadMem(uint8_t* pim_mem, uint8_t *data, size_t len);
size_t WriteMem(uint8_t* pim_mem, uint8_t *data, size_t len);

// PIM Kernel Executor
bool ExecuteKernel(uint8_t* pim_mem, uint8_t* pim_x, uint8_t* pim_y, uint8_t* pim_z, PIMKernel pim_kernel);
void ElementWiseExecute(uint8_t* pim_mem, uint8_t* pim_x, uint8_t* pim_y, uint8_t* pim_z, PIMKernel pim_kernel);
void BnExecute(uint8_t* pim_mem, uint8_t* pim_x, uint8_t* pim_y, uint8_t* pim_z, PIMKernel pim_kernel);
void GemvExecute(uint8_t* pim_mem, uint8_t* pim_x, uint8_t* pim_y, uint8_t* pim_z, PIMKernel pim_kernel);
void GemvExecuteBank(bool bank, uint8_t* pim_mem, uint8_t* pim_x, uint8_t* pim_y, uint8_t* pim_z, PIMKernel pim_kernel);
void LstmExecute(uint8_t* pim_mem, uint8_t* pim_x, uint8_t* pim_y, uint8_t* pim_z, PIMKernel pim_kernel);
void LstmExecuteBank(bool bank, uint8_t* pim_mem, uint8_t* pim_x, uint8_t* pim_y, uint8_t* pim_z, PIMKernel pim_kernel);

// Some tools
uint64_t Ceiling(uint64_t num, uint64_t stride);
void TryAddTransaction(uint8_t *pim_mem, uint8_t *data, bool is_write);
uint64_t GetAddress(int channel, int rank, int bankgroup, int bank, int row, int column);

#endif  // __PIM_RUNTIME_H_
