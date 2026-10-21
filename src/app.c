#include "app.h"

#include "ui/pda_shell.h"
#include "lvgl/lvgl.h"

static lv_group_t * g_group;

static void bind_keypad_group(lv_group_t * group)
{
    lv_indev_t * indev = NULL;
    while((indev = lv_indev_get_next(indev)) != NULL) {
        if(lv_indev_get_type(indev) == LV_INDEV_TYPE_KEYPAD) {
            lv_indev_set_group(indev, group);
        }
    }
}

void pda_app_init(void)
{
    g_group = lv_group_create();

    lv_obj_t * scr = lv_obj_create(NULL);
    pda_shell_create(scr, g_group);

    lv_screen_load(scr);
    bind_keypad_group(g_group);
}
