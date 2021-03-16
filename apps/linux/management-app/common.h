/*
 * Copyright 2020 NXP.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

void usage (void);
int check_response(uint16_t *expected, uint16_t *response, unsigned int len);
uint8_t *get_response_header(uint8_t *buf, uint16_t *id, uint16_t *length, uint16_t *status);
uint8_t *get_node_next(uint8_t *buf, uint16_t length);
uint8_t *get_node_header(uint8_t *buf, uint16_t *id, uint16_t *length, uint16_t *status);

