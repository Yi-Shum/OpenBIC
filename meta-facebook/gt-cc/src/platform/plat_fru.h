/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PLAT_FRU_H
#define PLAT_FRU_H

#define SWB_FRU_PORT 0x05
#define SWB_FRU_ADDR (0xA8 >> 1)
#define SWB_FRU_MUX_ADDR (0xE0 >> 1)
#define SWB_FRU_MUX_CHAN 7
#define FIO_FRU_PORT 0x04
#define FIO_FRU_ADDR (0xA2 >> 1)
#define HSC_MODULE_FRU_PORT 0x05
#define HSC_MODULE_FRU_ADDR (0xA2 >> 1)
#define HSC_MODULE_FRU_MUX_ADDR (0xE0 >> 1)
#define HSC_MODULE_FRU_MUX_CHAN 6

enum {
	SWB_FRU_ID,
	FIO_FRU_ID,
	HSC_MODULE_FRU_ID,
	// OTHER_FRU_ID,
	MAX_FRU_ID,
};

#endif
