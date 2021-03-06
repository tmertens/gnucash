/********************************************************************
 * gnc-backend-sql.h: load and save data to SQL                     *
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
\********************************************************************/

/**
 * @defgroup SQLBE SQL Backend Core
 @{
*/

/** @addtogroup Columns Columns
    @ingroup SQLBE
*/

/** The SQL backend core is a library which can form the core for a QOF
 *  backend based on an SQL library.
 *
 *  @file gnc-backend-sql.h
 *  @brief load and save data to SQL
 *  @author Copyright (c) 2006-2008 Phil Longstaff <plongstaff@rogers.com>
 */

#ifndef GNC_BACKEND_SQL_H
#define GNC_BACKEND_SQL_H
extern "C"
{
#include "qof.h"
#include "qofbackend-p.h"
#include <gmodule.h>
}

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>


using uint_t = unsigned int;
struct GncSqlColumnInfo;
class GncSqlColumnTableEntry;
using GncSqlColumnTableEntryPtr = std::shared_ptr<GncSqlColumnTableEntry>;
using EntryVec = std::vector<GncSqlColumnTableEntryPtr>;
using ColVec = std::vector<GncSqlColumnInfo>;
using StrVec = std::vector<std::string>;
using InstanceVec = std::vector<QofInstance*>;
using PairVec = std::vector<std::pair<std::string, std::string>>;
using VersionPair = std::pair<const std::string, unsigned int>;
using VersionVec = std::vector<VersionPair>;
class GncSqlConnection;
class GncSqlStatement;
using GncSqlStatementPtr = std::unique_ptr<GncSqlStatement>;
class GncSqlResult;
//using GncSqlResultPtr = std::unique_ptr<GncSqlResult>;
using GncSqlResultPtr = GncSqlResult*;

/**
 * @struct GncSqlBackend
 *
 * Main SQL backend structure.
 */
class GncSqlBackend
{
public:
    GncSqlBackend(GncSqlConnection *conn, QofBook* book,
                  const char* format = nullptr);
    virtual ~GncSqlBackend() = default;
    /** Connect the backend to a GncSqlConnection.
     * Sets up version info. Calling with nullptr clears the connection and
     * destroys the version info.
     */
    void connect(GncSqlConnection *conn) noexcept;
    /**
     * Initializes DB table version information.
     *
     * @param be SQL backend struct
     */
    void init_version_info() noexcept;
    bool reset_version_info() noexcept;
    /**
     * Finalizes DB table version information.
     *
     * @param be SQL backend struct
     */
    void finalize_version_info() noexcept;
    /* FIXME: These are just pass-throughs of m_conn functions. */
    GncSqlStatementPtr create_statement_from_sql(const std::string& str) const noexcept;
    /** Executes an SQL SELECT statement and returns the result rows.  If an
     * error occurs, an entry is added to the log, an error status is returned
     * to qof and nullptr is returned.
     *
     * @param statement Statement
     * @return Results, or nullptr if an error has occured
     */
    GncSqlResultPtr execute_select_statement(const GncSqlStatementPtr& stmt) const noexcept;
    int execute_nonselect_statement(const GncSqlStatementPtr& stmt) const noexcept;
    std::string quote_string(const std::string&) const noexcept;
    /**
     * Creates a table in the database
     *
     * @param table_name Table name
     * @param col_table DB table description
     * @return TRUE if successful, FALSE if unsuccessful
     */
    bool create_table(const std::string& table_name, const EntryVec& col_table) const noexcept;
    /**
     * Creates a table in the database and sets its version
     *
     * @param table_name Table name
     * @param table_version Table version
     * @param col_table DB table description
     * @return TRUE if successful, FALSE if unsuccessful
     */
    bool create_table(const std::string& table_name, int table_version,
                      const EntryVec& col_table) noexcept;
    /**
     * Creates an index in the database
     *
     * @param index_name Index name
     * @param table_name Table name
     * @param col_table Columns that the index should index
     * @return TRUE if successful, FALSE if unsuccessful
     */
    bool create_index(const std::string& index_name,
                      const std::string& table_name,
                      const EntryVec& col_table) const noexcept;
    /**
     * Adds one or more columns to an existing table.
     *
     * @param table_name SQL table name
     * @param new_col_table Column table for new columns
     * @return TRUE if successful, FALSE if unsuccessful
     */
    bool add_columns_to_table(const std::string& table_name,
                              const EntryVec& col_table) const noexcept;
    /**
     * Upgrades a table to a new structure.
     *
     * The upgrade is done by creating a new table with the new structure,
     * SELECTing the old data into the new table, deleting the old table, then
     * renaming the new table.  Therefore, this will only work if the new table
     * structure is similar enough to the old table that the SELECT will work.
     *
     * @param table_name SQL table name
     * @param col_table Column table
     */
    void upgrade_table (const std::string& table_name,
                        const EntryVec& col_table) noexcept;
    /**
     * Returns the version number for a DB table.
     *
     * @param table_name Table name
     * @return Version number, or 0 if the table does not exist
     */
    uint_t get_table_version(const std::string& table_name) const noexcept;
    bool set_table_version (const std::string& table_name, uint_t version) noexcept;
    /**
     * Converts a time64 value to a string value for the database.
     *
     * @param t time64 to be converted.
     * @return String representation of the Timespec
     */
    std::string time64_to_string (time64 t) const noexcept;

