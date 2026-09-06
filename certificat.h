#ifndef CERTIFICAT_H
#define CERTIFICAT_H

#include <QString>

// Journalise chaque certificat de participation généré (le PDF lui-même
// est produit à la volée dans MainWindow, cette classe garde uniquement
// une trace de son émission : qui, pour quel cours, quand).
class Certificat
{
public:
    static bool enregistrer(int idCours, const QString &nomParticipant);
};

#endif // CERTIFICAT_H
