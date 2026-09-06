#include "notification.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

bool Notification::ajouter(const QString &message)
{
    QSqlQuery query;
    query.prepare("INSERT INTO NOTIFICATIONS (id_notification, message, date_action) "
                  "VALUES (seq_notifications.NEXTVAL, :message, SYSDATE)");
    query.bindValue(":message", message);

    if (!query.exec()) {
        qDebug() << "Erreur ajout notification :" << query.lastError().text();
        return false;
    }
    return true;
}

QSqlQueryModel* Notification::lister(int limite)
{
    auto *modele = new QSqlQueryModel();
    // ROWNUM (plutôt que FETCH FIRST) pour rester compatible avec les
    // anciennes versions d'Oracle également.
    modele->setQuery(QString(
        "SELECT * FROM ("
        "  SELECT message AS \"Action\", "
        "         TO_CHAR(date_action, 'DD/MM/YYYY HH24:MI') AS \"Date\" "
        "  FROM NOTIFICATIONS ORDER BY date_action DESC"
        ") WHERE ROWNUM <= %1").arg(limite));
    return modele;
}
