#include "mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QDir>
#include <QUrl>
#include <QInputDialog>
#include <QSqlQuery>
#include "notification.h"
#include "certificat.h"
#include "participant.h"
#include "inscription.h"
#include "session.h"
#include "facture.h"
#include <QTextCharFormat>
#include <QDialog>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QPainter>
#include <QPrinter>
#include <QPageLayout>
#include <QDate>
#include <QSqlQueryModel>
#include <QGraphicsOpacityEffect>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QParallelAnimationGroup>
#include <QVariantAnimation>
#include <QEasingCurve>
#include <QButtonGroup>
#include <QQuickWidget>
#include <QQuickItem>
#include <QQmlError>
#include <QLocale>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>

QT_USE_NAMESPACE
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QT_CHARTS_USE_NAMESPACE
#endif

const QRegularExpression MainWindow::emailRegex("[\\w\\.-]+@[\\w\\.-]+\\.[a-z]{2,4}");
const QRegularExpression MainWindow::telRegex("^\\d{8}$");
const QRegularExpression MainWindow::nomRegex("^[a-zA-ZÀ-ÿ\\s-]+$");

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Gestion d'un Centre de Formation");
    resize(1200, 780);

    auto *conteneur = new QWidget(this);
    auto *layoutPrincipal = new QVBoxLayout(conteneur);
    layoutPrincipal->setContentsMargins(0, 0, 0, 0);
    layoutPrincipal->setSpacing(0);

    // ---------- Bandeau d'en-tete (fond shader + navigation) ----------
    QWidget *bandeau = creerBandeauEnTete();

    // ---------- Zone de contenu (une page par module) ----------
    mainStack = new QStackedWidget();
    mainStack->addWidget(creerPageAccueil());        // index 0
    mainStack->addWidget(creerOngletFormateurs());   // index 1
    mainStack->addWidget(creerOngletCours());        // index 2
    mainStack->addWidget(creerOngletParticipants()); // index 3
    mainStack->addWidget(creerOngletPlanning());     // index 4
    mainStack->addWidget(creerOngletFacturation());  // index 5

    connect(btnNavAccueil, &QPushButton::clicked, this, [this]() { basculerModulePrincipal(0); });
    connect(btnNavFormateurs, &QPushButton::clicked, this, [this]() { basculerModulePrincipal(1); });
    connect(btnNavCours, &QPushButton::clicked, this, [this]() { basculerModulePrincipal(2); });
    connect(btnNavParticipants, &QPushButton::clicked, this, [this]() { basculerModulePrincipal(3); });
    connect(btnNavPlanning, &QPushButton::clicked, this, [this]() { basculerModulePrincipal(4); });
    connect(btnNavFacturation, &QPushButton::clicked, this, [this]() { basculerModulePrincipal(5); });

    layoutPrincipal->addWidget(bandeau);
    layoutPrincipal->addWidget(mainStack, 1);
    setCentralWidget(conteneur);

    rafraichirListeFormateurs();
    rafraichirListeCours();
    rafraichirListeParticipants();
    rafraichirInscriptions();
    rafraichirPlanning();
    rafraichirFacturation();
    rafraichirDashboard();
}

// Construit le bandeau d'en-tete en DEUX zones empilees verticalement
// (jamais superposees) :
//   1. Une bannière animee (shader GLSL) qui contient elle-meme, en QML,
//      le titre de l'application.
//   2. Une zone de navigation classique (widgets Qt normaux) avec le
//      segmented control Accueil / Formateurs / Cours.
//
// Important : on n'affiche JAMAIS de QPushButton (ou tout autre widget
// classique) par-dessus le QQuickWidget. Qt ne garantit pas la
// composition correcte de widgets standards superposes a un QQuickWidget
// (voir la documentation de QQuickWidget) — une premiere version de ce
// bandeau superposait la navigation au shader via un QStackedLayout, ce
// qui rendait les boutons invisibles sur certaines configurations
// (notamment le backend Direct3D utilise par defaut sous Windows/Qt6).
// D'ou cette zone de navigation separee, toujours rendue normalement.
//
// Si QtQuick n'est pas disponible sur la machine cible (module QML
// manquant), on bascule silencieusement sur un simple bandeau de titre
// uni : l'appli reste pleinement fonctionnelle, seul l'effet visuel est
// absent.
QWidget* MainWindow::creerBandeauEnTete()
{
    auto *bandeau = new QWidget();
    bandeau->setObjectName("headerBar");

    auto *layoutBandeau = new QVBoxLayout(bandeau);
    layoutBandeau->setContentsMargins(0, 0, 0, 0);
    layoutBandeau->setSpacing(0);

    // ---- Zone 1 : banniere animee (shader) + titre ----
    auto *quick = new QQuickWidget();
    quick->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quick->setAttribute(Qt::WA_AlwaysStackOnTop, false);
    quick->setAttribute(Qt::WA_TranslucentBackground);
    quick->setClearColor(Qt::transparent);
    quick->setSource(QUrl(QStringLiteral("qrc:/resources/shaders/HeaderBackground.qml")));

    bool quickOk = quick->status() == QQuickWidget::Ready;
    if (quickOk) {
        // Le titre est un Text QML dans la meme scene que le shader (voir
        // HeaderBackground.qml) : aucun widget classique ne recouvre donc
        // le QQuickWidget.
        if (QQuickItem *racine = quick->rootObject()) {
            racine->setProperty("titre", tr("Centre de Formation"));
        }
        quick->setMinimumHeight(76);
        quick->setMaximumHeight(76);
        layoutBandeau->addWidget(quick);
    } else {
        for (const QQmlError &erreur : quick->errors())
            qWarning() << "Fond anime du bandeau indisponible :" << erreur.toString();
        delete quick;

        // Repli : bandeau statique classique, meme contenu, aucune animation.
        auto *secours = new QWidget();
        secours->setObjectName("headerContentSecours");
        auto *layoutSecours = new QVBoxLayout(secours);
        layoutSecours->setContentsMargins(0, 12, 0, 8);
        layoutSecours->setSpacing(6);

        auto *titreApp = new QLabel(tr("Centre de Formation"));
        titreApp->setObjectName("titreApplication");
        titreApp->setAlignment(Qt::AlignCenter);
        layoutSecours->addWidget(titreApp);

        layoutBandeau->addWidget(secours);
    }

    // ---- Zone 2 : navigation (widgets classiques, jamais superposes) ----
    auto *zoneNav = new QWidget();
    zoneNav->setObjectName("headerNav");
    auto *layoutNav = new QVBoxLayout(zoneNav);
    layoutNav->setContentsMargins(0, 10, 0, 10);
    layoutNav->setSpacing(0);

    auto *ligneNav = new QHBoxLayout();
    ligneNav->addStretch();

    btnNavAccueil = new QPushButton(tr("  Accueil"));
    btnNavFormateurs = new QPushButton(tr("  Gestion des Formateurs"));
    btnNavCours = new QPushButton(tr("  Gestion des Cours"));
    btnNavParticipants = new QPushButton(tr("  Participants"));
    btnNavPlanning = new QPushButton(tr("  Planning"));
    btnNavFacturation = new QPushButton(tr("  Facturation"));
    btnNavAccueil->setCheckable(true);
    btnNavFormateurs->setCheckable(true);
    btnNavCours->setCheckable(true);
    btnNavParticipants->setCheckable(true);
    btnNavPlanning->setCheckable(true);
    btnNavFacturation->setCheckable(true);
    btnNavAccueil->setChecked(true);
    styliserBouton(btnNavAccueil, "segment", QStyle::SP_TitleBarMenuButton);
    styliserBouton(btnNavFormateurs, "segment", QStyle::SP_FileDialogDetailedView);
    styliserBouton(btnNavCours, "segment", QStyle::SP_FileDialogDetailedView);
    styliserBouton(btnNavParticipants, "segment", QStyle::SP_DirHomeIcon);
    styliserBouton(btnNavPlanning, "segment", QStyle::SP_FileDialogListView);
    styliserBouton(btnNavFacturation, "segment", QStyle::SP_DriveHDIcon);
    btnNavAccueil->setMinimumSize(120, 44);
    btnNavFormateurs->setMinimumSize(200, 44);
    btnNavCours->setMinimumSize(200, 44);
    btnNavParticipants->setMinimumSize(170, 44);
    btnNavPlanning->setMinimumSize(140, 44);
    btnNavFacturation->setMinimumSize(160, 44);
    for (QPushButton *b : {btnNavAccueil, btnNavFormateurs, btnNavCours, btnNavParticipants, btnNavPlanning, btnNavFacturation})
        b->setIconSize(QSize(18, 18));

    auto *groupeNav = new QButtonGroup(this);
    groupeNav->setExclusive(true);
    groupeNav->addButton(btnNavAccueil);
    groupeNav->addButton(btnNavFormateurs);
    groupeNav->addButton(btnNavCours);
    groupeNav->addButton(btnNavParticipants);
    groupeNav->addButton(btnNavPlanning);
    groupeNav->addButton(btnNavFacturation);

    ligneNav->addWidget(btnNavAccueil);
    ligneNav->addWidget(btnNavFormateurs);
    ligneNav->addWidget(btnNavCours);
    ligneNav->addWidget(btnNavParticipants);
    ligneNav->addWidget(btnNavPlanning);
    ligneNav->addWidget(btnNavFacturation);
    ligneNav->addStretch();

    QPushButton *btnDeconnexion = new QPushButton(tr("  Se déconnecter"));
    styliserBouton(btnDeconnexion, "contour", QStyle::SP_DialogCloseButton);
    btnDeconnexion->setMinimumSize(150, 40);
    connect(btnDeconnexion, &QPushButton::clicked, this, [this]() {
        emit deconnexionDemandee();
    });
    ligneNav->addWidget(btnDeconnexion);

    layoutNav->addLayout(ligneNav);

    layoutBandeau->addWidget(zoneNav);

    return bandeau;
}

void MainWindow::basculerModulePrincipal(int index)
{
    btnNavAccueil->setChecked(index == 0);
    btnNavFormateurs->setChecked(index == 1);
    btnNavCours->setChecked(index == 2);
    btnNavParticipants->setChecked(index == 3);
    btnNavPlanning->setChecked(index == 4);
    btnNavFacturation->setChecked(index == 5);
    mainStack->setCurrentIndex(index);
    animerApparition(mainStack->currentWidget());
    if (index == 0) rafraichirDashboard();      // toujours a jour en arrivant sur l'accueil
    else if (index == 3) rafraichirInscriptions(); // combos à jour (nouveaux cours/participants)
    else if (index == 4) rafraichirPlanning();
    else if (index == 5) rafraichirFacturation();
}

// =====================================================================
//                    TABLEAU DE BORD (page ACCUEIL)
// =====================================================================

// Construit une "carte KPI" : un grand chiffre + un libellé, dans un
// QGroupBox ombré, cohérent avec le reste du design de l'application.
static QGroupBox* creerCarteKpi(const QString &titre, QLabel **labelValeurOut)
{
    QGroupBox *carte = new QGroupBox(titre);
    QVBoxLayout *layout = new QVBoxLayout(carte);
    QLabel *valeur = new QLabel("0");
    valeur->setAlignment(Qt::AlignCenter);
    valeur->setStyleSheet("font-size:26pt; font-weight:700; color:#2563EB;");
    layout->addWidget(valeur);
    *labelValeurOut = valeur;
    return carte;
}

QWidget* MainWindow::creerPageAccueil()
{
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);

    QLabel *titrePage = new QLabel(tr("Tableau de bord"));
    titrePage->setStyleSheet("font-size:16pt; font-weight:700; color:#1E3A5F;");
    layout->addWidget(titrePage);

    // ---- Ligne de cartes KPI ----
    QHBoxLayout *ligneKpi = new QHBoxLayout();
    QGroupBox *carte1 = creerCarteKpi(tr("Formateurs"), &lblKpiFormateurs);
    QGroupBox *carte2 = creerCarteKpi(tr("Cours"), &lblKpiCours);
    QGroupBox *carte3 = creerCarteKpi(tr("Revenu total (DT)"), &lblKpiRevenu);
    QGroupBox *carte4 = creerCarteKpi(tr("Cours à venir (7 jours)"), &lblKpiCoursAVenir);
    for (QGroupBox *c : {carte1, carte2, carte3, carte4}) { ombrerCarte(c, true); ligneKpi->addWidget(c); }
    layout->addLayout(ligneKpi);

    // ---- Deux colonnes : cours à venir / activité récente ----
    QHBoxLayout *ligneBas = new QHBoxLayout();

    QGroupBox *carteCoursAVenir = new QGroupBox(tr("Prochains cours"));
    QVBoxLayout *layoutCoursAVenir = new QVBoxLayout(carteCoursAVenir);
    tableCoursAVenirDash = new QTableView();
    tableCoursAVenirDash->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableCoursAVenirDash->verticalHeader()->setVisible(false);
    tableCoursAVenirDash->setAlternatingRowColors(true);
    layoutCoursAVenir->addWidget(tableCoursAVenirDash);
    ombrerCarte(carteCoursAVenir, true);

    QGroupBox *carteNotifications = new QGroupBox(tr("Activité récente"));
    QVBoxLayout *layoutNotifications = new QVBoxLayout(carteNotifications);
    tableNotificationsDash = new QTableView();
    tableNotificationsDash->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableNotificationsDash->verticalHeader()->setVisible(false);
    tableNotificationsDash->setAlternatingRowColors(true);
    layoutNotifications->addWidget(tableNotificationsDash);
    ombrerCarte(carteNotifications, true);

    ligneBas->addWidget(carteCoursAVenir, 1);
    ligneBas->addWidget(carteNotifications, 1);
    layout->addLayout(ligneBas, 1);

    return page;
}

// Recalcule les indicateurs et recharge les deux mini-tableaux du
// tableau de bord. Appelée au démarrage et à chaque retour sur l'accueil.
void MainWindow::rafraichirDashboard()
{
    QSqlQuery q;

    if (q.exec("SELECT COUNT(*) FROM FORMATEURS") && q.next())
        animerCompteur(lblKpiFormateurs, q.value(0).toInt(), 0);

    if (q.exec("SELECT COUNT(*) FROM COURS") && q.next())
        animerCompteur(lblKpiCours, q.value(0).toInt(), 0);

    if (q.exec("SELECT NVL(SUM(prix),0) FROM COURS") && q.next())
        animerCompteur(lblKpiRevenu, q.value(0).toDouble(), 2);

    if (q.exec("SELECT COUNT(*) FROM COURS WHERE date_debut BETWEEN SYSDATE AND SYSDATE + 7") && q.next())
        animerCompteur(lblKpiCoursAVenir, q.value(0).toInt(), 0);

    auto *modeleCoursAVenir = new QSqlQueryModel();
    modeleCoursAVenir->setQuery(
        "SELECT * FROM ("
        "  SELECT titre_cours AS \"Cours\", TO_CHAR(date_debut,'DD/MM/YYYY') AS \"Date\" "
        "  FROM COURS WHERE date_debut >= SYSDATE ORDER BY date_debut"
        ") WHERE ROWNUM <= 8");
    tableCoursAVenirDash->setModel(modeleCoursAVenir);
    ajusterColonnesTable(tableCoursAVenirDash);

    tableNotificationsDash->setModel(Notification::lister(10));
    ajusterColonnesTable(tableNotificationsDash);
}

// Journalise une action dans la table NOTIFICATIONS et rafraîchit le fil
// d'activité si le tableau de bord est déjà construit.
void MainWindow::ajouterNotification(const QString &message)
{
    Notification::ajouter(message);
    tableNotificationsDash->setModel(Notification::lister(10));
    ajusterColonnesTable(tableNotificationsDash);
}

MainWindow::~MainWindow()
{
}

// =====================================================================
//         DESIGN : icônes de boutons + transitions animées
// =====================================================================

// Applique une icône systeme + une "classe" visuelle (primaire/danger/
// contour/discret/nav/segment) a un bouton, plus une ombre portee legere
// qui s'intensifie au survol (voir eventFilter) pour donner une
// sensation de "lift" moderne, fluide, sans dependre d'assets externes.
void MainWindow::styliserBouton(QPushButton *bouton, const QString &classe, QStyle::StandardPixmap icone)
{
    bouton->setProperty("classe", classe);
    bouton->setIcon(style()->standardIcon(icone));
    bouton->setIconSize(QSize(16, 16));
    bouton->setCursor(Qt::PointingHandCursor);
    bouton->setMinimumHeight(36);

    auto *ombre = new QGraphicsDropShadowEffect(bouton);
    ombre->setBlurRadius(0);
    ombre->setOffset(0, 2);
    ombre->setColor(QColor(30, 58, 95, 90));
    bouton->setGraphicsEffect(ombre);

    bouton->installEventFilter(this);
}

