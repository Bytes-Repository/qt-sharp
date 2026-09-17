#include "qthsharp.h"

#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QCloseEvent>
#include <QString>

#include <cstring>
#include <cstdlib>
#include <deque>
#include <atomic>

// ─── Global state ──────────────────────────────────────────────────────────

namespace {

QApplication* g_app = nullptr;
std::atomic<bool> g_running{false};

struct Event {
    int32_t type;
    int64_t handle;
};

std::deque<Event> g_events;

void push_event(int32_t type, const void* handle) {
    g_events.push_back(Event{type, reinterpret_cast<int64_t>(handle)});
}

// QMainWindow subclass that turns closeEvent() into a QTH_EVENT_WINDOW_CLOSED
// instead of letting Qt tear the window down silently — H# code decides
// whether/when to actually quit.
class HWindow : public QMainWindow {
public:
    using QMainWindow::QMainWindow;

protected:
    void closeEvent(QCloseEvent* ev) override {
        push_event(QTH_EVENT_WINDOW_CLOSED, this);
        ev->ignore(); // H# calls qth_app_quit() / qth_widget_destroy() explicitly
    }
};

char* dup_cstr(const QString& s) {
    const QByteArray utf8 = s.toUtf8();
    char* out = static_cast<char*>(std::malloc(utf8.size() + 1));
    if (!out) return nullptr;
    std::memcpy(out, utf8.constData(), static_cast<size_t>(utf8.size()) + 1);
    return out;
}

inline QWidget* as_widget(int64_t h)   { return reinterpret_cast<QWidget*>(h); }
inline QLayout* as_layout(int64_t h)   { return reinterpret_cast<QLayout*>(h); }

} // namespace

// ─── Application lifecycle ──────────────────────────────────────────────────

extern "C" int64_t qth_app_init() {
    if (g_app) return 0;
    static int argc = 1;
    static char prog_name[] = "h-sharp-qt-app";
    static char* argv[] = { prog_name, nullptr };
    g_app = new QApplication(argc, argv);
    g_running.store(true);
    return 1;
}

extern "C" int32_t qth_app_exec() {
    if (!g_app) return -1;
    int rc = g_app->exec();
    g_running.store(false);
    return rc;
}

extern "C" void qth_app_quit() {
    g_running.store(false);
    if (g_app) g_app->quit();
}

extern "C" void qth_app_process_events() {
    if (g_app) QApplication::processEvents();
}

extern "C" int8_t qth_app_is_running() {
    return g_running.load() ? 1 : 0;
}

extern "C" void qth_app_set_style(const char* style_name) {
    if (style_name) QApplication::setStyle(QString::fromUtf8(style_name));
}

extern "C" void qth_app_set_app_name(const char* name) {
    if (name && g_app) g_app->setApplicationName(QString::fromUtf8(name));
}

// ─── Window / widget creation ───────────────────────────────────────────────

extern "C" int64_t qth_window_new(const char* title, int32_t width, int32_t height) {
    auto* w = new HWindow();
    if (title) w->setWindowTitle(QString::fromUtf8(title));
    if (width > 0 && height > 0) w->resize(width, height);
    return reinterpret_cast<int64_t>(static_cast<QWidget*>(w));
}

extern "C" int64_t qth_widget_new() {
    return reinterpret_cast<int64_t>(new QWidget());
}

extern "C" int64_t qth_button_new(const char* text) {
    auto* b = new QPushButton(text ? QString::fromUtf8(text) : QString());
    QObject::connect(b, &QPushButton::clicked, [b] {
        push_event(QTH_EVENT_CLICKED, b);
    });
    return reinterpret_cast<int64_t>(static_cast<QWidget*>(b));
}

extern "C" int64_t qth_label_new(const char* text) {
    auto* l = new QLabel(text ? QString::fromUtf8(text) : QString());
    return reinterpret_cast<int64_t>(static_cast<QWidget*>(l));
}

extern "C" int64_t qth_lineedit_new(const char* placeholder) {
    auto* e = new QLineEdit();
    if (placeholder) e->setPlaceholderText(QString::fromUtf8(placeholder));
    QObject::connect(e, &QLineEdit::textChanged, [e](const QString&) {
        push_event(QTH_EVENT_TEXT_CHANGED, e);
    });
    QObject::connect(e, &QLineEdit::returnPressed, [e] {
        push_event(QTH_EVENT_RETURN_PRESSED, e);
    });
    return reinterpret_cast<int64_t>(static_cast<QWidget*>(e));
}

