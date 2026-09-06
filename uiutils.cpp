#include "uiutils.h"

#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QTimer>
#include <QLabel>
#include <QColor>

void UiUtils::appliquerOmbre(QWidget *widget, int rayon, int decalageY, int alpha)
{
    auto *ombre = new QGraphicsDropShadowEffect(widget);
    ombre->setBlurRadius(rayon);
    ombre->setOffset(0, decalageY);
    ombre->setColor(QColor(30, 58, 95, alpha));
    widget->setGraphicsEffect(ombre);
}

void UiUtils::animerEntree(QWidget *widget, int delaiMs, int dureeMs, std::function<void()> auTerme)
{
    // Uniquement un fondu d'opacité : animer la position d'un widget geré
    // par un layout (QGridLayout des cartes, etc.) entre en conflit avec
    // le repositionnement automatique du layout et donne un rendu saccadé.
    auto *opacite = new QGraphicsOpacityEffect(widget);
    opacite->setOpacity(0.0);
    widget->setGraphicsEffect(opacite);

    QTimer::singleShot(delaiMs, widget, [widget, opacite, dureeMs, auTerme]() {
        auto *anim = new QPropertyAnimation(opacite, "opacity", widget);
        anim->setDuration(dureeMs);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        QObject::connect(anim, &QPropertyAnimation::finished, widget, [widget, auTerme]() {
            // Retire l'effet d'opacité une fois le fondu terminé : le
            // widget ne doit pas garder un QGraphicsEffect actif en
            // permanence, sous peine de rendu blanc/casse si un widget
            // enfant reçoit lui aussi un effet par la suite.
            widget->setGraphicsEffect(nullptr);
            if (auTerme) auTerme();
        });
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

QWidget* UiUtils::creerBadgeNiveau(const QString &niveau)
{
    QLabel *badge = new QLabel(niveau);
    QString fond, texte;

    if (niveau.compare("Debutant", Qt::CaseInsensitive) == 0) {
        fond = "#DCFCE7"; texte = "#15803D";
    } else if (niveau.compare("Intermediaire", Qt::CaseInsensitive) == 0) {
        fond = "#FEF3C7"; texte = "#B45309";
    } else {
        fond = "#FEE2E2"; texte = "#B91C1C";
    }

    badge->setStyleSheet(QString(
        "background:%1; color:%2; border-radius:9px; padding:2px 10px; "
        "font-size:8.5pt; font-weight:700;").arg(fond, texte));
    badge->setAlignment(Qt::AlignCenter);
    badge->setFixedHeight(20);
    badge->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    return badge;
}
