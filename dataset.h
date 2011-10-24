#ifndef DATASET_H
#define DATASET_H

#include <QAbstractTableModel>
#include <QMap>
#include <QVector>
#include <QStringList>

#define DS_ERR_SUCCEED           0
#define DS_ERR_FILE_OPEN_FAILED  1
#define DS_ERR_JSON_PARSE_FAILED 2

class Dataset : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit Dataset(QObject *parent = 0);
    int rowCount(const QModelIndex&) const;
    int columnCount(const QModelIndex&) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    int loadData(QString schema_file, QString data_file);
signals:

public slots:

private:
    QVector< QMap<QString, QVariant> > schema;
    QVector<QStringList> vdata;
};


#endif // DATASET_H