extern "C" int64_t qth_checkbox_new(const char* text) {
    auto* c = new QCheckBox(text ? QString::fromUtf8(text) : QString());
    QObject::connect(c, &QCheckBox::toggled, [c](bool) {
        push_event(QTH_EVENT_TOGGLED, c);
    });
    return reinterpret_cast<int64_t>(static_cast<QWidget*>(c));
}

extern "C" int64_t qth_slider_new(int32_t min, int32_t max, int32_t initial) {
    auto* s = new QSlider(Qt::Horizontal);
    s->setMinimum(min);
    s->setMaximum(max);
    s->setValue(initial);
    QObject::connect(s, &QSlider::valueChanged, [s](int) {
        push_event(QTH_EVENT_VALUE_CHANGED, s);
    });
    return reinterpret_cast<int64_t>(static_cast<QWidget*>(s));
}

// ─── Layouts ────────────────────────────────────────────────────────────────

extern "C" int64_t qth_vbox_new() {
    return reinterpret_cast<int64_t>(static_cast<QLayout*>(new QVBoxLayout()));
}

extern "C" int64_t qth_hbox_new() {
    return reinterpret_cast<int64_t>(static_cast<QLayout*>(new QHBoxLayout()));
}

extern "C" void qth_layout_add_widget(int64_t layout_h, int64_t widget_h) {
    if (auto* lay = as_layout(layout_h)) lay->addWidget(as_widget(widget_h));
}

extern "C" void qth_layout_add_spacing(int64_t layout_h, int32_t px) {
    if (auto* box = qobject_cast<QBoxLayout*>(as_layout(layout_h))) box->addSpacing(px);
}

extern "C" void qth_layout_add_stretch(int64_t layout_h, int32_t stretch) {
    if (auto* box = qobject_cast<QBoxLayout*>(as_layout(layout_h))) box->addStretch(stretch);
}

extern "C" void qth_layout_set_margins(int64_t layout_h, int32_t l, int32_t t, int32_t r, int32_t b) {
    if (auto* lay = as_layout(layout_h)) lay->setContentsMargins(l, t, r, b);
}

extern "C" void qth_widget_set_layout(int64_t widget_h, int64_t layout_h) {
    if (auto* w = as_widget(widget_h)) w->setLayout(as_layout(layout_h));
}

// ─── Generic QWidget operations ─────────────────────────────────────────────

extern "C" void qth_widget_show(int64_t h)   { if (auto* w = as_widget(h)) w->show(); }
extern "C" void qth_widget_hide(int64_t h)   { if (auto* w = as_widget(h)) w->hide(); }
extern "C" void qth_widget_close(int64_t h)  { if (auto* w = as_widget(h)) w->close(); }

extern "C" void qth_widget_resize(int64_t h, int32_t w, int32_t hgt) {
    if (auto* wid = as_widget(h)) wid->resize(w, hgt);
}

extern "C" void qth_widget_move(int64_t h, int32_t x, int32_t y) {
    if (auto* w = as_widget(h)) w->move(x, y);
}

extern "C" void qth_widget_set_enabled(int64_t h, int8_t enabled) {
    if (auto* w = as_widget(h)) w->setEnabled(enabled != 0);
}

extern "C" void qth_widget_set_visible(int64_t h, int8_t visible) {
    if (auto* w = as_widget(h)) w->setVisible(visible != 0);
}

extern "C" void qth_widget_set_style_sheet(int64_t h, const char* css) {
    if (auto* w = as_widget(h)) w->setStyleSheet(css ? QString::fromUtf8(css) : QString());
}

extern "C" void qth_widget_set_fixed_size(int64_t h, int32_t w, int32_t hgt) {
    if (auto* wid = as_widget(h)) wid->setFixedSize(w, hgt);
}

extern "C" void qth_window_set_central_widget(int64_t window_h, int64_t widget_h) {
    if (auto* win = qobject_cast<QMainWindow*>(as_widget(window_h))) {
        win->setCentralWidget(as_widget(widget_h));
    }
}

extern "C" void qth_window_set_title(int64_t h, const char* title) {
    if (auto* w = as_widget(h)) w->setWindowTitle(title ? QString::fromUtf8(title) : QString());
}

extern "C" void qth_widget_destroy(int64_t h) {
    if (auto* w = as_widget(h)) w->deleteLater();
}

// ─── QLabel ─────────────────────────────────────────────────────────────────

extern "C" void qth_label_set_text(int64_t h, const char* text) {
    if (auto* l = qobject_cast<QLabel*>(as_widget(h))) {
        l->setText(text ? QString::fromUtf8(text) : QString());
    }
}

