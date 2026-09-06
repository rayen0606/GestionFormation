#include "cours.h"
#include <QSqlError>
#include <QDebug>

Cours::Cours()
{
    id_cours = 0;
    titre_cours = "";
    description_cours = "";
    duree_heures = 0;
    niveau = "";
    prix = 0.0;
    date_debut = "";
    id_formateur = 0;
}

Cours::Cours(int id, QString titre, QString description, int duree, QString niveau,
             double prix, QString dateDebut, int idFormateur)
{
    this->id_cours = id;
    this->titre_cours = titre;
    this->description_cours = description;
    this->duree_heures = duree;
    this->niveau = niveau;
    this->prix = prix;
    this->date_debut = dateDebut;
    this->id_formateur = idFormateur;
}

int Cours::prochainId()
{
    QSqlQuery query;
    query.prepare("SELECT seq_cours.NEXTVAL FROM DUAL");
    if (query.exec() && query.next())
        return query.value(0).toInt();
    return -1;
}

bool Cours::ajouter()
{
    QSqlQuery query;
    query.prepare("INSERT INTO COURS "
                  "(id_cours, titre_cours, description_cours, duree_heures, niveau, prix, "
                  " date_debut, id_formateur) "
                  "VALUES (:id, :titre, :description, :duree, :niveau, :prix, "
                  " TO_DATE(:date_debut, 'YYYY-MM-DD'), :id_formateur)");

    query.bindValue(":id", id_cours);
    query.bindValue(":titre", titre_cours);
    query.bindValue(":description", description_cours);
    query.bindValue(":duree", duree_heures);
    query.bindValue(":niveau", niveau);
    query.bindValue(":prix", prix);
    query.bindValue(":date_debut", date_debut);
    query.bindValue(":id_formateur", id_formateur);

    if (!query.exec()) {
        qDebug() << "Erreur ajout cours :" << query.lastError().text();
        return false;
    }
    return true;
}

QSqlQueryModel* Cours::afficher()
{
    QSqlQueryModel *model = new QSqlQueryModel();
    // Jointure avec FORMATEURS pour afficher un libellé lisible (clé étrangère).
    model->setQuery("SELECT c.id_cours, c.titre_cours, c.description_cours, c.duree_heures, "
                     "c.niveau, c.prix, TO_CHAR(c.date_debut,'YYYY-MM-DD'), "
                     "c.id_formateur, f.nom_formateur || ' ' || f.prenom_formateur AS formateur "
                     "FROM COURS c LEFT JOIN FORMATEURS f ON c.id_formateur = f.id_formateur "
                     "ORDER BY c.id_cours");

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Titre"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Description"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Durée (h)"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Niveau"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Prix"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Date de début"));
    model->setHeaderData(7, Qt::Horizontal, QObject::tr("ID Formateur"));
    model->setHeaderData(8, Qt::Horizontal, QObject::tr("Formateur"));

    return model;
}

bool Cours::existe(int id)
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM COURS WHERE id_cours = :id");
    query.bindValue(":id", id);
    if (query.exec() && query.next())
        return query.value(0).toInt() > 0;
    return false;
}

bool Cours::modifier(int id, const QString &titre, const QString &description, int duree,
                      const QString &niveau, double prix, const QString &dateDebut, int idFormateur)
{
    QSqlQuery query;
    query.prepare("UPDATE COURS SET titre_cours = :titre, description_cours = :description, "
                  "duree_heures = :duree, niveau = :niveau, prix = :prix, "
                  "date_debut = TO_DATE(:date_debut, 'YYYY-MM-DD'), id_formateur = :id_formateur "
                  "WHERE id_cours = :id");

    query.bindValue(":id", id);
    query.bindValue(":titre", titre);
    query.bindValue(":description", description);
    query.bindValue(":duree", duree);
    query.bindValue(":niveau", niveau);
    query.bindValue(":prix", prix);
    query.bindValue(":date_debut", dateDebut);
    query.bindValue(":id_formateur", idFormateur);

    if (!query.exec()) {
        qDebug() << "Erreur modification cours :" << query.lastError().text();
        return false;
    }
    return true;
}

bool Cours::supprimer(int id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM COURS WHERE id_cours = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Erreur suppression cours :" << query.lastError().text();
        return false;
    }
    return true;
}

