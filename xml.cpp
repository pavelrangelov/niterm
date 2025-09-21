#include <QtXml>

#include "macrosdialog.h"
#include "ui_macrosdialog.h"

///////////////////////////////////////////////////////////////////////////////
QDomElement MacrosDialog::paramToNode(QDomDocument &d, const ItemClass &ic)
{
    QDomElement e = d.createElement("Row");

    e.setAttribute("Command",   ic.command);
    e.setAttribute("Data",      ic.data);
    e.setAttribute("Response",  ic.response);
    e.setAttribute("Comment",   ic.comment);

    return e;
}

///////////////////////////////////////////////////////////////////////////////
bool MacrosDialog::saveDocument(const QString &filename)
{
    QDomDocument doc("NITERM_ML");
    QDomElement root = doc.createElement("Macros");

    doc.appendChild(root);

    ItemClass ic;

    for (int i=0; i<ui->table->rowCount(); i++)
    {
        ic.command	= ui->table->item(i, COL_MACRO_COMMAND) ->text();
        ic.data		= ui->table->item(i, COL_MACRO_DATA)    ->text();
        ic.response = ui->table->item(i, COL_MACRO_RESPONSE)->text();
        ic.comment	= ui->table->item(i, COL_MACRO_COMMENT) ->text();

        root.appendChild(paramToNode(doc, ic));
    }

    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly))
    {
        return false;
    }

    QTextStream ts(&file);
    ts.setEncoding(QStringConverter::Utf8);
    ts << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\r\n";
    ts << doc.toString();

    file.close();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool MacrosDialog::loadDocument(const QString &filename)
{
    clearDocument();

    QDomDocument doc("NITERM_ML");

    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }

    if (!doc.setContent(&file))
    {
        file.close();
        return false;
    }

    file.close();

    QDomElement root = doc.documentElement();

    if (root.tagName() != "Macros")
    {
        return false;
    }

    QDomNode n = root.firstChild();

    while (!n.isNull())
    {
        QDomElement e = n.toElement();

        if (!e.isNull())
        {
            if (e.tagName() == "Row")
            {
                ItemClass ic;
                ic.command	= e.attribute("Command",	"");
                ic.data		= e.attribute("Data",		"");
                ic.response = e.attribute("Response",   "");
                ic.comment	= e.attribute("Comment",	"");

                int row = insertTableRow(ui->table->rowCount());

                ui->table->item(row, COL_MACRO_COMMAND)	->setText(ic.command);
                ui->table->item(row, COL_MACRO_DATA)    ->setText(ic.data);
                ui->table->item(row, COL_MACRO_RESPONSE)->setText(ic.response);
                ui->table->item(row, COL_MACRO_COMMENT)	->setText(ic.comment);
            }
        }

        n = n.nextSibling();
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
void MacrosDialog::clearDocument()
{
    if (ui->table->rowCount())
    {
        for (int i=ui->table->rowCount()-1; i>=0; i--)
        {
            ui->table->removeRow(i);
        }
    }
}