// Anime en douceur le rayon de flou de l'ombre portee d'un bouton ou
// d'une carte quand la souris entre/sort : c'est ce qui donne l'effet
// "hover" fluide demande, sans dependre du support (absent) des
// transitions CSS en QSS. Pour les cartes interactives, on ajoute en
// prime une legere elevation (translation verticale) façon "lift" au
// survol, courante dans les interfaces modernes (cartes Material/iOS).
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    auto *widget = qobject_cast<QWidget*>(watched);
    if (widget) {
        if (auto *ombre = qobject_cast<QGraphicsDropShadowEffect*>(widget->graphicsEffect())) {
            bool estCarte = widget->property("carteInteractive").toBool();
            qreal blurCible = -1, offsetCible = -1;

            if (event->type() == QEvent::Enter) {
                blurCible = estCarte ? 34 : 18;
                offsetCible = estCarte ? 8 : -1;
            } else if (event->type() == QEvent::Leave) {
                blurCible = estCarte ? 20 : 0;
                offsetCible = estCarte ? 4 : -1;
            }

            if (blurCible >= 0) {
                auto *anim = new QPropertyAnimation(ombre, "blurRadius", widget);
                anim->setDuration(190);
                anim->setStartValue(ombre->blurRadius());
                anim->setEndValue(blurCible);
                anim->setEasingCurve(QEasingCurve::OutCubic);
                anim->start(QAbstractAnimation::DeleteWhenStopped);

                if (estCarte && offsetCible >= 0) {
                    auto *animOffset = new QVariantAnimation(widget);
                    animOffset->setDuration(190);
                    animOffset->setStartValue(ombre->offset().y());
                    animOffset->setEndValue(offsetCible);
                    animOffset->setEasingCurve(QEasingCurve::OutCubic);
                    connect(animOffset, &QVariantAnimation::valueChanged, widget,
                            [ombre](const QVariant &v) { ombre->setOffset(0, v.toReal()); });
                    animOffset->start(QAbstractAnimation::DeleteWhenStopped);
                }
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

// Ajoute une ombre portee douce a une "carte" (QGroupBox) pour donner
// une impression de profondeur/relief moderne (effet de type "elevation").
// Quand `interactive` est vrai, la carte reagit en plus au survol de la
// souris avec une elevation animee (voir eventFilter).
void MainWindow::ombrerCarte(QWidget *carte, bool interactive)
{
    auto *ombre = new QGraphicsDropShadowEffect(carte);
    ombre->setBlurRadius(interactive ? 20 : 24);
    ombre->setOffset(0, interactive ? 4 : 4);
    ombre->setColor(QColor(30, 41, 59, interactive ? 55 : 40));
    carte->setGraphicsEffect(ombre);

    if (interactive) {
        carte->setProperty("carteInteractive", true);
        carte->setAttribute(Qt::WA_Hover, true);
        carte->installEventFilter(this);
    }
}

// Anime un QLabel numerique de sa valeur actuelle vers `cible`, comme un
// compteur qui "defile" : effet tres utilise dans les tableaux de bord
// professionnels pour attirer l'oeil sur une mise a jour de chiffre-cle.
void MainWindow::animerCompteur(QLabel *label, double cible, int decimales)
{
    if (!label) return;

    bool ok = false;
    double depart = QLocale(QLocale::C).toDouble(
        QString(label->text()).remove(' '), &ok);
    if (!ok) depart = 0.0;

    auto *anim = new QVariantAnimation(label);
    anim->setDuration(650);
    anim->setStartValue(depart);
    anim->setEndValue(cible);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(anim, &QVariantAnimation::valueChanged, label, [label, decimales](const QVariant &v) {
        label->setText(QString::number(v.toDouble(), 'f', decimales));
    });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

// Ajuste automatiquement la largeur des colonnes d'un QTableView au
// contenu reellement affiche, pour qu'aucune donnee ne soit tronquee,
// tout en laissant la derniere colonne s'etirer pour occuper l'espace
// restant. A rappeler apres chaque changement de modele (recherche...).
void MainWindow::ajusterColonnesTable(QTableView *table)
{
    if (!table || !table->model()) return;
    table->resizeColumnsToContents();

    int colonnes = table->model()->columnCount();
    for (int i = 0; i < colonnes; ++i) {
        int largeur = table->columnWidth(i);
        table->setColumnWidth(i, largeur + 18); // marge de confort
    }
    table->horizontalHeader()->setStretchLastSection(true);
}

// Fait apparaitre une page en fondu + leger glissement vertical (fade +
// slide-up) plutot que d'un coup sec : donne une sensation de transition
// fluide et "premium" a chaque changement de page, tout en restant tres
// legere (aucune dependance, juste QPropertyAnimation en parallele).
void MainWindow::animerApparition(QWidget *page)
{
    if (!page) return;

    auto *effet = new QGraphicsOpacityEffect(page);
    page->setGraphicsEffect(effet);

    const QRect positionFinale = page->geometry();
    const QRect positionDepart = positionFinale.translated(0, 18);

    auto *animOpacite = new QPropertyAnimation(effet, "opacity", page);
    animOpacite->setDuration(300);
    animOpacite->setStartValue(0.0);
    animOpacite->setEndValue(1.0);
    animOpacite->setEasingCurve(QEasingCurve::OutCubic);

    auto *animPosition = new QPropertyAnimation(page, "geometry", page);
    animPosition->setDuration(300);
    animPosition->setStartValue(positionDepart);
    animPosition->setEndValue(positionFinale);
    animPosition->setEasingCurve(QEasingCurve::OutCubic);

    auto *groupe = new QParallelAnimationGroup(page);
    groupe->addAnimation(animOpacite);
    groupe->addAnimation(animPosition);

    // On retire l'effet une fois l'animation terminee : un QGraphicsOpacityEffect
    // laisse en place degraderait le rendu (notamment celui des tableaux).
    connect(groupe, &QParallelAnimationGroup::finished, page, [page, positionFinale]() {
        page->setGraphicsEffect(nullptr);
        page->setGeometry(positionFinale);
    });
    groupe->start(QAbstractAnimation::DeleteWhenStopped);
}

// Change la page active d'un QStackedWidget avec une transition en fondu,
// au lieu d'un setCurrentIndex() brut qui bascule instantanement.
void MainWindow::basculerAvecAnimation(QStackedWidget *stack, int index)
{
    if (!stack || index < 0 || index == stack->currentIndex())
        return;

    stack->setCurrentIndex(index);
    animerApparition(stack->currentWidget());
}

// =====================================================================
//                         ONGLET FORMATEURS
// =====================================================================

QWidget* MainWindow::creerOngletFormateurs()
{
    QWidget *onglet = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(onglet);

    // Barre de navigation interne (remplace les boutons "Retour" de MaPharma)
    QHBoxLayout *nav = new QHBoxLayout();
    QPushButton *btnListe = new QPushButton(tr("Liste des formateurs"));
    QPushButton *btnAjouter = new QPushButton(tr("Ajouter un formateur"));
    QPushButton *btnStats = new QPushButton(tr("Statistiques"));
    styliserBouton(btnListe, "nav", QStyle::SP_FileDialogDetailedView);
    styliserBouton(btnAjouter, "nav", QStyle::SP_FileIcon);
    styliserBouton(btnStats, "nav", QStyle::SP_FileDialogInfoView);
    nav->addWidget(btnListe);
    nav->addWidget(btnAjouter);
    nav->addWidget(btnStats);
    nav->addStretch();
    layout->addLayout(nav);

    fStack = new QStackedWidget();
    fStack->addWidget(creerPageListeFormateurs());    // index 0
    fStack->addWidget(creerPageAjouterFormateur());   // index 1
    fStack->addWidget(creerPageModifierFormateur());  // index 2
    fStack->addWidget(creerPageStatsFormateurs());    // index 3
    layout->addWidget(fStack);

    connect(btnListe, &QPushButton::clicked, this, [this]() { basculerAvecAnimation(fStack, 0); });
    connect(btnAjouter, &QPushButton::clicked, this, [this]() { basculerAvecAnimation(fStack, 1); });
    connect(btnStats, &QPushButton::clicked, this, &MainWindow::onAfficherStatsFormateurs);

    return onglet;
}

QWidget* MainWindow::creerPageListeFormateurs()
{
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);

    // ---- Recherche & filtres dynamiques ----
    QGroupBox *rechBox = new QGroupBox(tr("Recherche et filtres dynamiques"));
    QHBoxLayout *rechLayout = new QHBoxLayout();

    fRechNom = new QLineEdit(); fRechNom->setPlaceholderText(tr("Nom / prénom"));
    fRechEmail = new QLineEdit(); fRechEmail->setPlaceholderText(tr("Email"));
    fRechSpecialiteCombo = new QComboBox(); // peuplé dynamiquement depuis la base

    fTriCombo = new QComboBox();
    fTriCombo->addItems({tr("Nom"), tr("Spécialité"), tr("Tarif horaire"), tr("Date d'embauche")});
    fOrdreAscCheck = new QCheckBox(tr("Croissant"));
    fOrdreAscCheck->setChecked(true);

    rechLayout->addWidget(new QLabel(tr("Nom :")));
    rechLayout->addWidget(fRechNom);
    rechLayout->addWidget(new QLabel(tr("Email :")));
    rechLayout->addWidget(fRechEmail);
    rechLayout->addWidget(new QLabel(tr("Spécialité :")));
    rechLayout->addWidget(fRechSpecialiteCombo);
    rechLayout->addWidget(new QLabel(tr("Trier par :")));
    rechLayout->addWidget(fTriCombo);
    rechLayout->addWidget(fOrdreAscCheck);
    rechBox->setLayout(rechLayout);
    layout->addWidget(rechBox); ombrerCarte(rechBox);

    // Anti-rebond : on attend une courte pause de frappe avant d'interroger
    // la base, pour éviter une requête à chaque caractère tapé.
    fRechTimer = new QTimer(this);
    fRechTimer->setSingleShot(true);
    fRechTimer->setInterval(300);
    connect(fRechTimer, &QTimer::timeout, this, &MainWindow::onFiltrerFormateurs);

    connect(fRechNom, &QLineEdit::textChanged, this, [this]() { fRechTimer->start(); });
    connect(fRechEmail, &QLineEdit::textChanged, this, [this]() { fRechTimer->start(); });
    connect(fRechSpecialiteCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onFiltrerFormateurs);
    connect(fTriCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onFiltrerFormateurs);
    connect(fOrdreAscCheck, &QCheckBox::toggled, this, &MainWindow::onFiltrerFormateurs);

    // ---- Table ----
    fTableView = new QTableView();
    fTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    fTableView->setAlternatingRowColors(true);
    fTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    fTableView->verticalHeader()->setVisible(false); // masque la numerotation automatique des lignes, seule la colonne ID (issue des donnees) est conservee
    fTableView->horizontalHeader()->setStretchLastSection(true);
    connect(fTableView, &QTableView::clicked, this, &MainWindow::onFormateurSelectionne);
    layout->addWidget(fTableView);

    fSelectionLabel = new QLabel(tr("Aucun formateur sélectionné."));
    layout->addWidget(fSelectionLabel);

    // ---- Actions ----
    QHBoxLayout *actions = new QHBoxLayout();
    QPushButton *btnModifier = new QPushButton(tr("Modifier la sélection"));
    QPushButton *btnSupprimer = new QPushButton(tr("Supprimer la sélection"));
    QPushButton *btnPdf = new QPushButton(tr("Exporter PDF"));
    QPushButton *btnCsvExport = new QPushButton(tr("Exporter CSV"));
    QPushButton *btnCsvImport = new QPushButton(tr("Importer CSV"));
    QPushButton *btnCharge = new QPushButton(tr("Charge horaire"));
    QPushButton *btnContact = new QPushButton(tr("Contacter par email"));
    styliserBouton(btnModifier, "contour", QStyle::SP_FileDialogDetailedView);
    styliserBouton(btnSupprimer, "danger", QStyle::SP_TrashIcon);
    styliserBouton(btnPdf, "contour", QStyle::SP_DialogSaveButton);
    styliserBouton(btnCsvExport, "contour", QStyle::SP_DialogSaveButton);
    styliserBouton(btnCsvImport, "contour", QStyle::SP_DialogOpenButton);
    styliserBouton(btnCharge, "contour", QStyle::SP_FileDialogContentsView);
    styliserBouton(btnContact, "contour", QStyle::SP_DialogYesButton);

    connect(btnModifier, &QPushButton::clicked, this, &MainWindow::onOuvrirModifierFormateur);
    connect(btnSupprimer, &QPushButton::clicked, this, &MainWindow::onSupprimerFormateur);
    connect(btnPdf, &QPushButton::clicked, this, &MainWindow::exporterFormateursPDF);
    connect(btnCsvExport, &QPushButton::clicked, this, &MainWindow::exporterFormateursCSV);
    connect(btnCsvImport, &QPushButton::clicked, this, &MainWindow::importerFormateursCSV);
    connect(btnCharge, &QPushButton::clicked, this, &MainWindow::onCalculerChargeHoraire);
    connect(btnContact, &QPushButton::clicked, this, &MainWindow::onContacterFormateur);

    actions->addWidget(btnModifier);
    actions->addWidget(btnSupprimer);
    actions->addWidget(btnPdf);
    actions->addWidget(btnCsvExport);
    actions->addWidget(btnCsvImport);
    actions->addWidget(btnCharge);
    actions->addWidget(btnContact);
    layout->addLayout(actions);

    fChargeLabel = new QLabel();
    layout->addWidget(fChargeLabel);

    return page;
}

QWidget* MainWindow::creerPageAjouterFormateur()
{
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);

    QGroupBox *ajoutBox = new QGroupBox(tr("Ajouter un formateur"));
    QFormLayout *form = new QFormLayout();
    fNomEdit = new QLineEdit();
    fPrenomEdit = new QLineEdit();
    fEmailEdit = new QLineEdit();
    fTelEdit = new QLineEdit();
    fTelEdit->setPlaceholderText(tr("8 chiffres"));
    fSpecialiteEdit = new QLineEdit();
    fDateEmbaucheEdit = new QDateEdit(QDate::currentDate());
    fDateEmbaucheEdit->setCalendarPopup(true);
    fTarifSpin = new QDoubleSpinBox();
    fTarifSpin->setRange(1, 2000);
    fTarifSpin->setSuffix(" DT/h");

    form->addRow(tr("Nom :"), fNomEdit);
    form->addRow(tr("Prénom :"), fPrenomEdit);
    form->addRow(tr("Email :"), fEmailEdit);
    form->addRow(tr("Téléphone :"), fTelEdit);
    form->addRow(tr("Spécialité :"), fSpecialiteEdit);
    form->addRow(tr("Date d'embauche :"), fDateEmbaucheEdit);
    form->addRow(tr("Tarif horaire :"), fTarifSpin);

    QPushButton *btnAjouter = new QPushButton(tr("Ajouter"));
    styliserBouton(btnAjouter, "primaire", QStyle::SP_DialogApplyButton);
    connect(btnAjouter, &QPushButton::clicked, this, &MainWindow::onAjouterFormateur);
    form->addRow(btnAjouter);

    ajoutBox->setLayout(form);
    layout->addWidget(ajoutBox); ombrerCarte(ajoutBox);
    layout->addStretch();

    return page;
}

QWidget* MainWindow::creerPageModifierFormateur()
{
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);

    QGroupBox *box = new QGroupBox(tr("Modifier le formateur sélectionné"));
    QFormLayout *form = new QFormLayout();

    fModId = new QLineEdit(); fModId->setReadOnly(true);
    fModNom = new QLineEdit();
    fModPrenom = new QLineEdit();
    fModEmail = new QLineEdit();
    fModTel = new QLineEdit();
    fModSpecialite = new QLineEdit();
    fModDate = new QDateEdit(); fModDate->setCalendarPopup(true);
    fModTarif = new QDoubleSpinBox(); fModTarif->setRange(1, 2000); fModTarif->setSuffix(" DT/h");

    form->addRow(tr("ID :"), fModId);
    form->addRow(tr("Nom :"), fModNom);
    form->addRow(tr("Prénom :"), fModPrenom);
    form->addRow(tr("Email :"), fModEmail);
    form->addRow(tr("Téléphone :"), fModTel);
    form->addRow(tr("Spécialité :"), fModSpecialite);
    form->addRow(tr("Date d'embauche :"), fModDate);
    form->addRow(tr("Tarif horaire :"), fModTarif);

    box->setLayout(form);
    layout->addWidget(box); ombrerCarte(box);

    QHBoxLayout *actions = new QHBoxLayout();
    QPushButton *btnEnregistrer = new QPushButton(tr("Enregistrer"));
    QPushButton *btnAnnuler = new QPushButton(tr("Annuler"));
    styliserBouton(btnEnregistrer, "primaire", QStyle::SP_DialogApplyButton);
    styliserBouton(btnAnnuler, "discret", QStyle::SP_DialogCancelButton);
    connect(btnEnregistrer, &QPushButton::clicked, this, &MainWindow::onEnregistrerModifierFormateur);
    connect(btnAnnuler, &QPushButton::clicked, this, &MainWindow::onAnnulerModifierFormateur);
    actions->addWidget(btnEnregistrer);
    actions->addWidget(btnAnnuler);
    layout->addLayout(actions);
    layout->addStretch();

    return page;
}

QWidget* MainWindow::creerPageStatsFormateurs()
{
    fStatsPage = new QWidget();
    new QVBoxLayout(fStatsPage); // rempli dynamiquement par rafraichirStatsFormateurs()
    return fStatsPage;
}

void MainWindow::rafraichirListeFormateurs()
{
    fTableView->setModel(formateur.afficher()); ajusterColonnesTable(fTableView);
    remplirComboFormateurs(cFormateurCombo);
    remplirComboFormateurs(cModFormateur);
    remplirFiltreFormateurs(cRechFormateurCombo);
    remplirFiltreSpecialites();
}

void MainWindow::remplirFiltreSpecialites()
{
    if (!fRechSpecialiteCombo)
        return;

    QString precedente = fRechSpecialiteCombo->currentIndex() > 0 ? fRechSpecialiteCombo->currentText() : "";

    fRechSpecialiteCombo->blockSignals(true);
    fRechSpecialiteCombo->clear();
    fRechSpecialiteCombo->addItem(tr("Toutes les spécialités"));

    QSqlQueryModel *model = formateur.specialitesDistinctes();
    for (int i = 0; i < model->rowCount(); ++i)
        fRechSpecialiteCombo->addItem(model->data(model->index(i, 0)).toString());

    int index = fRechSpecialiteCombo->findText(precedente);
    fRechSpecialiteCombo->setCurrentIndex(index >= 0 ? index : 0);
    fRechSpecialiteCombo->blockSignals(false);
}

void MainWindow::rafraichirStatsFormateurs()
{
    // On repart d'un layout propre à chaque rafraîchissement (mise à jour
    // dynamique du graphique dès qu'il y a un changement en base).
    QLayoutItem *item;
    while ((item = fStatsPage->layout()->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    QSqlQueryModel *model = formateur.statistiquesParSpecialite();

    QPieSeries *series = new QPieSeries();
    for (int i = 0; i < model->rowCount(); ++i) {
        QString specialite = model->data(model->index(i, 0)).toString();
        int nb = model->data(model->index(i, 1)).toInt();
        QPieSlice *slice = series->append(QString("%1 (%2)").arg(specialite).arg(nb), nb);
        slice->setLabelVisible(true);
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(tr("Répartition des formateurs par spécialité"));
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setStyleSheet("background: transparent;");

    QGroupBox *carteChart = new QGroupBox(tr("Statistiques"));
    QVBoxLayout *layoutCarte = new QVBoxLayout(carteChart);
    layoutCarte->addWidget(chartView);
    ombrerCarte(carteChart);
    fStatsPage->layout()->addWidget(carteChart);
}

// =====================================================================
//                            ONGLET COURS
// =====================================================================

QWidget* MainWindow::creerOngletCours()
{
    QWidget *onglet = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(onglet);

    QHBoxLayout *nav = new QHBoxLayout();
    QPushButton *btnListe = new QPushButton(tr("Liste des cours"));
    QPushButton *btnAjouter = new QPushButton(tr("Ajouter un cours"));
    QPushButton *btnStats = new QPushButton(tr("Statistiques"));
    styliserBouton(btnListe, "nav", QStyle::SP_FileDialogDetailedView);
    styliserBouton(btnAjouter, "nav", QStyle::SP_FileIcon);
    styliserBouton(btnStats, "nav", QStyle::SP_FileDialogInfoView);
    nav->addWidget(btnListe);
    nav->addWidget(btnAjouter);
    nav->addWidget(btnStats);
    nav->addStretch();
    layout->addLayout(nav);

    cStack = new QStackedWidget();
    cStack->addWidget(creerPageListeCours());    // index 0
    cStack->addWidget(creerPageAjouterCours());  // index 1
    cStack->addWidget(creerPageModifierCours()); // index 2
    cStack->addWidget(creerPageStatsCours());    // index 3
    layout->addWidget(cStack);

    connect(btnListe, &QPushButton::clicked, this, [this]() { basculerAvecAnimation(cStack, 0); });
    connect(btnAjouter, &QPushButton::clicked, this, [this]() { basculerAvecAnimation(cStack, 1); });
    connect(btnStats, &QPushButton::clicked, this, &MainWindow::onAfficherStatsCours);

    return onglet;
}

QWidget* MainWindow::creerPageListeCours()
{
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);

    // ---- Recherche & filtres dynamiques ----
    QGroupBox *rechBox = new QGroupBox(tr("Recherche et filtres dynamiques"));
    QHBoxLayout *rechLayout = new QHBoxLayout();

    cRechTitre = new QLineEdit(); cRechTitre->setPlaceholderText(tr("Titre"));
    cRechNiveauCombo = new QComboBox();
    cRechNiveauCombo->addItems({tr("Tous les niveaux"), "Debutant", "Intermediaire", "Avance"});
    cRechFormateurCombo = new QComboBox(); // peuplé dynamiquement (option "Tous les formateurs" + liste)

    cTriCombo = new QComboBox();
    cTriCombo->addItems({tr("Titre"), tr("Prix"), tr("Durée"), tr("Date de début")});
    cOrdreAscCheck = new QCheckBox(tr("Croissant"));
    cOrdreAscCheck->setChecked(true);

    rechLayout->addWidget(new QLabel(tr("Titre :")));
    rechLayout->addWidget(cRechTitre);
    rechLayout->addWidget(new QLabel(tr("Niveau :")));
    rechLayout->addWidget(cRechNiveauCombo);
    rechLayout->addWidget(new QLabel(tr("Formateur :")));
    rechLayout->addWidget(cRechFormateurCombo);
    rechLayout->addWidget(new QLabel(tr("Trier par :")));
    rechLayout->addWidget(cTriCombo);
    rechLayout->addWidget(cOrdreAscCheck);
    rechBox->setLayout(rechLayout);
    layout->addWidget(rechBox); ombrerCarte(rechBox);

    // Anti-rebond pour le champ texte (titre), filtres à effet immédiat
    // pour les listes déroulantes.
    cRechTimer = new QTimer(this);
    cRechTimer->setSingleShot(true);
    cRechTimer->setInterval(300);
    connect(cRechTimer, &QTimer::timeout, this, &MainWindow::onFiltrerCours);

    connect(cRechTitre, &QLineEdit::textChanged, this, [this]() { cRechTimer->start(); });
    connect(cRechNiveauCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onFiltrerCours);
    connect(cRechFormateurCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onFiltrerCours);
    connect(cTriCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onFiltrerCours);
    connect(cOrdreAscCheck, &QCheckBox::toggled, this, &MainWindow::onFiltrerCours);

    // ---- Table ----
    cTableView = new QTableView();
    cTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    cTableView->setAlternatingRowColors(true);
    cTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    cTableView->verticalHeader()->setVisible(false); // masque la numerotation automatique des lignes, seule la colonne ID (issue des donnees) est conservee
    cTableView->horizontalHeader()->setStretchLastSection(true);
    connect(cTableView, &QTableView::clicked, this, &MainWindow::onCoursSelectionne);
    layout->addWidget(cTableView);

    cSelectionLabel = new QLabel(tr("Aucun cours sélectionné."));
    layout->addWidget(cSelectionLabel);

    // ---- Actions ----
    QHBoxLayout *actions = new QHBoxLayout();
    QPushButton *btnModifier = new QPushButton(tr("Modifier la sélection"));
    QPushButton *btnSupprimer = new QPushButton(tr("Supprimer la sélection"));
    QPushButton *btnPdf = new QPushButton(tr("Exporter PDF"));
    QPushButton *btnCsvExport = new QPushButton(tr("Exporter CSV"));
    QPushButton *btnCsvImport = new QPushButton(tr("Importer CSV"));
    QPushButton *btnDupliquer = new QPushButton(tr("Dupliquer (nouvelle session)"));
    QPushButton *btnCertificat = new QPushButton(tr("Générer un certificat"));
    btnLireFichierCours = new QPushButton(tr("Lire le fichier"));
    styliserBouton(btnModifier, "contour", QStyle::SP_FileDialogDetailedView);
    styliserBouton(btnSupprimer, "danger", QStyle::SP_TrashIcon);
    styliserBouton(btnPdf, "contour", QStyle::SP_DialogSaveButton);
    styliserBouton(btnCsvExport, "contour", QStyle::SP_DialogSaveButton);
    styliserBouton(btnCsvImport, "contour", QStyle::SP_DialogOpenButton);
    styliserBouton(btnDupliquer, "contour", QStyle::SP_FileDialogNewFolder);
    styliserBouton(btnCertificat, "contour", QStyle::SP_DriveDVDIcon);
    styliserBouton(btnLireFichierCours, "contour", QStyle::SP_FileDialogContentsView);
    btnLireFichierCours->setVisible(false); // n'apparaît que lorsqu'un cours est sélectionné

    connect(btnModifier, &QPushButton::clicked, this, &MainWindow::onOuvrirModifierCours);
    connect(btnSupprimer, &QPushButton::clicked, this, &MainWindow::onSupprimerCours);
    connect(btnPdf, &QPushButton::clicked, this, &MainWindow::exporterCoursPDF);
    connect(btnCsvExport, &QPushButton::clicked, this, &MainWindow::exporterCoursCSV);
    connect(btnCsvImport, &QPushButton::clicked, this, &MainWindow::importerCoursCSV);
    connect(btnDupliquer, &QPushButton::clicked, this, &MainWindow::onDupliquerCours);
    connect(btnCertificat, &QPushButton::clicked, this, &MainWindow::onGenererCertificat);
    connect(btnLireFichierCours, &QPushButton::clicked, this, &MainWindow::onLireFichierCours);

    actions->addWidget(btnModifier);
    actions->addWidget(btnSupprimer);
    actions->addWidget(btnPdf);
    actions->addWidget(btnCsvExport);
    actions->addWidget(btnCsvImport);
    actions->addWidget(btnDupliquer);
    actions->addWidget(btnCertificat);
    actions->addWidget(btnLireFichierCours);
    layout->addLayout(actions);

    return page;
}

QWidget* MainWindow::creerPageAjouterCours()
{
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);

    QGroupBox *ajoutBox = new QGroupBox(tr("Ajouter un cours"));
    QFormLayout *form = new QFormLayout();

    cTitreEdit = new QLineEdit();
    cDescriptionEdit = new QLineEdit();
    cDureeSpin = new QSpinBox(); cDureeSpin->setRange(1, 500); cDureeSpin->setSuffix(" h");
    cNiveauCombo = new QComboBox();
    cNiveauCombo->addItems({"Debutant", "Intermediaire", "Avance"});
    cPrixSpin = new QDoubleSpinBox(); cPrixSpin->setRange(0, 100000); cPrixSpin->setSuffix(" DT");
    cDateDebutEdit = new QDateEdit(QDate::currentDate()); cDateDebutEdit->setCalendarPopup(true);
    cFormateurCombo = new QComboBox();

    form->addRow(tr("Titre :"), cTitreEdit);
    form->addRow(tr("Description :"), cDescriptionEdit);
    form->addRow(tr("Durée :"), cDureeSpin);
    form->addRow(tr("Niveau :"), cNiveauCombo);
    form->addRow(tr("Prix :"), cPrixSpin);
    form->addRow(tr("Date de début :"), cDateDebutEdit);
    form->addRow(tr("Formateur :"), cFormateurCombo);

    // ---- Fichier joint (optionnel) : tous types acceptés (txt, csv, pdf, doc, docx...) ----
    QHBoxLayout *ligneFichier = new QHBoxLayout();
    cAjoutFichierLabel = new QLabel(tr("Aucun fichier sélectionné"));
    cAjoutFichierLabel->setStyleSheet("color:#64748B;");
    QPushButton *btnChoisirFichier = new QPushButton(tr("Choisir un fichier..."));
    styliserBouton(btnChoisirFichier, "contour", QStyle::SP_DialogOpenButton);
    connect(btnChoisirFichier, &QPushButton::clicked, this, &MainWindow::onChoisirFichierAjoutCours);
    ligneFichier->addWidget(cAjoutFichierLabel, 1);
    ligneFichier->addWidget(btnChoisirFichier);
    form->addRow(tr("Fichier joint :"), ligneFichier);

    QPushButton *btnAjouter = new QPushButton(tr("Ajouter"));
    styliserBouton(btnAjouter, "primaire", QStyle::SP_DialogApplyButton);
    connect(btnAjouter, &QPushButton::clicked, this, &MainWindow::onAjouterCours);
    form->addRow(btnAjouter);

    ajoutBox->setLayout(form);
    layout->addWidget(ajoutBox); ombrerCarte(ajoutBox);
    layout->addStretch();

    return page;
}

QWidget* MainWindow::creerPageModifierCours()
{
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);

    QGroupBox *box = new QGroupBox(tr("Modifier le cours sélectionné"));
    QFormLayout *form = new QFormLayout();

    cModId = new QLineEdit(); cModId->setReadOnly(true);
    cModTitre = new QLineEdit();
    cModDescription = new QLineEdit();
    cModDuree = new QSpinBox(); cModDuree->setRange(1, 500); cModDuree->setSuffix(" h");
    cModNiveau = new QComboBox();
    cModNiveau->addItems({"Debutant", "Intermediaire", "Avance"});
    cModPrix = new QDoubleSpinBox(); cModPrix->setRange(0, 100000); cModPrix->setSuffix(" DT");
    cModDateDebut = new QDateEdit(); cModDateDebut->setCalendarPopup(true);
    cModFormateur = new QComboBox();

    form->addRow(tr("ID :"), cModId);
    form->addRow(tr("Titre :"), cModTitre);
    form->addRow(tr("Description :"), cModDescription);
    form->addRow(tr("Durée :"), cModDuree);
    form->addRow(tr("Niveau :"), cModNiveau);
    form->addRow(tr("Prix :"), cModPrix);
    form->addRow(tr("Date de début :"), cModDateDebut);
    form->addRow(tr("Formateur :"), cModFormateur);

    // ---- Fichier joint : affiche le fichier actuel, permet de le remplacer ----
    QHBoxLayout *ligneFichierMod = new QHBoxLayout();
    cModFichierLabel = new QLabel(tr("Aucun fichier joint"));
    cModFichierLabel->setStyleSheet("color:#64748B;");
    QPushButton *btnChangerFichier = new QPushButton(tr("Changer le fichier..."));
    styliserBouton(btnChangerFichier, "contour", QStyle::SP_DialogOpenButton);
    connect(btnChangerFichier, &QPushButton::clicked, this, &MainWindow::onChoisirFichierModifierCours);
    ligneFichierMod->addWidget(cModFichierLabel, 1);
    ligneFichierMod->addWidget(btnChangerFichier);
    form->addRow(tr("Fichier joint :"), ligneFichierMod);

    box->setLayout(form);
    layout->addWidget(box); ombrerCarte(box);

    QHBoxLayout *actions = new QHBoxLayout();
    QPushButton *btnEnregistrer = new QPushButton(tr("Enregistrer"));
    QPushButton *btnAnnuler = new QPushButton(tr("Annuler"));
    styliserBouton(btnEnregistrer, "primaire", QStyle::SP_DialogApplyButton);
    styliserBouton(btnAnnuler, "discret", QStyle::SP_DialogCancelButton);
    connect(btnEnregistrer, &QPushButton::clicked, this, &MainWindow::onEnregistrerModifierCours);
    connect(btnAnnuler, &QPushButton::clicked, this, &MainWindow::onAnnulerModifierCours);
    actions->addWidget(btnEnregistrer);
    actions->addWidget(btnAnnuler);
    layout->addLayout(actions);
    layout->addStretch();

    return page;
}

QWidget* MainWindow::creerPageStatsCours()
{
    cStatsPage = new QWidget();
    new QVBoxLayout(cStatsPage);
    return cStatsPage;
}

void MainWindow::rafraichirListeCours()
{
    cTableView->setModel(cours.afficher()); ajusterColonnesTable(cTableView);
    cTableView->setColumnHidden(7, true); // on masque l'id_formateur brut (on garde le libellé)
}

void MainWindow::rafraichirStatsCours()
{
    QLayoutItem *item;
    while ((item = cStatsPage->layout()->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    QSqlQueryModel *model = cours.statistiquesParNiveau();

    QBarSet *set = new QBarSet(tr("Nombre de cours"));
    QStringList categories;
    for (int i = 0; i < model->rowCount(); ++i) {
        categories << model->data(model->index(i, 0)).toString();
        *set << model->data(model->index(i, 1)).toInt();
    }

    QBarSeries *series = new QBarSeries();
    series->append(set);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(tr("Répartition des cours par niveau"));
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setStyleSheet("background: transparent;");

    QGroupBox *carteChart = new QGroupBox(tr("Statistiques"));
    QVBoxLayout *layoutCarte = new QVBoxLayout(carteChart);
    layoutCarte->addWidget(chartView);
    ombrerCarte(carteChart);
    cStatsPage->layout()->addWidget(carteChart);
}

void MainWindow::remplirComboFormateurs(QComboBox *combo)
{
    if (!combo)
        return;

    int idPrecedent = combo->currentIndex() >= 0 ? combo->currentData().toInt() : -1;
    combo->clear();

    QSqlQueryModel *model = formateur.afficher(); // réutilise le CRUD de la classe Formateur
    for (int i = 0; i < model->rowCount(); ++i) {
        int id = model->data(model->index(i, 0)).toInt();
        QString libelle = model->data(model->index(i, 1)).toString() + " " +
                           model->data(model->index(i, 2)).toString();
        combo->addItem(libelle, id);
    }

    int index = combo->findData(idPrecedent);
    if (index >= 0)
        combo->setCurrentIndex(index);
}

void MainWindow::remplirFiltreFormateurs(QComboBox *combo)
{
    if (!combo)
        return;

    QString precedent = combo->currentIndex() > 0 ? combo->currentText() : "";

    combo->blockSignals(true);
    combo->clear();
    combo->addItem(tr("Tous les formateurs"));

    QSqlQueryModel *model = formateur.afficher(); // réutilise le CRUD de la classe Formateur
    for (int i = 0; i < model->rowCount(); ++i) {
        QString libelle = model->data(model->index(i, 1)).toString() + " " +
                           model->data(model->index(i, 2)).toString();
        combo->addItem(libelle);
    }

    int index = combo->findText(precedent);
    combo->setCurrentIndex(index >= 0 ? index : 0);
    combo->blockSignals(false);
}

// =====================================================================
//                        SLOTS - FORMATEURS
// =====================================================================

void MainWindow::onAjouterFormateur()
{
    QString nom = fNomEdit->text().trimmed();
    QString prenom = fPrenomEdit->text().trimmed();
    QString email = fEmailEdit->text().trimmed();
    QString tel = fTelEdit->text().trimmed();
    QString specialite = fSpecialiteEdit->text().trimmed();
    QString dateEmbauche = fDateEmbaucheEdit->date().toString("yyyy-MM-dd");
    double tarif = fTarifSpin->value();

    if (nom.isEmpty() || prenom.isEmpty() || email.isEmpty() || specialite.isEmpty()) {
        QMessageBox::warning(this, tr("Champs manquants"), tr("Veuillez remplir tous les champs requis."));
        return;
    }
    if (!nomRegex.match(nom).hasMatch() || !nomRegex.match(prenom).hasMatch()) {
        QMessageBox::warning(this, tr("Nom invalide"), tr("Le nom et le prénom ne doivent contenir que des lettres."));
        return;
    }
    if (!emailRegex.match(email).hasMatch()) {
        QMessageBox::warning(this, tr("Email invalide"), tr("Veuillez saisir une adresse email valide."));
        return;
    }
    if (!telRegex.match(tel).hasMatch()) {
        QMessageBox::warning(this, tr("Téléphone invalide"), tr("Le téléphone doit comporter 8 chiffres."));
        return;
    }
    if (fDateEmbaucheEdit->date() > QDate::currentDate()) {
        QMessageBox::warning(this, tr("Date invalide"), tr("La date d'embauche ne peut pas être dans le futur."));
        return;
    }

    Formateur nouveau;
    nouveau.setId(formateur.prochainId());
    nouveau.setNom(nom);
    nouveau.setPrenom(prenom);
    nouveau.setEmail(email);
    nouveau.setTelephone(tel);
    nouveau.setSpecialite(specialite);
    nouveau.setDateEmbauche(dateEmbauche);
    nouveau.setTarifHoraire(tarif);

    if (nouveau.ajouter()) {
        ajouterNotification(tr("Formateur ajouté : %1 %2").arg(prenom, nom));
        QMessageBox::information(this, tr("OK"), tr("Formateur ajouté avec succès."));
        fNomEdit->clear(); fPrenomEdit->clear(); fEmailEdit->clear();
        fTelEdit->clear(); fSpecialiteEdit->clear(); fTarifSpin->setValue(1);
        rafraichirListeFormateurs();
    } else {
        QMessageBox::critical(this, tr("Erreur"), tr("L'ajout du formateur a échoué (email déjà utilisé ?)."));
    }
}

void MainWindow::onSupprimerFormateur()
{
    if (fSelectedFormateurId < 0) {
        QMessageBox::warning(this, tr("Aucune sélection"), tr("Veuillez sélectionner un formateur dans la liste."));
        return;
    }

    if (QMessageBox::question(this, tr("Confirmation"),
            tr("Supprimer définitivement ce formateur ?")) != QMessageBox::Yes)
        return;

    if (formateur.supprimer(fSelectedFormateurId)) {
        ajouterNotification(tr("Formateur supprimé : %1").arg(fSelectionLabel->text()));
        QMessageBox::information(this, tr("OK"), tr("Formateur supprimé."));
        fSelectedFormateurId = -1;
        fSelectionLabel->setText(tr("Aucun formateur sélectionné."));
        rafraichirListeFormateurs();
    } else {
        QMessageBox::critical(this, tr("Suppression impossible"),
            tr("Ce formateur est encore associé à un ou plusieurs cours.\n"
               "Réaffectez ou supprimez ces cours avant de continuer (contrainte d'intégrité)."));
    }
}

void MainWindow::onFiltrerFormateurs()
{
    QString specialite = fRechSpecialiteCombo->currentIndex() > 0 ? fRechSpecialiteCombo->currentText() : "";

    QSqlQueryModel *model = formateur.chercherEtTrier(
        fRechNom->text(), specialite, fRechEmail->text(),
        fTriCombo->currentText(), fOrdreAscCheck->isChecked());

    if (model)
        fTableView->setModel(model); ajusterColonnesTable(fTableView);
}

void MainWindow::onFormateurSelectionne(const QModelIndex &index)
{
    QAbstractItemModel *model = fTableView->model();
    fSelectedFormateurId = model->data(model->index(index.row(), 0)).toInt();
    QString nom = model->data(model->index(index.row(), 1)).toString();
    QString prenom = model->data(model->index(index.row(), 2)).toString();
    fSelectionLabel->setText(tr("Sélection : #%1 - %2 %3").arg(fSelectedFormateurId).arg(nom, prenom));
    fChargeLabel->clear();
}

void MainWindow::onOuvrirModifierFormateur()
{
    if (fSelectedFormateurId < 0) {
        QMessageBox::warning(this, tr("Aucune sélection"), tr("Veuillez sélectionner un formateur dans la liste."));
        return;
    }

    QAbstractItemModel *model = fTableView->model();
    int row = -1;
    for (int i = 0; i < model->rowCount(); ++i) {
        if (model->data(model->index(i, 0)).toInt() == fSelectedFormateurId) { row = i; break; }
    }
    if (row < 0) return;

    fModId->setText(QString::number(fSelectedFormateurId));
    fModNom->setText(model->data(model->index(row, 1)).toString());
    fModPrenom->setText(model->data(model->index(row, 2)).toString());
    fModEmail->setText(model->data(model->index(row, 3)).toString());
    fModTel->setText(model->data(model->index(row, 4)).toString());
    fModSpecialite->setText(model->data(model->index(row, 5)).toString());
    fModDate->setDate(QDate::fromString(model->data(model->index(row, 6)).toString(), "yyyy-MM-dd"));
    fModTarif->setValue(model->data(model->index(row, 7)).toDouble());

    basculerAvecAnimation(fStack, 2);
}

void MainWindow::onEnregistrerModifierFormateur()
{
    int id = fModId->text().toInt();
    QString nom = fModNom->text().trimmed();
    QString prenom = fModPrenom->text().trimmed();
    QString email = fModEmail->text().trimmed();
    QString tel = fModTel->text().trimmed();
    QString specialite = fModSpecialite->text().trimmed();
    QString date = fModDate->date().toString("yyyy-MM-dd");
    double tarif = fModTarif->value();

    if (!nomRegex.match(nom).hasMatch() || !nomRegex.match(prenom).hasMatch()) {
        QMessageBox::warning(this, tr("Nom invalide"), tr("Le nom et le prénom ne doivent contenir que des lettres."));
        return;
    }
    if (!emailRegex.match(email).hasMatch()) {
        QMessageBox::warning(this, tr("Email invalide"), tr("Veuillez saisir une adresse email valide."));
        return;
    }
    if (!telRegex.match(tel).hasMatch()) {
        QMessageBox::warning(this, tr("Téléphone invalide"), tr("Le téléphone doit comporter 8 chiffres."));
        return;
    }

    if (formateur.modifier(id, nom, prenom, email, tel, specialite, date, tarif)) {
        ajouterNotification(tr("Formateur modifié : %1 %2").arg(prenom, nom));
        QMessageBox::information(this, tr("OK"), tr("Formateur modifié avec succès."));
        rafraichirListeFormateurs();
        basculerAvecAnimation(fStack, 0);
    } else {
        QMessageBox::critical(this, tr("Erreur"), tr("La modification a échoué."));
    }
}

void MainWindow::onAnnulerModifierFormateur()
{
    basculerAvecAnimation(fStack, 0);
}

void MainWindow::onAfficherStatsFormateurs()
{
    rafraichirStatsFormateurs();
    basculerAvecAnimation(fStack, 3);
}

void MainWindow::onCalculerChargeHoraire()
{
    if (fSelectedFormateurId < 0) {
        QMessageBox::warning(this, tr("Aucune sélection"), tr("Veuillez sélectionner un formateur dans la liste."));
        return;
    }
    int nbCours = 0;
    int heures = formateur.calculerChargeHoraire(fSelectedFormateurId, &nbCours);
    fChargeLabel->setText(tr("Charge horaire : %1 cours pour un total de %2 heures.").arg(nbCours).arg(heures));
}

void MainWindow::onContacterFormateur()
{
    if (fSelectedFormateurId < 0) {
        QMessageBox::warning(this, tr("Aucune sélection"), tr("Veuillez sélectionner un formateur dans la liste."));
        return;
    }

    QAbstractItemModel *model = fTableView->model();
    QString email;
    for (int i = 0; i < model->rowCount(); ++i) {
        if (model->data(model->index(i, 0)).toInt() == fSelectedFormateurId) {
            email = model->data(model->index(i, 3)).toString();
            break;
        }
    }
    if (email.isEmpty()) return;

    if (formateur.contacterParEmail(email, tr("Centre de Formation"), tr("Bonjour, ...")))
        QMessageBox::information(this, tr("Email"), tr("Votre client de messagerie a été ouvert."));
    else
        QMessageBox::warning(this, tr("Email"), tr("Impossible d'ouvrir le client de messagerie."));
}

// =====================================================================
//                          SLOTS - COURS
// =====================================================================

void MainWindow::onAjouterCours()
{
    QString titre = cTitreEdit->text().trimmed();
    QString description = cDescriptionEdit->text().trimmed();
    int duree = cDureeSpin->value();
    QString niveau = cNiveauCombo->currentText();
    double prix = cPrixSpin->value();
    QString dateDebut = cDateDebutEdit->date().toString("yyyy-MM-dd");

    if (cFormateurCombo->count() == 0) {
        QMessageBox::warning(this, tr("Aucun formateur"), tr("Veuillez d'abord créer un formateur."));
        return;
    }
    int idFormateur = cFormateurCombo->currentData().toInt();

    if (titre.isEmpty()) {
        QMessageBox::warning(this, tr("Champ manquant"), tr("Veuillez saisir un titre pour le cours."));
        return;
    }
    if (duree <= 0) {
        QMessageBox::warning(this, tr("Durée invalide"), tr("La durée doit être positive."));
        return;
    }

    // Métier : on vérifie qu'il n'y a pas de conflit d'emploi du temps
    // pour le formateur choisi avant d'enregistrer le cours.
    if (!cours.verifierDisponibiliteFormateur(idFormateur, dateDebut)) {
        QMessageBox::warning(this, tr("Conflit d'emploi du temps"),
            tr("Ce formateur a déjà un cours programmé à cette date."));
        return;
    }

    Cours nouveau;
    nouveau.setId(cours.prochainId());
    nouveau.setTitre(titre);
    nouveau.setDescription(description);
    nouveau.setDureeHeures(duree);
    nouveau.setNiveau(niveau);
    nouveau.setPrix(prix);
    nouveau.setDateDebut(dateDebut);
    nouveau.setIdFormateur(idFormateur);

    if (nouveau.ajouter()) {
        if (!cAjoutFichierNom.isEmpty())
            cours.joindreFichier(nouveau.getId(), cAjoutFichierNom, cAjoutFichierContenu);

        ajouterNotification(tr("Cours ajouté : %1").arg(titre));
        QMessageBox::information(this, tr("OK"), tr("Cours ajouté avec succès."));
        cTitreEdit->clear(); cDescriptionEdit->clear(); cDureeSpin->setValue(1); cPrixSpin->setValue(0);
        cAjoutFichierNom.clear();
        cAjoutFichierContenu.clear();
        cAjoutFichierLabel->setText(tr("Aucun fichier sélectionné"));
        rafraichirListeCours();
    } else {
        QMessageBox::critical(this, tr("Erreur"), tr("L'ajout du cours a échoué."));
    }
}

void MainWindow::onSupprimerCours()
{
    if (cSelectedCoursId < 0) {
        QMessageBox::warning(this, tr("Aucune sélection"), tr("Veuillez sélectionner un cours dans la liste."));
        return;
    }
    if (QMessageBox::question(this, tr("Confirmation"),
            tr("Supprimer définitivement ce cours ?")) != QMessageBox::Yes)
        return;

    if (cours.supprimer(cSelectedCoursId)) {
        ajouterNotification(tr("Cours supprimé : %1").arg(cSelectionLabel->text()));
        QMessageBox::information(this, tr("OK"), tr("Cours supprimé."));
        cSelectedCoursId = -1;
        cSelectionLabel->setText(tr("Aucun cours sélectionné."));
        btnLireFichierCours->setVisible(false);
        rafraichirListeCours();
    } else {
        QMessageBox::critical(this, tr("Erreur"), tr("La suppression a échoué."));
    }
}

void MainWindow::onFiltrerCours()
{
    QString niveau = cRechNiveauCombo->currentIndex() > 0 ? cRechNiveauCombo->currentText() : "";
    QString formateurFiltre = cRechFormateurCombo->currentIndex() > 0 ? cRechFormateurCombo->currentText() : "";

    QSqlQueryModel *model = cours.chercherEtTrier(
        cRechTitre->text(), niveau, formateurFiltre,
        cTriCombo->currentText(), cOrdreAscCheck->isChecked());

    if (model) {
        cTableView->setModel(model); ajusterColonnesTable(cTableView);
        cTableView->setColumnHidden(7, true);
    }
}

void MainWindow::onCoursSelectionne(const QModelIndex &index)
{
    QAbstractItemModel *model = cTableView->model();
    cSelectedCoursId = model->data(model->index(index.row(), 0)).toInt();
    QString titre = model->data(model->index(index.row(), 1)).toString();
    cSelectionLabel->setText(tr("Sélection : #%1 - %2").arg(cSelectedCoursId).arg(titre));
    btnLireFichierCours->setVisible(true); // un cours est sélectionné : le bouton apparaît
}

void MainWindow::onOuvrirModifierCours()
{
    if (cSelectedCoursId < 0) {
        QMessageBox::warning(this, tr("Aucune sélection"), tr("Veuillez sélectionner un cours dans la liste."));
        return;
    }

    QAbstractItemModel *model = cTableView->model();
    int row = -1;
    for (int i = 0; i < model->rowCount(); ++i) {
        if (model->data(model->index(i, 0)).toInt() == cSelectedCoursId) { row = i; break; }
    }
    if (row < 0) return;

    cModId->setText(QString::number(cSelectedCoursId));
    cModTitre->setText(model->data(model->index(row, 1)).toString());
    cModDescription->setText(model->data(model->index(row, 2)).toString());
    cModDuree->setValue(model->data(model->index(row, 3)).toInt());
    cModNiveau->setCurrentText(model->data(model->index(row, 4)).toString());
    cModPrix->setValue(model->data(model->index(row, 5)).toDouble());
    cModDateDebut->setDate(QDate::fromString(model->data(model->index(row, 6)).toString(), "yyyy-MM-dd"));

    int idFormateur = model->data(model->index(row, 7)).toInt();
    remplirComboFormateurs(cModFormateur);
    int idx = cModFormateur->findData(idFormateur);
    if (idx >= 0) cModFormateur->setCurrentIndex(idx);

    // Affiche le fichier actuellement joint (s'il y en a un) et
    // reinitialise le fichier "en attente" (aucun changement par defaut).
    QString nomFichierActuel; QByteArray contenuIgnore;
    if (cours.lireFichier(cSelectedCoursId, nomFichierActuel, contenuIgnore) && !nomFichierActuel.isEmpty())
        cModFichierLabel->setText(tr("Fichier actuel : %1").arg(nomFichierActuel));
    else
        cModFichierLabel->setText(tr("Aucun fichier joint"));
    cModFichierNom.clear();
    cModFichierContenu.clear();

    basculerAvecAnimation(cStack, 2);
}

void MainWindow::onEnregistrerModifierCours()
{
    int id = cModId->text().toInt();
    QString titre = cModTitre->text().trimmed();
    QString description = cModDescription->text().trimmed();
    int duree = cModDuree->value();
    QString niveau = cModNiveau->currentText();
    double prix = cModPrix->value();
    QString dateDebut = cModDateDebut->date().toString("yyyy-MM-dd");
    int idFormateur = cModFormateur->currentData().toInt();

    if (titre.isEmpty()) {
        QMessageBox::warning(this, tr("Champ manquant"), tr("Veuillez saisir un titre pour le cours."));
        return;
    }

    if (!cours.verifierDisponibiliteFormateur(idFormateur, dateDebut, id)) {
        QMessageBox::warning(this, tr("Conflit d'emploi du temps"),
            tr("Ce formateur a déjà un autre cours programmé à cette date."));
        return;
    }

    if (cours.modifier(id, titre, description, duree, niveau, prix, dateDebut, idFormateur)) {
        if (!cModFichierNom.isEmpty())
            cours.joindreFichier(id, cModFichierNom, cModFichierContenu);

        ajouterNotification(tr("Cours modifié : %1").arg(titre));
        QMessageBox::information(this, tr("OK"), tr("Cours modifié avec succès."));
        rafraichirListeCours();
        basculerAvecAnimation(cStack, 0);
    } else {
        QMessageBox::critical(this, tr("Erreur"), tr("La modification a échoué."));
    }
}

void MainWindow::onAnnulerModifierCours()
{
    basculerAvecAnimation(cStack, 0);
}

void MainWindow::onAfficherStatsCours()
{
    rafraichirStatsCours();
    basculerAvecAnimation(cStack, 3);
}

void MainWindow::onDupliquerCours()
{
    if (cSelectedCoursId < 0) {
        QMessageBox::warning(this, tr("Aucune sélection"), tr("Veuillez sélectionner un cours dans la liste."));
        return;
    }

    QDate nouvelleDate = QDate::currentDate().addDays(7);
    if (cours.dupliquerCours(cSelectedCoursId, nouvelleDate.toString("yyyy-MM-dd"))) {
        ajouterNotification(tr("Cours dupliqué : %1 (nouvelle session le %2)")
            .arg(cSelectionLabel->text(), nouvelleDate.toString("dd/MM/yyyy")));
        QMessageBox::information(this, tr("OK"),
            tr("Une nouvelle session a été créée pour le %1.").arg(nouvelleDate.toString("dd/MM/yyyy")));
        rafraichirListeCours();
    } else {
        QMessageBox::critical(this, tr("Erreur"), tr("La duplication a échoué."));
    }
}

// Le formateur choisit un fichier (texte, PDF, CSV, Word...) depuis la
// page d'ajout d'un cours. Le contenu binaire est lu immédiatement et
// conservé en mémoire jusqu'à la validation du formulaire (onAjouterCours).
void MainWindow::onChoisirFichierAjoutCours()
{
    QString cheminFichier = QFileDialog::getOpenFileName(
        this, tr("Choisir un fichier à joindre"), QString(),
        tr("Documents (*.txt *.csv *.pdf *.doc *.docx *.odt *.rtf);;Tous les fichiers (*)"));
    if (cheminFichier.isEmpty()) return;

    QFile fichier(cheminFichier);
    if (!fichier.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Erreur"), tr("Impossible d'ouvrir le fichier sélectionné."));
        return;
    }
    cAjoutFichierContenu = fichier.readAll();
    fichier.close();

    cAjoutFichierNom = QFileInfo(cheminFichier).fileName();
    cAjoutFichierLabel->setText(cAjoutFichierNom);
}

// Même principe depuis la page de modification : le nouveau fichier
// choisi remplacera l'ancien uniquement au moment d'"Enregistrer"
// (onEnregistrerModifierCours). Tant qu'on ne choisit rien, le fichier
// déjà en base est conservé tel quel.
void MainWindow::onChoisirFichierModifierCours()
{
    QString cheminFichier = QFileDialog::getOpenFileName(
        this, tr("Choisir un nouveau fichier"), QString(),
        tr("Documents (*.txt *.csv *.pdf *.doc *.docx *.odt *.rtf);;Tous les fichiers (*)"));
    if (cheminFichier.isEmpty()) return;

    QFile fichier(cheminFichier);
    if (!fichier.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Erreur"), tr("Impossible d'ouvrir le fichier sélectionné."));
        return;
    }
    cModFichierContenu = fichier.readAll();
    fichier.close();

    cModFichierNom = QFileInfo(cheminFichier).fileName();
    cModFichierLabel->setText(tr("Nouveau fichier : %1").arg(cModFichierNom));
}

// Affiche le contenu du fichier joint au cours sélectionné dans la liste.
// Les formats texte (.txt/.csv/.md/.log) sont affichés directement dans
// l'application ; les autres formats (PDF, Word...) sont ouverts avec
// l'application par défaut du système, faute de lecteur intégré dans Qt.
void MainWindow::onLireFichierCours()
{
    if (cSelectedCoursId < 0) {
        QMessageBox::warning(this, tr("Aucune sélection"), tr("Veuillez sélectionner un cours dans la liste."));
        return;
    }

    QString nomFichier; QByteArray contenu;
    if (!cours.lireFichier(cSelectedCoursId, nomFichier, contenu)) {
        QMessageBox::critical(this, tr("Erreur"), tr("Impossible de récupérer le fichier."));
        return;
    }

    if (nomFichier.isEmpty() || contenu.isEmpty()) {
        QMessageBox::information(this, tr("Aucun fichier"),
            tr("Aucun fichier n'a encore été joint à ce cours."));
        return;
    }

    static const QStringList extensionsTexte = {"txt", "csv", "md", "log"};
    QString extension = QFileInfo(nomFichier).suffix().toLower();

    if (extensionsTexte.contains(extension)) {
        // Formats texte : affichage direct dans une fenêtre de lecture.
        QDialog dialogue(this);
        dialogue.setWindowTitle(tr("Lecture : %1").arg(nomFichier));
        dialogue.resize(600, 500);

        QVBoxLayout *layout = new QVBoxLayout(&dialogue);
        QLabel *titre = new QLabel(tr("<b>%1</b>").arg(nomFichier));
        layout->addWidget(titre);

        QTextEdit *zoneTexte = new QTextEdit();
        zoneTexte->setPlainText(QString::fromUtf8(contenu));
        zoneTexte->setReadOnly(true);
        layout->addWidget(zoneTexte);

        QDialogButtonBox *boutons = new QDialogButtonBox(QDialogButtonBox::Ok);
        connect(boutons, &QDialogButtonBox::accepted, &dialogue, &QDialog::accept);
        layout->addWidget(boutons);

        dialogue.exec();
    } else {
        // Autres formats (PDF, Word, etc.) : on écrit un fichier temporaire
        // et on l'ouvre avec l'application associée du système d'exploitation.
        QString dossierTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        QString cheminTemp = QDir(dossierTemp).filePath(nomFichier);

        QFile fichierTemp(cheminTemp);
        if (!fichierTemp.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, tr("Erreur"), tr("Impossible de préparer le fichier pour la lecture."));
            return;
        }
        fichierTemp.write(contenu);
        fichierTemp.close();

        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(cheminTemp))) {
            QMessageBox::warning(this, tr("Lecture impossible"),
                tr("Aucune application associée n'a pu ouvrir ce fichier (%1).").arg(nomFichier));
        }
    }
}

