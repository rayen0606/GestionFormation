#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QStackedWidget>
#include <QTableView>
#include <QLineEdit>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QRegularExpression>
#include <QTimer>
#include <QStyle>
#include <QPushButton>
#include <QEvent>
#include <QByteArray>
#include <QCalendarWidget>
#include <QDate>

#include "formateur.h"
#include "cours.h"
#include "participant.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void deconnexionDemandee();

protected:
    // Intercepte les evenements Enter/Leave des boutons/cartes pour animer
    // leur ombre portee (et une legere elevation) au survol (voir
    // styliserBouton() et ombrerCarte()).
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    // ---- Entités (les requêtes SQL vivent uniquement dans ces classes) ----
    // Inscription, Session et Facture n'ont que des méthodes statiques
    // (pas d'état à conserver entre deux opérations), donc pas d'instance ici.
    Formateur formateur;
    Cours cours;
    Participant participant;

    static const QRegularExpression emailRegex;
    static const QRegularExpression telRegex;
    static const QRegularExpression nomRegex;

    // ---- Conteneur principal : navigation centree façon segmented-control ----
    QStackedWidget *mainStack;
    QPushButton *btnNavAccueil;
    QPushButton *btnNavFormateurs;
    QPushButton *btnNavCours;
    QPushButton *btnNavParticipants;
    QPushButton *btnNavPlanning;
    QPushButton *btnNavFacturation;
    void basculerModulePrincipal(int index);

    // =======================  Onglet ACCUEIL (tableau de bord)  =======================
    QLabel *lblKpiFormateurs;
    QLabel *lblKpiCours;
    QLabel *lblKpiRevenu;
    QLabel *lblKpiCoursAVenir;
    QTableView *tableCoursAVenirDash;
    QTableView *tableNotificationsDash;

    QWidget* creerPageAccueil();
    void rafraichirDashboard();

    // Journalise une action (visible dans le fil d'activité du tableau
    // de bord) : à appeler après chaque opération CRUD réussie.
    void ajouterNotification(const QString &message);

    // =======================  Onglet FORMATEURS  =======================
    QStackedWidget *fStack;

    // Page Liste
    QLineEdit *fNomEdit, *fPrenomEdit, *fEmailEdit, *fTelEdit, *fSpecialiteEdit;
    QDateEdit *fDateEmbaucheEdit;
    QDoubleSpinBox *fTarifSpin;
    QLineEdit *fRechNom, *fRechEmail;
    QComboBox *fRechSpecialiteCombo;
    QTimer *fRechTimer;
    QComboBox *fTriCombo;
    QCheckBox *fOrdreAscCheck;
    QTableView *fTableView;
    QLabel *fChargeLabel;
    QLabel *fSelectionLabel;
    int fSelectedFormateurId = -1;

    // Page Modifier
    QLineEdit *fModId, *fModNom, *fModPrenom, *fModEmail, *fModTel, *fModSpecialite;
    QDateEdit *fModDate;
    QDoubleSpinBox *fModTarif;

    // Page Statistiques
    QWidget *fStatsPage;

    QWidget* creerOngletFormateurs();
    QWidget* creerPageListeFormateurs();
    QWidget* creerPageAjouterFormateur();
    QWidget* creerPageModifierFormateur();
    QWidget* creerPageStatsFormateurs();
    void rafraichirListeFormateurs();
    void rafraichirStatsFormateurs();
    void remplirFiltreSpecialites();

    // =======================  Onglet COURS  =======================
    QStackedWidget *cStack;

    // Page Liste
    QLineEdit *cTitreEdit, *cDescriptionEdit;
    QSpinBox *cDureeSpin;
    QComboBox *cNiveauCombo;
    QDoubleSpinBox *cPrixSpin;
    QDateEdit *cDateDebutEdit;
    QComboBox *cFormateurCombo;
    QLabel *cAjoutFichierLabel;        // affiche le nom du fichier choisi lors de l'ajout
    QString cAjoutFichierNom;          // nom du fichier en attente d'enregistrement
    QByteArray cAjoutFichierContenu;   // contenu binaire en attente d'enregistrement
    QLineEdit *cRechTitre;
    QComboBox *cRechNiveauCombo, *cRechFormateurCombo;
    QTimer *cRechTimer;
    QComboBox *cTriCombo;
    QCheckBox *cOrdreAscCheck;
    QTableView *cTableView;
    QLabel *cSelectionLabel;
    QPushButton *btnLireFichierCours; // visible uniquement quand un cours est sélectionné
    int cSelectedCoursId = -1;

    // Page Modifier
    QLineEdit *cModId, *cModTitre, *cModDescription;
    QSpinBox *cModDuree;
    QComboBox *cModNiveau;
    QDoubleSpinBox *cModPrix;
    QDateEdit *cModDateDebut;
    QComboBox *cModFormateur;
    QLabel *cModFichierLabel;          // affiche le fichier actuellement joint
    QString cModFichierNom;            // nouveau fichier en attente (vide = pas de changement)
    QByteArray cModFichierContenu;

    // Page Statistiques
    QWidget *cStatsPage;

    QWidget* creerOngletCours();
    QWidget* creerPageListeCours();
    QWidget* creerPageAjouterCours();
    QWidget* creerPageModifierCours();
    QWidget* creerPageStatsCours();
    void rafraichirListeCours();
    void rafraichirStatsCours();
    void remplirComboFormateurs(QComboBox *combo);
    void remplirFiltreFormateurs(QComboBox *combo);

    // =======================  Onglet PARTICIPANTS / INSCRIPTIONS  =======================
    QStackedWidget *pStack;

    // Page Liste des participants
    QLineEdit *pRechTexte;
    QComboBox *pTriCombo;
    QCheckBox *pOrdreAscCheck;
    QTimer *pRechTimer;
    QTableView *pTableView;
    QLabel *pSelectionLabel;
    int pSelectedParticipantId = -1;

    // Page Ajouter
    QLineEdit *pNomEdit, *pPrenomEdit, *pEmailEdit, *pTelEdit;
    QDateEdit *pDateNaissanceEdit;
    QCheckBox *pDateNaissanceRenseigneeCheck;

    // Page Modifier
    QLineEdit *pModId, *pModNom, *pModPrenom, *pModEmail, *pModTel;
    QDateEdit *pModDateNaissance;
    QCheckBox *pModDateNaissanceRenseigneeCheck;

    // Page Inscriptions
    QComboBox *iParticipantCombo, *iCoursCombo;
    QLineEdit *iRechTexte;
    QComboBox *iStatutCombo;
    QTimer *iRechTimer;
    QTableView *iTableView;
    QLabel *iSelectionLabel;
    int iSelectedInscriptionId = -1;

    QWidget* creerOngletParticipants();
    QWidget* creerPageListeParticipants();
    QWidget* creerPageAjouterParticipant();
    QWidget* creerPageModifierParticipant();
    QWidget* creerPageInscriptions();
    void rafraichirListeParticipants();
    void rafraichirInscriptions();
    void remplirComboParticipants(QComboBox *combo);
    void remplirComboCoursPourInscription(QComboBox *combo);

    // =======================  Onglet PLANNING (calendrier des séances)  =======================
    QCalendarWidget *planCalendrier;
    QComboBox *planCoursCombo;
    QLineEdit *planHeureDebutEdit, *planHeureFinEdit, *planSalleEdit;
    QSpinBox *planCapaciteSpin;
    QTableView *planSeancesJourTable;
    QTableView *planToutesSeancesTable;
    QLabel *planJourLabel;
    int planSelectedSessionId = -1;

    QWidget* creerOngletPlanning();
    void rafraichirPlanning();
    void rafraichirSeancesDuJour();
    void surlignerJoursAvecSeances();

    // =======================  Onglet FACTURATION  =======================
    QLabel *lblKpiFactureTotal, *lblKpiFactureEncaisse, *lblKpiFactureImpaye;
    QComboBox *factInscriptionCombo;
    QLineEdit *factRechTexte;
    QComboBox *factStatutCombo;
    QTimer *factRechTimer;
    QTableView *factTableView;
    QLabel *factSelectionLabel;
    int factSelectedFactureId = -1;

    QWidget* creerOngletFacturation();
    void rafraichirFacturation();

    // ---- Export PDF (document personnalisé, pas de capture d'écran) ----
    void exporterFormateursPDF();
    void exporterCoursPDF();

    // ---- Import/Export CSV (fonctionnalité avancée) ----
    void exporterFormateursCSV();
    void importerFormateursCSV();
    void exporterCoursCSV();
    void importerCoursCSV();

    // ---- Design : icônes de boutons + transitions animées entre pages ----
    void styliserBouton(QPushButton *bouton, const QString &classe, QStyle::StandardPixmap icone);
    void animerApparition(QWidget *page);
    void basculerAvecAnimation(QStackedWidget *stack, int index);
    void ombrerCarte(QWidget *carte, bool interactive = false);
    void ajusterColonnesTable(QTableView *table);

    // Anime un QLabel numerique de sa valeur actuelle vers `cible` (effet
    // "compteur qui defile", tres utilise dans les tableaux de bord pro).
    // `suffixe` permet de reformatter (ex: 2 decimales pour un montant).
    void animerCompteur(QLabel *label, double cible, int decimales = 0);

    // Construit le bandeau d'en-tete avec son fond anime (shader QML/GLSL)
    // en fallback gracieux vers un simple widget si QtQuick est indisponible.
    QWidget* creerBandeauEnTete();

