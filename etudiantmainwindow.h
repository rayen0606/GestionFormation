#ifndef ETUDIANTMAINWINDOW_H
#define ETUDIANTMAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QScrollArea>
#include <QGridLayout>
#include <QLineEdit>
#include <QComboBox>
#include "cours.h"

// Fenêtre principale de l'espace étudiant, ouverte après une connexion ou
// une création de compte réussie (voir LoginWindow). Affiche le catalogue
// de tous les cours en grille ; chaque carte permet de payer, puis de lire
// ou télécharger le support une fois le paiement effectué.
class EtudiantMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit EtudiantMainWindow(int idParticipant, const QString &nomComplet, QWidget *parent = nullptr);

signals:
    void deconnexionDemandee();

private slots:
    void onPayerCours(int idCours, const QString &titre, double prix);
    void onOuvrirCours(int idCours);
    void onTelechargerCours(int idCours);
    void onFiltrerCatalogue();

private:
    QLineEdit *rechercheEdit;
    QComboBox *niveauCombo;
    QComboBox *statutCombo;
    QLabel *compteurLabel;

    QScrollArea *catalogueScroll;
    QWidget *catalogueConteneur;
    QGridLayout *catalogueGrille;

    Cours cours;
    int idParticipantConnecte;
    QString nomCompletConnecte;

    QWidget* creerEntete();
    QWidget* creerPageCatalogue();
    QWidget* creerCarteCours(int idCours, const QString &titre, const QString &niveau,
                              int duree, double prix, const QString &description, bool paye);
    void rafraichirCatalogue();
    void ouvrirOuTelechargerCours(int idCours, bool telecharger);
};

#endif // ETUDIANTMAINWINDOW_H
