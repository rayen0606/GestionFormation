#ifndef FACTURE_H
#define FACTURE_H

#include <QString>
#include <QSqlQueryModel>

// Gère la facturation et le suivi des paiements (table FACTURES),
// toujours rattachée à une inscription (une facture par inscription).
class Facture
{
public:
    // Génère une facture pour une inscription (montant = prix du cours).
    // Retourne "" en cas de succès, sinon un message d'erreur à afficher.
    static QString genererDepuisInscription(int idInscription);

    // Liste jointe : participant, cours, montant, date, statut, mode de paiement.
    static QSqlQueryModel* afficher();

    // Recherche texte (participant/cours) + filtre par statut de paiement.
    static QSqlQueryModel* chercher(const QString &texteCritere, const QString &statutCritere);

    // Inscriptions confirmées qui n'ont pas encore de facture (pour la
    // combo-box "Générer une facture").
    static QSqlQueryModel* inscriptionsSansFacture();

    static bool marquerPayee(int id, const QString &modePaiement);
    static bool annuler(int id);
    static bool supprimer(int id);

    // Indicateurs pour le tableau de bord / la page Facturation.
    static void statistiques(double &totalFacture, double &totalEncaisse, double &totalImpaye);
};

#endif // FACTURE_H