extern "C" const char* qth_label_get_text(int64_t h) {
    if (auto* l = qobject_cast<QLabel*>(as_widget(h))) return dup_cstr(l->text());
    return dup_cstr(QString());
}

// ─── QPushButton ────────────────────────────────────────────────────────────

extern "C" void qth_button_set_text(int64_t h, const char* text) {
    if (auto* b = qobject_cast<QPushButton*>(as_widget(h))) {
        b->setText(text ? QString::fromUtf8(text) : QString());
    }
}

// ─── QLineEdit ──────────────────────────────────────────────────────────────

extern "C" const char* qth_lineedit_get_text(int64_t h) {
    if (auto* e = qobject_cast<QLineEdit*>(as_widget(h))) return dup_cstr(e->text());
    return dup_cstr(QString());
}

extern "C" void qth_lineedit_set_text(int64_t h, const char* text) {
    if (auto* e = qobject_cast<QLineEdit*>(as_widget(h))) {
        e->setText(text ? QString::fromUtf8(text) : QString());
    }
}

extern "C" void qth_lineedit_set_placeholder(int64_t h, const char* text) {
    if (auto* e = qobject_cast<QLineEdit*>(as_widget(h))) {
        e->setPlaceholderText(text ? QString::fromUtf8(text) : QString());
    }
}

extern "C" void qth_lineedit_set_read_only(int64_t h, int8_t read_only) {
    if (auto* e = qobject_cast<QLineEdit*>(as_widget(h))) e->setReadOnly(read_only != 0);
}

extern "C" void qth_lineedit_set_echo_password(int64_t h, int8_t is_password) {
    if (auto* e = qobject_cast<QLineEdit*>(as_widget(h))) {
        e->setEchoMode(is_password != 0 ? QLineEdit::Password : QLineEdit::Normal);
    }
}

// ─── QCheckBox ──────────────────────────────────────────────────────────────

extern "C" int8_t qth_checkbox_is_checked(int64_t h) {
    if (auto* c = qobject_cast<QCheckBox*>(as_widget(h))) return c->isChecked() ? 1 : 0;
    return 0;
}

extern "C" void qth_checkbox_set_checked(int64_t h, int8_t checked) {
    if (auto* c = qobject_cast<QCheckBox*>(as_widget(h))) c->setChecked(checked != 0);
}

// ─── QSlider ────────────────────────────────────────────────────────────────

extern "C" int32_t qth_slider_get_value(int64_t h) {
    if (auto* s = qobject_cast<QSlider*>(as_widget(h))) return s->value();
    return 0;
}

extern "C" void qth_slider_set_value(int64_t h, int32_t value) {
    if (auto* s = qobject_cast<QSlider*>(as_widget(h))) s->setValue(value);
}

// ─── Dialogs ────────────────────────────────────────────────────────────────

extern "C" void qth_show_info(int64_t parent_h, const char* title, const char* text) {
    QMessageBox::information(as_widget(parent_h),
        title ? QString::fromUtf8(title) : QString(),
        text  ? QString::fromUtf8(text)  : QString());
}

extern "C" void qth_show_warning(int64_t parent_h, const char* title, const char* text) {
    QMessageBox::warning(as_widget(parent_h),
        title ? QString::fromUtf8(title) : QString(),
        text  ? QString::fromUtf8(text)  : QString());
}

extern "C" void qth_show_error(int64_t parent_h, const char* title, const char* text) {
    QMessageBox::critical(as_widget(parent_h),
        title ? QString::fromUtf8(title) : QString(),
        text  ? QString::fromUtf8(text)  : QString());
}

extern "C" int8_t qth_show_question(int64_t parent_h, const char* title, const char* text) {
    auto rc = QMessageBox::question(as_widget(parent_h),
        title ? QString::fromUtf8(title) : QString(),
        text  ? QString::fromUtf8(text)  : QString(),
        QMessageBox::Yes | QMessageBox::No);
    return rc == QMessageBox::Yes ? 1 : 0;
}

// ─── Event queue ────────────────────────────────────────────────────────────

namespace { int64_t g_last_event_handle = 0; }

extern "C" int32_t qth_poll_event() {
    if (g_events.empty()) return QTH_EVENT_NONE;
    Event ev = g_events.front();
    g_events.pop_front();
    g_last_event_handle = ev.handle;
    return ev.type;
}

extern "C" int64_t qth_event_handle() {
    return g_last_event_handle;
}

// ─── Memory ─────────────────────────────────────────────────────────────────

extern "C" void qth_free_string(const char* s) {
    std::free(const_cast<char*>(s));
}
