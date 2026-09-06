#include "etudiantmainwindow.h"
#include "portailetudiant.h"
#include "uiutils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QTextEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QFileDialog>
#include <QSqlQueryModel>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QTimer>

EtudiantMainWindow::EtudiantMainWindow(int idParticipant, const QString &nomComplet, QWidget *parent)
    : QMainWindow(parent)
    , idParticipantConnecte(idParticipant)
    , nomCompletConnecte(nomComplet)
{
    setWindowTitle(tr("Centre de Formation — Espace Étudiant"));
    resize(1180, 720);
    setStyleSheet("QMainWindow { background:#F1F5F9; }");

    QWidget *central = new QWidget();
    QVBoxLayout *layoutCentral = new QVBoxLayout(central);
    layoutCentral->setContentsMargins(0, 0, 0, 0);
    layoutCentral->setSpacing(0);

    layoutCentral->addWidget(creerEntete());
    layoutCentral->addWidget(creerPageCatalogue(), 1);

    setCentralWidget(central);
    rafraichirCatalogue();
}

QWidget* EtudiantMainWindow::creerEntete()
{
    QFrame *entete = new QFrame();
    entete->setObjectName("entete");
    entete->setAttribute(Qt::WA_StyledBackground, true);
    entete->setStyleSheet(
        "#entete { background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "  stop:0 #1E3A5F, stop:1 #2F6FED); }");
    entete->setMinimumHeight(150);

    QVBoxLayout *layout = new QVBoxLayout(entete);
    layout->setContentsMargins(36, 24, 36, 24);
    layout->setSpacing(16);

    QHBoxLayout *ligneHaut = new QHBoxLayout();
    QVBoxLayout *colTitres = new QVBoxLayout();
    QLabel *titre = new QLabel(tr("🎓 Catalogue des cours"));
    titre->setStyleSheet("color:white; font-size:19pt; font-weight:800;");
    QLabel *bienvenue = new QLabel(tr("Bonjour, %1 — parcourez, payez, apprenez.").arg(nomCompletConnecte));
    bienvenue->setStyleSheet("color:#CBDCF5; font-size:10.5pt;");
    colTitres->addWidget(titre);
    colTitres->addWidget(bienvenue);
    ligneHaut->addLayout(colTitres);
    ligneHaut->addStretch();

    QPushButton *btnDeconnexion = new QPushButton(tr("Se déconnecter"));
    btnDeconnexion->setCursor(Qt::PointingHandCursor);
    btnDeconnexion->setMinimumHeight(36);
    btnDeconnexion->setStyleSheet(
        "QPushButton { background:rgba(255,255,255,0.12); color:white; border:1px solid rgba(255,255,255,0.4); "
        "  border-radius:8px; padding:6px 16px; font-weight:600; }"
        "QPushButton:hover { background:rgba(255,255,255,0.22); }");
    connect(btnDeconnexion, &QPushButton::clicked, this, [this]() { emit deconnexionDemandee(); });
    ligneHaut->addWidget(btnDeconnexion, 0, Qt::AlignTop);
    layout->addLayout(ligneHaut);

    // ---- Barre de recherche / filtres, flottante sur la bannière ----
    QFrame *barre = new QFrame();
    barre->setObjectName("barreFiltre");
    barre->setAttribute(Qt::WA_StyledBackground, true);
    barre->setStyleSheet("#barreFiltre { background:white; border-radius:12px; }");
    UiUtils::appliquerOmbre(barre, 26, 8, 40);
    QHBoxLayout *barreLayout = new QHBoxLayout(barre);
    barreLayout->setContentsMargins(16, 10, 16, 10);
    barreLayout->setSpacing(10);

    rechercheEdit = new QLineEdit();
    rechercheEdit->setPlaceholderText(tr("🔍  Rechercher un cours..."));
    rechercheEdit->setStyleSheet(
        "QLineEdit { border:1px solid #E2E8F0; border-radius:8px; padding:8px 12px; background:#F8FAFC; }"
        "QLineEdit:focus { border:1.5px solid #2F6FED; background:white; }");
    connect(rechercheEdit, &QLineEdit::textChanged, this, &EtudiantMainWindow::onFiltrerCatalogue);
    barreLayout->addWidget(rechercheEdit, 1);

    niveauCombo = new QComboBox();
    niveauCombo->addItems({tr("Tous niveaux"), tr("Debutant"), tr("Intermediaire"), tr("Avance")});
    connect(niveauCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &EtudiantMainWindow::onFiltrerCatalogue);
    barreLayout->addWidget(niveauCombo);

    statutCombo = new QComboBox();
    statutCombo->addItems({tr("Tous les cours"), tr("Déjà payés"), tr("À débloquer")});
    connect(statutCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &EtudiantMainWindow::onFiltrerCatalogue);
    barreLayout->addWidget(statutCombo);

    for (QComboBox *c : {niveauCombo, statutCombo}) {
        c->setStyleSheet(
            "QComboBox { border:1px solid #E2E8F0; border-radius:8px; padding:7px 10px; background:#F8FAFC; min-width:130px; }"
            "QComboBox:focus { border:1.5px solid #2F6FED; }");
    }

    layout->addWidget(barre);
    return entete;
}

QWidget* EtudiantMainWindow::creerPageCatalogue()
{
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(36, 20, 36, 24);

    compteurLabel = new QLabel();
    compteurLabel->setStyleSheet("color:#6B7A90; font-size:9.5pt; font-weight:600;");
    layout->addWidget(compteurLabel);

    catalogueScroll = new QScrollArea();
    catalogueScroll->setWidgetResizable(true);
    catalogueScroll->setFrameShape(QFrame::NoFrame);
    catalogueScroll->setStyleSheet("QScrollArea { background:transparent; }");

    catalogueConteneur = new QWidget();
    catalogueConteneur->setStyleSheet("background:transparent;");
    catalogueGrille = new QGridLayout(catalogueConteneur);
    catalogueGrille->setSpacing(20);
    catalogueScroll->setWidget(catalogueConteneur);

    layout->addWidget(catalogueScroll);
    return page;
}

QWidget* EtudiantMainWindow::creerCarteCours(int idCours, const QString &titre, const QString &niveau,
                                              int duree, double prix, const QString &description, bool paye)
{
    QFrame *carte = new QFrame();
    carte->setObjectName("carteCours");
    // Sans cet attribut, un QFrame stylé en QSS (fond, coins arrondis) ne
    // peint pas toujours correctement son arrière-plan selon la plateforme
    // : la carte peut alors sembler terne, transparente ou peu lisible.
    carte->setAttribute(Qt::WA_StyledBackground, true);
    carte->setMinimumSize(270, 250);
    carte->setMaximumWidth(320);
    carte->setStyleSheet(QString(
        "#carteCours { background:#FFFFFF; border:1px solid %1; border-radius:14px; }")
        .arg(paye ? "#BBE5CC" : "#E5EAF0"));

    QVBoxLayout *layout = new QVBoxLayout(carte);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(10);

    QHBoxLayout *ligneBadge = new QHBoxLayout();
    ligneBadge->addWidget(UiUtils::creerBadgeNiveau(niveau));
    ligneBadge->addStretch();
    QLabel *dureeLabel = new QLabel(tr("⏱ %1 h").arg(duree));
    dureeLabel->setStyleSheet("color:#5B6B7F; font-size:8.5pt; font-weight:600; background:transparent;");
    ligneBadge->addWidget(dureeLabel);
    layout->addLayout(ligneBadge);

    QLabel *titreLabel = new QLabel(titre);
    titreLabel->setWordWrap(true);
    titreLabel->setStyleSheet("font-size:13pt; font-weight:800; color:#0F2A47; background:transparent;");
    layout->addWidget(titreLabel);

    QLabel *descLabel = new QLabel(description.isEmpty() ? tr("Aucune description.") : description);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("color:#475569; font-size:9.5pt; background:transparent;");
    descLabel->setMinimumHeight(48);
    descLabel->setMaximumHeight(64);
    descLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    layout->addWidget(descLabel, 1);

    QFrame *separateur = new QFrame();
    separateur->setFrameShape(QFrame::HLine);
    separateur->setStyleSheet("background:#EEF2F6; max-height:1px; border:none;");
    layout->addWidget(separateur);

    QHBoxLayout *lignePrix = new QHBoxLayout();
    QLabel *prixLabel = new QLabel(tr("%1 DT").arg(QString::number(prix, 'f', 2)));
    prixLabel->setStyleSheet("font-size:14pt; font-weight:800; color:#0F2A47; background:transparent;");
    lignePrix->addWidget(prixLabel);
    lignePrix->addStretch();
    if (paye) {
        QLabel *badgePaye = new QLabel(tr("✔ Payé"));
        badgePaye->setStyleSheet("color:#15803D; font-weight:700; font-size:9pt; background:transparent;");
        lignePrix->addWidget(badgePaye);
    }
    layout->addLayout(lignePrix);

    if (paye) {
        QHBoxLayout *actions = new QHBoxLayout();
        actions->setSpacing(8);
        QPushButton *btnLire = new QPushButton(tr("📖 Lire"));
        QPushButton *btnTelecharger = new QPushButton(tr("⬇"));
        btnLire->setCursor(Qt::PointingHandCursor);
        btnTelecharger->setCursor(Qt::PointingHandCursor);
        btnLire->setMinimumHeight(38);
        btnTelecharger->setMinimumHeight(38);
        btnTelecharger->setMaximumWidth(46);
        btnLire->setStyleSheet(
            "QPushButton { background:#2F6FED; color:white; border-radius:8px; font-weight:700; font-size:9.5pt; }"
            "QPushButton:hover { background:#255BC7; }"
            "QPushButton:pressed { background:#1D4CA6; }");
        btnTelecharger->setStyleSheet(
            "QPushButton { border:1.5px solid #2F6FED; color:#2F6FED; border-radius:8px; font-weight:700; background:white; }"
            "QPushButton:hover { background:#EEF4FF; }"
            "QPushButton:pressed { background:#DCE9FF; }");
        connect(btnLire, &QPushButton::clicked, this, [this, idCours]() { onOuvrirCours(idCours); });
        connect(btnTelecharger, &QPushButton::clicked, this, [this, idCours]() { onTelechargerCours(idCours); });
        actions->addWidget(btnLire, 1);
        actions->addWidget(btnTelecharger);
        layout->addLayout(actions);
    } else {
        QPushButton *btnPayer = new QPushButton(tr("🔒 Payer et débloquer"));
        btnPayer->setCursor(Qt::PointingHandCursor);
        btnPayer->setMinimumHeight(40);
        btnPayer->setStyleSheet(
            "QPushButton { background:#1E3A5F; color:white; border-radius:8px; font-weight:700; font-size:9.5pt; }"
            "QPushButton:hover { background:#2A4E7C; }"
            "QPushButton:pressed { background:#16304F; }");
        connect(btnPayer, &QPushButton::clicked, this, [this, idCours, titre, prix]() { onPayerCours(idCours, titre, prix); });
        layout->addWidget(btnPayer);
    }

    return carte;
}

void EtudiantMainWindow::rafraichirCatalogue()
{
    QLayoutItem *item;
    while ((item = catalogueGrille->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        delete item;
    }

    QSqlQueryModel *model = PortailEtudiant::catalogueCours(idParticipantConnecte);
    QString texteRecherche = rechercheEdit ? rechercheEdit->text().trimmed().toLower() : QString();
    QString niveauFiltre = niveauCombo ? niveauCombo->currentText() : tr("Tous niveaux");
    QString statutFiltre = statutCombo ? statutCombo->currentText() : tr("Tous les cours");

    const int colonnes = 3;
    int position = 0;
    for (int i = 0; i < model->rowCount(); ++i) {
        int idCours = model->data(model->index(i, 0)).toInt();
        QString titre = model->data(model->index(i, 1)).toString();
        QString niveau = model->data(model->index(i, 2)).toString();
        int duree = model->data(model->index(i, 3)).toInt();
        double prix = model->data(model->index(i, 4)).toDouble();
        QString description = model->data(model->index(i, 5)).toString();
        bool paye = model->data(model->index(i, 6)).toInt() == 1;

        if (!texteRecherche.isEmpty() && !titre.toLower().contains(texteRecherche)
            && !description.toLower().contains(texteRecherche))
            continue;
        if (niveauFiltre != tr("Tous niveaux") && niveau.compare(niveauFiltre, Qt::CaseInsensitive) != 0)
            continue;
        if (statutFiltre == tr("Déjà payés") && !paye) continue;
        if (statutFiltre == tr("À débloquer") && paye) continue;

        QWidget *carte = creerCarteCours(idCours, titre, niveau, duree, prix, description, paye);
        catalogueGrille->addWidget(carte, position / colonnes, position % colonnes);
        UiUtils::animerEntree(carte, (position % colonnes) * 60 + (position / colonnes) * 40, 260,
            [carte]() {
                // L'ombre n'est appliquée qu'une fois le fondu terminé,
                // jamais en même temps que l'effet d'opacité.
                UiUtils::appliquerOmbre(carte, 22, 6, 28);
            });
        ++position;
    }

    compteurLabel->setText(position == 0
        ? tr("Aucun cours ne correspond à votre recherche.")
        : tr("%1 cours disponible(s)").arg(position));

    if (position == 0) {
        QLabel *vide = new QLabel(tr("Aucun cours trouvé. Essayez d'autres critères."));
        vide->setStyleSheet("color:#9AA6B5; font-size:10pt; padding:40px;");
        vide->setAlignment(Qt::AlignCenter);
        catalogueGrille->addWidget(vide, 0, 0, 1, colonnes);
    }
}

void EtudiantMainWindow::onFiltrerCatalogue()
{
    rafraichirCatalogue();
}

void EtudiantMainWindow::onPayerCours(int idCours, const QString &titre, double prix)
{
    QDialog dialogue(this);
    dialogue.setWindowTitle(tr("Paiement"));
    dialogue.setMinimumWidth(380);
    dialogue.setStyleSheet(
        "QDialog { background:white; }"
        "QComboBox { border:1px solid #DCE3EA; border-radius:8px; padding:7px; }");

    QVBoxLayout *layout = new QVBoxLayout(&dialogue);
    layout->setContentsMargins(24, 24, 24, 20);

    QLabel *titreDialogue = new QLabel(tr("💳 Confirmer le paiement"));
    titreDialogue->setStyleSheet("font-size:13pt; font-weight:800; color:#1E3A5F;");
    layout->addWidget(titreDialogue);

    QLabel *recap = new QLabel(tr("<b>%1</b><br><span style='color:#6B7A90;'>Montant à régler</span><br>"
                                   "<span style='font-size:16pt; font-weight:800; color:#1E3A5F;'>%2 DT</span>")
        .arg(titre, QString::number(prix, 'f', 2)));
    recap->setStyleSheet("padding:14px; background:#F8FAFC; border-radius:10px;");
    layout->addWidget(recap);

    QFormLayout *form = new QFormLayout();
    QComboBox *modeCombo = new QComboBox();
    modeCombo->addItems({tr("Carte bancaire"), tr("Virement"), tr("Espèces"), tr("Chèque")});
    form->addRow(tr("Mode de paiement :"), modeCombo);
    layout->addLayout(form);

    QDialogButtonBox *boutons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    boutons->button(QDialogButtonBox::Ok)->setText(tr("Confirmer le paiement"));
    boutons->button(QDialogButtonBox::Ok)->setStyleSheet(
        "background:#1E3A5F; color:white; border-radius:8px; padding:8px 16px; font-weight:700;");
    boutons->button(QDialogButtonBox::Cancel)->setStyleSheet("border-radius:8px; padding:8px 16px;");
    connect(boutons, &QDialogButtonBox::accepted, &dialogue, &QDialog::accept);
    connect(boutons, &QDialogButtonBox::rejected, &dialogue, &QDialog::reject);
    layout->addWidget(boutons);

    if (dialogue.exec() != QDialog::Accepted) return;

    QString erreur = PortailEtudiant::payerCours(idParticipantConnecte, idCours, modeCombo->currentText());
    if (!erreur.isEmpty()) {
        QMessageBox::warning(this, tr("Paiement impossible"), erreur);
        return;
    }

    QMessageBox::information(this, tr("Paiement réussi"),
        tr("Le paiement a été effectué. Vous pouvez maintenant lire et télécharger ce cours."));
    rafraichirCatalogue();
}

void EtudiantMainWindow::onOuvrirCours(int idCours)
{
    ouvrirOuTelechargerCours(idCours, false);
}

void EtudiantMainWindow::onTelechargerCours(int idCours)
{
    ouvrirOuTelechargerCours(idCours, true);
}

void EtudiantMainWindow::ouvrirOuTelechargerCours(int idCours, bool telecharger)
{
    if (!PortailEtudiant::aPaye(idParticipantConnecte, idCours)) {
        QMessageBox::warning(this, tr("Accès refusé"), tr("Vous devez d'abord payer ce cours."));
        return;
    }

    QString nomFichier; QByteArray contenu;
    if (!cours.lireFichier(idCours, nomFichier, contenu)) {
        QMessageBox::critical(this, tr("Erreur"), tr("Impossible de récupérer le support de cours."));
        return;
    }
    if (nomFichier.isEmpty() || contenu.isEmpty()) {
        QMessageBox::information(this, tr("Aucun support"),
            tr("Aucun support n'a encore été mis en ligne pour ce cours."));
        return;
    }

    if (telecharger) {
        QString cheminDestination = QFileDialog::getSaveFileName(this, tr("Enregistrer le support de cours"), nomFichier);
        if (cheminDestination.isEmpty()) return;

        QFile fichier(cheminDestination);
        if (!fichier.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, tr("Erreur"), tr("Impossible d'enregistrer le fichier."));
            return;
        }
        fichier.write(contenu);
        fichier.close();
        QMessageBox::information(this, tr("Téléchargement terminé"), tr("Le fichier a été enregistré avec succès."));
        return;
    }

    static const QStringList extensionsTexte = {"txt", "csv", "md", "log"};
    QString extension = QFileInfo(nomFichier).suffix().toLower();

    if (extensionsTexte.contains(extension)) {
        QDialog dialogue(this);
        dialogue.setWindowTitle(tr("Lecture : %1").arg(nomFichier));
        dialogue.resize(600, 500);

        QVBoxLayout *layout = new QVBoxLayout(&dialogue);
        QLabel *titreLabel = new QLabel(tr("<b>%1</b>").arg(nomFichier));
        layout->addWidget(titreLabel);

        QTextEdit *zoneTexte = new QTextEdit();
        zoneTexte->setPlainText(QString::fromUtf8(contenu));
        zoneTexte->setReadOnly(true);
        layout->addWidget(zoneTexte);

        QDialogButtonBox *boutons = new QDialogButtonBox(QDialogButtonBox::Ok);
        connect(boutons, &QDialogButtonBox::accepted, &dialogue, &QDialog::accept);
        layout->addWidget(boutons);

        dialogue.exec();
    } else {
        QString dossierTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        QString cheminTemp = QDir(dossierTemp).filePath(nomFichier);

        QFile fichierTemp(cheminTemp);
        if (!fichierTemp.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, tr("Erreur"), tr("Impossible de préparer le fichier pour la lecture."));
            return;
        }
        fichierTemp.write(contenu);
        fichierTemp.close();

        QDesktopServices::openUrl(QUrl::fromLocalFile(cheminTemp));
    }
}
