#include "session.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

int Session::prochainId()
{
    QSqlQuery query;
    query.prepare("SELECT seq_sessions.NEXTVAL FROM DUAL");
    if (query.exec() && query.next())
        return query.value(0).toInt();
    return -1;
}

bool Session::ajouter(int idCours, const QDate &date, const QString &heureDebut,
                       const QString &heureFin, const QString &salle, int capaciteMax)
{
    int id = prochainId();
    if (id < 0) return false;

    QSqlQuery query;
    query.prepare("INSERT INTO SESSIONS (id_session, id_cours, date_session, heure_debut, heure_fin, salle, capacite_max) "
                  "VALUES (:id, :cours, TO_DATE(:date, 'YYYY-MM-DD'), :hd, :hf, :salle, :cap)");
    query.bindValue(":id", id);
    query.bindValue(":cours", idCours);
    query.bindValue(":date", date.toString("yyyy-MM-dd"));
    query.bindValue(":hd", heureDebut);
    query.bindValue(":hf", heureFin);
    query.bindValue(":salle", salle);
    query.bindValue(":cap", capaciteMax);

    if (!query.exec()) {
        qDebug() << "Erreur ajout séance :" << query.lastError().text();
        return false;
    }
    return true;
}

bool Session::modifier(int id, int idCours, const QDate &date, const QString &heureDebut,
                        const QString &heureFin, const QString &salle, int capaciteMax)
{
    QSqlQuery query;
    query.prepare("UPDATE SESSIONS SET id_cours = :cours, date_session = TO_DATE(:date, 'YYYY-MM-DD'), "
                  "heure_debut = :hd, heure_fin = :hf, salle = :salle, capacite_max = :cap "
                  "WHERE id_session = :id");
    query.bindValue(":cours", idCours);
    query.bindValue(":date", date.toString("yyyy-MM-dd"));
    query.bindValue(":hd", heureDebut);
    query.bindValue(":hf", heureFin);
    query.bindValue(":salle", salle);
    query.bindValue(":cap", capaciteMax);
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Erreur modification séance :" << query.lastError().text();
        return false;
    }
    return true;
}

bool Session::supprimer(int id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM SESSIONS WHERE id_session = :id");
    query.bindValue(":id", id);
    if (!query.exec()) {
        qDebug() << "Erreur suppression séance :" << query.lastError().text();
        return false;
    }
    return true;
}

QSqlQueryModel* Session::afficher()
{
    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery(
        "SELECT s.id_session, c.titre_cours, TO_CHAR(s.date_session,'DD/MM/YYYY') AS date_session, "
        "s.heure_debut || ' - ' || s.heure_fin AS horaire, s.salle, s.capacite_max "
        "FROM SESSIONS s JOIN COURS c ON c.id_cours = s.id_cours "
        "ORDER BY s.date_session, s.heure_debut");

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Cours"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Date"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Horaire"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Salle"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Capacité"));
    return model;
}

QSqlQueryModel* Session::sessionsDuJour(const QDate &date)
{
    QSqlQueryModel *model = new QSqlQueryModel();
    QSqlQuery query;
    query.prepare(
        "SELECT s.id_session, c.titre_cours, s.heure_debut || ' - ' || s.heure_fin AS horaire, "
        "s.salle, s.capacite_max "
        "FROM SESSIONS s JOIN COURS c ON c.id_cours = s.id_cours "
        "WHERE s.date_session = TO_DATE(:date, 'YYYY-MM-DD') ORDER BY s.heure_debut");
    query.bindValue(":date", date.toString("yyyy-MM-dd"));

    if (!query.exec()) {
        qDebug() << "Erreur chargement séances du jour :" << query.lastError().text();
        return model;
    }
    model->setQuery(query);
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Cours"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Horaire"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Salle"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Capacité"));
    return model;
}

QList<int> Session::joursAvecSessions(int mois, int annee)
{
    QList<int> jours;
    QSqlQuery query;
    query.prepare("SELECT DISTINCT EXTRACT(DAY FROM date_session) FROM SESSIONS "
                  "WHERE EXTRACT(MONTH FROM date_session) = :mois AND EXTRACT(YEAR FROM date_session) = :annee");
    query.bindValue(":mois", mois);
    query.bindValue(":annee", annee);
    if (query.exec()) {
        while (query.next())
            jours << query.value(0).toInt();
    }
    return jours;
}

QSqlQueryModel* Session::prochainesSeances(int limite)
{
    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery(QString(
        "SELECT * FROM ("
        "  SELECT c.titre_cours AS \"Cours\", TO_CHAR(s.date_session,'DD/MM/YYYY') AS \"Date\", "
        "         s.heure_debut || ' - ' || s.heure_fin AS \"Horaire\" "
        "  FROM SESSIONS s JOIN COURS c ON c.id_cours = s.id_cours "
        "  WHERE s.date_session >= TRUNC(SYSDATE) ORDER BY s.date_session, s.heure_debut"
        ") WHERE ROWNUM <= %1").arg(limite));
    return model;
}
