#include "utilisateur.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QDebug>

bool Utilisateur::authentifier(const QString &login, const QString &motDePasse, QString &roleOut)
{
    QString hash = QString::fromUtf8(
        QCryptographicHash::hash(motDePasse.toUtf8(), QCryptographicHash::Sha256).toHex());

    QSqlQuery query;
    query.prepare("SELECT role FROM UTILISATEURS WHERE login = :login AND mot_de_passe_hash = :hash");
    query.bindValue(":login", login);
    query.bindValue(":hash", hash);

    if (!query.exec()) {
        qDebug() << "Erreur authentification :" << query.lastError().text();
        return false;
    }
    if (!query.next()) return false; // identifiants incorrects

    roleOut = query.value(0).toString();
    return true;
}
