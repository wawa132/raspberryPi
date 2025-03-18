#ifndef __HASH_H__
#define __HASH_H__

#include "var.h"
#include "database.h"

void init_hash();
void calculate_sha256(const char *file_path, unsigned char *output_hash);

#endif