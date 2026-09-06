#ifndef ADMINISTRATEUR_H
#define ADMINISTRATEUR_H

#include <QString>

// Authentification de l'équipe du centre (table ADMINISTRATEURS).
// Compte par défaut fourni par le script SQL : admin@centre.tn / admin123
class Administrateur
{
public:
    // Vérifie email + mot de passe. Si valides, nomOut contient le nom
    // de l'administrateur (pour l'afficher dans le dashboard).
    static bool authentifier(const QString &email, const QString &motDePasse, QString &nomOut);
};

#endif // ADMINISTRATEUR_H