// Génère un certificat de participation en PDF pour le cours sélectionné :
// demande le nom du participant, produit un document mis en forme, et
// journalise l'émission (table CERTIFICATS + notification du tableau de bord).
void MainWindow::onGenererCertificat()
{
    if (cSelectedCoursId < 0) {
        QMessageBox::warning(this, tr("Aucune sélection"), tr("Veuillez sélectionner un cours dans la liste."));
        return;
    }

    bool ok = false;
    QString nomParticipant = QInputDialog::getText(this, tr("Générer un certificat"),
        tr("Nom complet du participant :"), QLineEdit::Normal, QString(), &ok);
    if (!ok || nomParticipant.trimmed().isEmpty()) return;
    nomParticipant = nomParticipant.trimmed();

    QSqlQuery q;
    q.prepare("SELECT c.titre_cours, c.duree_heures, TO_CHAR(c.date_debut,'DD/MM/YYYY'), "
              "f.nom_formateur || ' ' || f.prenom_formateur "
              "FROM COURS c LEFT JOIN FORMATEURS f ON c.id_formateur = f.id_formateur "
              "WHERE c.id_cours = :id");
    q.bindValue(":id", cSelectedCoursId);
    if (!q.exec() || !q.next()) {
        QMessageBox::critical(this, tr("Erreur"), tr("Impossible de récupérer les informations du cours."));
        return;
    }
    QString titreCours = q.value(0).toString();
    int dureeHeures = q.value(1).toInt();
    QString dateDebut = q.value(2).toString();
    QString nomFormateur = q.value(3).toString();

    QString fileName = QFileDialog::getSaveFileName(this, tr("Enregistrer le certificat"),
        tr("certificat_%1.pdf").arg(nomParticipant.simplified().replace(' ', '_')), tr("Fichiers PDF (*.pdf)"));
    if (fileName.isEmpty()) return;

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    printer.setPageOrientation(QPageLayout::Landscape);

    QPainter painter(&printer);
    int largeur = printer.width(), hauteur = printer.height();

    // Cadre décoratif
    QPen cadre(QColor(30, 58, 95), 24);
    painter.setPen(cadre);
    painter.drawRect(60, 60, largeur - 120, hauteur - 120);
    QPen cadreInterieur(QColor(37, 99, 235), 6);
    painter.setPen(cadreInterieur);
    painter.drawRect(110, 110, largeur - 220, hauteur - 220);

    painter.setPen(QColor(30, 58, 95));
    painter.setFont(QFont("Georgia", 42, QFont::Bold));
    painter.drawText(QRect(0, hauteur * 0.18, largeur, 700), Qt::AlignCenter, tr("CERTIFICAT DE PARTICIPATION"));

    painter.setFont(QFont("Arial", 16));
    painter.drawText(QRect(0, hauteur * 0.34, largeur, 500), Qt::AlignCenter, tr("Ce certificat est décerné à"));

    painter.setFont(QFont("Georgia", 34, QFont::Bold));
    painter.setPen(QColor(37, 99, 235));
    painter.drawText(QRect(0, hauteur * 0.42, largeur, 700), Qt::AlignCenter, nomParticipant);

    painter.setPen(QColor(30, 41, 59));
    painter.setFont(QFont("Arial", 16));
    QString phrase = tr("pour avoir suivi avec succès la formation \"%1\"\nd'une durée de %2 heures, dispensée par %3, débutée le %4.")
        .arg(titreCours).arg(dureeHeures).arg(nomFormateur, dateDebut);
    painter.drawText(QRect(largeur * 0.15, hauteur * 0.58, largeur * 0.7, 800), Qt::AlignCenter | Qt::TextWordWrap, phrase);

    painter.setFont(QFont("Arial", 11));
    painter.setPen(QColor(100, 116, 139));
    painter.drawText(QRect(0, hauteur * 0.85, largeur, 400), Qt::AlignCenter,
        tr("Centre de Formation — Document généré le %1").arg(QDate::currentDate().toString("dd/MM/yyyy")));

    painter.end();

    Certificat::enregistrer(cSelectedCoursId, nomParticipant);
    ajouterNotification(tr("Certificat généré pour %1 (cours : %2)").arg(nomParticipant, titreCours));
    QMessageBox::information(this, tr("Certificat généré"),
        tr("Le certificat de %1 a été enregistré dans %2.").arg(nomParticipant, fileName));
}

