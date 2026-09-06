#include "inscription.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QCoreApplication>

QString Inscription::inscrire(int idParticipant, int idCours)
{
    QSqlQuery check;
    check.prepare("SELECT COUNT(*) FROM INSCRIPTIONS WHERE id_participant = :p AND id_cours = :c");
    check.bindValue(":p", idParticipant);
    check.bindValue(":c", idCours);
    if (check.exec() && check.next() && check.value(0).toInt() > 0)
        return QCoreApplication::translate("Inscription", "Ce participant est déjà inscrit à ce cours.");

    QSqlQuery seq;
    seq.prepare("SELECT seq_inscriptions.NEXTVAL FROM DUAL");
    if (!seq.exec() || !seq.next())
        return QCoreApplication::translate("Inscription", "Erreur interne (séquence).");
    int id = seq.value(0).toInt();

    QSqlQuery query;
    query.prepare("INSERT INTO INSCRIPTIONS (id_inscription, id_participant, id_cours, statut) "
                  "VALUES (:id, :p, :c, 'En attente')");
    query.bindValue(":id", id);
    query.bindValue(":p", idParticipant);
    query.bindValue(":c", idCours);

    if (!query.exec()) {
        qDebug() << "Erreur inscription :" << query.lastError().text();
        return QCoreApplication::translate("Inscription", "Erreur lors de l'inscription.");
    }
    return QString(); // succès
}

QSqlQueryModel* Inscription::afficher()
{
    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery(
        "SELECT i.id_inscription, p.nom_participant || ' ' || p.prenom_participant AS participant, "
        "c.titre_cours, TO_CHAR(i.date_inscription,'DD/MM/YYYY') AS date_inscription, i.statut "
        "FROM INSCRIPTIONS i "
        "JOIN PARTICIPANTS p ON p.id_participant = i.id_participant "
        "JOIN COURS c ON c.id_cours = i.id_cours "
        "ORDER BY i.id_inscription DESC");

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Participant"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Cours"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Date d'inscription"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Statut"));
    return model;
}

QSqlQueryModel* Inscription::chercher(const QString &texteCritere, const QString &statutCritere)
{
    QString queryString =
        "SELECT i.id_inscription, p.nom_participant || ' ' || p.prenom_participant AS participant, "
        "c.titre_cours, TO_CHAR(i.date_inscription,'DD/MM/YYYY') AS date_inscription, i.statut "
        "FROM INSCRIPTIONS i "
        "JOIN PARTICIPANTS p ON p.id_participant = i.id_participant "
        "JOIN COURS c ON c.id_cours = i.id_cours WHERE 1=1";

    if (!texteCritere.trimmed().isEmpty())
        queryString += " AND (LOWER(p.nom_participant) LIKE :texte OR LOWER(p.prenom_participant) LIKE :texte "
                        "OR LOWER(c.titre_cours) LIKE :texte)";
    if (!statutCritere.isEmpty() && statutCritere != QObject::tr("Tous"))
        queryString += " AND i.statut = :statut";

    queryString += " ORDER BY i.id_inscription DESC";

    QSqlQuery query;
    query.prepare(queryString);
    if (!texteCritere.trimmed().isEmpty())
        query.bindValue(":texte", "%" + texteCritere.toLower() + "%");
    if (!statutCritere.isEmpty() && statutCritere != QObject::tr("Tous"))
        query.bindValue(":statut", statutCritere);

    QSqlQueryModel *model = new QSqlQueryModel();
    if (!query.exec()) {
        qDebug() << "Erreur recherche inscriptions :" << query.lastError().text();
        delete model;
        return nullptr;
    }

    model->setQuery(query);
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Participant"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Cours"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Date d'inscription"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Statut"));
    return model;
}

bool Inscription::changerStatut(int id, const QString &nouveauStatut)
{
    QSqlQuery query;
    query.prepare("UPDATE INSCRIPTIONS SET statut = :statut WHERE id_inscription = :id");
    query.bindValue(":statut", nouveauStatut);
    query.bindValue(":id", id);
    if (!query.exec()) {
        qDebug() << "Erreur changement de statut :" << query.lastError().text();
        return false;
    }
    return true;
}

bool Inscription::supprimer(int id)
{
    // Une facture liée bloque la suppression (protège FACTURES.id_inscription).
    QSqlQuery check;
    check.prepare("SELECT COUNT(*) FROM FACTURES WHERE id_inscription = :id");
    check.bindValue(":id", id);
    if (check.exec() && check.next() && check.value(0).toInt() > 0) {
        qDebug() << "Suppression refusée : une facture dépend de cette inscription.";
        return false;
    }

    QSqlQuery query;
    query.prepare("DELETE FROM INSCRIPTIONS WHERE id_inscription = :id");
    query.bindValue(":id", id);
    if (!query.exec()) {
        qDebug() << "Erreur suppression inscription :" << query.lastError().text();
        return false;
    }
    return true;
}

bool Inscription::infosPourFacture(int idInscription, QString &participantOut, QString &coursOut, double &prixOut)
{
    QSqlQuery query;
    query.prepare(
        "SELECT p.nom_participant || ' ' || p.prenom_participant, c.titre_cours, c.prix "
        "FROM INSCRIPTIONS i "
        "JOIN PARTICIPANTS p ON p.id_participant = i.id_participant "
        "JOIN COURS c ON c.id_cours = i.id_cours "
        "WHERE i.id_inscription = :id");
    query.bindValue(":id", idInscription);
    if (query.exec() && query.next()) {
        participantOut = query.value(0).toString();
        coursOut = query.value(1).toString();
        prixOut = query.value(2).toDouble();
        return true;
    }
    return false;
}
