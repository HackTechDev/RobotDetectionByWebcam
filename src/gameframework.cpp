#include "gameframework.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QListWidget>
#include <QLocale>
#include <QPainter>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <cmath>

const GameObject *GameContext::object(int id) const
{
    for (const GameObject &o : tracked) {
        if (o.id == id)
            return &o;
    }
    return nullptr;
}

const DetectedMarker *GameContext::marker(int id) const
{
    if (!markers)
        return nullptr;
    for (const DetectedMarker &m : *markers) {
        if (m.id == id)
            return &m;
    }
    return nullptr;
}

const Zone *GameContext::zone(int id) const
{
    if (!zones)
        return nullptr;
    for (const Zone &z : *zones) {
        if (z.id == id)
            return &z;
    }
    return nullptr;
}

QRectF GameContext::zoneRect(int id) const
{
    const Zone *z = zone(id);
    return z ? ZoneManager::imageRect(*z, imageSize) : QRectF();
}

QString Game::checkReady(const GameSettings &settings, const GameContext &context) const
{
    for (const GameParam &p : params()) {
        switch (p.kind) {
        case GameParam::Tracked:
            if (!context.object(settings.value(p.key)))
                return QObject::tr("Ajoutez d'abord un objet suivi (onglet Suivi, « Ajouter des objets ») pour « %1 ».")
                    .arg(p.label);
            break;
        case GameParam::Zone:
            if (!context.zone(settings.value(p.key)))
                return QObject::tr("Tracez d'abord une zone (onglet Zones, « Ajouter une zone ») pour « %1 ».").arg(p.label);
            break;
        default:
            break;
        }
    }
    return QString();
}

void Game::start(const GameSettings &settings, const GameContext &context)
{
    m_settings = settings;
    m_active = true;
    m_running = true;
    onStart(context);
}

QJsonObject Game::toJson(const GameContext &) const
{
    return QJsonObject{{QStringLiteral("mode"), key()}, {QStringLiteral("running"), m_running}};
}

MotionEvent Game::event(const GameContext &context, const QString &code, const QString &text, int objectId,
                        int count) const
{
    MotionEvent e;
    e.type = MotionEvent::GameInfo;
    e.timeMs = context.timeMs;
    e.gameCode = code;
    e.gameText = text;
    e.count = count;
    e.objectId = objectId;
    if (const GameObject *o = context.object(objectId)) {
        e.objectName = o->name;
        e.onFloor = o->state->hasFloor;
        e.position = o->state->hasFloor ? o->state->floorPosition : o->state->center;
    } else {
        e.objectName = title();
    }
    return e;
}

GameDialog::GameDialog(Game *game, QWidget *parent)
    : QDialog(parent)
    , m_game(game)
{
    setWindowTitle(tr("Jeu : %1").arg(game->title()));
    auto *layout = new QVBoxLayout(this);
    auto *rules = new QLabel(game->rules(), this);
    rules->setWordWrap(true);
    layout->addWidget(rules);
    auto *form = new QFormLayout;
    for (const GameParam &p : game->params()) {
        ParamWidget w;
        w.param = p;
        QWidget *widget = nullptr;
        switch (p.kind) {
        case GameParam::Marker:
            w.spin = new QSpinBox(this);
            w.spin->setRange(0, 999);
            w.spin->setValue(p.defaultValue);
            widget = w.spin;
            break;
        case GameParam::ZoneList:
            w.list = new QListWidget(this);
            w.list->setMaximumHeight(110);
            widget = w.list;
            break;
        case GameParam::Choice:
            w.combo = new QComboBox(this);
            w.combo->addItems(p.choices);
            w.combo->setCurrentIndex(p.defaultValue);
            widget = w.combo;
            break;
        default:
            w.combo = new QComboBox(this);
            widget = w.combo;
            break;
        }
        widget->setToolTip(p.tooltip);
        form->addRow(p.label, widget);
        m_widgets.push_back(w);
    }
    layout->addLayout(form);

    m_stateLabel = new QLabel(this);
    m_stateLabel->setAlignment(Qt::AlignCenter);
    m_stateLabel->setMinimumHeight(80);
    m_stateLabel->setWordWrap(true);
    QFont big = m_stateLabel->font();
    big.setPointSizeF(big.pointSizeF() * 1.8);
    big.setBold(true);
    m_stateLabel->setFont(big);
    layout->addWidget(m_stateLabel);
    m_countersLabel = new QLabel(this);
    m_countersLabel->setAlignment(Qt::AlignCenter);
    m_countersLabel->setWordWrap(true);
    layout->addWidget(m_countersLabel);

    auto *buttons = new QDialogButtonBox(this);
    m_startButton = buttons->addButton(tr("Démarrer"), QDialogButtonBox::ActionRole);
    if (!game->actionLabel().isEmpty()) {
        m_actionButton = buttons->addButton(game->actionLabel(), QDialogButtonBox::ActionRole);
        connect(m_actionButton, &QPushButton::clicked, this, &GameDialog::actionRequested);
    }
    QPushButton *close = buttons->addButton(tr("Fermer"), QDialogButtonBox::RejectRole);
    layout->addWidget(buttons);
    connect(m_startButton, &QPushButton::clicked, this, [this] {
        emit m_running ? stopRequested() : startRequested();
    });
    connect(close, &QPushButton::clicked, this, &QDialog::close);
    m_stateLabel->setText(tr("Prêt"));
    m_stateLabel->setStyleSheet(QStringLiteral("background: #ddd; color: #444; border-radius: 6px;"));
    setRunning(false);
    resize(480, sizeHint().height());
}