// =====================================================================
//                    EXPORT PDF (document personnalisé)
// =====================================================================

void MainWindow::exporterFormateursPDF()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Enregistrer PDF"), "formateurs.pdf", tr("Fichiers PDF (*.pdf)"));
    if (fileName.isEmpty()) return;

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);

    QPainter painter(&printer);

    QFont titleFont("Arial", 18, QFont::Bold);
    QFont subFont("Arial", 10);
    QFont headerFont("Arial", 11, QFont::Bold);
    QFont rowFont("Arial", 9);

    painter.setFont(titleFont);
    painter.drawText(300, 400, tr("Centre de Formation - Liste des Formateurs"));
    painter.setFont(subFont);
    painter.drawText(300, 650, tr("Document généré le %1").arg(QDate::currentDate().toString("dd/MM/yyyy")));

    QSqlQueryModel *model = formateur.afficher();
    QStringList headers = {"ID", "Nom", "Prénom", "Email", "Tél.", "Spécialité", "Embauche", "Tarif"};
    int columnWidths[] = {500, 1300, 1300, 2200, 1200, 1800, 1400, 1200};

    int x = 300, y = 900, lineHeight = 280;

    painter.setFont(headerFont);
    painter.setBrush(QColor(200, 220, 255));
    int currentX = x;
    for (int i = 0; i < headers.size(); ++i) {
        painter.drawRect(currentX, y, columnWidths[i], lineHeight);
        painter.drawText(currentX + 10, y + 190, headers[i]);
        currentX += columnWidths[i];
    }
    y += lineHeight;

    painter.setFont(rowFont);
    for (int i = 0; i < model->rowCount(); ++i) {
        currentX = x;
        painter.setBrush(i % 2 == 0 ? QColor(245, 245, 245) : QColor(255, 255, 255));
        for (int j = 0; j < headers.size(); ++j) {
            painter.drawRect(currentX, y, columnWidths[j], lineHeight);
            painter.drawText(currentX + 10, y + 190, model->data(model->index(i, j)).toString());
            currentX += columnWidths[j];
        }
        y += lineHeight;

        if (y + lineHeight > printer.height() - 300) {
            painter.end();
            printer.newPage();
            painter.begin(&printer);
            y = 300;
        }
    }

    painter.end();
    QMessageBox::information(this, tr("PDF exporté"), tr("La liste des formateurs a été exportée en PDF."));
}

