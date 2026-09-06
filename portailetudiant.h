#ifndef PORTAILETUDIANT_H
#define PORTAILETUDIANT_H

#include <QString>
#include <QSqlQueryModel>

// Espace étudiant en libre-service :
//  - création de compte (email + mot de passe, hashé en SHA-256)
//  - connexion
//  - catalogue de tous les cours, avec indication "payé / non payé"
//  - paiement (simulé) qui débloque immédiatement la lecture/le téléchargement
class PortailEtudiant
{
public:
    // Crée un compte. Si l'email correspond déjà à un participant connu
    // (ajouté côté administration, sans mot de passe) et que ce dernier n'a
    // pas encore de compte, ce compte est "récupéré" (mot de passe défini
    // dessus) plutôt que dupliqué. Retourne "" en cas de succès, sinon un
    // message d'erreur à afficher.
    static QString creerCompte(const QString &nom, const QString &prenom, const QString &email,
                                const QString &telephone, const QString &motDePasse,
                                int &idParticipantOut);

    // Vérifie email + mot de passe. Retourne true et renseigne l'id/le nom
    // si les identifiants sont corrects et qu'un compte a bien été créé.
    static bool seConnecter(const QString &email, const QString &motDePasse,
                             int &idParticipantOut, QString &nomCompletOut);

    // Catalogue complet des cours, avec une colonne "paye" (0/1) indiquant
    // si CE participant a déjà réglé ce cours.
    static QSqlQueryModel* catalogueCours(int idParticipant);

    // Effectue le paiement d'un cours pour un participant : crée
    // l'inscription si besoin, la confirme, génère la facture et la marque
    // payée. Retourne "" en cas de succès, sinon un message d'erreur.
    static QString payerCours(int idParticipant, int idCours, const QString &modePaiement);

    static bool aPaye(int idParticipant, int idCours);
};

#endif // PORTAILETUDIANT_H
