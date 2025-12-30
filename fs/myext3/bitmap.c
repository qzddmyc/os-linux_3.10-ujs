/*
 *  linux/fs/myext3/bitmap.c
 *
 * Copyright (C) 1992, 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 */

#include "myext3.h"

#ifdef MYEXT3FS_DEBUG

unsigned long myext3_count_free (struct buffer_head * map, unsigned int numchars)
{
	return numchars * BITS_PER_BYTE - memweight(map->b_data, numchars);
}

#endif  /*  MYEXT3FS_DEBUG  */

