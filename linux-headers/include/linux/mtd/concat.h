/*
 * MTD device concatenation layer definitions
 *
 * (C) 2002 Robert Kaiser <rkaiser@sysgo.de>
 *
 * This code is GPL
 *
 * $Id: concat.h,v 1.1.1.1 2006/04/11 21:47:07 cchavva Exp $
 */

#ifndef MTD_CONCAT_H
#define MTD_CONCAT_H


struct mtd_info *mtd_concat_create(
    struct mtd_info *subdev[],  /* subdevices to concatenate */
    int num_devs,               /* number of subdevices      */
    char *name);                /* name for the new device   */

void mtd_concat_destroy(struct mtd_info *mtd);

#endif

