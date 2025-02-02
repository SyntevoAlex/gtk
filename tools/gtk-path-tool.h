#pragma once

void do_info      (int *argc, const char ***argv);
void do_decompose (int *argc, const char ***argv);
void do_render    (int *argc, const char ***argv);
void do_show      (int *argc, const char ***argv);

GskPath *get_path       (const char *arg);
int      get_enum_value (GType       type,
                         const char *type_nick,
                         const char *str);
void     get_color      (GdkRGBA    *rgba,
                         const char *str);
