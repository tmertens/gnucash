/********************************************************************\
 * gnc-order-sql.c -- order sql backend                             *
 *                                                                  *
 * This program is free software; you can redistribute it and/or    *
 * modify it under the terms of the GNU General Public License as   *
 * published by the Free Software Foundation; either version 2 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU General Public License for more details.                     *
 *                                                                  *
 * You should have received a copy of the GNU General Public License*
 * along with this program; if not, contact:                        *
 *                                                                  *
 * Free Software Foundation           Voice:  +1-617-542-5942       *
 * 51 Franklin Street, Fifth Floor    Fax:    +1-617-542-2652       *
 * Boston, MA  02110-1301,  USA       gnu@gnu.org                   *
 *                                                                  *
\********************************************************************/

/** @file gnc-order-sql.c
 *  @brief load and save address data to SQL
 *  @author Copyright (c) 2007-2008 Phil Longstaff <plongstaff@rogers.com>
 *
 * This file implements the top-level QofBackend API for saving/
 * restoring data to/from an SQL database
 */

#include <guid.hpp>
extern "C"
{
#include "config.h"

#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include "gncOrderP.h"
}
#include "gnc-backend-sql.h"
#include "gnc-slots-sql.h"
#include "gnc-order-sql.h"

#define _GNC_MOD_NAME   GNC_ID_ORDER

static QofLogModule log_module = G_LOG_DOMAIN;

#define TABLE_NAME "orders"
#define TABLE_VERSION 1

#define MAX_ID_LEN 2048
#define MAX_NOTES_LEN 2048
#define MAX_REFERENCE_LEN 2048

static EntryVec col_table
({
    gnc_sql_make_table_entry<CT_GUID>("guid", 0, COL_NNUL | COL_PKEY, "guid"),
    gnc_sql_make_table_entry<CT_STRING>("id", MAX_ID_LEN, COL_NNUL, "id"),
    gnc_sql_make_table_entry<CT_STRING>("notes", MAX_NOTES_LEN, COL_NNUL,
                                        "notes"),
    gnc_sql_make_table_entry<CT_STRING>(
        "reference", MAX_REFERENCE_LEN, COL_NNUL, "reference"),
    gnc_sql_make_table_entry<CT_BOOLEAN>("active", 0, COL_NNUL, "order"),
    gnc_sql_make_table_entry<CT_TIMESPEC>("date_opened", 0, COL_NNUL,
                                          "date-opened"),
    gnc_sql_make_table_entry<CT_TIMESPEC>("date_closed", 0, COL_NNUL,
                                          "date-closed"),
    gnc_sql_make_table_entry<CT_OWNERREF>("owner", 0, COL_NNUL,
                                          ORDER_OWNER, true),
});

class GncSqlOrderBackend : public GncSqlObjectBackend
{
public:
    GncSqlOrderBackend(int version, const std::string& type,
                      const std::string& table, const EntryVec& vec) :
        GncSqlObjectBackend(version, type, table, vec) {}
    void load_all(GncSqlBackend*) override;
    bool write(GncSqlBackend*) override;
};

static GncOrder*
load_single_order (GncSqlBackend* be, GncSqlRow& row)
{
    const GncGUID* guid;
    GncOrder* pOrder;

    g_return_val_if_fail (be != NULL, NULL);

    guid = gnc_sql_load_guid (be, row);
    pOrder = gncOrderLookup (be->book(), guid);
    if (pOrder == NULL)
    {
        pOrder = gncOrderCreate (be->book());
    }
    gnc_sql_load_object (be, row, GNC_ID_ORDER, pOrder, col_table);
    qof_instance_mark_clean (QOF_INSTANCE (pOrder));

    return pOrder;
}

void
GncSqlOrderBackend::load_all (GncSqlBackend* be)
{
    g_return_if_fail (be != NULL);

    std::stringstream sql;
    sql << "SELECT * FROM " << TABLE_NAME;
    auto stmt = be->create_statement_from_sql(sql.str());
    auto result = be->execute_select_statement(stmt);
    InstanceVec instances;

    for (auto row : *result)
    {
        GncOrder* pOrder = load_single_order (be, row);
        if (pOrder != nullptr)
            instances.push_back(QOF_INSTANCE(pOrder));
    }

    if (!instances.empty())
        gnc_sql_slots_load_for_instancevec (be, instances);
}

/* ================================================================= */
static gboolean
order_should_be_saved (GncOrder* order)
{
    const char* id;

    g_return_val_if_fail (order != NULL, FALSE);

    /* make sure this is a valid order before we save it -- should have an ID */
    id = gncOrderGetID (order);
    if (id == NULL || *id == '\0')
    {
        return FALSE;
    }

    return TRUE;
}

static void
write_single_order (QofInstance* term_p, gpointer data_p)
{
    auto s = reinterpret_cast<write_objects_t*>(data_p);

    g_return_if_fail (term_p != NULL);
    g_return_if_fail (GNC_IS_ORDER (term_p));
    g_return_if_fail (data_p != NULL);

    if (s->is_ok && order_should_be_saved (GNC_ORDER (term_p)))
    {
        s->commit (term_p);
    }
}

bool
GncSqlOrderBackend::write (GncSqlBackend* be)
{
    g_return_val_if_fail (be != NULL, FALSE);
    write_objects_t data{be, true, this};

    qof_object_foreach (GNC_ID_ORDER, be->book(), write_single_order, &data);

    return data.is_ok;
}

/* ================================================================= */
template<> void
GncSqlColumnTableEntryImpl<CT_ORDERREF>::load (const GncSqlBackend* be,
                                                 GncSqlRow& row,
                                                 QofIdTypeConst obj_name,
                                                 gpointer pObject) const noexcept
{
    load_from_guid_ref(row, obj_name, pObject,
                       [be](GncGUID* g){
                           return gncOrderLookup(be->book(), g);
                       });
}

template<> void
GncSqlColumnTableEntryImpl<CT_ORDERREF>::add_to_table(const GncSqlBackend* be,
                                                 ColVec& vec) const noexcept
{
    add_objectref_guid_to_table(be, vec);
}

template<> void
GncSqlColumnTableEntryImpl<CT_ORDERREF>::add_to_query(const GncSqlBackend* be,
                                                    QofIdTypeConst obj_name,
                                                    const gpointer pObject,
                                                    PairVec& vec) const noexcept
{
    add_objectref_guid_to_query(be, obj_name, pObject, vec);
}
/* ================================================================= */
void
gnc_order_sql_initialize (void)
{
    static GncSqlOrderBackend be_data {
        GNC_SQL_BACKEND_VERSION, GNC_ID_ORDER, TABLE_NAME, col_table};

    gnc_sql_register_backend(&be_data);
}
/* ========================== END OF FILE ===================== */
