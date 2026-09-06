#include "administrateur.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QDebug>

bool Administrateur::authentifier(const QString &email, const QString &motDePasse, QString &nomOut)
{
    QString hash = QString::fromUtf8(
        QCryptographicHash::hash(motDePasse.toUtf8(), QCryptographicHash::Sha256).toHex());

    QSqlQuery query;
    query.prepare("SELECT nom_administrateur FROM ADMINISTRATEURS "
                  "WHERE LOWER(email_administrateur) = LOWER(:email) AND mot_de_passe_hash = :hash");
    query.bindValue(":email", email.trimmed());
    query.bindValue(":hash", hash);

    if (!query.exec()) {
        qDebug() << "Erreur authentification administrateur :" << query.lastError().text();
        return false;
    }
    if (!query.next()) return false; // identifiants incorrects

    nomOut = query.value(0).toString();
    return true;
}