void GameDialog::setChoices(const std::vector<std::pair<int, QString>> &objects,
                            const std::vector<std::pair<int, QString>> &zones)
{
    if (m_running)
        return;  // choix figés pendant la partie
    for (ParamWidget &w : m_widgets) {
        switch (w.param.kind) {
        case GameParam::Tracked: {
            const int current = w.combo->count() ? w.combo->currentData().toInt() : -1;
            w.combo->clear();
            for (const auto &[id, name] : objects)
                w.combo->addItem(name, id);
            int index = w.combo->findData(current);
            if (index < 0)
                index = std::min(w.param.defaultValue, w.combo->count() - 1);
            w.combo->setCurrentIndex(std::max(0, index));
            break;
        }
        case GameParam::Zone:
        case GameParam::ZoneOptional: {
            const int current = w.combo->count() ? w.combo->currentData().toInt() : -1;
            w.combo->clear();
            if (w.param.kind == GameParam::ZoneOptional)
                w.combo->addItem(w.param.noneLabel, 0);
            for (const auto &[id, name] : zones)
                w.combo->addItem(name, id);
            w.combo->setCurrentIndex(std::max(0, w.combo->findData(current)));
            break;
        }
        case GameParam::ZoneList: {
            // Zones déjà présentes : leur case est conservée ; nouvelle zone : cochée.
            QList<int> unchecked;
            for (int i = 0; i < w.list->count(); ++i) {
                if (w.list->item(i)->checkState() != Qt::Checked)
                    unchecked << w.list->item(i)->data(Qt::UserRole).toInt();
            }
            w.list->clear();
            for (const auto &[id, name] : zones) {
                auto *item = new QListWidgetItem(name, w.list);
                item->setData(Qt::UserRole, id);
                item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                item->setCheckState(unchecked.contains(id) ? Qt::Unchecked : Qt::Checked);
            }
            break;
        }
        default:
            break;
        }
    }
}

GameSettings GameDialog::settings() const
{
    GameSettings s;
    for (const ParamWidget &w : m_widgets) {
        switch (w.param.kind) {
        case GameParam::Marker:
            s.values.insert(w.param.key, w.spin->value());
            break;
        case GameParam::ZoneList: {
            QList<int> ids;
            for (int i = 0; i < w.list->count(); ++i) {
                if (w.list->item(i)->checkState() == Qt::Checked)
                    ids << w.list->item(i)->data(Qt::UserRole).toInt();
            }
            s.lists.insert(w.param.key, ids);
            break;
        }
        case GameParam::Choice:
            s.values.insert(w.param.key, w.combo->currentIndex());
            break;
        default:
            s.values.insert(w.param.key, w.combo->currentData().toInt());
            break;
        }
    }
    return s;
}

void GameDialog::setRunning(bool running)
{
    m_running = running;
    m_startButton->setText(running ? tr("Arrêter") : tr("Démarrer"));
    if (m_actionButton)
        m_actionButton->setEnabled(running);
    for (ParamWidget &w : m_widgets) {
        for (QWidget *widget : std::initializer_list<QWidget *>{w.combo, w.spin, w.list}) {
            if (widget)
                widget->setEnabled(!running);
        }
    }
}

void GameDialog::showStopped()
{
    setRunning(false);
    m_stateLabel->setText(tr("Partie arrêtée"));
    m_stateLabel->setStyleSheet(QStringLiteral("background: #ddd; color: #444; border-radius: 6px;"));
    m_countersLabel->clear();
}

void GameDialog::showState(const GameContext &context)
{
    m_stateLabel->setText(m_game->stateText(context));
    m_stateLabel->setStyleSheet(QStringLiteral("background: %1; color: white; border-radius: 6px;")
                                    .arg(m_game->bannerColor(context).name()));
    m_countersLabel->setText(m_game->countersText(context));
    if (m_running && !m_game->isRunning())
        setRunning(false);  // partie terminée : résultat affiché, nouvelle partie possible
}

namespace GameUtil {

QColor grey() { return QColor(90, 90, 90); }
QColor green() { return QColor(26, 154, 58); }
QColor red() { return QColor(201, 48, 44); }
QColor orange() { return QColor(224, 142, 11); }
QColor blue() { return QColor(30, 100, 200); }
QColor gold() { return QColor(200, 150, 0); }
QColor purple() { return QColor(150, 20, 140); }

QString seconds(double s)
{
    return QLocale().toString(s, 'f', 1);
}

QString clock(double s)
{
    const int t = int(std::ceil(std::max(0.0, s)));
    return QStringLiteral("%1:%2").arg(t / 60).arg(t % 60, 2, 10, QLatin1Char('0'));
}

double distance(const QPointF &a, const QPointF &b)
{
    return std::hypot(a.x() - b.x(), a.y() - b.y());
}

bool insideUnion(const QRectF &box, const std::vector<QRectF> &rects)
{
    for (const QPointF &p : {box.topLeft(), box.topRight(), box.bottomRight(), box.bottomLeft(), box.center()}) {
        bool in = false;
        for (const QRectF &r : rects)
            in = in || r.contains(p);
        if (!in)
            return false;
    }
    return true;
}

void label(const GameDrawContext &draw, const QPointF &displayPos, const QString &text, const QColor &color)
{
    QPainter *p = draw.painter;
    const QFontMetrics fm(draw.font);
    const QRectF box(displayPos.x(), displayPos.y() - fm.height(), fm.horizontalAdvance(text) + draw.penWidth * 2,
                     fm.height());
    p->setPen(Qt::NoPen);
    p->setBrush(QColor(0, 0, 0, 140));
    p->drawRect(box);
    p->setPen(color);
    p->setFont(draw.font);
    p->drawText(box, Qt::AlignCenter, text);
    p->setBrush(Qt::NoBrush);
}

} // namespace GameUtil