    QofBook* book() const noexcept { return m_book; }

    bool pristine() const noexcept { return m_is_pristine_db; }
    void update_progress() const noexcept;
    void finish_progress() const noexcept;
    void set_loading(bool val) noexcept { m_loading = val; }
    const char* timespec_format() const noexcept { return m_timespec_format; }

    friend void gnc_sql_load(GncSqlBackend*, QofBook*, QofBackendLoadType);
    friend void gnc_sql_sync_all(GncSqlBackend*, QofBook*);
    friend void gnc_sql_commit_edit(GncSqlBackend*, QofInstance*);

protected:
    QofBackend be;           /**< QOF backend. Not a pointer, nor really a member */
    GncSqlConnection* m_conn;  /**< SQL connection */
    QofBook* m_book;           /**< The primary, main open book */
    bool m_loading;        /**< We are performing an initial load */
    bool m_in_query;       /**< We are processing a query */
    bool m_is_pristine_db; /**< Are we saving to a new pristine db? */
    VersionVec m_versions;    /**< Version number for each table */
    const char* m_timespec_format;   /**< Format string for SQL for timespec values */
private:
};

/**
 * Initialize the SQL backend.
 *
 * @param be SQL backend
 */
void gnc_sql_init (GncSqlBackend* be);

/**
 * Load the contents of an SQL database into a book.
 *
 * @param be SQL backend
 * @param book Book to be loaded
 */
void gnc_sql_load (GncSqlBackend* be,  QofBook* book,
                   QofBackendLoadType loadType);

/**
 * Register a commodity to be committed after loading is complete.
 *
 * Necessary to save corrections made while loading.
 * @param be SQL backend
 * @param comm The commodity item to be committed.
 */
void gnc_sql_push_commodity_for_postload_processing (GncSqlBackend* be,
                                                     gpointer comm);

/**
 * Save the contents of a book to an SQL database.
 *
 * @param be SQL backend
 * @param book Book to be saved
 */
void gnc_sql_sync_all (GncSqlBackend* be,  QofBook* book);

/**
 * An object is about to be edited.
 *
 * @param be SQL backend
 * @param inst Object being edited
 */
void gnc_sql_begin_edit (GncSqlBackend* be, QofInstance* inst);

/**
 * Object editing has been cancelled.
 *
 * @param qbe SQL backend
 * @param inst Object being edited
 */
void gnc_sql_rollback_edit (GncSqlBackend* qbe, QofInstance* inst);

/**
 * Object editting is complete and the object should be saved.
 *
 * @param qbe SQL backend
 * @param inst Object being edited
 */
void gnc_sql_commit_edit (GncSqlBackend* qbe, QofInstance* inst);

/**
 */

/**
 * SQL statement provider.
 */
class GncSqlStatement
{
public:
    virtual ~GncSqlStatement() {}
    virtual const char* to_sql() const = 0;
    virtual void add_where_cond (QofIdTypeConst, const PairVec&) = 0;
};

/**
 * Encapsulate the connection to the database. 
 */