// =====================================================================
//                    IMPORT / EXPORT CSV — FORMATEURS
// =====================================================================
void MainWindow::exporterFormateursCSV()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Exporter en CSV"), "formateurs.csv", tr("Fichiers CSV (*.csv)"));
    if (fileName.isEmpty()) return;

    QFile fichier(fileName);
    if (!fichier.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Erreur"), tr("Impossible de créer le fichier."));
        return;
    }

    QTextStream out(&fichier);
    QSqlQueryModel *model = formateur.afficher();

    out << "ID;Nom;Prenom;Email;Telephone;Specialite;DateEmbauche;TarifHoraire\n";
    for (int i = 0; i < model->rowCount(); ++i) {
        QStringList champs;
        for (int j = 0; j < model->columnCount(); ++j)
            champs << model->data(model->index(i, j)).toString();
        out << champs.join(";") << "\n";
    }
    fichier.close();

    ajouterNotification(tr("Export CSV des formateurs (%1 lignes)").arg(model->rowCount()));
    QMessageBox::information(this, tr("Export réussi"), tr("%1 formateur(s) exporté(s) vers %2.").arg(model->rowCount()).arg(fileName));
}

void MainWindow::importerFormateursCSV()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Importer un CSV"), QString(), tr("Fichiers CSV (*.csv)"));
    if (fileName.isEmpty()) return;

    QFile fichier(fileName);
    if (!fichier.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Erreur"), tr("Impossible d'ouvrir le fichier."));
        return;
    }

    QTextStream in(&fichier);
    int reussites = 0, echecs = 0;
    bool premiereLigne = true;

    while (!in.atEnd()) {
        QString ligne = in.readLine();
        if (premiereLigne) { premiereLigne = false; continue; } // en-tête ignorée
        if (ligne.trimmed().isEmpty()) continue;

        // Colonnes attendues : ID;Nom;Prenom;Email;Telephone;Specialite;DateEmbauche;TarifHoraire
        QStringList champs = ligne.split(';');
        if (champs.size() < 8) { echecs++; continue; }

        Formateur nouveau;
        nouveau.setId(formateur.prochainId());
        nouveau.setNom(champs[1].trimmed());
        nouveau.setPrenom(champs[2].trimmed());
        nouveau.setEmail(champs[3].trimmed());
        nouveau.setTelephone(champs[4].trimmed());
        nouveau.setSpecialite(champs[5].trimmed());
        nouveau.setDateEmbauche(champs[6].trimmed());
        nouveau.setTarifHoraire(champs[7].trimmed().toDouble());

        if (!nouveau.getNom().isEmpty() && !nouveau.getEmail().isEmpty() && nouveau.ajouter())
            reussites++;
        else
            echecs++;
    }
    fichier.close();

    rafraichirListeFormateurs();
    ajouterNotification(tr("Import CSV formateurs : %1 réussi(s), %2 échec(s)").arg(reussites).arg(echecs));
    QMessageBox::information(this, tr("Import terminé"),
        tr("%1 formateur(s) importé(s) avec succès.\n%2 ligne(s) en échec (email dupliqué ou données invalides).")
            .arg(reussites).arg(echecs));
}

