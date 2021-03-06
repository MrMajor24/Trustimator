#include "dataset.h"
#include <QAbstractTableModel>
#include <QColor>
#include <QJson/parser.h>
#include <QFile>
#include <iostream>
#include <cassert>

using namespace std;

Dataset::Dataset(QObject *parent) :
    QAbstractTableModel(parent)
{
}

int Dataset::loadData(QString schema_file_name, QString data_file)
{
    // Reset Data
    vdata.clear();
    schema.clear();

    // CSV File Import

    FILE* f = fopen(data_file.toUtf8().data(), "r");

    if (! f)
        return DS_ERR_FILE_OPEN_FAILED;

    char line[1024];

    while(!feof(f))
    {
        memset(line, 0, 1024);
        fgets(line, 1024, f);
        if (strlen(line) < 2)
            continue;

        QString qline = line;
        QStringList row = qline.split(",");

        if (row.size() < 4)
            continue;

        vdata.append(row);
    }
    cerr << "DS::loadData() : " << vdata.size() << " records." << endl;
    fclose(f);

    // JSON Schema File Import

    QJson::Parser json_parser;
    QFile schemaFile(schema_file_name);

    if (! schemaFile.exists())
        return DS_ERR_FILE_OPEN_FAILED;

    bool json_ok;
    QVariant result = json_parser.parse(&schemaFile, &json_ok);

    if (! json_ok)
    {
        cerr << "Failed to parse schema json string." << endl;
        return DS_ERR_JSON_PARSE_FAILED;
    }

    // Copy values to field container
    QVariantList columns = result.toList();
    for (int i = 0; i < columns.size(); ++i) {
        QMap<QString, QVariant> col = columns.at(i).toMap();
//        QVariantMap jcol = ;

//        QMapIterator<QString, QVariant> i(jcol);
//        while (i.hasNext()) {
//            i.next();
//            col[i.key()] = i.value();
//        }

        this->schema.append(col);
    }

    return DS_ERR_SUCCEED;
}

int Dataset::rowCount(const QModelIndex&) const {
    return vdata.size();
}

int Dataset::columnCount(const QModelIndex&) const
{
    return schema.size();
}

QVariant Dataset::data(const QModelIndex &index, int role) const
{
    switch(role)
    {
    case Qt::DisplayRole:
        {
            QString v = vdata.at(index.row()).at(index.column());

            if (schema[index.column()].value("Type").toInt() == COL_TYPE_CAT)
            {
                int iv = QVariant(v).toInt();
                QVariantList values = schema[index.column()].value("Values").toList();
                return values.at(iv-1).toString();
            }

            return v;
            break;
        }
    case Qt::TextAlignmentRole:
        return Qt::AlignCenter;
    case Qt::BackgroundColorRole:
        return index.row() % 2 ? QColor(220, 220, 220) : Qt::white;
    default:
        return QVariant();
    }

    return QVariant();
}

QVariant Dataset::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Vertical)
        {
            return QVariant(section+1).toString();
        } else {
            return schema.at(section).value("Name").toString();
        }
    }
    else
        return QVariant();
}

QVector< QMap<QString, QVariant> > Dataset::getSchema()
{
    return this->schema;
}

// Learning algorithm dataset integeration
void Dataset::get_training_dimensions(unsigned int *num_data, unsigned int *num_input, unsigned int *num_output)
{
    *num_data = vdata.size();
    *num_input = schema.size()-1;
    *num_output = 1;

    // Leave one out
    if (leave_item != DONT_LEAVE_ANY)
        (*num_data)--;

    // Filter Meta Columns
    for(int i=0; i<schema.size(); i++)
    {
        if (schema[i].value("Type").toInt() == COL_TYPE_META)
        {
            // cerr << "Removing column " << schema[i].value("Name").toString().toStdString() << endl;
            (*num_input)--;
        }

        if (schema[i].value("Type").toInt() == COL_TYPE_CAT)
        {
            QVariantList values = schema[i].value("Values").toList();
            // cerr << "Feeding " << schema[i].value("Name").toString().toStdString() << " - " << values.size() << " combinations." << endl;

            (*num_input) += values.size() - 1;
        }
    }
}

void Dataset::leave_item_out(unsigned int row)
{
    leave_item = row;
}

void Dataset::get_training_row(unsigned int row, float input[], float output[])
{
//    unsigned int num_data, num_input, num_output;

    assert(row < (unsigned int)vdata.size());

    if (leave_item != DONT_LEAVE_ANY)
    {
        if (row >= (unsigned)leave_item)
            row++;
    }

    QStringList cur_row = vdata[row];
    unsigned input_index = 0;
    for(int i=0; i<schema.size() - 1; i++)
    {
        switch(schema[i].value("Type").toInt())
        {
        case COL_TYPE_META:
            continue;

        case COL_TYPE_CAT:
            {
                QVariantList values = schema[i].value("Values").toList();
                int cval = QVariant( cur_row.at(i) ).toInt();
                for(int j=0; j<values.size(); j++)
                {
                    input[input_index] = (cval == (j+1)) ? 1 : 0;
                    input_index++;
                }
                break;
            }

        case COL_TYPE_NUM:
            input[input_index] = QVariant( cur_row.at(i) ).toFloat();
            input_index++;
            break;

        default:
            qFatal("Unhandled variable type");
        }
    }

    output[0] = QVariant( cur_row.at(schema.size()-1) ).toFloat();
}

void Dataset::get_scale_flags(int should_scale[])
{
    unsigned input_index = 0;
    for(int i=0; i<schema.size() - 1; i++)
    {
        switch(schema[i].value("Type").toInt())
        {
        case COL_TYPE_META:
            continue;

        case COL_TYPE_CAT:
            {
                QVariantList values = schema[i].value("Values").toList();
                for(int j=0; j<values.size(); j++)
                {
                    should_scale[input_index] = 0;
                    input_index++;
                }
                break;
            }

        case COL_TYPE_NUM:
            should_scale[input_index] = 1;
            input_index++;
            break;

        default:
            qFatal("Unhandled variable type");
        }
    }
}