class GncSqlConnection
{
public:
    /** Returns NULL if error */
    virtual ~GncSqlConnection() = default;
    virtual GncSqlResultPtr execute_select_statement (const GncSqlStatementPtr&)
        noexcept = 0;
    /** Returns false if error */
    virtual int execute_nonselect_statement (const GncSqlStatementPtr&)
        noexcept = 0;
    virtual GncSqlStatementPtr create_statement_from_sql (const std::string&)
        const noexcept = 0;
    /** Returns true if successful */
    virtual bool does_table_exist (const std::string&) const noexcept = 0;
    /** Returns TRUE if successful, false if error */
    virtual bool begin_transaction () noexcept = 0;
    /** Returns TRUE if successful, FALSE if error */
    virtual bool rollback_transaction () const noexcept = 0;
    /** Returns TRUE if successful, FALSE if error */
    virtual bool commit_transaction () const noexcept = 0;
    /** Returns TRUE if successful, FALSE if error */
    virtual bool create_table (const std::string&, const ColVec&)
        const noexcept = 0;
    /** Returns TRUE if successful, FALSE if error */
    virtual bool create_index (const std::string&, const std::string&,
                               const EntryVec&) const noexcept = 0;
    /** Returns TRUE if successful, FALSE if error */
    virtual bool add_columns_to_table (const std::string&, const ColVec&)
        const noexcept = 0;
    virtual std::string quote_string (const std::string&)
        const noexcept = 0;
    /** Get the connection error value.
     * If not 0 will normally be meaningless outside of implementation code.
     */
    virtual int dberror() const noexcept = 0;
    virtual void set_error(int error, unsigned int repeat,  bool retry) noexcept = 0;
    virtual bool verify() noexcept = 0;
    virtual bool retry_connection(const char* msg) noexcept = 0;

};

/**
 * Struct used to represent a row in the result of an SQL SELECT statement.
 * SQL backends must provide a structure which implements all of the functions.
 */

class GncSqlRow;
/**
 * Pure virtual class to iterate over a query result set.
 */
class GncSqlResult
{
public:
    virtual ~GncSqlResult() = default;
    virtual uint64_t size() const noexcept = 0;
    virtual GncSqlRow& begin() = 0;
    virtual GncSqlRow& end() = 0;
    friend GncSqlRow;
protected:
    class IteratorImpl {
    public:
        virtual ~IteratorImpl() = default;
        virtual GncSqlRow& operator++() = 0;
        virtual GncSqlResult* operator*() = 0;
        virtual int64_t get_int_at_col (const char* col) const = 0;
        virtual float get_float_at_col (const char* col) const = 0;
        virtual double get_double_at_col (const char* col) const = 0;
        virtual std::string get_string_at_col (const char* col) const = 0;
        virtual time64 get_time64_at_col (const char* col) const = 0;
        virtual bool is_col_null (const char* col) const noexcept = 0;
    };
};

/**
 * Row of SQL Query results.
 *
 * This is a "pointer" class of a pimpl pattern, the implementation being
 * GncSqlResult::IteratorImpl. It's designed to present a std::forward_iterator
 * like interface for use with range-for while allowing for wrapping a C API.
 *
 * Important Implementation Note: Operator++() as written requires that the
 * sentinel GncSqlRow returned by GncSqlResult::end() has m_impl = nullptr in
 * order to terminate the loop condition. This is a bit of a hack and might be a
 * problem with a different SQL interface library from libdbi.
 *
 * Note that GncSqlResult::begin and GncSqlRow::operator++() return
 * GncSqlRow&. This is necessary for operator++() to be called: Using operator
 * ++() on a pointer performs pointer arithmetic rather than calling the
 * pointed-to-class's operator++() and C++'s range-for uses operator++()
 * directly on whatever begin() gives it.
 */
class GncSqlRow
{
public:
    GncSqlRow (GncSqlResult::IteratorImpl* iter) : m_iter{iter} {}
    ~GncSqlRow() { }
    GncSqlRow& operator++();
    GncSqlRow& operator*() { return *this; }
    friend bool operator!=(const GncSqlRow&, const GncSqlRow&);
    int64_t get_int_at_col (const char* col) const {
        return m_iter->get_int_at_col (col); }
    float get_float_at_col (const char* col) const {
        return m_iter->get_float_at_col (col); }
    double get_double_at_col (const char* col) const {
        return m_iter->get_double_at_col (col); }
    std::string get_string_at_col (const char* col) const {
        return m_iter->get_string_at_col (col); }
    time64 get_time64_at_col (const char* col) const {
        return m_iter->get_time64_at_col (col); }
    bool is_col_null (const char* col) const noexcept {
        return m_iter->is_col_null (col); }
private:
    GncSqlResult::IteratorImpl* m_iter;
};

inline bool operator!=(const GncSqlRow& lr, const GncSqlRow& rr) {
    return lr.m_iter != rr.m_iter;
}

inline bool operator==(const GncSqlRow& lr, const GncSqlRow& rr) {
    return !(lr != rr);
}

