/* 
 *  libpinyin
 *  Library to deal with pinyin.
 *  
 *  Copyright (C) 2016 Peng Wu <alexepico@gmail.com>
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "chewing_large_table2.h"
#include <errno.h>
#include "bdb_utils.h"

namespace pinyin{

ChewingLargeTable2::ChewingLargeTable2() {
    /* create in-memory db. */
    m_db = NULL;
    int ret = db_create(&m_db, NULL, 0);
    assert(0 == ret);

    ret = m_db->open(m_db, NULL, NULL, NULL,
                     DB_BTREE, DB_CREATE, 0600);
    assert(0 == ret);

    m_entries = NULL;
    init_entries();
}

void ChewingLargeTable2::init_entries() {
    assert(NULL == m_entries);

    m_entries = g_ptr_array_new();
    /* NULL for the first pointer. */
    g_ptr_array_set_size(m_entries, MAX_PHRASE_LENGTH + 1);

#define CASE(len) case len:                         \
    {                                               \
        ChewingTableEntry<len> * entry =            \
            new ChewingTableEntry<len>;             \
        g_ptr_array_index(m_entries, len) = entry;  \
    }

    for (size_t i = 1; i < m_entries->len; i++) {
        switch(i) {
            CASE(1);
            CASE(2);
            CASE(3);
            CASE(4);
            CASE(5);
            CASE(6);
            CASE(7);
            CASE(8);
            CASE(9);
            CASE(10);
            CASE(11);
            CASE(12);
            CASE(13);
            CASE(14);
            CASE(15);
            CASE(16);
        default:
            assert(false);
        }
    }

#undef CASE
}

void ChewingLargeTable2::reset() {
    if (m_db) {
        m_db->sync(m_db, 0);
        m_db->close(m_db, 0);
        m_db = NULL;
    }

#define CASE(len) case len:                     \
    {                                           \
        ChewingTableEntry<len> * entry =        \
            (ChewingTableEntry<len> *)          \
            g_ptr_array_index(m_entries, len);  \
        delete entry;                           \
    }

    if (m_entries) {
        assert(MAX_PHRASE_LENGTH + 1 == m_entries->len);

        for (size_t i = 1; i < m_entries->len; i++) {
            switch(i) {
                CASE(1);
                CASE(2);
                CASE(3);
                CASE(4);
                CASE(5);
                CASE(6);
                CASE(7);
                CASE(8);
                CASE(9);
                CASE(10);
                CASE(11);
                CASE(12);
                CASE(13);
                CASE(14);
                CASE(15);
                CASE(16);
            default:
                assert(false);
            }
        }

        g_ptr_array_free(m_entries, TRUE);
        m_entries = NULL;
    }

#undef CASE
}

/* attach method */
bool ChewingLargeTable2::attach(const char * dbfile, guint32 flags) {
    reset();

    u_int32_t db_flags = attach_options(flags);

    if (!dbfile)
        return false;

    int ret = db_create(&m_db, NULL, 0);
    assert(0 == ret);

    ret = m_db->open(m_db, NULL, dbfile, NULL,
                     DB_BTREE, db_flags, 0644);
    if (ret != 0)
        return false;

    init_entries();

    return true;
}

/* load/store method */
bool ChewingLargeTable2::load_db(const char * filename) {
    reset();

    /* create in-memory db. */
    int ret = db_create(&m_db, NULL, 0);
    assert(0 == ret);

    ret = m_db->open(m_db, NULL, NULL, NULL,
                     DB_BTREE, DB_CREATE, 0600);
    if (ret != 0)
        return false;

    /* load db into memory. */
    DB * tmp_db = NULL;
    ret = db_create(&tmp_db, NULL, 0);
    assert(0 == ret);

    if (NULL == tmp_db)
        return false;

    ret = tmp_db->open(tmp_db, NULL, filename, NULL,
                       DB_BTREE, DB_RDONLY, 0600);
    if (ret != 0)
        return false;

    if (!copy_bdb(tmp_db, m_db))
        return false;

    if (tmp_db != NULL)
        tmp_db->close(tmp_db, 0);

    init_entries();

    return true;
}