void MainWindow::exporterCoursPDF()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Enregistrer PDF"), "cours.pdf", tr("Fichiers PDF (*.pdf)"));
    if (fileName.isEmpty()) return;

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);

    QPainter painter(&printer);

    QFont titleFont("Arial", 18, QFont::Bold);
    QFont subFont("Arial", 10);
    QFont headerFont("Arial", 11, QFont::Bold);
    QFont rowFont("Arial", 9);

    painter.setFont(titleFont);
    painter.drawText(300, 400, tr("Centre de Formation - Catalogue des Cours"));
    painter.setFont(subFont);
    painter.drawText(300, 650, tr("Document généré le %1").arg(QDate::currentDate().toString("dd/MM/yyyy")));

    QSqlQueryModel *model = cours.afficher();
    QStringList headers = {"ID", "Titre", "Durée", "Niveau", "Prix", "Début", "Formateur"};
    int columnWidths[] = {500, 2200, 900, 1400, 900, 1300, 2000};
    // colonnes du modèle utilisées : 0,1,3,4,5,6,8 (on saute description et id_formateur brut)
    int colIndexes[] = {0, 1, 3, 4, 5, 6, 8};

    int x = 300, y = 900, lineHeight = 280;

    painter.setFont(headerFont);
    painter.setBrush(QColor(255, 225, 200));
    int currentX = x;
    for (int i = 0; i < headers.size(); ++i) {
        painter.drawRect(currentX, y, columnWidths[i], lineHeight);
        painter.drawText(currentX + 10, y + 190, headers[i]);
        currentX += columnWidths[i];
    }
    y += lineHeight;

    painter.setFont(rowFont);
    for (int i = 0; i < model->rowCount(); ++i) {
        currentX = x;
        painter.setBrush(i % 2 == 0 ? QColor(245, 245, 245) : QColor(255, 255, 255));
        for (int j = 0; j < 7; ++j) {
            painter.drawRect(currentX, y, columnWidths[j], lineHeight);
            painter.drawText(currentX + 10, y + 190, model->data(model->index(i, colIndexes[j])).toString());
            currentX += columnWidths[j];
        }
        y += lineHeight;

        if (y + lineHeight > printer.height() - 300) {
            painter.end();
            printer.newPage();
            painter.begin(&printer);
            y = 300;
        }
    }

    painter.end();
    QMessageBox::information(this, tr("PDF exporté"), tr("Le catalogue des cours a été exporté en PDF."));
}

// =====================================================================
//                    IMPORT / EXPORT CSV — COURS
// =====================================================================
void MainWindow::exporterCoursCSV()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Exporter en CSV"), "cours.csv", tr("Fichiers CSV (*.csv)"));
    if (fileName.isEmpty()) return;

    QFile fichier(fileName);
    if (!fichier.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Erreur"), tr("Impossible de créer le fichier."));
        return;
    }

    QTextStream out(&fichier);
    QSqlQueryModel *model = cours.afficher();

    out << "ID;Titre;Description;DureeHeures;Niveau;Prix;DateDebut;IdFormateur;Formateur\n";
    for (int i = 0; i < model->rowCount(); ++i) {
        QStringList champs;
        for (int j = 0; j < model->columnCount(); ++j) {
            QString valeur = model->data(model->index(i, j)).toString();
            valeur.replace(';', ','); // évite de casser le séparateur CSV (ex. dans la description)
            champs << valeur;
        }
        out << champs.join(";") << "\n";
    }
    fichier.close();

    ajouterNotification(tr("Export CSV des cours (%1 lignes)").arg(model->rowCount()));
    QMessageBox::information(this, tr("Export réussi"), tr("%1 cours exporté(s) vers %2.").arg(model->rowCount()).arg(fileName));
}

void MainWindow::importerCoursCSV()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Importer un CSV"), QString(), tr("Fichiers CSV (*.csv)"));
    if (fileName.isEmpty()) return;

    QFile fichier(fileName);
    if (!fichier.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Erreur"), tr("Impossible d'ouvrir le fichier."));
        return;
    }

    QTextStream in(&fichier);
    int reussites = 0, echecs = 0;
    bool premiereLigne = true;

    while (!in.atEnd()) {
        QString ligne = in.readLine();
        if (premiereLigne) { premiereLigne = false; continue; } // en-tête ignorée
        if (ligne.trimmed().isEmpty()) continue;

        // Colonnes attendues : ID;Titre;Description;DureeHeures;Niveau;Prix;DateDebut;IdFormateur;Formateur
        QStringList champs = ligne.split(';');
        if (champs.size() < 8) { echecs++; continue; }

        Cours nouveau;
        nouveau.setId(cours.prochainId());
        nouveau.setTitre(champs[1].trimmed());
        nouveau.setDescription(champs[2].trimmed());
        nouveau.setDureeHeures(champs[3].trimmed().toInt());
        nouveau.setNiveau(champs[4].trimmed());
        nouveau.setPrix(champs[5].trimmed().toDouble());
        nouveau.setDateDebut(champs[6].trimmed());
        nouveau.setIdFormateur(champs[7].trimmed().toInt());

        if (!nouveau.getTitre().isEmpty() && nouveau.getIdFormateur() > 0 && nouveau.ajouter())
            reussites++;
        else
            echecs++;
    }
    fichier.close();

    rafraichirListeCours();
    ajouterNotification(tr("Import CSV cours : %1 réussi(s), %2 échec(s)").arg(reussites).arg(echecs));
    QMessageBox::information(this, tr("Import terminé"),
        tr("%1 cours importé(s) avec succès.\n%2 ligne(s) en échec (formateur inexistant ou données invalides).")
            .arg(reussites).arg(echecs));
}

// =====================================================================
//              ONGLET PARTICIPANTS / INSCRIPTIONS
// =====================================================================

QWidget* MainWindow::creerOngletParticipants()
{
    QWidget *onglet = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(onglet);

    QHBoxLayout *nav = new QHBoxLayout();
    QPushButton *btnListe = new QPushButton(tr("Liste des participants"));
    QPushButton *btnAjouter = new QPushButton(tr("Ajouter un participant"));
    QPushButton *btnInscriptions = new QPushButton(tr("Inscriptions"));
    styliserBouton(btnListe, "nav", QStyle::SP_FileDialogDetailedView);
    styliserBouton(btnAjouter, "nav", QStyle::SP_FileIcon);
    styliserBouton(btnInscriptions, "nav", QStyle::SP_DialogYesButton);
    nav->addWidget(btnListe);
    nav->addWidget(btnAjouter);
    nav->addWidget(btnInscriptions);
    nav->addStretch();
    layout->addLayout(nav);

    pStack = new QStackedWidget();
    pStack->addWidget(creerPageListeParticipants());   // index 0
    pStack->addWidget(creerPageAjouterParticipant());  // index 1
    pStack->addWidget(creerPageModifierParticipant()); // index 2
    pStack->addWidget(creerPageInscriptions());        // index 3
    layout->addWidget(pStack);

    connect(btnListe, &QPushButton::clicked, this, [this]() { basculerAvecAnimation(pStack, 0); });
    connect(btnAjouter, &QPushButton::clicked, this, [this]() { basculerAvecAnimation(pStack, 1); });
    connect(btnInscriptions, &QPushButton::clicked, this, [this]() { rafraichirInscriptions(); basculerAvecAnimation(pStack, 3); });

    return onglet;
}

QWidget* MainWindow::creerPageListeParticipants()
{
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);

    QGroupBox *rechBox = new QGroupBox(tr("Recherche et tri dynamiques"));
    QHBoxLayout *rechLayout = new QHBoxLayout();
    pRechTexte = new QLineEdit(); pRechTexte->setPlaceholderText(tr("Nom / prénom / email"));
    pTriCombo = new QComboBox();
    pTriCombo->addItems({tr("Nom"), tr("Email"), tr("Date de naissance")});
    pOrdreAscCheck = new QCheckBox(tr("Croissant"));
    pOrdreAscCheck->setChecked(true);
    rechLayout->addWidget(new QLabel(tr("Recherche :")));
    rechLayout->addWidget(pRechTexte);
    rechLayout->addWidget(new QLabel(tr("Trier par :")));
    rechLayout->addWidget(pTriCombo);
    rechLayout->addWidget(pOrdreAscCheck);
    rechBox->setLayout(rechLayout);
    layout->addWidget(rechBox); ombrerCarte(rechBox);

    pRechTimer = new QTimer(this);
    pRechTimer->setSingleShot(true);
    pRechTimer->setInterval(300);
    connect(pRechTimer, &QTimer::timeout, this, &MainWindow::onFiltrerParticipants);
    connect(pRechTexte, &QLineEdit::textChanged, this, [this]() { pRechTimer->start(); });
    connect(pTriCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onFiltrerParticipants);
    connect(pOrdreAscCheck, &QCheckBox::toggled, this, &MainWindow::onFiltrerParticipants);

    pTableView = new QTableView();
    pTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    pTableView->setAlternatingRowColors(true);
    pTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    pTableView->verticalHeader()->setVisible(false);
    pTableView->horizontalHeader()->setStretchLastSection(true);
    connect(pTableView, &QTableView::clicked, this, &MainWindow::onParticipantSelectionne);
    layout->addWidget(pTableView);

    pSelectionLabel = new QLabel(tr("Aucun participant sélectionné."));
    layout->addWidget(pSelectionLabel);

    QHBoxLayout *actions = new QHBoxLayout();
    QPushButton *btnModifier = new QPushButton(tr("Modifier la sélection"));
    QPushButton *btnSupprimer = new QPushButton(tr("Supprimer la sélection"));
    styliserBouton(btnModifier, "contour", QStyle::SP_FileDialogDetailedView);
    styliserBouton(btnSupprimer, "danger", QStyle::SP_TrashIcon);
    connect(btnModifier, &QPushButton::clicked, this, &MainWindow::onOuvrirModifierParticipant);
    connect(btnSupprimer, &QPushButton::clicked, this, &MainWindow::onSupprimerParticipant);
    actions->addWidget(btnModifier);
    actions->addWidget(btnSupprimer);
    actions->addStretch();
    layout->addLayout(actions);

    return page;
}

QWidget* MainWindow::creerPageAjouterParticipant()
{
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);

    QGroupBox *box = new QGroupBox(tr("Ajouter un participant"));
    QFormLayout *form = new QFormLayout();
    pNomEdit = new QLineEdit();
    pPrenomEdit = new QLineEdit();
    pEmailEdit = new QLineEdit();
    pTelEdit = new QLineEdit(); pTelEdit->setPlaceholderText(tr("8 chiffres"));
    pDateNaissanceEdit = new QDateEdit(QDate::currentDate().addYears(-20));
    pDateNaissanceEdit->setCalendarPopup(true);
    pDateNaissanceRenseigneeCheck = new QCheckBox(tr("Date de naissance connue"));
    pDateNaissanceRenseigneeCheck->setChecked(false);
    pDateNaissanceEdit->setEnabled(false);
    connect(pDateNaissanceRenseigneeCheck, &QCheckBox::toggled, pDateNaissanceEdit, &QDateEdit::setEnabled);

    form->addRow(tr("Nom :"), pNomEdit);
    form->addRow(tr("Prénom :"), pPrenomEdit);
    form->addRow(tr("Email :"), pEmailEdit);
    form->addRow(tr("Téléphone :"), pTelEdit);
    form->addRow(pDateNaissanceRenseigneeCheck);
    form->addRow(tr("Date de naissance :"), pDateNaissanceEdit);

    QPushButton *btnAjouter = new QPushButton(tr("Ajouter"));
    styliserBouton(btnAjouter, "primaire", QStyle::SP_DialogApplyButton);
    connect(btnAjouter, &QPushButton::clicked, this, &MainWindow::onAjouterParticipant);
    form->addRow(btnAjouter);

    box->setLayout(form);
    layout->addWidget(box); ombrerCarte(box);
    layout->addStretch();
    return page;
}

QWidget* MainWindow::creerPageModifierParticipant()
{
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);

    QGroupBox *box = new QGroupBox(tr("Modifier le participant sélectionné"));
    QFormLayout *form = new QFormLayout();
    pModId = new QLineEdit(); pModId->setReadOnly(true);
    pModNom = new QLineEdit();
    pModPrenom = new QLineEdit();
    pModEmail = new QLineEdit();
    pModTel = new QLineEdit();
    pModDateNaissance = new QDateEdit(); pModDateNaissance->setCalendarPopup(true);
    pModDateNaissanceRenseigneeCheck = new QCheckBox(tr("Date de naissance connue"));
    connect(pModDateNaissanceRenseigneeCheck, &QCheckBox::toggled, pModDateNaissance, &QDateEdit::setEnabled);

    form->addRow(tr("ID :"), pModId);
    form->addRow(tr("Nom :"), pModNom);
    form->addRow(tr("Prénom :"), pModPrenom);
    form->addRow(tr("Email :"), pModEmail);
    form->addRow(tr("Téléphone :"), pModTel);
    form->addRow(pModDateNaissanceRenseigneeCheck);
    form->addRow(tr("Date de naissance :"), pModDateNaissance);

    box->setLayout(form);
    layout->addWidget(box); ombrerCarte(box);

    QHBoxLayout *actions = new QHBoxLayout();
    QPushButton *btnEnregistrer = new QPushButton(tr("Enregistrer"));
    QPushButton *btnAnnuler = new QPushButton(tr("Annuler"));
    styliserBouton(btnEnregistrer, "primaire", QStyle::SP_DialogApplyButton);
    styliserBouton(btnAnnuler, "discret", QStyle::SP_DialogCancelButton);
    connect(btnEnregistrer, &QPushButton::clicked, this, &MainWindow::onEnregistrerModifierParticipant);
    connect(btnAnnuler, &QPushButton::clicked, this, &MainWindow::onAnnulerModifierParticipant);
    actions->addWidget(btnEnregistrer);
    actions->addWidget(btnAnnuler);
    layout->addLayout(actions);
    layout->addStretch();
    return page;
}