#define GNC_SQL_BACKEND             "gnc:sql:1"
#define GNC_SQL_BACKEND_VERSION 1

/**
 * Encapsulates per-class table schema with functions to load, create a table,
 * commit a changed front-end object (note that database transaction semantics
 * are not yet implemented; edit/commit applies to the front-end object!) and
 * write all front-end objects of the type to the database. Additional functions
 * for creating and runing queries existed but were unused and untested. They've
 * been temporarily removed until the front end is ready to use them.
 */
class GncSqlObjectBackend
{
public:
    GncSqlObjectBackend (int version, const std::string& type,
                         const std::string& table, const EntryVec& vec) :
        m_table_name{table}, m_version{version}, m_type_name{type},
        m_col_table(vec) {}
    /**
     * Load all objects of m_type in the database into memory.
     * @param be The GncSqlBackend containing the database connection.
     */
    virtual void load_all (GncSqlBackend*) = 0;
    /**
     * Conditionally create or update a database table from m_col_table. The
     * condition is the version returned by querying the database's version
     * table: If it's 0 then the table wasn't found and will be created; All
     * tables areat least version 1. If the database's version is less than the
     * compiled version then the table schema is upgraded but the data isn't,
     * that's the engine's responsibility when the object is loaded. If the
     * version is greater than the compiled version then nothing is touched.
     * @param be The GncSqlBackend containing the database connection.
     */
    virtual void create_tables (GncSqlBackend*);
    /**
     * UPDATE/INSERT a single instance of m_type_name into the database.
     * @param be The GncSqlBackend containing the database.
     * @param inst The QofInstance to be written out.
     */
    virtual bool commit (GncSqlBackend* be, QofInstance* inst);
    /**
     * Write all objects of m_type_name to the database.
     * @param be The GncSqlBackend containing the database.
     * @return true if the objects were successfully written, false otherwise.
     */
    virtual bool write (GncSqlBackend*) { return true; }
    /**
     * Return the m_type_name for the class. This value is created at
     * compilation time and is called QofIdType or QofIdTypeConst in other parts
     * of GnuCash. Most values are defined in src/engine/gnc-engine.h.
     * @return m_type_name.
     */
    const char* type () const noexcept { return m_type_name.c_str(); }
    /**
     * Compare a version with the compiled version (m_version).
     * @return true if they match.
     */
    const bool is_version (int version) const noexcept {
        return version == m_version;
    }
protected:
    const std::string m_table_name;
    const int m_version;
    const std::string m_type_name; /// The front-end QofIdType
    const EntryVec& m_col_table;   /// The ORM table definition.
};

using GncSqlObjectBackendPtr = GncSqlObjectBackend*;

using OBEEntry = std::tuple<std::string, GncSqlObjectBackendPtr>;
using OBEVec = std::vector<OBEEntry>;
void gnc_sql_register_backend(OBEEntry&&);
void gnc_sql_register_backend(GncSqlObjectBackendPtr);
const OBEVec& gnc_sql_get_backend_registry();
GncSqlObjectBackendPtr gnc_sql_get_object_backend(const std::string& table_name);

/**
 * Data-passing struct for callbacks to qof_object_foreach() used in
 * GncSqlObjectBackend::write(). Once QofCollection is rewritten to use C++
 * containers we'll use std::foreach() and lambdas instead of callbacks and this
 * can go away.
 */
struct write_objects_t
{
    write_objects_t() = default;
    write_objects_t (GncSqlBackend* b, bool o, GncSqlObjectBackendPtr e) :
        be{b}, is_ok{o}, obe{e} {}
    void commit (QofInstance* inst) {
        if (is_ok) is_ok = obe->commit (be, inst);
    }
    GncSqlBackend* be;
    bool is_ok;
    GncSqlObjectBackendPtr obe;
};

/**
 * Basic column type
 */
typedef enum
{
    BCT_STRING,
    BCT_INT,
    BCT_INT64,
    BCT_DATE,
    BCT_DOUBLE,
    BCT_DATETIME
} GncSqlBasicColumnType;


// Type for conversion of db row to object.
enum GncSqlObjectType
{
    CT_STRING,
    CT_GUID,
    CT_INT,
    CT_INT64,
    CT_TIMESPEC,
    CT_GDATE,
    CT_NUMERIC,
    CT_DOUBLE,
    CT_BOOLEAN,
    CT_ACCOUNTREF,
    CT_BUDGETREF,
    CT_COMMODITYREF,
    CT_LOTREF,
    CT_TXREF,
    CT_ADDRESS,
    CT_BILLTERMREF,
    CT_INVOICEREF,
    CT_ORDERREF,
    CT_OWNERREF,
    CT_TAXTABLEREF
};

