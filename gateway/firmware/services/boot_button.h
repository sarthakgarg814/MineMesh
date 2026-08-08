#ifndef BOOT_BUTTON_H
#define BOOT_BUTTON_H

typedef void (*boot_button_reset_cb_t)(void);
void boot_button_start(boot_button_reset_cb_t reset_cb);

#endif