private slots:
    // Formateurs
    void onAjouterFormateur();
    void onSupprimerFormateur();
    void onFiltrerFormateurs(); // recherche + tri dynamiques (filtres live)
    void onFormateurSelectionne(const QModelIndex &index);
    void onOuvrirModifierFormateur();
    void onEnregistrerModifierFormateur();
    void onAnnulerModifierFormateur();
    void onAfficherStatsFormateurs();
    void onCalculerChargeHoraire();
    void onContacterFormateur();

    // Cours
    void onAjouterCours();
    void onSupprimerCours();
    void onFiltrerCours(); // recherche + tri dynamiques (filtres live)
    void onCoursSelectionne(const QModelIndex &index);
    void onOuvrirModifierCours();
    void onEnregistrerModifierCours();
    void onAnnulerModifierCours();
    void onAfficherStatsCours();
    void onDupliquerCours();
    void onChoisirFichierAjoutCours();
    void onChoisirFichierModifierCours();
    void onLireFichierCours();
    void onGenererCertificat();

    // Participants
    void onAjouterParticipant();
    void onSupprimerParticipant();
    void onFiltrerParticipants();
    void onParticipantSelectionne(const QModelIndex &index);
    void onOuvrirModifierParticipant();
    void onEnregistrerModifierParticipant();
    void onAnnulerModifierParticipant();

    // Inscriptions
    void onInscrireParticipant();
    void onFiltrerInscriptions();
    void onInscriptionSelectionnee(const QModelIndex &index);
    void onConfirmerInscription();
    void onAnnulerInscriptionStatut();
    void onSupprimerInscription();

    // Planning
    void onDateCalendrierSelectionnee();
    void onAjouterSession();
    void onSessionJourSelectionnee(const QModelIndex &index);
    void onSupprimerSession();

    // Facturation
    void onGenererFacture();
    void onFiltrerFactures();
    void onFactureSelectionnee(const QModelIndex &index);
    void onMarquerFecturePayee();
    void onAnnulerFacture();
    void onSupprimerFacture();
};

#endif // MAINWINDOW_H