bool ChewingLargeTable2::store_db(const char * new_filename) {
    DB * tmp_db = NULL;

    int ret = unlink(new_filename);
    if (ret != 0 && errno != ENOENT)
        return false;

    ret = db_create(&tmp_db, NULL, 0);
    assert(0 == ret);

    if (NULL == tmp_db)
        return false;

    ret = tmp_db->open(tmp_db, NULL, new_filename, NULL,
                       DB_BTREE, DB_CREATE, 0600);
    if (ret != 0)
        return false;

    if (!copy_bdb(m_db, tmp_db))
        return false;

    if (tmp_db != NULL) {
        tmp_db->sync(m_db, 0);
        tmp_db->close(tmp_db, 0);
    }

    return true;
}

template<int phrase_length>
int ChewingLargeTable2::search_internal(/* in */ const ChewingKey index[],
                                        /* in */ const ChewingKey keys[],
                                        /* out */ PhraseIndexRanges ranges) const {
    int result = SEARCH_NONE;

    ChewingTableEntry<phrase_length> * entry =
        (ChewingTableEntry<phrase_length> *)
        g_ptr_array_index(m_entries, phrase_length);
    assert(NULL != entry);

    DBT db_key;
    memset(&db_key, 0, sizeof(DBT));
    db_key.data = (void *) index;
    db_key.size = phrase_length * sizeof(ChewingKey);

    DBT db_data;
    memset(&db_data, 0, sizeof(DBT));
    int ret = m_db->get(m_db, NULL, &db_key, &db_data, 0);
    if (ret != 0)
        return result;

    /* continue searching. */
    result |= SEARCH_CONTINUED;

    entry->m_chunk.set_chunk(db_data.data, db_data.size, NULL);

    result = entry->search(keys, ranges) | result;

    return result;
}

int ChewingLargeTable2::search_internal(int phrase_length,
                                        /* in */ const ChewingKey index[],
                                        /* in */ const ChewingKey keys[],
                                        /* out */ PhraseIndexRanges ranges) const {
#define CASE(len) case len:                                     \
    {                                                           \
        return search_internal<len>(index, keys, ranges);        \
    }

    switch(phrase_length) {
        CASE(1);
        CASE(2);
        CASE(3);
        CASE(4);
        CASE(5);
        CASE(6);
        CASE(7);
        CASE(8);
        CASE(9);
        CASE(10);
        CASE(11);
        CASE(12);
        CASE(13);
        CASE(14);
        CASE(15);
        CASE(16);
    default:
        assert(false);
    }

#undef CASE

    return SEARCH_NONE;
}

/* search method */
int ChewingLargeTable2::search(int phrase_length,
                               /* in */ const ChewingKey keys[],
                               /* out */ PhraseIndexRanges ranges) const {
    ChewingKey index[MAX_PHRASE_LENGTH];
    assert(NULL != m_db);

    if (contains_incomplete_pinyin(keys, phrase_length)) {
        compute_incomplete_chewing_index(keys, index, phrase_length);
        return search_internal(phrase_length, index, keys, ranges);
    } else {
        compute_chewing_index(keys, index, phrase_length);
        return search_internal(phrase_length, index, keys, ranges);
    }

    return SEARCH_NONE;
}

template<int phrase_length>
int ChewingLargeTable2::add_index_internal(/* in */ const ChewingKey index[],
                                           /* in */ const ChewingKey keys[],
                                           /* in */ phrase_token_t token) {
    ChewingTableEntry<phrase_length> * entry =
        (ChewingTableEntry<phrase_length> *)
        g_ptr_array_index(m_entries, phrase_length);
    assert(NULL != entry);

    /* load chewing table entry. */
    DBT db_key;
    memset(&db_key, 0, sizeof(DBT));
    db_key.data = (void *) index;
    db_key.size = phrase_length * sizeof(ChewingKey);

    DBT db_data;
    memset(&db_data, 0, sizeof(DBT));
    int ret = m_db->get(m_db, NULL, &db_key, &db_data, 0);

    if (ret != 0) {
        /* new entry. */
        ChewingTableEntry<phrase_length> new_entry;
        new_entry.add_index(keys, token);

        memset(&db_data, 0, sizeof(DBT));
        db_data.data = new_entry.m_chunk.begin();
        db_data.size = new_entry.m_chunk.size();
        ret = m_db->put(m_db, NULL, &db_key, &db_data, 0);
        if (ret != 0)
            return ERROR_FILE_CORRUPTION;

        /* recursively add keys for continued information. */
        for (size_t len = phrase_length - 1; len > 0; --len) {
            memset(&db_key, 0, sizeof(DBT));
            db_key.data = (void *) index;
            db_key.size = len * sizeof(ChewingKey);

            memset(&db_data, 0, sizeof(DBT));

            ret = m_db->get(m_db, NULL, &db_key, &db_data, 0);
            /* found entry. */
            if (0 == ret)
                return ERROR_OK;

            /* new entry with empty content. */
            memset(&db_data, 0, sizeof(DBT));

            ret = m_db->put(m_db, NULL, &db_key, &db_data, 0);
            if (ret != 0)
                return ERROR_FILE_CORRUPTION;
        }

        return ERROR_OK;
    }

    /* already have keys. */
    entry->m_chunk.set_chunk(db_data.data, db_data.size, NULL);
    int result = entry->add_index(keys, token);

    /* store the entry. */
    memset(&db_data, 0, sizeof(DBT));
    db_data.data = entry->m_chunk.begin();
    db_data.size = entry->m_chunk.size();
    ret = m_db->put(m_db, NULL, &db_key, &db_data, 0);
    if (ret != 0)
        return ERROR_FILE_CORRUPTION;

    return result;
}


