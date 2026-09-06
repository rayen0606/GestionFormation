#ifndef ESPACEETUDIANTDIALOG_H
#define ESPACEETUDIANTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QStackedWidget>
#include <QScrollArea>
#include <QGridLayout>
#include "cours.h"

// Fenêtre dédiée aux étudiants (indépendante de l'espace d'administration) :
//  - création de compte / connexion (email + mot de passe)
//  - catalogue de tous les cours affiché en grille de cartes
//  - paiement direct depuis une carte, qui débloque aussitôt la lecture et
//    le téléchargement du support de cours.
class EspaceEtudiantDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EspaceEtudiantDialog(QWidget *parent = nullptr);

private slots:
    void onSeConnecter();
    void onCreerCompte();
    void onSeDeconnecter();
    void onPayerCours(int idCours, const QString &titre, double prix);
    void onOuvrirCours(int idCours);
    void onTelechargerCours(int idCours);

private:
    QStackedWidget *stack;      // 0 = authentification, 1 = catalogue
    QStackedWidget *authStack;  // 0 = connexion, 1 = inscription

    // Connexion
    QLineEdit *loginEmailEdit, *loginMotDePasseEdit;
    QLabel *loginErreurLabel;

    // Inscription (création de compte)
    QLineEdit *regNomEdit, *regPrenomEdit, *regEmailEdit, *regTelEdit;
    QLineEdit *regMotDePasseEdit, *regMotDePasseConfirmEdit;
    QLabel *regErreurLabel;

    // Catalogue
    QLabel *bienvenueLabel;
    QScrollArea *catalogueScroll;
    QWidget *catalogueConteneur;
    QGridLayout *catalogueGrille;

    Cours cours;
    int idParticipantConnecte = -1;

    QWidget* creerPageAuthentification();
    QWidget* creerPageConnexion();
    QWidget* creerPageInscription();
    QWidget* creerPageCatalogue();
    QWidget* creerCarteCours(int idCours, const QString &titre, const QString &niveau,
                              int duree, double prix, const QString &description, bool paye);
    void rafraichirCatalogue();
    void ouvrirOuTelechargerCours(int idCours, bool telecharger);
};

#endif // ESPACEETUDIANTDIALOG_H
