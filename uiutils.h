#ifndef UIUTILS_H
#define UIUTILS_H

#include <QWidget>
#include <QString>
#include <functional>

// Petits utilitaires visuels partagés par les fenêtres publiques
// (connexion, catalogue étudiant) : ombres portées et animations
// d'entrée cohérentes, pour un rendu soigné sans dupliquer le code
// entre les fenêtres.
namespace UiUtils {

    // Ombre portée douce (cartes, panneaux).
    void appliquerOmbre(QWidget *widget, int rayon = 28, int decalageY = 10, int alpha = 50);

    // Fondu d'apparition (0 -> 1 d'opacité). "delaiMs" permet d'échelonner
    // l'apparition de plusieurs widgets (ex : les cartes d'une grille).
    // IMPORTANT : l'effet d'opacité est retiré dès que le fondu est terminé
    // (sinon un widget resterait en permanence avec un QGraphicsEffect actif,
    // ce qui casse le rendu si un descendant reçoit lui aussi un effet -
    // Qt ne gère pas bien les effets graphiques imbriqués parent/enfant).
    // "auTerme" s'exécute juste après, utile pour ré-appliquer une ombre
    // portée une fois le fondu terminé.
    void animerEntree(QWidget *widget, int delaiMs = 0, int dureeMs = 320,
                       std::function<void()> auTerme = nullptr);

    // Petit badge coloré selon le niveau du cours (Debutant/Intermediaire/Avance).
    QWidget* creerBadgeNiveau(const QString &niveau);
}

#endif // UIUTILS_H