QSqlQueryModel* Cours::chercherCours(const QString &titreCritere,
                                      const QString &niveauCritere,
                                      const QString &formateurCritere)
{
    QSqlQueryModel *model = new QSqlQueryModel();

    QString queryString = "SELECT c.id_cours, c.titre_cours, c.description_cours, c.duree_heures, "
                           "c.niveau, c.prix, TO_CHAR(c.date_debut,'YYYY-MM-DD'), "
                           "c.id_formateur, f.nom_formateur || ' ' || f.prenom_formateur AS formateur "
                           "FROM COURS c LEFT JOIN FORMATEURS f ON c.id_formateur = f.id_formateur "
                           "WHERE 1=1";

    if (!titreCritere.trimmed().isEmpty())
        queryString += " AND LOWER(c.titre_cours) LIKE :titre";
    if (!niveauCritere.trimmed().isEmpty())
        queryString += " AND LOWER(c.niveau) LIKE :niveau";
    if (!formateurCritere.trimmed().isEmpty())
        queryString += " AND LOWER(f.nom_formateur || ' ' || f.prenom_formateur) LIKE :formateur";

    QSqlQuery query;
    query.prepare(queryString);

    if (!titreCritere.trimmed().isEmpty())
        query.bindValue(":titre", "%" + titreCritere.toLower() + "%");
    if (!niveauCritere.trimmed().isEmpty())
        query.bindValue(":niveau", "%" + niveauCritere.toLower() + "%");
    if (!formateurCritere.trimmed().isEmpty())
        query.bindValue(":formateur", "%" + formateurCritere.toLower() + "%");

    if (!query.exec()) {
        qDebug() << "Erreur recherche cours :" << query.lastError().text();
        delete model;
        return nullptr;
    }

    model->setQuery(query);
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Titre"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Description"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Durée (h)"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Niveau"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Prix"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Date de début"));
    model->setHeaderData(7, Qt::Horizontal, QObject::tr("ID Formateur"));
    model->setHeaderData(8, Qt::Horizontal, QObject::tr("Formateur"));

    return model;
}

QSqlQueryModel* Cours::trierCours(const QString &colonne, bool ordreAscendant)
{
    QString col = "c.titre_cours";
    if (colonne == "Titre") col = "c.titre_cours";
    else if (colonne == "Prix") col = "c.prix";
    else if (colonne == "Durée") col = "c.duree_heures";
    else if (colonne == "Date de début") col = "c.date_debut";

    QString queryString = "SELECT c.id_cours, c.titre_cours, c.description_cours, c.duree_heures, "
                           "c.niveau, c.prix, TO_CHAR(c.date_debut,'YYYY-MM-DD'), "
                           "c.id_formateur, f.nom_formateur || ' ' || f.prenom_formateur AS formateur "
                           "FROM COURS c LEFT JOIN FORMATEURS f ON c.id_formateur = f.id_formateur "
                           "ORDER BY " + col + (ordreAscendant ? " ASC" : " DESC");

    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery(queryString);

    if (model->lastError().isValid()) {
        qDebug() << "Erreur tri cours :" << model->lastError().text();
        delete model;
        return nullptr;
    }

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Titre"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Description"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Durée (h)"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Niveau"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Prix"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Date de début"));
    model->setHeaderData(7, Qt::Horizontal, QObject::tr("ID Formateur"));
    model->setHeaderData(8, Qt::Horizontal, QObject::tr("Formateur"));

    return model;
}

QSqlQueryModel* Cours::chercherEtTrier(const QString &titreCritere,
                                        const QString &niveauCritere,
                                        const QString &formateurCritere,
                                        const QString &colonneTri,
                                        bool ordreAscendant)
{
    // Utilisée par la recherche dynamique : combine les filtres (WHERE)
    // et le tri (ORDER BY) en une seule requête, recalculée à chaque
    // changement d'un champ de filtre ou de tri côté interface.
    QString col = "c.titre_cours";
    if (colonneTri == "Titre") col = "c.titre_cours";
    else if (colonneTri == "Prix") col = "c.prix";
    else if (colonneTri == "Durée") col = "c.duree_heures";
    else if (colonneTri == "Date de début") col = "c.date_debut";

    QString queryString = "SELECT c.id_cours, c.titre_cours, c.description_cours, c.duree_heures, "
                           "c.niveau, c.prix, TO_CHAR(c.date_debut,'YYYY-MM-DD'), "
                           "c.id_formateur, f.nom_formateur || ' ' || f.prenom_formateur AS formateur "
                           "FROM COURS c LEFT JOIN FORMATEURS f ON c.id_formateur = f.id_formateur "
                           "WHERE 1=1";

    if (!titreCritere.trimmed().isEmpty())
        queryString += " AND LOWER(c.titre_cours) LIKE :titre";
    if (!niveauCritere.trimmed().isEmpty())
        queryString += " AND LOWER(c.niveau) LIKE :niveau";
    if (!formateurCritere.trimmed().isEmpty())
        queryString += " AND LOWER(f.nom_formateur || ' ' || f.prenom_formateur) LIKE :formateur";

    queryString += " ORDER BY " + col + (ordreAscendant ? " ASC" : " DESC");

    QSqlQuery query;
    query.prepare(queryString);

    if (!titreCritere.trimmed().isEmpty())
        query.bindValue(":titre", "%" + titreCritere.toLower() + "%");
    if (!niveauCritere.trimmed().isEmpty())
        query.bindValue(":niveau", "%" + niveauCritere.toLower() + "%");
    if (!formateurCritere.trimmed().isEmpty())
        query.bindValue(":formateur", "%" + formateurCritere.toLower() + "%");

    QSqlQueryModel *model = new QSqlQueryModel();

    if (!query.exec()) {
        qDebug() << "Erreur recherche/tri cours :" << query.lastError().text();
        delete model;
        return nullptr;
    }

    model->setQuery(query);
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Titre"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Description"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Durée (h)"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Niveau"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Prix"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Date de début"));
    model->setHeaderData(7, Qt::Horizontal, QObject::tr("ID Formateur"));
    model->setHeaderData(8, Qt::Horizontal, QObject::tr("Formateur"));

    return model;
}