int ChewingLargeTable2::add_index_internal(int phrase_length,
                                           /* in */ const ChewingKey index[],
                                           /* in */ const ChewingKey keys[],
                                           /* in */ phrase_token_t token) {
#define CASE(len) case len:                                     \
    {                                                           \
        return add_index_internal<len>(index, keys, token);     \
    }

    switch(phrase_length) {
        CASE(1);
        CASE(2);
        CASE(3);
        CASE(4);
        CASE(5);
        CASE(6);
        CASE(7);
        CASE(8);
        CASE(9);
        CASE(10);
        CASE(11);
        CASE(12);
        CASE(13);
        CASE(14);
        CASE(15);
        CASE(16);
    default:
        assert(false);
    }

#undef CASE

    return ERROR_FILE_CORRUPTION;
}

/* add/remove index method */
int ChewingLargeTable2::add_index(int phrase_length,
                                  /* in */ const ChewingKey keys[],
                                  /* in */ phrase_token_t token) {
    ChewingKey index[MAX_PHRASE_LENGTH];
    assert(NULL != m_db);
    int result = ERROR_OK;

    /* for in-complete chewing index */
    compute_incomplete_chewing_index(keys, index, phrase_length);
    result = add_index_internal(phrase_length, index, keys, token);
    assert(ERROR_OK == result || ERROR_INSERT_ITEM_EXISTS == result);
    if (ERROR_OK != result)
        return result;

    /* for chewing index */
    compute_chewing_index(keys, index, phrase_length);
    result = add_index_internal(phrase_length, index, keys, token);
    assert(ERROR_OK == result || ERROR_INSERT_ITEM_EXISTS == result);
    return result;
}

template<int phrase_length>
int ChewingLargeTable2::remove_index_internal(/* in */ const ChewingKey index[],
                                              /* in */ const ChewingKey keys[],
                                              /* in */ phrase_token_t token) {
    ChewingTableEntry<phrase_length> * entry =
        (ChewingTableEntry<phrase_length> *)
        g_ptr_array_index(m_entries, phrase_length);
    assert(NULL != entry);

    DBT db_key;
    memset(&db_key, 0, sizeof(DBT));
    db_key.data = (void *) index;
    db_key.size = phrase_length * sizeof(ChewingKey);

    DBT db_data;
    memset(&db_data, 0, sizeof(DBT));
    int ret = m_db->get(m_db, NULL, &db_key, &db_data, 0);
    if (ret != 0)
        return ERROR_REMOVE_ITEM_DONOT_EXISTS;

    entry->m_chunk.set_chunk(db_data.data, db_data.size, NULL);

    int result = entry->remove_index(keys, token);
    if (ERROR_OK != result)
        return result;

    /* removed the token. */
    memset(&db_data, 0, sizeof(DBT));
    db_data.data = entry->m_chunk.begin();
    db_data.size = entry->m_chunk.size();

    ret = m_db->put(m_db, NULL, &db_key, &db_data, 0);
    if (ret != 0)
        return ERROR_FILE_CORRUPTION;

    return ERROR_OK;
}