QWidget* MainWindow::creerPageInscriptions()
{
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);

    // ---- Nouvelle inscription ----
    QGroupBox *inscBox = new QGroupBox(tr("Inscrire un participant à un cours"));
    QHBoxLayout *inscLayout = new QHBoxLayout();
    iParticipantCombo = new QComboBox();
    iCoursCombo = new QComboBox();
    QPushButton *btnInscrire = new QPushButton(tr("Inscrire"));
    styliserBouton(btnInscrire, "primaire", QStyle::SP_DialogApplyButton);
    connect(btnInscrire, &QPushButton::clicked, this, &MainWindow::onInscrireParticipant);
    inscLayout->addWidget(new QLabel(tr("Participant :")));
    inscLayout->addWidget(iParticipantCombo, 1);
    inscLayout->addWidget(new QLabel(tr("Cours :")));
    inscLayout->addWidget(iCoursCombo, 1);
    inscLayout->addWidget(btnInscrire);
    inscBox->setLayout(inscLayout);
    layout->addWidget(inscBox); ombrerCarte(inscBox);

    // ---- Recherche / filtre ----
    QGroupBox *rechBox = new QGroupBox(tr("Recherche et filtre par statut"));
    QHBoxLayout *rechLayout = new QHBoxLayout();
    iRechTexte = new QLineEdit(); iRechTexte->setPlaceholderText(tr("Participant / cours"));
    iStatutCombo = new QComboBox();
    iStatutCombo->addItems({tr("Tous"), tr("En attente"), tr("Confirmee"), tr("Annulee")});
    rechLayout->addWidget(new QLabel(tr("Recherche :")));
    rechLayout->addWidget(iRechTexte);
    rechLayout->addWidget(new QLabel(tr("Statut :")));
    rechLayout->addWidget(iStatutCombo);
    rechBox->setLayout(rechLayout);
    layout->addWidget(rechBox); ombrerCarte(rechBox);

    iRechTimer = new QTimer(this);
    iRechTimer->setSingleShot(true);
    iRechTimer->setInterval(300);
    connect(iRechTimer, &QTimer::timeout, this, &MainWindow::onFiltrerInscriptions);
    connect(iRechTexte, &QLineEdit::textChanged, this, [this]() { iRechTimer->start(); });
    connect(iStatutCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onFiltrerInscriptions);

    iTableView = new QTableView();
    iTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    iTableView->setAlternatingRowColors(true);
    iTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    iTableView->verticalHeader()->setVisible(false);
    iTableView->horizontalHeader()->setStretchLastSection(true);
    connect(iTableView, &QTableView::clicked, this, &MainWindow::onInscriptionSelectionnee);
    layout->addWidget(iTableView);

    iSelectionLabel = new QLabel(tr("Aucune inscription sélectionnée."));
    layout->addWidget(iSelectionLabel);

    QHBoxLayout *actions = new QHBoxLayout();
    QPushButton *btnConfirmer = new QPushButton(tr("Confirmer"));
    QPushButton *btnAnnulerInsc = new QPushButton(tr("Annuler l'inscription"));
    QPushButton *btnSupprimer = new QPushButton(tr("Supprimer"));
    styliserBouton(btnConfirmer, "contour", QStyle::SP_DialogYesButton);
    styliserBouton(btnAnnulerInsc, "contour", QStyle::SP_DialogNoButton);
    styliserBouton(btnSupprimer, "danger", QStyle::SP_TrashIcon);
    connect(btnConfirmer, &QPushButton::clicked, this, &MainWindow::onConfirmerInscription);
    connect(btnAnnulerInsc, &QPushButton::clicked, this, &MainWindow::onAnnulerInscriptionStatut);
    connect(btnSupprimer, &QPushButton::clicked, this, &MainWindow::onSupprimerInscription);
    actions->addWidget(btnConfirmer);
    actions->addWidget(btnAnnulerInsc);
    actions->addWidget(btnSupprimer);
    actions->addStretch();
    layout->addLayout(actions);

    return page;
}

void MainWindow::remplirComboParticipants(QComboBox *combo)
{
    if (!combo) return;
    int idPrecedent = combo->currentIndex() >= 0 ? combo->currentData().toInt() : -1;
    combo->blockSignals(true);
    combo->clear();
    QSqlQueryModel *model = participant.listePourCombo();
    for (int i = 0; i < model->rowCount(); ++i) {
        int id = model->data(model->index(i, 0)).toInt();
        combo->addItem(model->data(model->index(i, 1)).toString(), id);
    }
    int idx = combo->findData(idPrecedent);
    combo->setCurrentIndex(idx >= 0 ? idx : (combo->count() > 0 ? 0 : -1));
    combo->blockSignals(false);
}

void MainWindow::remplirComboCoursPourInscription(QComboBox *combo)
{
    if (!combo) return;
    int idPrecedent = combo->currentIndex() >= 0 ? combo->currentData().toInt() : -1;
    combo->blockSignals(true);
    combo->clear();
    QSqlQueryModel *model = cours.listePourCombo();
    for (int i = 0; i < model->rowCount(); ++i) {
        int id = model->data(model->index(i, 0)).toInt();
        combo->addItem(model->data(model->index(i, 1)).toString(), id);
    }
    int idx = combo->findData(idPrecedent);
    combo->setCurrentIndex(idx >= 0 ? idx : (combo->count() > 0 ? 0 : -1));
    combo->blockSignals(false);
}

void MainWindow::rafraichirListeParticipants()
{
    pTableView->setModel(participant.afficher());
    ajusterColonnesTable(pTableView);
    remplirComboParticipants(iParticipantCombo);
}

void MainWindow::rafraichirInscriptions()
{
    remplirComboParticipants(iParticipantCombo);
    remplirComboCoursPourInscription(iCoursCombo);
    iTableView->setModel(Inscription::afficher());
    ajusterColonnesTable(iTableView);
}

void MainWindow::onAjouterParticipant()
{
    QString nom = pNomEdit->text().trimmed();
    QString prenom = pPrenomEdit->text().trimmed();
    QString email = pEmailEdit->text().trimmed();

    if (!nomRegex.match(nom).hasMatch() || !nomRegex.match(prenom).hasMatch()) {
        QMessageBox::warning(this, tr("Champ invalide"), tr("Le nom et le prénom ne doivent contenir que des lettres."));
        return;
    }
    if (!emailRegex.match(email).hasMatch()) {
        QMessageBox::warning(this, tr("Champ invalide"), tr("Adresse email invalide."));
        return;
    }
    if (!pTelEdit->text().trimmed().isEmpty() && !telRegex.match(pTelEdit->text().trimmed()).hasMatch()) {
        QMessageBox::warning(this, tr("Champ invalide"), tr("Le téléphone doit contenir exactement 8 chiffres."));
        return;
    }

    Participant p;
    p.setId(participant.prochainId());
    p.setNom(nom);
    p.setPrenom(prenom);
    p.setEmail(email);
    p.setTelephone(pTelEdit->text().trimmed());
    p.setDateNaissance(pDateNaissanceRenseigneeCheck->isChecked()
                        ? pDateNaissanceEdit->date().toString("yyyy-MM-dd") : "");

    if (p.ajouter()) {
        pNomEdit->clear(); pPrenomEdit->clear(); pEmailEdit->clear(); pTelEdit->clear();
        pDateNaissanceRenseigneeCheck->setChecked(false);
        rafraichirListeParticipants();
        ajouterNotification(tr("Participant ajouté : %1 %2").arg(prenom, nom));
        QMessageBox::information(this, tr("Succès"), tr("Participant ajouté avec succès."));
        basculerAvecAnimation(pStack, 0);
    } else {
        QMessageBox::critical(this, tr("Erreur"), tr("Impossible d'ajouter ce participant (email déjà utilisé ?)."));
    }
}

void MainWindow::onSupprimerParticipant()
{
    if (pSelectedParticipantId < 0) {
        QMessageBox::warning(this, tr("Aucune sélection"), tr("Veuillez sélectionner un participant."));
        return;
    }
    if (QMessageBox::question(this, tr("Confirmation"), tr("Supprimer ce participant ?")) != QMessageBox::Yes)
        return;

    if (participant.supprimer(pSelectedParticipantId)) {
        ajouterNotification(tr("Participant supprimé (ID %1)").arg(pSelectedParticipantId));
        pSelectedParticipantId = -1;
        pSelectionLabel->setText(tr("Aucun participant sélectionné."));
        rafraichirListeParticipants();
    } else {
        QMessageBox::warning(this, tr("Suppression impossible"),
            tr("Ce participant a des inscriptions ; annulez-les d'abord."));
    }
}

void MainWindow::onFiltrerParticipants()
{
    pTableView->setModel(participant.chercherEtTrier(pRechTexte->text(), pTriCombo->currentText(), pOrdreAscCheck->isChecked()));
    ajusterColonnesTable(pTableView);
}

void MainWindow::onParticipantSelectionne(const QModelIndex &index)
{
    if (!index.isValid()) return;
    int row = index.row();
    pSelectedParticipantId = pTableView->model()->data(pTableView->model()->index(row, 0)).toInt();
    QString nom = pTableView->model()->data(pTableView->model()->index(row, 1)).toString();
    QString prenom = pTableView->model()->data(pTableView->model()->index(row, 2)).toString();
    pSelectionLabel->setText(tr("Sélectionné : %1 %2 (ID %3)").arg(nom, prenom).arg(pSelectedParticipantId));
}

void MainWindow::onOuvrirModifierParticipant()
{
    if (pSelectedParticipantId < 0) {
        QMessageBox::warning(this, tr("Aucune sélection"), tr("Veuillez sélectionner un participant."));
        return;
    }
    QSqlQueryModel *model = static_cast<QSqlQueryModel*>(pTableView->model());
    for (int row = 0; row < model->rowCount(); ++row) {
        if (model->data(model->index(row, 0)).toInt() == pSelectedParticipantId) {
            pModId->setText(QString::number(pSelectedParticipantId));
            pModNom->setText(model->data(model->index(row, 1)).toString());
            pModPrenom->setText(model->data(model->index(row, 2)).toString());
            pModEmail->setText(model->data(model->index(row, 3)).toString());
            pModTel->setText(model->data(model->index(row, 4)).toString());
            QString dn = model->data(model->index(row, 5)).toString();
            pModDateNaissanceRenseigneeCheck->setChecked(!dn.isEmpty());
            pModDateNaissance->setEnabled(!dn.isEmpty());
            pModDateNaissance->setDate(dn.isEmpty() ? QDate::currentDate().addYears(-20) : QDate::fromString(dn, "yyyy-MM-dd"));
            break;
        }
    }
    basculerAvecAnimation(pStack, 2);
}

void MainWindow::onEnregistrerModifierParticipant()
{
    QString nom = pModNom->text().trimmed();
    QString prenom = pModPrenom->text().trimmed();
    QString email = pModEmail->text().trimmed();

    if (!nomRegex.match(nom).hasMatch() || !nomRegex.match(prenom).hasMatch()) {
        QMessageBox::warning(this, tr("Champ invalide"), tr("Le nom et le prénom ne doivent contenir que des lettres."));
        return;
    }
    if (!emailRegex.match(email).hasMatch()) {
        QMessageBox::warning(this, tr("Champ invalide"), tr("Adresse email invalide."));
        return;
    }

    bool ok = participant.modifier(pModId->text().toInt(), nom, prenom, email, pModTel->text().trimmed(),
        pModDateNaissanceRenseigneeCheck->isChecked() ? pModDateNaissance->date().toString("yyyy-MM-dd") : "");

    if (ok) {
        ajouterNotification(tr("Participant modifié : %1 %2").arg(prenom, nom));
        rafraichirListeParticipants();
        basculerAvecAnimation(pStack, 0);
    } else {
        QMessageBox::critical(this, tr("Erreur"), tr("Impossible de modifier ce participant."));
    }
}

void MainWindow::onAnnulerModifierParticipant()
{
    basculerAvecAnimation(pStack, 0);
}

void MainWindow::onInscrireParticipant()
{
    if (iParticipantCombo->currentIndex() < 0 || iCoursCombo->currentIndex() < 0) {
        QMessageBox::warning(this, tr("Champs requis"), tr("Sélectionnez un participant et un cours."));
        return;
    }
    int idParticipant = iParticipantCombo->currentData().toInt();
    int idCours = iCoursCombo->currentData().toInt();

    QString erreur = Inscription::inscrire(idParticipant, idCours);
    if (erreur.isEmpty()) {
        ajouterNotification(tr("Inscription : %1 → %2").arg(iParticipantCombo->currentText(), iCoursCombo->currentText()));
        rafraichirInscriptions();
        QMessageBox::information(this, tr("Succès"), tr("Inscription enregistrée (statut : En attente)."));
    } else {
        QMessageBox::warning(this, tr("Inscription impossible"), erreur);
    }
}

void MainWindow::onFiltrerInscriptions()
{
    iTableView->setModel(Inscription::chercher(iRechTexte->text(), iStatutCombo->currentText()));
    ajusterColonnesTable(iTableView);
}

void MainWindow::onInscriptionSelectionnee(const QModelIndex &index)
{
    if (!index.isValid()) return;
    int row = index.row();
    iSelectedInscriptionId = iTableView->model()->data(iTableView->model()->index(row, 0)).toInt();
    QString participantNom = iTableView->model()->data(iTableView->model()->index(row, 1)).toString();
    iSelectionLabel->setText(tr("Sélectionnée : %1 (ID %2)").arg(participantNom).arg(iSelectedInscriptionId));
}

void MainWindow::onConfirmerInscription()
{
    if (iSelectedInscriptionId < 0) {
        QMessageBox::warning(this, tr("Aucune sélection"), tr("Veuillez sélectionner une inscription."));
        return;
    }
    if (Inscription::changerStatut(iSelectedInscriptionId, "Confirmee")) {
        ajouterNotification(tr("Inscription confirmée (ID %1)").arg(iSelectedInscriptionId));
        rafraichirInscriptions();
        rafraichirFacturation();
    }
}

void MainWindow::onAnnulerInscriptionStatut()
{
    if (iSelectedInscriptionId < 0) {
        QMessageBox::warning(this, tr("Aucune sélection"), tr("Veuillez sélectionner une inscription."));
        return;
    }
    if (Inscription::changerStatut(iSelectedInscriptionId, "Annulee")) {
        ajouterNotification(tr("Inscription annulée (ID %1)").arg(iSelectedInscriptionId));
        rafraichirInscriptions();
    }
}

void MainWindow::onSupprimerInscription()
{
    if (iSelectedInscriptionId < 0) {
        QMessageBox::warning(this, tr("Aucune sélection"), tr("Veuillez sélectionner une inscription."));
        return;
    }
    if (QMessageBox::question(this, tr("Confirmation"), tr("Supprimer cette inscription ?")) != QMessageBox::Yes)
        return;

    if (Inscription::supprimer(iSelectedInscriptionId)) {
        ajouterNotification(tr("Inscription supprimée (ID %1)").arg(iSelectedInscriptionId));
        iSelectedInscriptionId = -1;
        iSelectionLabel->setText(tr("Aucune inscription sélectionnée."));
        rafraichirInscriptions();
    } else {
        QMessageBox::warning(this, tr("Suppression impossible"), tr("Une facture est déjà rattachée à cette inscription."));
    }
}

// =====================================================================
//                    ONGLET PLANNING (calendrier des séances)
// =====================================================================