QSqlQueryModel* Cours::statistiquesParNiveau()
{
    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery("SELECT niveau, COUNT(*) AS nb FROM COURS "
                     "GROUP BY niveau ORDER BY niveau");
    return model;
}

bool Cours::verifierDisponibiliteFormateur(int idFormateur, const QString &dateDebut, int idCoursAExclure)
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM COURS WHERE id_formateur = :id_formateur "
                  "AND date_debut = TO_DATE(:date_debut, 'YYYY-MM-DD') "
                  "AND id_cours != :id_exclure");
    query.bindValue(":id_formateur", idFormateur);
    query.bindValue(":date_debut", dateDebut);
    query.bindValue(":id_exclure", idCoursAExclure);

    if (query.exec() && query.next())
        return query.value(0).toInt() == 0; // true = disponible (aucun conflit)

    return false;
}

bool Cours::dupliquerCours(int idCoursSource, const QString &nouvelleDateDebut)
{
    QSqlQuery lecture;
    lecture.prepare("SELECT titre_cours, description_cours, duree_heures, niveau, prix, id_formateur "
                    "FROM COURS WHERE id_cours = :id");
    lecture.bindValue(":id", idCoursSource);

    if (!lecture.exec() || !lecture.next()) {
        qDebug() << "Cours source introuvable pour duplication.";
        return false;
    }

    Cours nouveauCours;
    nouveauCours.setId(prochainId());
    nouveauCours.setTitre(lecture.value(0).toString());
    nouveauCours.setDescription(lecture.value(1).toString());
    nouveauCours.setDureeHeures(lecture.value(2).toInt());
    nouveauCours.setNiveau(lecture.value(3).toString());
    nouveauCours.setPrix(lecture.value(4).toDouble());
    nouveauCours.setDateDebut(nouvelleDateDebut);
    nouveauCours.setIdFormateur(lecture.value(5).toInt());

    return nouveauCours.ajouter();
}

bool Cours::joindreFichier(int idCours, const QString &nomFichier, const QByteArray &contenuFichier)
{
    QSqlQuery query;
    query.prepare("UPDATE COURS SET nom_fichier = :nom, contenu_fichier = :contenu "
                  "WHERE id_cours = :id");
    query.bindValue(":nom", nomFichier);
    query.bindValue(":contenu", contenuFichier);
    query.bindValue(":id", idCours);

    if (!query.exec()) {
        qDebug() << "Erreur jointure fichier cours :" << query.lastError().text();
        return false;
    }
    return true;
}

bool Cours::lireFichier(int idCours, QString &nomFichierOut, QByteArray &contenuFichierOut)
{
    QSqlQuery query;
    query.prepare("SELECT nom_fichier, contenu_fichier FROM COURS WHERE id_cours = :id");
    query.bindValue(":id", idCours);

    if (!query.exec() || !query.next()) {
        qDebug() << "Erreur lecture fichier cours :" << query.lastError().text();
        return false;
    }

    nomFichierOut = query.value(0).toString();
    contenuFichierOut = query.value(1).toByteArray();
    return true;
}

QSqlQueryModel* Cours::listePourCombo()
{
    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery("SELECT id_cours, titre_cours FROM COURS ORDER BY titre_cours");
    return model;
}
