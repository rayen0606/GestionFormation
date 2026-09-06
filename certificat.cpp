#include "certificat.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

bool Certificat::enregistrer(int idCours, const QString &nomParticipant)
{
    QSqlQuery query;
    query.prepare("INSERT INTO CERTIFICATS (id_certificat, id_cours, nom_participant, date_emission) "
                  "VALUES (seq_certificats.NEXTVAL, :idCours, :nom, SYSDATE)");
    query.bindValue(":idCours", idCours);
    query.bindValue(":nom", nomParticipant);

    if (!query.exec()) {
        qDebug() << "Erreur enregistrement certificat :" << query.lastError().text();
        return false;
    }
    return true;
}
