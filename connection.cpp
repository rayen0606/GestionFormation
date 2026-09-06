#include "connection.h"
#include <QDebug>

Connection::Connection()
{
}

bool Connection::createconnect()
{
    bool test = false;

    // Driver ODBC (identique au projet MaPharma-GestionClients).
    // Nécessite un DSN ODBC pointant vers Oracle, configuré au préalable
    // dans l'outil "Sources de données ODBC" (odbcad32 sous Windows) ou
    // via odbc.ini / odbcinst.ini sous Linux.
    db = QSqlDatabase::addDatabase("QODBC");

    // ---- A ADAPTER selon votre environnement ----
    const QString dsn  = "GestionFormation"; // nom du DSN ODBC créé
    const QString user = "azer";
    const QString pass = "azer";

    db.setDatabaseName(dsn);
    db.setUserName(user);
    db.setPassword(pass);

    if (db.open())
        test = true;
    else
        qDebug() << "Erreur de connexion Oracle (ODBC) :" << db.lastError().text();

    return test;
}

void Connection::closeConnection()
{
    db.close();
}