enum ColumnFlags : int
{
    COL_NO_FLAG = 0,
        COL_PKEY = 0x01,        /**< The column is a primary key */
        COL_NNUL = 0x02,    /**< The column may not contain a NULL value */
        COL_UNIQUE = 0x04,  /**< The column must contain unique values */
        COL_AUTOINC = 0x08 /**< The column is an auto-incrementing int */
        };

/**
 * Contains all of the information required to copy information between an
 * object and the database for a specific object property.
 *
 * If an entry contains a gobj_param_name value, this string is used as the
 * property name for a call to g_object_get() or g_object_set().  If the
 * gobj_param_name value is NULL but qof_param_name is not NULL, this value
 * is used as the parameter name for a call to
 * qof_class_get_parameter_getter().  If both of these values are NULL, getter
 * and setter are the addresses of routines to return or set the parameter
 * value, respectively.
 *
 * The database description for an object consists of an array of
 * GncSqlColumnTableEntry objects, with a final member having col_name == NULL.
 */

class GncSqlColumnTableEntry
{
public:
    GncSqlColumnTableEntry (const char* name, const GncSqlObjectType type,
                            unsigned int s,
                            int f, const char* gobj_name = nullptr,
                            const char* qof_name = nullptr,
                            QofAccessFunc get = nullptr,
                            QofSetterFunc set = nullptr) :
        m_col_name{name}, m_col_type{type}, m_size{s},
        m_flags{static_cast<ColumnFlags>(f)},
        m_gobj_param_name{gobj_name}, m_qof_param_name{qof_name}, m_getter{get},
        m_setter{set} {}

    /**
     * Load a value into an object from the database row.
     */
    virtual void load(const GncSqlBackend* be, GncSqlRow& row,
                      QofIdTypeConst obj_name, gpointer pObject) const noexcept = 0;
    /**
     * Add a GncSqlColumnInfo structure for the column type to a
     * ColVec.
     */
    virtual void add_to_table(const GncSqlBackend* be, ColVec& vec) const noexcept = 0;
    /**
     * Add a pair of the table column heading and object's value's string
     * representation to a PairVec; used for constructing WHERE clauses and
     * UPDATE statements.
     */
    virtual void add_to_query(const GncSqlBackend* be, QofIdTypeConst obj_name,
                              gpointer pObject, PairVec& vec) const noexcept = 0;
    /**
     * Retrieve the getter function depending on whether it's an auto-increment
     * field, a QofClass getter, or a function passed to the constructor.
     */
    QofAccessFunc get_getter(QofIdTypeConst obj_name) const noexcept;
    /**
     * Retrieve the setter function depending on whether it's an auto-increment
     * field, a QofClass getter, or a function passed to the constructor.
     */
    QofSetterFunc get_setter(QofIdTypeConst obj_name) const noexcept;
    /**
     * Retrieve the field name so that we don't need to make
     * create_single_col_select_statement and friend.
     */
    const char* name() const noexcept { return m_col_name; }
    /**
     * Report if the entry is an auto-increment field.
     */
    bool is_autoincr() const noexcept { return m_flags & COL_AUTOINC; }
    /* On the other hand, our implementation class and GncSqlColumnInfo need to
     * be able to read our member variables.
     */
    template<GncSqlObjectType Otype> friend class GncSqlColumnTableEntryImpl;
    friend struct GncSqlColumnInfo;
    template<typename T> void load_from_guid_ref(GncSqlRow& row,
                                                 QofIdTypeConst obj_name,
                                                 gpointer pObject, T get_ref)
        const noexcept
        {
            g_return_if_fail (pObject != NULL);

            try
            {
                GncGUID guid;
                auto val = row.get_string_at_col (m_col_name);
                (void)string_to_guid (val.c_str(), &guid);
                auto target = get_ref(&guid);
                if (target != nullptr)
                    set_parameter (pObject, target, get_setter(obj_name),
                                   m_gobj_param_name);
            }
            catch (std::invalid_argument) {}
        }

protected:
    template <typename T> T
    get_row_value_from_object(QofIdTypeConst obj_name, const gpointer pObject) const;
    template <typename T> void
    add_value_to_vec(const GncSqlBackend* be, QofIdTypeConst obj_name,
                     const gpointer pObject, PairVec& vec) const;
/**
 * Adds a name/guid std::pair to a PairVec for creating a query.
 *
 * @param be SQL backend struct
 * @param obj_name QOF object type name
 * @param pObject Object
 * @param pList List
 */
    void add_objectref_guid_to_query (const GncSqlBackend* be,
                                      QofIdTypeConst obj_name,
                                      const gpointer pObject,
                                      PairVec& vec) const noexcept;
/**
 * Adds a column info structure for an object reference GncGUID to a ColVec.
 *
 * @param be SQL backend struct
 * @param pList List
 */
    void add_objectref_guid_to_table (const GncSqlBackend* be,
                                      ColVec& vec) const noexcept;
private:
    const char* m_col_name;        /**< Column name */
    const GncSqlObjectType m_col_type;        /**< Column type */
    unsigned int m_size;       /**< Column size in bytes, for string columns */
    ColumnFlags m_flags;           /**< Column flags */
    const char* m_gobj_param_name; /**< If non-null, g_object param name */
    const char* m_qof_param_name;  /**< If non-null, qof parameter name */
    QofAccessFunc m_getter;        /**< General access function */
    QofSetterFunc m_setter;        /**< General setter function */
    template <typename T> T get_row_value_from_object(QofIdTypeConst obj_name,
                                                      const gpointer pObject,
                                                      std::true_type) const;
    template <typename T> T get_row_value_from_object(QofIdTypeConst obj_name,
                                                      const gpointer pObject,
                                                      std::false_type) const;
    template <typename T> void add_value_to_vec(const GncSqlBackend* be,
                                                QofIdTypeConst obj_name,
                                                const gpointer pObject,
                                                PairVec& vec, std::true_type) const;
    template <typename T> void add_value_to_vec(const GncSqlBackend* be,
                                                QofIdTypeConst obj_name,
                                                const gpointer pObject,
                                                PairVec& vec, std::false_type) const;

};

