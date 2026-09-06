#ifndef UTILISATEUR_H
#define UTILISATEUR_H

#include <QString>

// Gère l'authentification : vérifie un couple login/mot de passe contre
// la table UTILISATEURS (le mot de passe est stocké sous forme de hash
// SHA-256, jamais en clair).
class Utilisateur
{
public:
    // Vérifie les identifiants. Si valides, roleOut contient le rôle
    // de l'utilisateur (ex. "Administrateur").
    static bool authentifier(const QString &login, const QString &motDePasse, QString &roleOut);
};

#endif // UTILISATEUR_H
