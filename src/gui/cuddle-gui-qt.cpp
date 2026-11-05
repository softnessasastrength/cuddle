#include "cuddle.h"
#include <cstdio>
#include <stdexcept>
#include <cstring>
#include <QApplication>
#include <QCheckBox>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QScrollArea>
#include <QShortcut>
#include <QVBoxLayout>
#include <QWidget>

class RoomWindow : public QWidget {
  cuddle_session session_{};
  QScrollArea *scroll_;
  QVBoxLayout *body_ = nullptr;
  void page() {
    if (auto *old = scroll_->takeWidget()) old->deleteLater();
    auto *content = new QWidget;
    body_ = new QVBoxLayout(content);
    body_->setSpacing(16);
    scroll_->setWidget(content);
  }
  void label(const char *text, bool heading = false) {
    auto *w = new QLabel(QString::fromUtf8(text));
    w->setWordWrap(true);
    w->setTextFormat(Qt::PlainText);
    w->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    if (heading) { auto font = w->font(); font.setBold(true); font.setPointSize(font.pointSize() + 4); w->setFont(font); }
    body_->addWidget(w);
  }
  template<typename Callback> QPushButton *button(const char *text, Callback callback) {
    auto *b = new QPushButton(QString::fromUtf8(text));
    connect(b, &QPushButton::clicked, this, callback);
    body_->addWidget(b);
    return b;
  }
  void hub() {
    cuddle_session_stop(&session_);
    page();
    label("What would help right now?", true);
    label("Local personal containment. Not therapy. No scores or stored answers. Touch Language is never consent.");
    for (size_t i = 0; i < cuddle_room_count(); i++) {
      const auto *room = cuddle_room_at(i);
      button(room->title, [this, room] { cuddle_session_begin(&session_, room->id); render(); });
      label(room->summary);
    }
    body_->addStretch();
  }
  void stop() { cuddle_session_stop(&session_); render(); }
  void render() {
    page();
    const auto *room = cuddle_session_room(&session_);
    if (!room) {
      label(session_.status == CUDDLE_SESSION_FINISHED ? "You can land here." : "Soft Signal", true);
      label(session_.status == CUDDLE_SESSION_FINISHED ? "Nothing is saved. Completing a room proves nothing." : cuddle_soft_signal_text());
      button("Return to rooms", [this] { hub(); });
    } else {
      label(room->title, true); label(room->summary);
      if (room->kind == CUDDLE_ROOM_TOUCH) {
        label("A map, never permission. Every new visit starts off limits.");
        for (size_t i = 0; i < CUDDLE_TOUCH_ZONES; i++) {
          const QString text = QString::fromUtf8(cuddle_touch_zone(i)) + " — " + QString::fromUtf8(cuddle_touch_status(session_.zones[i]));
          auto *b = new QPushButton(text);
          connect(b, &QPushButton::clicked, this, [this, i, b] {
            cuddle_session_cycle_zone(&session_, i);
            b->setText(QString::fromUtf8(cuddle_touch_zone(i)) + " — " + QString::fromUtf8(cuddle_touch_status(session_.zones[i])));
          });
          body_->addWidget(b);
        }
      } else {
        const auto *step = cuddle_session_step(&session_);
        label(step->title, true); label(step->body);
        if ((!std::strcmp(room->id, "morning") || !std::strcmp(room->id, "spoon")) && session_.step_index + 1 == room->step_count) {
          char summary[4096];
          if (!cuddle_session_summary(&session_, summary, sizeof(summary))) label(summary);
        }
        if (room->kind == CUDDLE_ROOM_INTEREST && session_.step_index == 6) {
          auto reading = cuddle_interest_read(session_.signal_bits, session_.answered, session_.unknown);
          label(reading.label, true); label(reading.advice);
        }
        for (size_t i = 0; i < (step->choice_count ? step->choice_count : 1); i++) {
          auto *b = button(step->choice_count ? step->choices[i].label : "Next / skip", [this, i] { cuddle_session_choose(&session_, i); render(); });
          if (i == 0) b->setFocus();
        }
        if (session_.history_count) button("Back", [this] { cuddle_session_back(&session_); render(); });
      }
    }
    body_->addStretch();
  }
public:
  RoomWindow(bool self_test = false) {
    setWindowTitle(QStringLiteral("Cuddle — rooms for ordinary human weather"));
    resize(680, 760);
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);
    auto *toolbar = new QHBoxLayout;
    root->addLayout(toolbar);
    auto *rooms = new QPushButton(QStringLiteral("Rooms"));
    auto *stopButton = new QPushButton(QStringLiteral("STOP · Soft Signal (q / Esc)"));
    toolbar->addWidget(rooms); toolbar->addWidget(stopButton);
    connect(rooms, &QPushButton::clicked, this, [this] { hub(); });
    connect(stopButton, &QPushButton::clicked, this, [this] { stop(); });
    auto *quiet = new QCheckBox(QStringLiteral("Larger, quieter view"));
    root->addWidget(quiet);
    connect(quiet, &QCheckBox::toggled, this, [this](bool enabled) {
      setStyleSheet(enabled ? "QWidget { font-size: 22px; } QPushButton { padding: 12px; }" : "QPushButton { padding: 12px; }");
    });
    const auto *preferences = cuddle_preferences_get();
    quiet->setChecked(preferences->large_text);
    if (preferences->appearance) {
      QPalette palette = QApplication::palette();
      bool dark = preferences->appearance == 2;
      palette.setColor(QPalette::Window, dark ? QColor(32, 33, 36) : QColor(250, 250, 250));
      palette.setColor(QPalette::WindowText, dark ? Qt::white : Qt::black);
      palette.setColor(QPalette::Base, dark ? QColor(24, 25, 28) : Qt::white);
      palette.setColor(QPalette::Text, dark ? Qt::white : Qt::black);
      palette.setColor(QPalette::Button, dark ? QColor(48, 49, 52) : QColor(235, 235, 235));
      palette.setColor(QPalette::ButtonText, dark ? Qt::white : Qt::black);
      setPalette(palette); setAutoFillBackground(true);
    }
    scroll_ = new QScrollArea;
    scroll_->setWidgetResizable(true);
    root->addWidget(scroll_);
    root->addWidget(new QLabel(QStringLiteral("Local only · nothing saved · Soft Signal needs no reason")));
    for (const auto &key : {QKeySequence(Qt::Key_Q), QKeySequence(Qt::Key_Escape)}) {
      auto *shortcut = new QShortcut(key, this);
      connect(shortcut, &QShortcut::activated, this, [this] { stop(); });
    }
    hub();
    if (!self_test && std::strcmp(preferences->start_room, "hub")) {
      if (!cuddle_session_begin(&session_, preferences->start_room)) render();
    }
  }
  void smoke() {
    auto click = [this](const char *text) {
      for (auto *b : scroll_->widget()->findChildren<QPushButton *>()) {
        if (b->text() == QString::fromUtf8(text)) { b->click(); return; }
      }
      throw std::runtime_error(text);
    };
    click("Conflict Practice");
    click("Repair script");
    if (session_.step_index != 4) throw std::runtime_error("repair branch");
    stop(); stop();
    if (session_.status != CUDDLE_SESSION_STOPPED) throw std::runtime_error("stop resumed a room");
    click("Return to rooms"); click("Touch Language");
    click("head / scalp — X off limits");
    if (session_.zones[0] != 1) throw std::runtime_error("touch cycle");
    stop(); click("Return to rooms"); click("Touch Language");
    if (session_.zones[0]) throw std::runtime_error("touch not reset");
    hub(); click("Want or Should?"); click("Not sure");
    for (int i = 0; i < 5; i++) click("No");
    bool unknown = false;
    for (auto *w : scroll_->widget()->findChildren<QLabel *>())
      if (w->text() == QStringLiteral("Unknown / knowing can wait")) unknown = true;
    if (!unknown) throw std::runtime_error("uncertainty reading");
    click("Soft Signal");
    if (session_.answered || session_.signal_bits || session_.unknown) throw std::runtime_error("answers survived stop");
    click("Return to rooms"); click("Morning Cuddle");
    click("Crispy — awake / board meeting"); click("Micro (2–5 minutes)");
    click("More styles"); click("Jetpack Mode"); click("Company only / no touch");
    click("You good, gremlin?");
    bool summary = false;
    for (auto *w : scroll_->widget()->findChildren<QLabel *>())
      if (w->text().contains(QStringLiteral("Jetpack Mode")) && w->text().contains(QStringLiteral("Crispy"))) summary = true;
    if (!summary) throw std::runtime_error("morning discussion summary");
    stop();
    std::puts("ok Qt widget navigation, branch, touch reset, uncertainty, Morning summary and stop");
  }
  ~RoomWindow() override { cuddle_session_stop(&session_); }
};
int main(int argc, char **argv) {
  if (argc > 1 && !std::strcmp(argv[1], "--version")) { std::printf("cuddle-gui %s\n", cuddle_version()); return 0; }
  if (argc > 1 && !std::strcmp(argv[1], "--help")) { std::puts("Usage: cuddle-gui\nShared Litmo rooms. q / Esc stops; nothing is saved."); return 0; }
  cuddle_config cfg;
  cuddle_i18n_init(); cuddle_config_load(&cfg); cuddle_config_apply(&cfg);
  QApplication app(argc, argv);
  QApplication::setDesktopFileName(QStringLiteral("org.softnessasstrength.Cuddle"));
  RoomWindow window(argc > 1 && !std::strcmp(argv[1], "--self-test"));
  if (argc > 1 && !std::strcmp(argv[1], "--self-test")) {
    try { window.smoke(); } catch (const std::exception &e) { std::fprintf(stderr, "Qt self-test: %s\n", e.what()); return 1; }
    return 0;
  }
  window.show();
  return app.exec();
}