template <GncSqlObjectType Type>
class GncSqlColumnTableEntryImpl : public GncSqlColumnTableEntry
{
public:
    GncSqlColumnTableEntryImpl (const char* name, const GncSqlObjectType type,
                                unsigned int s,
                                int f, const char* gobj_name = nullptr,
                                const char* qof_name = nullptr,
                                QofAccessFunc get = nullptr,
                                QofSetterFunc set = nullptr) :
        GncSqlColumnTableEntry (name, type, s, f, gobj_name,qof_name, get, set)
        {} 
    void load(const GncSqlBackend* be, GncSqlRow& row,  QofIdTypeConst obj_name,
              gpointer pObject) const noexcept override;
    void add_to_table(const GncSqlBackend* be, ColVec& vec) const noexcept override;
    void add_to_query(const GncSqlBackend* be, QofIdTypeConst obj_name,
                      gpointer pObject, PairVec& vec) const noexcept override;
};

template <GncSqlObjectType Type>
std::shared_ptr<GncSqlColumnTableEntryImpl<Type>>
gnc_sql_make_table_entry(const char* name, unsigned int s, int f)
{
    return std::make_shared<GncSqlColumnTableEntryImpl<Type>>(name, Type, s, f);
}

template <GncSqlObjectType Type>
std::shared_ptr<GncSqlColumnTableEntryImpl<Type>>
gnc_sql_make_table_entry(const char* name, unsigned int s, int f,
                         const char* param)
{
    return std::make_shared<GncSqlColumnTableEntryImpl<Type>>(name, Type, s,
                                                              f, param);
}

class is_qof : public std::true_type {};

template <GncSqlObjectType Type>
std::shared_ptr<GncSqlColumnTableEntryImpl<Type>>
gnc_sql_make_table_entry(const char* name, unsigned int s, int f,
                         const char* param, bool qofp)
{
    return std::make_shared<GncSqlColumnTableEntryImpl<Type>>(name, Type, s,
                                                              f, nullptr,
                                                              param);
}

template <GncSqlObjectType Type>
std::shared_ptr<GncSqlColumnTableEntryImpl<Type>>
gnc_sql_make_table_entry(const char* name, unsigned int s, int f,
                         QofAccessFunc get, QofSetterFunc set)
{
    return std::make_shared<GncSqlColumnTableEntryImpl<Type>>(
        name, Type, s, f, nullptr, nullptr, get, set);
}

/**
 *  information required to create a column in a table.
 */
