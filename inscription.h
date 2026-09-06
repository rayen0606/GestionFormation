#ifndef INSCRIPTION_H
#define INSCRIPTION_H

#include <QString>
#include <QSqlQueryModel>

// Gère l'association Participant <-> Cours (table INSCRIPTIONS) :
// une personne s'inscrit à un cours, avec un statut qui évolue
// ("En attente" -> "Confirmee" ou "Annulee").
class Inscription
{
public:
    // Retourne "" en cas de succès, ou un message d'erreur à afficher
    // (ex : le participant est déjà inscrit à ce cours).
    static QString inscrire(int idParticipant, int idCours);

    // Liste jointe : participant, cours, date, statut (+ indique si une
    // facture existe déjà, pour piloter le bouton "Générer facture").
    static QSqlQueryModel* afficher();

    // Recherche (nom du participant ou titre du cours) + filtre par statut.
    static QSqlQueryModel* chercher(const QString &texteCritere, const QString &statutCritere);

    static bool changerStatut(int id, const QString &nouveauStatut);
    static bool supprimer(int id);

    // Renseignements utiles pour générer une facture depuis une inscription.
    static bool infosPourFacture(int idInscription, QString &participantOut, QString &coursOut, double &prixOut);
};

#endif // INSCRIPTION_H
