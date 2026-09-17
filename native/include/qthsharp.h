#ifndef QTHSHARP_H
#define QTHSHARP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Event type constants (returned by qth_poll_event) ──────────────── */
#define QTH_EVENT_NONE            0
#define QTH_EVENT_CLICKED         1  /* QPushButton clicked            */
#define QTH_EVENT_TEXT_CHANGED    2  /* QLineEdit textChanged           */
#define QTH_EVENT_RETURN_PRESSED  3  /* QLineEdit returnPressed         */
#define QTH_EVENT_TOGGLED         4  /* QCheckBox toggled               */
#define QTH_EVENT_WINDOW_CLOSED   5  /* QMainWindow close event         */
#define QTH_EVENT_VALUE_CHANGED   6  /* QSlider / QSpinBox valueChanged */

/* ── Application lifecycle ───────────────────────────────────────────── */
int64_t qth_app_init(void);                 /* creates QApplication once; returns 1 first time, 0 if already running */
int32_t qth_app_exec(void);                 /* blocking Qt event loop; returns process exit code */
void    qth_app_quit(void);                 /* QApplication::quit() */
void    qth_app_process_events(void);       /* QCoreApplication::processEvents() — for manual polling loops */
int8_t  qth_app_is_running(void);           /* 1 while no quit() has been requested */
void    qth_app_set_style(const char* style_name); /* e.g. "Fusion" */
void    qth_app_set_app_name(const char* name);

/* ── Window / widget creation (all return an opaque QWidget* handle) ── */
int64_t qth_window_new(const char* title, int32_t width, int32_t height); /* QMainWindow */
int64_t qth_widget_new(void);                                             /* plain QWidget container */
int64_t qth_button_new(const char* text);                                 /* QPushButton */
int64_t qth_label_new(const char* text);                                  /* QLabel */
int64_t qth_lineedit_new(const char* placeholder);                        /* QLineEdit */
int64_t qth_checkbox_new(const char* text);                               /* QCheckBox */
int64_t qth_slider_new(int32_t min, int32_t max, int32_t initial);        /* horizontal QSlider */

/* ── Layouts (opaque QLayout* handles) ───────────────────────────────── */
int64_t qth_vbox_new(void);   /* QVBoxLayout */
int64_t qth_hbox_new(void);   /* QHBoxLayout */
void    qth_layout_add_widget(int64_t layout_h, int64_t widget_h);
void    qth_layout_add_spacing(int64_t layout_h, int32_t px);
void    qth_layout_add_stretch(int64_t layout_h, int32_t stretch);
void    qth_layout_set_margins(int64_t layout_h, int32_t l, int32_t t, int32_t r, int32_t b);
void    qth_widget_set_layout(int64_t widget_h, int64_t layout_h);

/* ── Generic QWidget operations ──────────────────────────────────────── */
void    qth_widget_show(int64_t h);
void    qth_widget_hide(int64_t h);
void    qth_widget_close(int64_t h);
void    qth_widget_resize(int64_t h, int32_t w, int32_t hgt);
void    qth_widget_move(int64_t h, int32_t x, int32_t y);
void    qth_widget_set_enabled(int64_t h, int8_t enabled);
void    qth_widget_set_visible(int64_t h, int8_t visible);
void    qth_widget_set_style_sheet(int64_t h, const char* css);
void    qth_widget_set_fixed_size(int64_t h, int32_t w, int32_t hgt);
void    qth_window_set_central_widget(int64_t window_h, int64_t widget_h); /* QMainWindow only */
void    qth_window_set_title(int64_t h, const char* title);
void    qth_widget_destroy(int64_t h); /* deleteLater() — release a handle you no longer need */

/* ── QLabel ───────────────────────────────────────────────────────────── */
void        qth_label_set_text(int64_t h, const char* text);
const char* qth_label_get_text(int64_t h); /* caller must qth_free_string() the result */

/* ── QPushButton ──────────────────────────────────────────────────────── */
void qth_button_set_text(int64_t h, const char* text);

/* ── QLineEdit ────────────────────────────────────────────────────────── */
const char* qth_lineedit_get_text(int64_t h); /* caller must qth_free_string() the result */
void        qth_lineedit_set_text(int64_t h, const char* text);
void        qth_lineedit_set_placeholder(int64_t h, const char* text);
void        qth_lineedit_set_read_only(int64_t h, int8_t read_only);
void        qth_lineedit_set_echo_password(int64_t h, int8_t is_password);

/* ── QCheckBox ────────────────────────────────────────────────────────── */
int8_t qth_checkbox_is_checked(int64_t h);
void   qth_checkbox_set_checked(int64_t h, int8_t checked);

/* ── QSlider ──────────────────────────────────────────────────────────── */
int32_t qth_slider_get_value(int64_t h);
void    qth_slider_set_value(int64_t h, int32_t value);

/* ── Dialogs (blocking, no event needed) ─────────────────────────────── */
void    qth_show_info(int64_t parent_h, const char* title, const char* text);
void    qth_show_warning(int64_t parent_h, const char* title, const char* text);
void    qth_show_error(int64_t parent_h, const char* title, const char* text);
int8_t  qth_show_question(int64_t parent_h, const char* title, const char* text); /* 1 = Yes, 0 = No */

/* ── Event queue (poll-based — see file header comment) ──────────────── */
/* Pops the next queued event; returns QTH_EVENT_NONE if the queue is
 * empty. Call qth_event_handle() right after a non-zero return to find
 * out which widget it came from. */
int32_t qth_poll_event(void);
int64_t qth_event_handle(void);

/* ── Memory ───────────────────────────────────────────────────────────── */
void qth_free_string(const char* s); /* frees strings returned by qth_*_get_text */

#ifdef __cplusplus
}
#endif

#endif /* QTHSHARP_H */
