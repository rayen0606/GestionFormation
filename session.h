#ifndef SESSION_H
#define SESSION_H

#include <QString>
#include <QDate>
#include <QList>
#include <QSqlQueryModel>

// Gère les séances planifiées d'un cours (table SESSIONS) : c'est le
// planning / calendrier du centre (date, horaire, salle, capacité).
class Session
{
public:
    static int prochainId();

    static bool ajouter(int idCours, const QDate &date, const QString &heureDebut,
                         const QString &heureFin, const QString &salle, int capaciteMax);

    static bool modifier(int id, int idCours, const QDate &date, const QString &heureDebut,
                          const QString &heureFin, const QString &salle, int capaciteMax);

    static bool supprimer(int id);

    // Toutes les séances (jointes au titre du cours), triées par date/heure.
    static QSqlQueryModel* afficher();

    // Séances d'un jour précis (pour l'affichage du calendrier).
    static QSqlQueryModel* sessionsDuJour(const QDate &date);

    // Jours (1-31) du mois/année donnés qui contiennent au moins une
    // séance, pour mettre le calendrier en surbrillance.
    static QList<int> joursAvecSessions(int mois, int annee);

    // Prochaines séances (à partir d'aujourd'hui), pour le tableau de bord.
    static QSqlQueryModel* prochainesSeances(int limite = 8);
};

#endif // SESSION_H