struct GncSqlColumnInfo
{
    GncSqlColumnInfo (std::string&& name, GncSqlBasicColumnType type,
                      unsigned int size = 0, bool unicode = false,
                      bool autoinc = false, bool primary = false,
                      bool not_null = false) :
        m_name{name}, m_type{type}, m_size{size}, m_unicode{unicode},
        m_autoinc{autoinc}, m_primary_key{primary}, m_not_null{not_null}
        {}
    GncSqlColumnInfo(const GncSqlColumnTableEntry& e, GncSqlBasicColumnType t,
                     unsigned int size = 0, bool unicode = true) :
        m_name{e.m_col_name}, m_type{t}, m_size{size}, m_unicode{unicode},
        m_autoinc(e.m_flags & COL_AUTOINC),
        m_primary_key(e.m_flags & COL_PKEY),
        m_not_null(e.m_flags & COL_NNUL) {}
    std::string m_name; /**< Column name */
    GncSqlBasicColumnType m_type; /**< Column basic type */
    unsigned int m_size; /**< Column size (string types) */
    bool m_unicode; /**< Column is unicode (string types) */
    bool m_autoinc; /**< Column is autoinc (int type) */
    bool m_primary_key; /**< Column is the primary key */
    bool m_not_null; /**< Column forbids NULL values */
};

inline bool operator==(const GncSqlColumnInfo& l,
                       const GncSqlColumnInfo& r)
{
    return l.m_name == r.m_name && l.m_type == r.m_type;
}

inline bool operator!=(const GncSqlColumnInfo& l,
                       const GncSqlColumnInfo& r)
{
    return !(l == r);
}

typedef enum
{
    OP_DB_INSERT,
    OP_DB_UPDATE,
    OP_DB_DELETE
} E_DB_OPERATION;


/**
 * Set an object property with a setter function.
 * @param pObject void* to the object being set.
 * @param item the value to be set in the property.
 * @param setter The function to set the property.
 * The void* is an obvious wart occasioned by the fact that we're using GLists
 * to hold objects. As the rewrite progresses we'll replace that with another
 * template paramter.
 */
template <typename T, typename P, typename F>
void set_parameter(T object, P item, F& setter)
{
    (*setter)(object, item);
}

template <typename T, typename P>
void set_parameter(T object, P item, QofSetterFunc setter, std::true_type)
{
    (*setter)(object, (void*)item);
}

template <typename T, typename P>
void set_parameter(T object, P item, QofSetterFunc setter, std::false_type)
{
    (*setter)(object, (void*)(&item));
}

template <typename T, typename P>
void set_parameter(T object, P item, QofSetterFunc setter)
{
    set_parameter(object, item, setter, std::is_pointer<P>());
}

/**
 * Set an object property with g_object_set.
 * @param pObject void* to the object being set.
 * @param item the value to set in the property.
 * @param property the property name.
 * The void* is an obvious wart. So is g_object_set, partly because it's GObject
 * but mostly because it works off of string comparisons.
 */
template <typename T, typename P>
void set_parameter(T object, P item, const char* property)
{
    qof_instance_increase_editlevel(object);
    g_object_set(object, property, item, nullptr);
    qof_instance_decrease_editlevel(object);
};

/**
 * Set an object property with either a g_object_set or a setter.
 *
 * See previous templates for the parameter meanings. This is clunky but fits in
 * the current architecture for refactoring.
 */
template <typename T, typename P, typename F>
void set_parameter(T object, P item, F setter, const char* property)
{
    if (property)
        set_parameter(object, item, property);
    else
        set_parameter(object, item, setter);
}

/**
 * Performs an operation on the database.
 *
 * @param be SQL backend struct
 * @param op Operation type
 * @param table_name SQL table name
 * @param obj_name QOF object type name
 * @param pObject Gnucash object
 * @param table DB table description
 * @return TRUE if successful, FALSE if not
 */
gboolean gnc_sql_do_db_operation (GncSqlBackend* be,
                                  E_DB_OPERATION op,
                                  const gchar* table_name,
                                  QofIdTypeConst obj_name,
                                  gpointer pObject,
                                  const EntryVec& table);


/**
 * Loads a Gnucash object from the database.
 *
 * @param be SQL backend struct
 * @param row DB result row
 * @param obj_name QOF object type name
 * @param pObject Object to be loaded
 * @param table DB table description
 */
void gnc_sql_load_object (const GncSqlBackend* be, GncSqlRow& row,
                          QofIdTypeConst obj_name, gpointer pObject,
                          const EntryVec& table);

