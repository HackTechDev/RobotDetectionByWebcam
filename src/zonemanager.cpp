#include "zonemanager.h"

#include <QObject>
#include <QSettings>

#include <algorithm>

namespace {

constexpr qint64 kExitDelayMs = 300;
constexpr double kExitMargin = 0.01;  // fraction de l'image

QColor zoneColor(int id)
{
    // Couleurs vives, distinctes de celles des objets suivis.
    static const QColor palette[] = {QColor(255, 210, 0),   QColor(0, 200, 120),  QColor(255, 90, 200),
                                     QColor(80, 160, 255),  QColor(255, 120, 40), QColor(160, 230, 60)};
    return palette[(id - 1) % (sizeof(palette) / sizeof(palette[0]))];
}

} // namespace

int ZoneManager::add(const QString &name, const QRectF &imageRect, const QSize &imageSize)
{
    Zone zone;
    zone.id = m_nextId++;
    zone.name = name;
    zone.color = zoneColor(zone.id);
    const double w = imageSize.width(), h = imageSize.height();
    zone.rect = QRectF(imageRect.x() / w, imageRect.y() / h, imageRect.width() / w, imageRect.height() / h)
                    .intersected(QRectF(0, 0, 1, 1));
    m_zones.push_back(zone);
    return zone.id;
}

void ZoneManager::remove(int id)
{
    m_zones.erase(std::remove_if(m_zones.begin(), m_zones.end(), [id](const Zone &z) { return z.id == id; }),
                  m_zones.end());
    m_presence.remove(id);
}

void ZoneManager::clear()
{
    m_zones.clear();
    m_presence.clear();
    m_nextId = 1;
}

void ZoneManager::rename(int id, const QString &name)
{
    for (Zone &z : m_zones) {
        if (z.id == id)
            z.name = name;
    }
}

QString ZoneManager::nextName() const
{
    return QObject::tr("Zone %1").arg(m_nextId);
}

QColor ZoneManager::colorOf(int id) const
{
    for (const Zone &z : m_zones) {
        if (z.id == id)
            return z.color;
    }
    return QColor();
}

QRectF ZoneManager::imageRect(const Zone &zone, const QSize &imageSize)
{
    const double w = imageSize.width(), h = imageSize.height();
    return QRectF(zone.rect.x() * w, zone.rect.y() * h, zone.rect.width() * w, zone.rect.height() * h);
}

void ZoneManager::load(QSettings &settings, const QString &group)
{
    clear();
    settings.beginGroup(group);
    const int count = settings.beginReadArray(QStringLiteral("zones"));
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        // « x y largeur hauteur », en fraction de l'image : lisible et modifiable à la main.
        const QStringList v = settings.value(QStringLiteral("rect")).toString().split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (v.size() != 4)
            continue;
        const QRectF rect(v[0].toDouble(), v[1].toDouble(), v[2].toDouble(), v[3].toDouble());
        if (rect.isEmpty())
            continue;
        Zone zone;
        zone.id = m_nextId++;
        zone.name = settings.value(QStringLiteral("name"), QObject::tr("Zone %1").arg(zone.id)).toString();
        zone.color = zoneColor(zone.id);
        zone.rect = rect;
        m_zones.push_back(zone);
    }
    settings.endArray();
    settings.endGroup();
}

void ZoneManager::save(QSettings &settings, const QString &group) const
{
    settings.beginGroup(group);
    settings.remove(QStringLiteral("zones"));
    settings.beginWriteArray(QStringLiteral("zones"), int(m_zones.size()));
    for (int i = 0; i < int(m_zones.size()); ++i) {
        settings.setArrayIndex(i);
        settings.setValue(QStringLiteral("name"), m_zones[i].name);
        const QRectF &r = m_zones[i].rect;
        settings.setValue(QStringLiteral("rect"), QStringLiteral("%1 %2 %3 %4")
                                                      .arg(r.x(), 0, 'f', 4)
                                                      .arg(r.y(), 0, 'f', 4)
                                                      .arg(r.width(), 0, 'f', 4)
                                                      .arg(r.height(), 0, 'f', 4));
    }
    settings.endArray();
    settings.endGroup();
}

std::vector<MotionEvent> ZoneManager::update(const std::vector<ZoneOccupant> &occupants, const QSize &imageSize,
                                             qint64 timeMs)
{
    std::vector<MotionEvent> events;
    if (imageSize.isEmpty())
        return events;
    const double mx = kExitMargin * imageSize.width(), my = kExitMargin * imageSize.height();
    auto makeEvent = [&](MotionEvent::Type type, const Zone &zone, const ZoneOccupant &o) {
        MotionEvent e;
        e.type = type;
        e.timeMs = timeMs;
        e.onFloor = o.hasFloor;
        e.position = o.hasFloor ? o.floorPos : o.imagePos;
        e.objectId = o.objectId;
        e.objectName = o.name;
        e.zoneId = zone.id;
        e.zoneName = zone.name;
        events.push_back(e);
    };
    for (Zone &zone : m_zones) {
        const QRectF rect = imageRect(zone, imageSize);
        const QRectF loose = rect.adjusted(-mx, -my, mx, my);
        QMap<QString, Presence> &presence = m_presence[zone.id];
        for (const ZoneOccupant &o : occupants) {
            Presence &p = presence[o.key];
            const bool in = p.inside ? loose.contains(o.imagePos) : rect.contains(o.imagePos);
            if (in) {
                if (!p.inside)
                    makeEvent(MotionEvent::ZoneEnter, zone, o);
                p.inside = true;
                p.lastInsideMs = timeMs;
                p.last = o;
            }
        }
        // Sorties : resté dehors (ou plus vu) au-delà du délai.
        zone.occupants.clear();
        for (auto it = presence.begin(); it != presence.end();) {
            Presence &p = it.value();
            if (p.inside && timeMs - p.lastInsideMs > kExitDelayMs) {
                makeEvent(MotionEvent::ZoneExit, zone, p.last);
                p.inside = false;
            }
            if (p.inside)
                zone.occupants << p.last.name;
            it = p.inside ? std::next(it) : presence.erase(it);
        }
    }
    return events;
}
