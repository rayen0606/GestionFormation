#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <QString>
#include <QSqlQueryModel>

// Journalise toutes les actions effectuées dans l'application (ajout,
// modification, suppression, import/export, certificats...) afin de les
// afficher dans le fil d'activité du tableau de bord.
class Notification
{
public:
    static bool ajouter(const QString &message);

    // Les "limite" notifications les plus récentes, triées par date décroissante.
    static QSqlQueryModel* lister(int limite = 15);
};

#endif // NOTIFICATION_H