/**
 * Checks whether an object is in the database or not.
 *
 * @param be SQL backend struct
 * @param table_name DB table name
 * @param obj_name QOF object type name
 * @param pObject Object to be checked
 * @param table DB table description
 * @return TRUE if the object is in the database, FALSE otherwise
 */
gboolean gnc_sql_object_is_it_in_db (GncSqlBackend* be,
                                     const gchar* table_name,
                                     QofIdTypeConst obj_name,
                                     const gpointer pObject,
                                     const EntryVec& table );
/**
 * Loads the object guid from a database row.  The table must have a column
 * named "guid" with type CT_GUID.
 *
 * @param be SQL backend struct
 * @param row Database row
 * @return GncGUID
 */

const GncGUID* gnc_sql_load_guid (const GncSqlBackend* be, GncSqlRow& row);


/**
 * Appends the ascii strings for a list of GUIDs to the end of an SQL string.
 *
 * @param str SQL string
 * @param list List of GUIDs
 * @param maxCount Max # of GUIDs to append
 * @return Number of GUIDs appended
 */
uint_t gnc_sql_append_guids_to_sql (std::stringstream& sql,
                                    const InstanceVec& instances);

void _retrieve_guid_ (gpointer pObject,  gpointer pValue);

gpointer gnc_sql_compile_query (QofBackend* pBEnd, QofQuery* pQuery);
void gnc_sql_free_query (QofBackend* pBEnd, gpointer pQuery);
void gnc_sql_run_query (QofBackend* pBEnd, gpointer pQuery);

template <typename T> T
GncSqlColumnTableEntry::get_row_value_from_object(QofIdTypeConst obj_name,
                                                  const gpointer pObject) const
{
    return get_row_value_from_object<T>(obj_name, pObject,
                                        std::is_pointer<T>());
}

template <typename T> T
GncSqlColumnTableEntry::get_row_value_from_object(QofIdTypeConst obj_name,
                                                  const gpointer pObject,
                                                  std::true_type) const
{
    g_return_val_if_fail(obj_name != nullptr && pObject != nullptr, nullptr);
    T result = nullptr;
    if (m_gobj_param_name != nullptr)
        g_object_get(pObject, m_gobj_param_name, &result, NULL );
    else
    {
        QofAccessFunc getter = get_getter(obj_name);
        if (getter != nullptr)
            result = reinterpret_cast<T>((getter)(pObject, nullptr));
    }
    return result;
}

template <typename T> T
GncSqlColumnTableEntry::get_row_value_from_object(QofIdTypeConst obj_name,
                                                  const gpointer pObject,
                                                  std::false_type) const
{
    g_return_val_if_fail(obj_name != nullptr && pObject != nullptr,
                         static_cast<T>(0));
    T result = static_cast<T>(0);
    if (m_gobj_param_name != nullptr)
        g_object_get(pObject, m_gobj_param_name, &result, NULL );
    else
    {
        QofAccessFunc getter = get_getter(obj_name);
        if (getter != nullptr)
            result = reinterpret_cast<T>((getter)(pObject, nullptr));
    }
    return result;
}

template <typename T> void
GncSqlColumnTableEntry::add_value_to_vec(const GncSqlBackend* be,
                                         QofIdTypeConst obj_name,
                                         const gpointer pObject,
                                         PairVec& vec) const
{
    add_value_to_vec<T>(be, obj_name, pObject, vec, std::is_pointer<T>());
}

template <typename T> void
GncSqlColumnTableEntry::add_value_to_vec(const GncSqlBackend* be,
                                         QofIdTypeConst obj_name,
                                         const gpointer pObject,
                                         PairVec& vec, std::true_type) const
{
    T s = get_row_value_from_object<T>(obj_name, pObject);

    if (s != nullptr)
    {
        std::ostringstream stream;
        stream << *s;
        vec.emplace_back(std::make_pair(std::string{m_col_name}, stream.str()));
        return;
    }
}

template <typename T> void
GncSqlColumnTableEntry::add_value_to_vec(const GncSqlBackend* be,
                                         QofIdTypeConst obj_name,
                                         const gpointer pObject,
                                         PairVec& vec, std::false_type) const
{
    T s = get_row_value_from_object<T>(obj_name, pObject);

    std::ostringstream stream;
    stream << s;
    vec.emplace_back(std::make_pair(std::string{m_col_name}, stream.str()));
    return;
}

#endif /* GNC_BACKEND_SQL_H */

/**
   @}  end of the SQL Backend Core doxygen group
*/