int ChewingLargeTable2::remove_index_internal(int phrase_length,
                                              /* in */ const ChewingKey index[],
                                              /* in */ const ChewingKey keys[],
                                              /* in */ phrase_token_t token) {
#define CASE(len) case len:                                     \
    {                                                           \
        return remove_index_internal<len>(index, keys, token);  \
    }

    switch(phrase_length) {
        CASE(1);
        CASE(2);
        CASE(3);
        CASE(4);
        CASE(5);
        CASE(6);
        CASE(7);
        CASE(8);
        CASE(9);
        CASE(10);
        CASE(11);
        CASE(12);
        CASE(13);
        CASE(14);
        CASE(15);
        CASE(16);
    default:
        assert(false);
    }

#undef CASE

    return ERROR_FILE_CORRUPTION;
}

int ChewingLargeTable2::remove_index(int phrase_length,
                                     /* in */ const ChewingKey keys[],
                                     /* in */ phrase_token_t token) {
    ChewingKey index[MAX_PHRASE_LENGTH];
    assert(NULL != m_db);
    int result = ERROR_OK;

    /* for in-complete chewing index */
    compute_incomplete_chewing_index(keys, index, phrase_length);
    result = remove_index_internal(phrase_length, index, keys, token);
    assert(ERROR_OK == result || ERROR_REMOVE_ITEM_DONOT_EXISTS == result);
    if (ERROR_OK != result)
        return result;

    /* for chewing index */
    compute_chewing_index(keys, index, phrase_length);
    result = remove_index_internal(phrase_length, index, keys, token);
    assert(ERROR_OK == result || ERROR_REMOVE_ITEM_DONOT_EXISTS == result);
    return result;
}

/* mask out method */
/* assume it is in-memory dbm. */
bool ChewingLargeTable2::mask_out(phrase_token_t mask,
                                  phrase_token_t value) {
    /* use copy and sweep algorithm here. */
    DB * tmp_db = NULL;

    int ret = db_create(&tmp_db, NULL, 0);
    assert(0 == ret);

    if (NULL == tmp_db)
        return false;

    /* create in memory db. */
    ret = tmp_db->open(tmp_db, NULL, NULL, NULL,
                       DB_BTREE, DB_CREATE, 0600);
    if (ret != 0)
        return false;

    DBC * cursorp = NULL;
    DBT db_key, db_data;

    /* Get a cursor */
    m_db->cursor(m_db, NULL, &cursorp, 0);

    if (NULL == cursorp)
        return false;

    /* Initialize our DBTs. */
    memset(&db_key, 0, sizeof(DBT));
    memset(&db_data, 0, sizeof(DBT));

    /* Iterate over the database, retrieving each record in turn. */
    while((ret = cursorp->c_get(cursorp, &db_key, &db_data, DB_NEXT)) == 0) {
        int phrase_length = db_key.size / sizeof(ChewingKey);

#define CASE(len) case len:                                             \
        {                                                               \
            ChewingTableEntry<len> * entry =                            \
                (ChewingTableEntry<len> *)                              \
                g_ptr_array_index(m_entries, phrase_length);            \
            assert(NULL != entry);                                      \
                                                                        \
            entry->m_chunk.set_chunk(db_data.data, db_data.size, NULL); \
                                                                        \
            entry->mask_out(mask, value);                               \
                                                                        \
            memset(&db_data, 0, sizeof(DBT));                           \
            db_data.data = entry->m_chunk.begin();                      \
            db_data.size = entry->m_chunk.size();                       \
            int ret = tmp_db->put(tmp_db, NULL, &db_key, &db_data, 0);  \
            assert(ret == 0);                                           \
        }

        switch(phrase_length) {
            CASE(1);
            CASE(2);
            CASE(3);
            CASE(4);
            CASE(5);
            CASE(6);
            CASE(7);
            CASE(8);
            CASE(9);
            CASE(10);
            CASE(11);
            CASE(12);
            CASE(13);
            CASE(14);
            CASE(15);
            CASE(16);
        default:
            assert(false);
        }

#undef CASE

    }
    assert(ret == DB_NOTFOUND);

    /* Cursors must be closed */
    if (cursorp != NULL)
        cursorp->c_close(cursorp);

    m_db->sync(m_db, 0);
    m_db->close(m_db, 0);

    m_db = tmp_db;
    return true;
}

};
