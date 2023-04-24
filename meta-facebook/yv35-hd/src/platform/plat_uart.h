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

#ifndef PLAT_UART_H
#define PLAT_UART_H

#define PROT_UART_PORT "UART_3"

#define UART_RING_BUFFER_SIZE 1024
#define MAX_NUM_TRIES 512
#define UART_RX_BUFF_SIZE 64
#define UART_HANDLER_STACK_SIZE 512
#define PROT_ERR_STR_LEN 6
#define PROT_ERR_STR_PREFIX "[E"
#define PROT_ERR_STR_SUFFIX ']'

#define STR_PREFIX_LEN 5

void uart_init();

#endif