QWidget* MainWindow::creerOngletPlanning()
{
    QWidget *onglet = new QWidget();
    QVBoxLayout *layoutPrincipal = new QVBoxLayout(onglet);

    QLabel *titrePage = new QLabel(tr("Planning des séances"));
    titrePage->setStyleSheet("font-size:16pt; font-weight:700; color:#1E3A5F;");
    layoutPrincipal->addWidget(titrePage);

    QHBoxLayout *ligne = new QHBoxLayout();

    // ---- Colonne gauche : calendrier ----
    QGroupBox *carteCalendrier = new QGroupBox(tr("Calendrier"));
    QVBoxLayout *layoutCalendrier = new QVBoxLayout(carteCalendrier);
    planCalendrier = new QCalendarWidget();
    planCalendrier->setGridVisible(true);
    connect(planCalendrier, &QCalendarWidget::selectionChanged, this, &MainWindow::onDateCalendrierSelectionnee);
    connect(planCalendrier, &QCalendarWidget::currentPageChanged, this, [this]() { surlignerJoursAvecSeances(); });
    layoutCalendrier->addWidget(planCalendrier);
    ombrerCarte(carteCalendrier, true);

    // ---- Colonne droite : séances du jour + formulaire d'ajout ----
    QGroupBox *carteJour = new QGroupBox(tr("Séances du jour sélectionné"));
    QVBoxLayout *layoutJour = new QVBoxLayout(carteJour);

    planJourLabel = new QLabel();
    planJourLabel->setStyleSheet("font-weight:600; color:#1E3A5F;");
    layoutJour->addWidget(planJourLabel);

    planSeancesJourTable = new QTableView();
    planSeancesJourTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    planSeancesJourTable->setAlternatingRowColors(true);
    planSeancesJourTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    planSeancesJourTable->verticalHeader()->setVisible(false);
    planSeancesJourTable->horizontalHeader()->setStretchLastSection(true);
    connect(planSeancesJourTable, &QTableView::clicked, this, &MainWindow::onSessionJourSelectionnee);
    layoutJour->addWidget(planSeancesJourTable);

    QPushButton *btnSupprimerSeance = new QPushButton(tr("Supprimer la séance sélectionnée"));
    styliserBouton(btnSupprimerSeance, "danger", QStyle::SP_TrashIcon);
    connect(btnSupprimerSeance, &QPushButton::clicked, this, &MainWindow::onSupprimerSession);
    layoutJour->addWidget(btnSupprimerSeance);

    QFormLayout *form = new QFormLayout();
    planCoursCombo = new QComboBox();
    planHeureDebutEdit = new QLineEdit("09:00"); planHeureDebutEdit->setPlaceholderText(tr("HH:MM"));
    planHeureFinEdit = new QLineEdit("12:00"); planHeureFinEdit->setPlaceholderText(tr("HH:MM"));
    planSalleEdit = new QLineEdit();
    planCapaciteSpin = new QSpinBox(); planCapaciteSpin->setRange(1, 500); planCapaciteSpin->setValue(20);
    form->addRow(tr("Cours :"), planCoursCombo);
    form->addRow(tr("Heure de début :"), planHeureDebutEdit);
    form->addRow(tr("Heure de fin :"), planHeureFinEdit);
    form->addRow(tr("Salle :"), planSalleEdit);
    form->addRow(tr("Capacité max :"), planCapaciteSpin);
    layoutJour->addLayout(form);

    QPushButton *btnAjouterSeance = new QPushButton(tr("Planifier cette séance"));
    styliserBouton(btnAjouterSeance, "primaire", QStyle::SP_DialogApplyButton);
    connect(btnAjouterSeance, &QPushButton::clicked, this, &MainWindow::onAjouterSession);
    layoutJour->addWidget(btnAjouterSeance);

    ombrerCarte(carteJour, true);

    ligne->addWidget(carteCalendrier, 1);
    ligne->addWidget(carteJour, 1);
    layoutPrincipal->addLayout(ligne, 1);

    // ---- Toutes les séances planifiées ----
    QGroupBox *carteToutes = new QGroupBox(tr("Toutes les séances planifiées"));
    QVBoxLayout *layoutToutes = new QVBoxLayout(carteToutes);
    planToutesSeancesTable = new QTableView();
    planToutesSeancesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    planToutesSeancesTable->setAlternatingRowColors(true);
    planToutesSeancesTable->verticalHeader()->setVisible(false);
    planToutesSeancesTable->horizontalHeader()->setStretchLastSection(true);
    layoutToutes->addWidget(planToutesSeancesTable);
    ombrerCarte(carteToutes, true);
    layoutPrincipal->addWidget(carteToutes, 1);

    return onglet;
}

void MainWindow::surlignerJoursAvecSeances()
{
    QDate visible = planCalendrier->selectedDate();
    QTextCharFormat formatNormal;
    // Réinitialise le format de tous les jours du mois affiché.
    QDate premier(visible.year(), visible.month(), 1);
    for (int j = 1; j <= premier.daysInMonth(); ++j)
        planCalendrier->setDateTextFormat(QDate(visible.year(), visible.month(), j), formatNormal);

    QList<int> jours = Session::joursAvecSessions(visible.month(), visible.year());
    QTextCharFormat formatSeance;
    formatSeance.setBackground(QColor("#DBEAFE"));
    formatSeance.setForeground(QColor("#1E3A5F"));
    formatSeance.setFontWeight(QFont::Bold);
    for (int j : jours)
        planCalendrier->setDateTextFormat(QDate(visible.year(), visible.month(), j), formatSeance);
}

void MainWindow::rafraichirSeancesDuJour()
{
    QDate date = planCalendrier->selectedDate();
    planJourLabel->setText(tr("Séances du %1").arg(date.toString("dd/MM/yyyy")));
    planSeancesJourTable->setModel(Session::sessionsDuJour(date));
    ajusterColonnesTable(planSeancesJourTable);
    planSelectedSessionId = -1;
}

void MainWindow::rafraichirPlanning()
{
    remplirComboCoursPourInscription(planCoursCombo);
    surlignerJoursAvecSeances();
    rafraichirSeancesDuJour();
    planToutesSeancesTable->setModel(Session::afficher());
    ajusterColonnesTable(planToutesSeancesTable);
}

void MainWindow::onDateCalendrierSelectionnee()
{
    rafraichirSeancesDuJour();
}

void MainWindow::onSessionJourSelectionnee(const QModelIndex &index)
{
    if (!index.isValid()) return;
    planSelectedSessionId = planSeancesJourTable->model()->data(planSeancesJourTable->model()->index(index.row(), 0)).toInt();
}

void MainWindow::onAjouterSession()
{
    if (planCoursCombo->currentIndex() < 0) {
        QMessageBox::warning(this, tr("Champ requis"), tr("Sélectionnez un cours."));
        return;
    }
    static const QRegularExpression heureRegex("^([01]\\d|2[0-3]):[0-5]\\d$");
    if (!heureRegex.match(planHeureDebutEdit->text().trimmed()).hasMatch() ||
        !heureRegex.match(planHeureFinEdit->text().trimmed()).hasMatch()) {
        QMessageBox::warning(this, tr("Champ invalide"), tr("Les heures doivent être au format HH:MM (ex. 09:00)."));
        return;
    }
    if (planHeureFinEdit->text().trimmed() <= planHeureDebutEdit->text().trimmed()) {
        QMessageBox::warning(this, tr("Champ invalide"), tr("L'heure de fin doit être après l'heure de début."));
        return;
    }

    int idCours = planCoursCombo->currentData().toInt();
    QDate date = planCalendrier->selectedDate();

    if (Session::ajouter(idCours, date, planHeureDebutEdit->text().trimmed(), planHeureFinEdit->text().trimmed(),
                          planSalleEdit->text().trimmed(), planCapaciteSpin->value())) {
        ajouterNotification(tr("Séance planifiée : %1 le %2").arg(planCoursCombo->currentText(), date.toString("dd/MM/yyyy")));
        rafraichirPlanning();
    } else {
        QMessageBox::critical(this, tr("Erreur"), tr("Impossible de planifier cette séance."));
    }
}

void MainWindow::onSupprimerSession()
{
    if (planSelectedSessionId < 0) {
        QMessageBox::warning(this, tr("Aucune sélection"), tr("Sélectionnez une séance dans la liste du jour."));
        return;
    }
    if (QMessageBox::question(this, tr("Confirmation"), tr("Supprimer cette séance ?")) != QMessageBox::Yes)
        return;

    if (Session::supprimer(planSelectedSessionId)) {
        ajouterNotification(tr("Séance supprimée (ID %1)").arg(planSelectedSessionId));
        rafraichirPlanning();
    }
}

// =====================================================================
//                         ONGLET FACTURATION
// =====================================================================

QWidget* MainWindow::creerOngletFacturation()
{
    QWidget *onglet = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(onglet);

    QLabel *titrePage = new QLabel(tr("Facturation et paiements"));
    titrePage->setStyleSheet("font-size:16pt; font-weight:700; color:#1E3A5F;");
    layout->addWidget(titrePage);

    // ---- KPI ----
    QHBoxLayout *ligneKpi = new QHBoxLayout();
    QGroupBox *carte1 = creerCarteKpi(tr("Total facturé (DT)"), &lblKpiFactureTotal);
    QGroupBox *carte2 = creerCarteKpi(tr("Encaissé (DT)"), &lblKpiFactureEncaisse);
    QGroupBox *carte3 = creerCarteKpi(tr("Impayé (DT)"), &lblKpiFactureImpaye);
    for (QGroupBox *c : {carte1, carte2, carte3}) { ombrerCarte(c, true); ligneKpi->addWidget(c); }
    layout->addLayout(ligneKpi);

    // ---- Générer une facture ----
    QGroupBox *genBox = new QGroupBox(tr("Générer une facture depuis une inscription confirmée"));
    QHBoxLayout *genLayout = new QHBoxLayout();
    factInscriptionCombo = new QComboBox();
    QPushButton *btnGenerer = new QPushButton(tr("Générer la facture"));
    styliserBouton(btnGenerer, "primaire", QStyle::SP_FileDialogNewFolder);
    connect(btnGenerer, &QPushButton::clicked, this, &MainWindow::onGenererFacture);
    genLayout->addWidget(factInscriptionCombo, 1);
    genLayout->addWidget(btnGenerer);
    genBox->setLayout(genLayout);
    layout->addWidget(genBox); ombrerCarte(genBox);

    // ---- Recherche / filtre ----
    QGroupBox *rechBox = new QGroupBox(tr("Recherche et filtre par statut"));
    QHBoxLayout *rechLayout = new QHBoxLayout();
    factRechTexte = new QLineEdit(); factRechTexte->setPlaceholderText(tr("Participant / cours"));
    factStatutCombo = new QComboBox();
    factStatutCombo->addItems({tr("Tous"), tr("En attente"), tr("Payee"), tr("Annulee")});
    rechLayout->addWidget(new QLabel(tr("Recherche :")));
    rechLayout->addWidget(factRechTexte);
    rechLayout->addWidget(new QLabel(tr("Statut :")));
    rechLayout->addWidget(factStatutCombo);
    rechBox->setLayout(rechLayout);
    layout->addWidget(rechBox); ombrerCarte(rechBox);

    factRechTimer = new QTimer(this);
    factRechTimer->setSingleShot(true);
    factRechTimer->setInterval(300);
    connect(factRechTimer, &QTimer::timeout, this, &MainWindow::onFiltrerFactures);
    connect(factRechTexte, &QLineEdit::textChanged, this, [this]() { factRechTimer->start(); });
    connect(factStatutCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onFiltrerFactures);

    factTableView = new QTableView();
    factTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    factTableView->setAlternatingRowColors(true);
    factTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    factTableView->verticalHeader()->setVisible(false);
    factTableView->horizontalHeader()->setStretchLastSection(true);
    connect(factTableView, &QTableView::clicked, this, &MainWindow::onFactureSelectionnee);
    layout->addWidget(factTableView, 1);

    factSelectionLabel = new QLabel(tr("Aucune facture sélectionnée."));
    layout->addWidget(factSelectionLabel);

    QHBoxLayout *actions = new QHBoxLayout();
    QPushButton *btnPayee = new QPushButton(tr("Marquer payée"));
    QPushButton *btnAnnulerFact = new QPushButton(tr("Annuler la facture"));
    QPushButton *btnSupprimer = new QPushButton(tr("Supprimer"));
    styliserBouton(btnPayee, "contour", QStyle::SP_DialogYesButton);
    styliserBouton(btnAnnulerFact, "contour", QStyle::SP_DialogNoButton);
    styliserBouton(btnSupprimer, "danger", QStyle::SP_TrashIcon);
    connect(btnPayee, &QPushButton::clicked, this, &MainWindow::onMarquerFecturePayee);
    connect(btnAnnulerFact, &QPushButton::clicked, this, &MainWindow::onAnnulerFacture);
    connect(btnSupprimer, &QPushButton::clicked, this, &MainWindow::onSupprimerFacture);
    actions->addWidget(btnPayee);
    actions->addWidget(btnAnnulerFact);
    actions->addWidget(btnSupprimer);
    actions->addStretch();
    layout->addLayout(actions);

    return onglet;
}

void MainWindow::rafraichirFacturation()
{
    double total, encaisse, impaye;
    Facture::statistiques(total, encaisse, impaye);
    animerCompteur(lblKpiFactureTotal, total, 2);
    animerCompteur(lblKpiFactureEncaisse, encaisse, 2);
    animerCompteur(lblKpiFactureImpaye, impaye, 2);

    int idPrecedent = factInscriptionCombo->currentIndex() >= 0 ? factInscriptionCombo->currentData().toInt() : -1;
    factInscriptionCombo->blockSignals(true);
    factInscriptionCombo->clear();
    QSqlQueryModel *modeleInsc = Facture::inscriptionsSansFacture();
    for (int i = 0; i < modeleInsc->rowCount(); ++i) {
        int id = modeleInsc->data(modeleInsc->index(i, 0)).toInt();
        factInscriptionCombo->addItem(modeleInsc->data(modeleInsc->index(i, 1)).toString(), id);
    }
    int idx = factInscriptionCombo->findData(idPrecedent);
    factInscriptionCombo->setCurrentIndex(idx >= 0 ? idx : (factInscriptionCombo->count() > 0 ? 0 : -1));
    factInscriptionCombo->blockSignals(false);

    factTableView->setModel(Facture::afficher());
    ajusterColonnesTable(factTableView);
}

void MainWindow::onGenererFacture()
{
    if (factInscriptionCombo->currentIndex() < 0) {
        QMessageBox::warning(this, tr("Aucune inscription"), tr("Aucune inscription confirmée sans facture n'est disponible."));
        return;
    }
    int idInscription = factInscriptionCombo->currentData().toInt();
    QString erreur = Facture::genererDepuisInscription(idInscription);
    if (erreur.isEmpty()) {
        ajouterNotification(tr("Facture générée pour : %1").arg(factInscriptionCombo->currentText()));
        rafraichirFacturation();
        QMessageBox::information(this, tr("Succès"), tr("Facture générée avec succès."));
    } else {
        QMessageBox::warning(this, tr("Génération impossible"), erreur);
    }
}

void MainWindow::onFiltrerFactures()
{
    factTableView->setModel(Facture::chercher(factRechTexte->text(), factStatutCombo->currentText()));
    ajusterColonnesTable(factTableView);
}

void MainWindow::onFactureSelectionnee(const QModelIndex &index)
{
    if (!index.isValid()) return;
    int row = index.row();
    factSelectedFactureId = factTableView->model()->data(factTableView->model()->index(row, 0)).toInt();
    QString participantNom = factTableView->model()->data(factTableView->model()->index(row, 1)).toString();
    factSelectionLabel->setText(tr("Sélectionnée : %1 (ID %2)").arg(participantNom).arg(factSelectedFactureId));
}

void MainWindow::onMarquerFecturePayee()
{
    if (factSelectedFactureId < 0) {
        QMessageBox::warning(this, tr("Aucune sélection"), tr("Veuillez sélectionner une facture."));
        return;
    }
    bool ok = false;
    QStringList modes = {tr("Espèces"), tr("Chèque"), tr("Virement"), tr("Carte bancaire")};
    QString mode = QInputDialog::getItem(this, tr("Mode de paiement"), tr("Mode de paiement utilisé :"), modes, 0, false, &ok);
    if (!ok) return;

    if (Facture::marquerPayee(factSelectedFactureId, mode)) {
        ajouterNotification(tr("Facture payée (ID %1, %2)").arg(factSelectedFactureId).arg(mode));
        rafraichirFacturation();
    }
}

void MainWindow::onAnnulerFacture()
{
    if (factSelectedFactureId < 0) {
        QMessageBox::warning(this, tr("Aucune sélection"), tr("Veuillez sélectionner une facture."));
        return;
    }
    if (QMessageBox::question(this, tr("Confirmation"), tr("Annuler cette facture ?")) != QMessageBox::Yes)
        return;

    if (Facture::annuler(factSelectedFactureId)) {
        ajouterNotification(tr("Facture annulée (ID %1)").arg(factSelectedFactureId));
        rafraichirFacturation();
    }
}

void MainWindow::onSupprimerFacture()
{
    if (factSelectedFactureId < 0) {
        QMessageBox::warning(this, tr("Aucune sélection"), tr("Veuillez sélectionner une facture."));
        return;
    }
    if (QMessageBox::question(this, tr("Confirmation"), tr("Supprimer définitivement cette facture ?")) != QMessageBox::Yes)
        return;

    if (Facture::supprimer(factSelectedFactureId)) {
        ajouterNotification(tr("Facture supprimée (ID %1)").arg(factSelectedFactureId));
        factSelectedFactureId = -1;
        factSelectionLabel->setText(tr("Aucune facture sélectionnée."));
        rafraichirFacturation();
    }
}
