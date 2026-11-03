#ifndef PERMISSIONS_H
#define PERMISSIONS_H

#include <sys/types.h>

int  perm_ensure_dir(const char *path, mode_t mode);
int  perm_get_mode(const char *path, mode_t *mode);       /* stat()  */
int  perm_set_mode(const char *path, mode_t mode);        /* chmod() */
void perm_to_string(mode_t mode, char out[10]);           /* rw-r----- */
int  perm_backup_file(const char *src, const char *dst);  /* copy + keep mode/times */

#endif